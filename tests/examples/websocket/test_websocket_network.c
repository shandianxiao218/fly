#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

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
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#endif

#include "src/web/websocket.h"
#include "src/utils/utils.h"

/* 全局变量 */
static int server_running = 1;
static WebSocketServer* ws_server = NULL;
static HttpServer* http_server = NULL;

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
    
    printf("收到消息来自 %s:%d\n", 
           message->connection->client_ip, 
           message->connection->client_port);
    
    if (message->data && message->data_length > 0) {
        printf("消息内容: %.*s\n", message->data_length, message->data);
        
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
    
    printf("新连接来自 %s:%d\n", 
           message->connection->client_ip, 
           message->connection->client_port);
    
    /* 发送欢迎消息 */
    const char* welcome = "欢迎连接到WebSocket服务器！";
    websocket_connection_send_text(message->connection, welcome);
    
    return 0;
}

/* WebSocket断开连接处理回调 */
int disconnect_handler(const WebSocketMessage* message) {
    if (!message || !message->connection) {
        return -1;
    }
    
    printf("连接断开: %s:%d\n", 
           message->connection->client_ip, 
           message->connection->client_port);
    
    return 0;
}

/* 简单的WebSocket客户端 */
typedef struct {
    int socket_fd;
    char server_ip[64];
    int server_port;
} WebSocketClient;

/* 创建WebSocket客户端 */
WebSocketClient* websocket_client_create(const char* server_ip, int server_port) {
    WebSocketClient* client = (WebSocketClient*)malloc(sizeof(WebSocketClient));
    if (!client) {
        return NULL;
    }
    
    client->socket_fd = -1;
    strncpy(client->server_ip, server_ip, sizeof(client->server_ip) - 1);
    client->server_port = server_port;
    
    return client;
}

/* 销毁WebSocket客户端 */
void websocket_client_destroy(WebSocketClient* client) {
    if (!client) {
        return;
    }
    
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
    }
    
    free(client);
}

