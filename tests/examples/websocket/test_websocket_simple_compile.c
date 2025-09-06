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

/* 简单的WebSocket编译测试 */
int main() {
    printf("🚀 开始WebSocket编译测试\n\n");
    
    /* 初始化日志系统 */
    logger_init("websocket_compilation_test.log", LOG_LEVEL_DEBUG);
    
    /* 测试1: 创建和销毁WebSocket服务器 */
    printf("测试1: 创建WebSocket服务器...\n");
    
    /* 创建一个简单的HTTP服务器结构用于测试 */
    typedef struct {
        int dummy;
    } TestHttpServer;
    
    TestHttpServer* http_server = (TestHttpServer*)malloc(sizeof(TestHttpServer));
    if (http_server) {
        memset(http_server, 0, sizeof(TestHttpServer));
        
        WebSocketServer* ws_server = websocket_server_create((HttpServer*)http_server);
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
            
            /* 测试帧处理 */
            printf("测试3: 帧处理功能...\n");
            char test_payload[] = "Hello WebSocket";
            char frame_buffer[1024];
            int frame_length = websocket_frame_create(WS_FRAME_TEXT, test_payload, strlen(test_payload), frame_buffer, sizeof(frame_buffer));
            if (frame_length > 0) {
                printf("✅ WebSocket帧创建成功，长度: %d\n", frame_length);
                
                /* 测试帧解析 */
                WebSocketFrameHeader header;
                char* parsed_payload = NULL;
                if (websocket_frame_parse(frame_buffer, frame_length, &header, &parsed_payload)) {
                    printf("✅ WebSocket帧解析成功\n");
                    printf("   - 类型: %s\n", websocket_frame_type_to_string(header.opcode));
                    printf("   - 载荷长度: %d\n", header.payload_len);
                    printf("   - 载荷内容: %s\n", parsed_payload);
                    if (parsed_payload) {
                        safe_free((void**)&parsed_payload);
                    }
                } else {
                    printf("❌ WebSocket帧解析失败\n");
                }
            } else {
                printf("❌ WebSocket帧创建失败\n");
            }
            
            /* 测试工具函数 */
            printf("测试4: 工具函数...\n");
            printf("   - Base64编码测试...\n");
            char base64_input[] = "Hello WebSocket";
            char base64_output[256];
            int base64_length = websocket_base64_encode((unsigned char*)base64_input, strlen(base64_input), base64_output, sizeof(base64_output));
            if (base64_length > 0) {
                printf("   ✅ Base64编码成功: %s\n", base64_output);
            } else {
                printf("   ❌ Base64编码失败\n");
            }
            
            printf("   - SHA1哈希测试...\n");
            unsigned char sha1_output[20];
            if (websocket_sha1_hash("test", 4, sha1_output)) {
                printf("   ✅ SHA1哈希计算成功\n");
            } else {
                printf("   ❌ SHA1哈希计算失败\n");
            }
            
            /* 测试消息处理 */
            printf("测试5: 消息处理...\n");
            WebSocketMessage message;
            memset(&message, 0, sizeof(message));
            if (websocket_message_create(&message, WS_FRAME_TEXT, "Test Message", 12)) {
                printf("✅ WebSocket消息创建成功\n");
                printf("   - 类型: %s\n", websocket_message_type_to_string(message.msg_type));
                printf("   - 数据: %s\n", message.data);
                websocket_message_destroy(&message);
            } else {
                printf("❌ WebSocket消息创建失败\n");
            }
            
            /* 测试服务器统计 */
            printf("测试6: 统计信息...\n");
            int connection_count = websocket_get_connection_count(ws_server);
            int total_connections = websocket_get_total_connections(ws_server);
            int sent_count, received_count;
            websocket_get_total_messages(ws_server, &sent_count, &received_count);
            
            printf("   - 当前连接数: %d\n", connection_count);
            printf("   - 总连接数: %d\n", total_connections);
            printf("   - 发送消息数: %d\n", sent_count);
            printf("   - 接收消息数: %d\n", received_count);
            
            /* 测试广播功能 */
            printf("测试7: 广播功能...\n");
            const char* broadcast_message = "Hello from broadcast";
            int broadcast_result = websocket_broadcast_text(ws_server, broadcast_message);
            printf("   - 广播结果: %d (预期为0，因为没有连接)\n", broadcast_result);
            
            /* 测试类型转换函数 */
            printf("测试8: 类型转换函数...\n");
            printf("   - 帧类型: %s\n", websocket_frame_type_to_string(WS_FRAME_TEXT));
            printf("   - 连接状态: %s\n", websocket_connection_state_to_string(WS_STATE_OPEN));
            printf("   - 消息类型: %s\n", websocket_message_type_to_string(WS_MESSAGE_DATA));
            
            websocket_server_destroy(ws_server);
        } else {
            printf("❌ WebSocket服务器创建失败\n");
        }
        free(http_server);
    } else {
        printf("❌ HTTP服务器内存分配失败\n");
    }
    
    /* 清理日志系统 */
    logger_cleanup();
    
    printf("\n🎉 WebSocket编译测试完成\n");
    return 0;
}