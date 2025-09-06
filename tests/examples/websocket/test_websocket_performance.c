#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#define usleep(x) Sleep((x) / 1000)
#define sleep(x) Sleep((x) * 1000)
#define close(x) closesocket(x)
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include "src/web/websocket.h"
#include "src/utils/utils.h"

/* 性能测试配置 */
#define STRESS_TEST_CLIENTS 100
#define STRESS_TEST_DURATION 60
#define PERFORMANCE_TEST_ROUNDS 5
#define MESSAGE_SIZES 5
#define MAX_MESSAGE_SIZE 4096

/* 性能统计结构 */
typedef struct {
    long long total_bytes_sent;
    long long total_bytes_received;
    long long total_messages_sent;
    long long total_messages_received;
    double total_time_ms;
    double min_latency_ms;
    double max_latency_ms;
    double total_latency_ms;
    int latency_samples;
} PerformanceStats;

/* 全局变量 */
static int server_running = 1;
static WebSocketServer* ws_server = NULL;
static HttpServer* http_server = NULL;
static PerformanceStats global_stats = {0};
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 获取当前时间（毫秒） */
long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* 信号处理函数 */
void signal_handler(int sig) {
    printf("\n接收到信号 %d，正在关闭服务器...\n", sig);
    server_running = 0;
}

/* WebSocket消息处理回调 */
int message_handler(const WebSocketMessage* message) {
    if (!message || !message->connection) {
        return -1;
    }
    
    pthread_mutex_lock(&stats_mutex);
    global_stats.total_messages_received++;
    global_stats.total_bytes_received += message->data_length;
    pthread_mutex_unlock(&stats_mutex);
    
    /* 回显消息 */
    websocket_connection_send_text(message->connection, message->data);
    
    return 0;
}

/* WebSocket连接处理回调 */
int connect_handler(const WebSocketMessage* message) {
    if (!message || !message->connection) {
        return -1;
    }
    
    return 0;
}

/* WebSocket断开连接处理回调 */
int disconnect_handler(const WebSocketMessage* message) {
    if (!message || !message->connection) {
        return -1;
    }
    
    return 0;
}

/* 创建测试消息 */
void create_test_message(char* buffer, int size, int message_id) {
    snprintf(buffer, size, "Performance Test Message %d with size %d bytes", 
             message_id, size);
    
    /* 填充剩余空间 */
    int header_len = strlen(buffer);
    if (header_len < size) {
        memset(buffer + header_len, 'A', size - header_len - 1);
        buffer[size - 1] = '\0';
    }
}

/* 性能测试客户端 */
typedef struct {
    int id;
    int socket_fd;
    pthread_t thread;
    int message_size;
    int message_count;
    PerformanceStats stats;
    int connected;
} PerformanceClient;