/* 连接到WebSocket服务器 */
int websocket_client_connect(WebSocketClient* client) {
    if (!client) {
        return -1;
    }
    
    /* 创建套接字 */
    client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_fd < 0) {
        printf("❌ 创建套接字失败\n");
        return -1;
    }
    
    /* 设置服务器地址 */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(client->server_port);
    
    if (inet_pton(AF_INET, client->server_ip, &server_addr.sin_addr) <= 0) {
        printf("❌ 无效的IP地址\n");
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    /* 连接到服务器 */
    if (connect(client->socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("❌ 连接到服务器失败\n");
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    printf("✅ 连接到服务器 %s:%d\n", client->server_ip, client->server_port);
    return 0;
}

/* 发送WebSocket握手请求 */
int websocket_client_handshake(WebSocketClient* client) {
    if (!client || client->socket_fd < 0) {
        return -1;
    }
    
    /* 发送握手请求 */
    const char* handshake_request = 
        "GET /ws HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";
    
    if (send(client->socket_fd, handshake_request, strlen(handshake_request), 0) < 0) {
        printf("❌ 发送握手请求失败\n");
        return -1;
    }
    
    printf("✅ 发送握手请求\n");
    
    /* 接收握手响应 */
    char response[1024];
    int response_len = recv(client->socket_fd, response, sizeof(response) - 1, 0);
    if (response_len <= 0) {
        printf("❌ 接收握手响应失败\n");
        return -1;
    }
    
    response[response_len] = '\0';
    printf("握手响应: %.*s\n", response_len, response);
    
    /* 检查是否成功 */
    if (strstr(response, "101 Switching Protocols") == NULL) {
        printf("❌ 握手失败\n");
        return -1;
    }
    
    printf("✅ 握手成功\n");
    return 0;
}

/* 发送WebSocket消息 */
int websocket_client_send_text(WebSocketClient* client, const char* message) {
    if (!client || client->socket_fd < 0 || !message) {
        return -1;
    }
    
    /* 创建WebSocket帧 */
    char frame[1024];
    int message_len = strlen(message);
    
    /* 简单的文本帧 */
    frame[0] = 0x81; // FIN + TEXT
    if (message_len < 126) {
        frame[1] = message_len;
        memcpy(frame + 2, message, message_len);
        return send(client->socket_fd, frame, 2 + message_len, 0);
    } else {
        printf("❌ 消息过长\n");
        return -1;
    }
}

/* 接收WebSocket消息 */
int websocket_client_receive(WebSocketClient* client, char* buffer, int buffer_size) {
    if (!client || client->socket_fd < 0 || !buffer || buffer_size <= 0) {
        return -1;
    }
    
    int received = recv(client->socket_fd, buffer, buffer_size - 1, 0);
    if (received <= 0) {
        return received;
    }
    
    buffer[received] = '\0';
    
    /* 简单的帧解析 */
    if (received >= 2 && (buffer[0] & 0x80) && (buffer[0] & 0x0F) == 0x01) {
        /* 文本帧 */
        int payload_len = buffer[1] & 0x7F;
        if (payload_len < 126 && received >= 2 + payload_len) {
            memmove(buffer, buffer + 2, payload_len);
            buffer[payload_len] = '\0';
            return payload_len;
        }
    }
    
    return received;
}

/* 测试WebSocket服务器和客户端通信 */
void test_websocket_network_communication() {
    printf("=== WebSocket网络通信测试 ===\n");
    
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
    sleep(1);
    
    /* 创建客户端 */
    WebSocketClient* client = websocket_client_create("127.0.0.1", 8080);
    if (!client) {
        printf("❌ 创建客户端失败\n");
        websocket_server_stop(ws_server);
        websocket_server_destroy(ws_server);
        free(http_server);
        return;
    }
    
    /* 连接客户端 */
    if (websocket_client_connect(client) != 0) {
        printf("❌ 客户端连接失败\n");
        websocket_client_destroy(client);
        websocket_server_stop(ws_server);
        websocket_server_destroy(ws_server);
        free(http_server);
        return;
    }
    
    /* 执行握手 */
    if (websocket_client_handshake(client) != 0) {
        printf("❌ 客户端握手失败\n");
        websocket_client_destroy(client);
        websocket_server_stop(ws_server);
        websocket_server_destroy(ws_server);
        free(http_server);
        return;
    }
    
    printf("✅ 客户端连接成功\n");
    
    /* 测试消息发送 */
    const char* test_messages[] = {
        "Hello, WebSocket!",
        "测试消息1",
        "测试消息2",
        "这是一条较长的测试消息，用于验证WebSocket通信的稳定性",
        "最后一条消息"
    };
    
    for (int i = 0; i < 5; i++) {
        printf("发送消息 %d: %s\n", i + 1, test_messages[i]);
        
        if (websocket_client_send_text(client, test_messages[i]) > 0) {
            printf("✅ 消息发送成功\n");
            
            /* 等待响应 */
            char response[1024];
            int response_len = websocket_client_receive(client, response, sizeof(response));
            if (response_len > 0) {
                printf("收到响应: %s\n", response);
            } else {
                printf("❌ 未收到响应\n");
            }
        } else {
            printf("❌ 消息发送失败\n");
        }
        
        sleep(1);
    }
    
    /* 清理资源 */
    printf("清理资源...\n");
    websocket_client_destroy(client);
    websocket_server_stop(ws_server);
    websocket_server_destroy(ws_server);
    free(http_server);
    
    printf("✅ 网络通信测试完成\n");
}

/* 主函数 */
int main() {
    printf("WebSocket网络通信测试开始...\n\n");
    
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
    
    /* 运行网络通信测试 */
    test_websocket_network_communication();
    
#ifdef _WIN32
    /* 清理Winsock */
    WSACleanup();
#endif
    
    printf("WebSocket网络通信测试完成\n");
    return 0;
}