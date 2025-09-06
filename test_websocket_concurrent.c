#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

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

/* 测试配置 */
#define MAX_CLIENTS 50
#define TEST_MESSAGES_PER_CLIENT 10
#define SERVER_PORT 8080
#define TEST_DURATION_SECONDS 30

/* 全局变量 */
static int server_running = 1;
static WebSocketServer* ws_server = NULL;
static HttpServer* http_server = NULL;
static int clients_connected = 0;
static int messages_sent = 0;
static int messages_received = 0;
static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 客户端数据结构 */
typedef struct {
    int id;
    int socket_fd;
    pthread_t thread;
    int messages_sent;
    int messages_received;
    int connected;
    char client_ip[64];
} ClientData;

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
    messages_received++;
    pthread_mutex_unlock(&stats_mutex);
    
    if (message->data && message->data_length > 0) {
        /* 解析客户端ID */
        int client_id = -1;
        if (sscanf(message->data, "Client %d:", &client_id) == 1) {
            printf("收到来自客户端 %d 的消息\n", client_id);
        }
        
        /* 回显消息 */
        websocket_connection_send_text(message->connection, message->data);
    }
    
    return 0;
}

/* WebSocket连接处理回调 */
int connect_handler(const WebSocketMessage* message) {
    if (!message || !message->connection) {
        return -1;
    }
    
    pthread_mutex_lock(&stats_mutex);
    clients_connected++;
    pthread_mutex_unlock(&stats_mutex);
    
    printf("新连接来自 %s:%d (总连接数: %d)\n", 
           message->connection->client_ip, 
           message->connection->client_port,
           clients_connected);
    
    return 0;
}

/* WebSocket断开连接处理回调 */
int disconnect_handler(const WebSocketMessage* message) {
    if (!message || !message->connection) {
        return -1;
    }
    
    pthread_mutex_lock(&stats_mutex);
    clients_connected--;
    pthread_mutex_unlock(&stats_mutex);
    
    printf("连接断开: %s:%d (剩余连接数: %d)\n", 
           message->connection->client_ip, 
           message->connection->client_port,
           clients_connected);
    
    return 0;
}

