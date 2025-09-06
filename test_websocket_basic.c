#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pthread.h>
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

/* 简单的Base64编码测试 */
void test_base64(void) {
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

/* 简单的SHA1哈希测试 */
void test_sha1(void) {
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

/* 简单的帧处理测试 */
void test_frame(void) {
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

/* 主测试函数 */
int main() {
    printf("WebSocket基础功能测试开始...\n\n");
    
    /* 初始化 */
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("❌ WSAStartup失败\n");
        return 1;
    }
#endif
    
    /* 运行测试 */
    test_base64();
    test_sha1();
    test_frame();
    
    /* 清理 */
#ifdef _WIN32
    WSACleanup();
#endif
    
    printf("WebSocket基础功能测试完成\n");
    return 0;
}