/* 性能测试客户端线程 */
void* performance_client_thread(void* arg) {
    PerformanceClient* client = (PerformanceClient*)arg;
    if (!client) {
        return NULL;
    }
    
    printf("性能测试客户端 %d 开始 (消息大小: %d bytes)\n", 
           client->id, client->message_size);
    
    /* 创建套接字 */
    client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_fd < 0) {
        printf("客户端 %d: 创建套接字失败\n", client->id);
        return NULL;
    }
    
    /* 设置服务器地址 */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    /* 连接到服务器 */
    if (connect(client->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("客户端 %d: 连接失败\n", client->id);
        close(client->socket_fd);
        return NULL;
    }
    
    /* 发送WebSocket握手请求 */
    const char* handshake_request = 
        "GET /ws HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";
    
    if (send(client->socket_fd, handshake_request, strlen(handshake_request), 0) < 0) {
        printf("客户端 %d: 握手失败\n", client->id);
        close(client->socket_fd);
        return NULL;
    }
    
    /* 接收握手响应 */
    char response[1024];
    int response_len = recv(client->socket_fd, response, sizeof(response) - 1, 0);
    if (response_len <= 0 || strstr(response, "101 Switching Protocols") == NULL) {
        printf("客户端 %d: 握手失败\n", client->id);
        close(client->socket_fd);
        return NULL;
    }
    
    client->connected = 1;
    client->stats.total_time_ms = 0;
    
    /* 准备测试消息 */
    char message_buffer[MAX_MESSAGE_SIZE];
    create_test_message(message_buffer, client->message_size, client->id);
    
    /* 创建WebSocket帧 */
    char frame[MAX_MESSAGE_SIZE + 10];
    frame[0] = 0x81; // FIN + TEXT
    frame[1] = client->message_size;
    memcpy(frame + 2, message_buffer, client->message_size);
    int frame_size = 2 + client->message_size;
    
    /* 性能测试循环 */
    long long start_time = get_current_time_ms();
    
    for (int i = 0; i < client->message_count && server_running; i++) {
        long long send_time = get_current_time_ms();
        
        /* 发送消息 */
        if (send(client->socket_fd, frame, frame_size, 0) > 0) {
            client->stats.total_messages_sent++;
            client->stats.total_bytes_sent += client->message_size;
            
            /* 接收响应 */
            char recv_buffer[MAX_MESSAGE_SIZE + 10];
            int recv_len = recv(client->socket_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
            
            if (recv_len > 0) {
                long long recv_time = get_current_time_ms();
                double latency = recv_time - send_time;
                
                client->stats.total_messages_received++;
                client->stats.total_bytes_received += client->message_size;
                client->stats.total_latency_ms += latency;
                client->stats.latency_samples++;
                
                if (latency < client->stats.min_latency_ms || client->stats.latency_samples == 1) {
                    client->stats.min_latency_ms = latency;
                }
                if (latency > client->stats.max_latency_ms) {
                    client->stats.max_latency_ms = latency;
                }
            }
        }
    }
    
    client->stats.total_time_ms = get_current_time_ms() - start_time;
    
    /* 关闭连接 */
    close(client->socket_fd);
    client->connected = 0;
    
    return NULL;
}

/* 运行性能测试 */
void run_performance_test(int message_size, int message_count, int client_count) {
    printf("\n=== 性能测试 (消息大小: %d bytes, 客户端数: %d) ===\n", 
           message_size, client_count);
    
    PerformanceClient clients[client_count];
    memset(clients, 0, sizeof(clients));
    
    /* 创建HTTP服务器 */
    http_server = (HttpServer*)malloc(sizeof(HttpServer));
    memset(http_server, 0, sizeof(HttpServer));
    
    ws_server = websocket_server_create(http_server);
    websocket_set_handlers(ws_server, message_handler, connect_handler, disconnect_handler);
    websocket_server_start(ws_server);
    
    /* 等待服务器启动 */
    sleep(1);
    
    /* 初始化客户端 */
    for (int i = 0; i < client_count; i++) {
        clients[i].id = i + 1;
        clients[i].socket_fd = -1;
        clients[i].message_size = message_size;
        clients[i].message_count = message_count;
        clients[i].connected = 0;
        memset(&clients[i].stats, 0, sizeof(PerformanceStats));
        clients[i].stats.min_latency_ms = 999999;
    }
    
    /* 启动客户端线程 */
    long long test_start_time = get_current_time_ms();
    
    for (int i = 0; i < client_count; i++) {
        pthread_create(&clients[i].thread, NULL, performance_client_thread, &clients[i]);
        usleep(10000); // 10ms间隔
    }
    
    /* 等待所有客户端完成 */
    for (int i = 0; i < client_count; i++) {
        pthread_join(clients[i].thread, NULL);
    }
    
    long long test_end_time = get_current_time_ms();
    
    /* 汇总统计 */
    PerformanceStats total_stats = {0};
    total_stats.min_latency_ms = 999999;
    
    for (int i = 0; i < client_count; i++) {
        if (clients[i].stats.total_messages_sent > 0) {
            total_stats.total_messages_sent += clients[i].stats.total_messages_sent;
            total_stats.total_messages_received += clients[i].stats.total_messages_received;
            total_stats.total_bytes_sent += clients[i].stats.total_bytes_sent;
            total_stats.total_bytes_received += clients[i].stats.total_bytes_received;
            total_stats.total_latency_ms += clients[i].stats.total_latency_ms;
            total_stats.latency_samples += clients[i].stats.latency_samples;
            
            if (clients[i].stats.min_latency_ms < total_stats.min_latency_ms) {
                total_stats.min_latency_ms = clients[i].stats.min_latency_ms;
            }
            if (clients[i].stats.max_latency_ms > total_stats.max_latency_ms) {
                total_stats.max_latency_ms = clients[i].stats.max_latency_ms;
            }
        }
    }
    
    total_stats.total_time_ms = test_end_time - test_start_time;
    
    /* 显示性能测试结果 */
    printf("\n--- 性能测试结果 ---\n");
    printf("测试时间: %.2f 秒\n", total_stats.total_time_ms / 1000.0);
    printf("总发送消息数: %lld\n", total_stats.total_messages_sent);
    printf("总接收消息数: %lld\n", total_stats.total_messages_received);
    printf("消息成功率: %.2f%%\n", 
           total_stats.total_messages_sent > 0 ? 
           (double)total_stats.total_messages_received / total_stats.total_messages_sent * 100 : 0);
    printf("总发送字节数: %lld\n", total_stats.total_bytes_sent);
    printf("总接收字节数: %lld\n", total_stats.total_bytes_received);
    
    if (total_stats.total_time_ms > 0) {
        double throughput_mbps = (total_stats.total_bytes_sent * 8.0) / (total_stats.total_time_ms / 1000.0) / 1000000.0;
        double messages_per_second = total_stats.total_messages_sent / (total_stats.total_time_ms / 1000.0);
        
        printf("吞吐量: %.2f Mbps\n", throughput_mbps);
        printf("消息速率: %.2f 消息/秒\n", messages_per_second);
    }
    
    if (total_stats.latency_samples > 0) {
        double avg_latency = total_stats.total_latency_ms / total_stats.latency_samples;
        printf("平均延迟: %.2f ms\n", avg_latency);
        printf("最小延迟: %.2f ms\n", total_stats.min_latency_ms);
        printf("最大延迟: %.2f ms\n", total_stats.max_latency_ms);
    }
    
    /* 清理资源 */
    websocket_server_stop(ws_server);
    websocket_server_destroy(ws_server);
    free(http_server);
}

/* 压力测试 */
void run_stress_test() {
    printf("\n=== 压力测试 ===\n");
    printf("配置: %d 个客户端, %d 秒持续时间\n", STRESS_TEST_CLIENTS, STRESS_TEST_DURATION);
    
    /* 创建HTTP服务器 */
    http_server = (HttpServer*)malloc(sizeof(HttpServer));
    memset(http_server, 0, sizeof(HttpServer));
    
    ws_server = websocket_server_create(http_server);
    websocket_set_handlers(ws_server, message_handler, connect_handler, disconnect_handler);
    websocket_server_start(ws_server);
    
    /* 等待服务器启动 */
    sleep(1);
    
    /* 创建压力测试客户端 */
    PerformanceClient clients[STRESS_TEST_CLIENTS];
    memset(clients, 0, sizeof(clients));
    
    /* 初始化客户端 */
    for (int i = 0; i < STRESS_TEST_CLIENTS; i++) {
        clients[i].id = i + 1;
        clients[i].socket_fd = -1;
        clients[i].message_size = 128;
        clients[i].message_count = 999999; // 持续发送
        clients[i].connected = 0;
        memset(&clients[i].stats, 0, sizeof(PerformanceStats));
        clients[i].stats.min_latency_ms = 999999;
    }
    
    /* 启动客户端线程 */
    for (int i = 0; i < STRESS_TEST_CLIENTS; i++) {
        pthread_create(&clients[i].thread, NULL, performance_client_thread, &clients[i]);
        usleep(50000); // 50ms间隔
    }
    
    printf("✅ 所有压力测试客户端已启动\n");
    
    /* 监控测试过程 */
    time_t start_time = time(NULL);
    while (server_running && (time(NULL) - start_time) < STRESS_TEST_DURATION) {
        sleep(5);
        
        /* 显示当前状态 */
        pthread_mutex_lock(&stats_mutex);
        printf("\r压力测试进行中... 已用时间: %ld 秒, 发送: %lld, 接收: %lld", 
               time(NULL) - start_time, 
               global_stats.total_messages_sent, 
               global_stats.total_messages_received);
        fflush(stdout);
        pthread_mutex_unlock(&stats_mutex);
    }
    
    /* 停止所有客户端 */
    server_running = 0;
    
    /* 等待所有客户端完成 */
    for (int i = 0; i < STRESS_TEST_CLIENTS; i++) {
        pthread_join(clients[i].thread, NULL);
    }
    
    /* 显示压力测试结果 */
    printf("\n\n--- 压力测试结果 ---\n");
    printf("测试持续时间: %d 秒\n", STRESS_TEST_DURATION);
    printf("总发送消息数: %lld\n", global_stats.total_messages_sent);
    printf("总接收消息数: %lld\n", global_stats.total_messages_received);
    printf("总发送字节数: %lld\n", global_stats.total_bytes_sent);
    printf("总接收字节数: %lld\n", global_stats.total_bytes_received);
    
    if (STRESS_TEST_DURATION > 0) {
        double throughput_mbps = (global_stats.total_bytes_sent * 8.0) / STRESS_TEST_DURATION / 1000000.0;
        double messages_per_second = global_stats.total_messages_sent / STRESS_TEST_DURATION;
        
        printf("平均吞吐量: %.2f Mbps\n", throughput_mbps);
        printf("平均消息速率: %.2f 消息/秒\n", messages_per_second);
    }
    
    /* 清理资源 */
    websocket_server_stop(ws_server);
    websocket_server_destroy(ws_server);
    free(http_server);
}

/* 主函数 */
int main() {
    printf("WebSocket性能和压力测试开始...\n\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
#ifdef _WIN32
    /* 初始化Winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("❌ WSAStartup失败\n");
        return 1;
    }
#endif
    
    /* 测试不同消息大小的性能 */
    int message_sizes[MESSAGE_SIZES] = {64, 256, 512, 1024, 4096};
    
    for (int i = 0; i < MESSAGE_SIZES; i++) {
        for (int round = 0; round < PERFORMANCE_TEST_ROUNDS; round++) {
            printf("\n第 %d 轮测试，消息大小 %d bytes\n", round + 1, message_sizes[i]);
            run_performance_test(message_sizes[i], 100, 10);
            
            if (round < PERFORMANCE_TEST_ROUNDS - 1) {
                printf("等待 3 秒后进行下一轮测试...\n");
                sleep(3);
            }
        }
    }
    
    /* 运行压力测试 */
    if (server_running) {
        run_stress_test();
    }
    
#ifdef _WIN32
    /* 清理Winsock */
    WSACleanup();
#endif
    
    printf("\nWebSocket性能和压力测试完成\n");
    return 0;
}