/* 客户端线程函数 */
void* client_thread(void* arg) {
    ClientData* client = (ClientData*)arg;
    if (!client) {
        return NULL;
    }
    
    printf("客户端 %d 开始运行\n", client->id);
    
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
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    /* 连接到服务器 */
    if (connect(client->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("客户端 %d: 连接失败\n", client->id);
        close(client->socket_fd);
        return NULL;
    }
    
    printf("客户端 %d: 连接成功\n", client->id);
    
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
        printf("客户端 %d: 握手请求发送失败\n", client->id);
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
    printf("客户端 %d: 握手成功\n", client->id);
    
    /* 发送测试消息 */
    for (int i = 0; i < TEST_MESSAGES_PER_CLIENT && server_running; i++) {
        char message[256];
        snprintf(message, sizeof(message), "Client %d: Message %d/%d", 
                client->id, i + 1, TEST_MESSAGES_PER_CLIENT);
        
        /* 创建WebSocket帧 */
        char frame[512];
        int msg_len = strlen(message);
        
        frame[0] = 0x81; // FIN + TEXT
        frame[1] = msg_len;
        memcpy(frame + 2, message, msg_len);
        
        if (send(client->socket_fd, frame, 2 + msg_len, 0) > 0) {
            client->messages_sent++;
            pthread_mutex_lock(&stats_mutex);
            messages_sent++;
            pthread_mutex_unlock(&stats_mutex);
            
            printf("客户端 %d: 发送消息 %d\n", client->id, i + 1);
            
            /* 等待响应 */
            char recv_buffer[1024];
            int recv_len = recv(client->socket_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
            if (recv_len > 0) {
                client->messages_received++;
                
                /* 简单解析响应 */
                if (recv_len >= 2 && (recv_buffer[0] & 0x80) && (recv_buffer[0] & 0x0F) == 0x01) {
                    int payload_len = recv_buffer[1] & 0x7F;
                    if (payload_len < 126 && recv_len >= 2 + payload_len) {
                        recv_buffer[2 + payload_len] = '\0';
                        printf("客户端 %d: 收到响应: %s\n", client->id, recv_buffer + 2);
                    }
                }
            }
        }
        
        /* 随机延迟 */
        usleep(rand() % 500000 + 100000); // 100-600ms
    }
    
    /* 关闭连接 */
    close(client->socket_fd);
    client->connected = 0;
    printf("客户端 %d: 连接关闭\n", client->id);
    
    return NULL;
}

/* 测试WebSocket多客户端并发连接 */
void test_websocket_concurrent_clients() {
    printf("=== WebSocket多客户端并发测试 ===\n");
    printf("配置: %d 个客户端, 每个客户端 %d 条消息\n", MAX_CLIENTS, TEST_MESSAGES_PER_CLIENT);
    
    /* 创建HTTP服务器 */
    http_server = (HttpServer*)malloc(sizeof(HttpServer));
    if (!http_server) {
        printf("❌ 创建HTTP服务器失败\n");
        return;
    }
    
    memset(http_server, 0, sizeof(HttpServer));
    
    /* 创建WebSocket服务器 */
    ws_server = websocket_server_create(http_server);
    if (!ws_server) {
        printf("❌ 创建WebSocket服务器失败\n");
        free(http_server);
        return;
    }
    
    /* 设置回调函数 */
    websocket_set_handlers(ws_server, message_handler, connect_handler, disconnect_handler);
    
    printf("✅ WebSocket服务器创建成功\n");
    
    /* 启动服务器 */
    if (websocket_server_start(ws_server) != 0) {
        printf("❌ 启动WebSocket服务器失败\n");
        websocket_server_destroy(ws_server);
        free(http_server);
        return;
    }
    
    printf("✅ WebSocket服务器启动成功\n");
    
    /* 等待服务器启动 */
    sleep(2);
    
    /* 创建客户端 */
    ClientData clients[MAX_CLIENTS];
    memset(clients, 0, sizeof(clients));
    
    /* 初始化客户端 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].id = i + 1;
        clients[i].socket_fd = -1;
        clients[i].connected = 0;
        clients[i].messages_sent = 0;
        clients[i].messages_received = 0;
    }
    
    /* 启动客户端线程 */
    printf("启动 %d 个客户端线程...\n", MAX_CLIENTS);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (pthread_create(&clients[i].thread, NULL, client_thread, &clients[i]) != 0) {
            printf("❌ 创建客户端 %d 线程失败\n", i + 1);
            continue;
        }
        
        /* 错开启动时间 */
        usleep(rand() % 100000 + 50000); // 50-150ms
    }
    
    printf("✅ 所有客户端线程已启动\n");
    
    /* 等待测试完成 */
    time_t start_time = time(NULL);
    while (server_running && (time(NULL) - start_time) < TEST_DURATION_SECONDS) {
        sleep(1);
        
        /* 显示统计信息 */
        pthread_mutex_lock(&stats_mutex);
        printf("\r当前状态: 连接数=%d, 发送消息=%d, 接收消息=%d", 
               clients_connected, messages_sent, messages_received);
        fflush(stdout);
        pthread_mutex_unlock(&stats_mutex);
    }
    
    printf("\n\n等待所有客户端完成...\n");
    
    /* 等待所有客户端线程完成 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_join(clients[i].thread, NULL);
    }
    
    /* 显示最终统计信息 */
    printf("\n=== 测试结果统计 ===\n");
    int total_sent = 0;
    int total_received = 0;
    int successful_clients = 0;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].messages_sent > 0) {
            successful_clients++;
            total_sent += clients[i].messages_sent;
            total_received += clients[i].messages_received;
            
            printf("客户端 %2d: 发送=%2d, 接收=%2d, 成功率=%.1f%%\n", 
                   clients[i].id, 
                   clients[i].messages_sent, 
                   clients[i].messages_received,
                   clients[i].messages_sent > 0 ? 
                   (float)clients[i].messages_received / clients[i].messages_sent * 100 : 0);
        }
    }
    
    printf("\n=== 总体统计 ===\n");
    printf("总客户端数: %d\n", MAX_CLIENTS);
    printf("成功客户端数: %d\n", successful_clients);
    printf("总发送消息数: %d\n", total_sent);
    printf("总接收消息数: %d\n", total_received);
    printf("消息成功率: %.1f%%\n", total_sent > 0 ? (float)total_received / total_sent * 100 : 0);
    printf("客户端成功率: %.1f%%\n", (float)successful_clients / MAX_CLIENTS * 100);
    
    /* 清理资源 */
    printf("\n清理资源...\n");
    websocket_server_stop(ws_server);
    websocket_server_destroy(ws_server);
    free(http_server);
    
    printf("✅ 多客户端并发测试完成\n");
}

/* 主函数 */
int main() {
    printf("WebSocket多客户端并发测试开始...\n\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 初始化随机数种子 */
    srand(time(NULL));
    
#ifdef _WIN32
    /* 初始化Winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("❌ WSAStartup失败\n");
        return 1;
    }
#endif
    
    /* 运行多客户端并发测试 */
    test_websocket_concurrent_clients();
    
#ifdef _WIN32
    /* 清理Winsock */
    WSACleanup();
#endif
    
    printf("WebSocket多客户端并发测试完成\n");
    return 0;
}