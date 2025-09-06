#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#define usleep(x) Sleep((x) / 1000)
#define sleep(x) Sleep((x) * 1000)
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

/* 简单的WebSocket测试函数 */
void test_websocket_basic(void) {
    printf("=== WebSocket基本功能测试 ===\n");
    
    /* 测试1: 创建和销毁WebSocket服务器 */
    printf("测试1: 创建WebSocket服务器...\n");
    HttpServer* http_server = (HttpServer*)malloc(sizeof(HttpServer));
    if (http_server) {
        memset(http_server, 0, sizeof(HttpServer));
        
        WebSocketServer* ws_server = websocket_server_create(http_server);
        if (ws_server) {
            printf("✅ WebSocket服务器创建成功\n");
            
            /* 测试连接管理 */
            printf("测试2: 连接管理功能...\n");
            WebSocketConnection* conn = websocket_connection_create(-1, "127.0.0.1", 8080);
            if (conn) {
                printf("✅ WebSocket连接创建成功\n");
                websocket_connection_destroy(conn);
            } else {
                printf("❌ WebSocket连接创建失败\n");
            }
            
            websocket_server_destroy(ws_server);
            printf("✅ WebSocket服务器销毁成功\n");
        } else {
            printf("❌ WebSocket服务器创建失败\n");
        }
        
        free(http_server);
    } else {
        printf("❌ HTTP服务器创建失败\n");
    }
    
    printf("\n");
}

/* 测试WebSocket帧处理 */
void test_websocket_frame(void) {
    printf("=== WebSocket帧处理测试 ===\n");
    
    /* 测试帧创建 */
    printf("测试1: 帧创建功能...\n");
    const char* test_message = "Hello, WebSocket!";
    char frame_buffer[1024];
    
    int frame_length = websocket_frame_create(WS_FRAME_TEXT, test_message, strlen(test_message), 
                                           frame_buffer, sizeof(frame_buffer));
    
    if (frame_length > 0) {
        printf("✅ WebSocket帧创建成功，长度: %d\n", frame_length);
        
        /* 测试帧解析 */
        printf("测试2: 帧解析功能...\n");
        WebSocketFrameHeader header;
        char* payload = NULL;
        
        if (websocket_frame_parse(frame_buffer, frame_length, &header, &payload)) {
            printf("✅ WebSocket帧解析成功\n");
            printf("  - 帧类型: %d\n", header.opcode);
            printf("  - 载荷长度: %d\n", header.payload_len);
            printf("  - FIN位: %d\n", header.fin);
            
            if (payload) {
                printf("  - 载荷内容: %s\n", payload);
                if (strcmp(payload, test_message) == 0) {
                    printf("✅ 载荷内容匹配\n");
                } else {
                    printf("❌ 载荷内容不匹配\n");
                }
                free(payload);
            }
        } else {
            printf("❌ WebSocket帧解析失败\n");
        }
    } else {
        printf("❌ WebSocket帧创建失败\n");
    }
    
    printf("\n");
}

/* 测试Base64编码/解码 */
void test_websocket_base64(void) {
    printf("=== Base64编码/解码测试 ===\n");
    
    const char* test_string = "Hello, WebSocket!";
    const int test_length = strlen(test_string);
    
    /* 测试编码 */
    printf("测试1: Base64编码...\n");
    char encoded[256];
    int encoded_length = websocket_base64_encode((const unsigned char*)test_string, test_length, 
                                                  encoded, sizeof(encoded));
    
    if (encoded_length > 0) {
        printf("✅ Base64编码成功: %s\n", encoded);
        
        /* 测试解码 */
        printf("测试2: Base64解码...\n");
        unsigned char decoded[256];
        int decoded_length = websocket_base64_decode(encoded, encoded_length, decoded, sizeof(decoded));
        
        if (decoded_length > 0) {
            decoded[decoded_length] = '\0';
            printf("✅ Base64解码成功: %s\n", decoded);
            
            if (strcmp((const char*)decoded, test_string) == 0) {
                printf("✅ 编码解码结果匹配\n");
            } else {
                printf("❌ 编码解码结果不匹配\n");
            }
        } else {
            printf("❌ Base64解码失败\n");
        }
    } else {
        printf("❌ Base64编码失败\n");
    }
    
    printf("\n");
}

/* 测试SHA1哈希 */
void test_websocket_sha1(void) {
    printf("=== SHA1哈希测试 ===\n");
    
    const char* test_string = "Hello, WebSocket!";
    const int test_length = strlen(test_string);
    
    printf("测试1: SHA1哈希计算...\n");
    unsigned char hash[20];
    
    if (websocket_sha1_hash(test_string, test_length, hash)) {
        printf("✅ SHA1哈希计算成功\n");
        
        /* 输出哈希值 */
        printf("  - 哈希值: ");
        for (int i = 0; i < 20; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n");
    } else {
        printf("❌ SHA1哈希计算失败\n");
    }
    
    printf("\n");
}

/* 测试WebSocket握手 */
void test_websocket_handshake(void) {
    printf("=== WebSocket握手测试 ===\n");
    
    /* 模拟WebSocket握手请求 */
    const char* handshake_request = 
        "GET /ws HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";
    
    printf("测试1: 握手验证...\n");
    if (websocket_validate_handshake(handshake_request)) {
        printf("✅ WebSocket握手验证成功\n");
        
        /* 测试握手处理 */
        printf("测试2: 握手处理...\n");
        HttpRequest* request = http_request_create();
        HttpResponse* response = http_response_create();
        
        if (request && response) {
            /* 解析请求 */
            if (http_request_parse(handshake_request, request)) {
                printf("✅ HTTP请求解析成功\n");
                
                if (websocket_handshake(request, response)) {
                    printf("✅ WebSocket握手处理成功\n");
                    printf("  - 状态码: %d\n", response->status_code);
                    printf("  - 响应长度: %d\n", response->content_length);
                } else {
                    printf("❌ WebSocket握手处理失败\n");
                }
            } else {
                printf("❌ HTTP请求解析失败\n");
            }
            
            http_request_destroy(request);
            http_response_destroy(response);
        } else {
            printf("❌ HTTP请求/响应创建失败\n");
        }
    } else {
        printf("❌ WebSocket握手验证失败\n");
    }
    
    printf("\n");
}

/* 主测试函数 */
int main() {
    printf("WebSocket功能测试开始...\n\n");
    
    /* 初始化 */
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("❌ WSAStartup失败\n");
        return 1;
    }
#endif
    
    /* 运行测试 */
    test_websocket_basic();
    test_websocket_frame();
    test_websocket_base64();
    test_websocket_sha1();
    test_websocket_handshake();
    
    /* 清理 */
#ifdef _WIN32
    WSACleanup();
#endif
    
    printf("WebSocket功能测试完成\n");
    return 0;
}