#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

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

/* 简化的WebSocket头文件 */
#define WebSocket_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WebSocket_MAX_FRAME_SIZE 65536
#define WebSocket_BUFFER_SIZE 8192

/* WebSocket帧类型 */
typedef enum {
    WS_FRAME_CONTINUATION = 0x0,
    WS_FRAME_TEXT = 0x1,
    WS_FRAME_BINARY = 0x2,
    WS_FRAME_CLOSE = 0x8,
    WS_FRAME_PING = 0x9,
    WS_FRAME_PONG = 0xA
} WebSocketFrameType;

/* WebSocket帧头 */
typedef struct {
    unsigned char fin : 1;
    unsigned char rsv1 : 1;
    unsigned char rsv2 : 1;
    unsigned char rsv3 : 1;
    unsigned char opcode : 4;
    unsigned char mask : 1;
    unsigned char payload_len : 7;
    unsigned char extended_len[8];
    unsigned char masking_key[4];
} WebSocketFrameHeader;

/* 工具函数 */
int websocket_base64_encode(const unsigned char* input, int input_length, char* output, int output_size) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    if (input_length <= 0 || input == NULL || output == NULL || output_size <= 0) {
        return -1;
    }
    
    int output_length = ((input_length + 2) / 3) * 4;
    if (output_length >= output_size) {
        return -1;
    }
    
    int i = 0, j = 0;
    int char_array_3[3], char_array_4[4];
    
    while (input_length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++) {
                output[j++] = base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }
    
    if (i) {
        for (int k = i; k < 3; k++) {
            char_array_3[k] = '\0';
        }
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (int k = 0; k < i + 1; k++) {
            output[j++] = base64_chars[char_array_4[k]];
        }
        
        while (i++ < 3) {
            output[j++] = '=';
        }
    }
    
    output[j] = '\0';
    return j;
}

int websocket_base64_decode(const char* input, int input_length, unsigned char* output, int output_size) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    if (input_length <= 0 || input == NULL || output == NULL || output_size <= 0) {
        return -1;
    }
    
    int i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    while (input_length-- && input[in_] != '=' && input[in_] != '\n' && input[in_] != '\r') {
        char_array_4[i++] = input[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                char_array_4[i] = strchr(base64_chars, char_array_4[i]) - base64_chars;
            }
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; i < 3; i++) {
                if (j >= output_size) return -1;
                output[j++] = char_array_3[i];
            }
            i = 0;
        }
    }
    
    if (i) {
        int k;
        for (k = i; k < 4; k++) {
            char_array_4[k] = '\0';
        }
        
        for (k = 0; k < 4; k++) {
            char_array_4[k] = strchr(base64_chars, char_array_4[k]) - base64_chars;
        }
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        
        for (k = 0; k < i - 1; k++) {
            if (j >= output_size) return -1;
            output[j++] = char_array_3[k];
        }
    }
    
    output[j] = '\0';
    return j;
}

int websocket_sha1_hash(const char* input, int input_length, unsigned char* output) {
    /* 简化的SHA1实现 - 实际项目中应该使用OpenSSL或其他加密库 */
    if (input == NULL || input_length <= 0 || output == NULL) {
        return -1;
    }
    
    /* 这里只是示例，实际应该实现完整的SHA1算法 */
    memset(output, 0, 20);
    for (int i = 0; i < 20 && i < input_length; i++) {
        output[i] = (unsigned char)input[i];
    }
    
    return 0;
}

int websocket_frame_create(WebSocketFrameType frame_type, const char* payload, int payload_length, 
                         char* frame_buffer, int frame_buffer_size) {
    if (payload == NULL || payload_length < 0 || frame_buffer == NULL || frame_buffer_size <= 0) {
        return -1;
    }
    
    int frame_length = 2 + payload_length;
    if (frame_length > frame_buffer_size) {
        return -1;
    }
    
    /* 创建帧头 */
    frame_buffer[0] = 0x80 | frame_type; // FIN + opcode
    
    if (payload_length < 126) {
        frame_buffer[1] = payload_length;
        memcpy(frame_buffer + 2, payload, payload_length);
        return frame_length;
    } else if (payload_length < 65536) {
        frame_length = 4 + payload_length;
        if (frame_length > frame_buffer_size) {
            return -1;
        }
        frame_buffer[1] = 126;
        frame_buffer[2] = (payload_length >> 8) & 0xFF;
        frame_buffer[3] = payload_length & 0xFF;
        memcpy(frame_buffer + 4, payload, payload_length);
        return frame_length;
    } else {
        frame_length = 10 + payload_length;
        if (frame_length > frame_buffer_size) {
            return -1;
        }
        frame_buffer[1] = 127;
        /* 64位长度 */
        for (int i = 0; i < 8; i++) {
            frame_buffer[2 + i] = (payload_length >> (56 - i * 8)) & 0xFF;
        }
        memcpy(frame_buffer + 10, payload, payload_length);
        return frame_length;
    }
}

int websocket_frame_parse(const char* frame_data, int frame_length, WebSocketFrameHeader* header, char** payload) {
    if (frame_data == NULL || frame_length < 2 || header == NULL) {
        return -1;
    }
    
    /* 解析帧头 */
    unsigned char first_byte = frame_data[0];
    unsigned char second_byte = frame_data[1];
    
    header->fin = (first_byte >> 7) & 1;
    header->rsv1 = (first_byte >> 6) & 1;
    header->rsv2 = (first_byte >> 5) & 1;
    header->rsv3 = (first_byte >> 4) & 1;
    header->opcode = first_byte & 0x0F;
    
    header->mask = (second_byte >> 7) & 1;
    header->payload_len = second_byte & 0x7F;
    
    int payload_start = 2;
    int payload_length = header->payload_len;
    
    /* 处理扩展长度 */
    if (header->payload_len == 126) {
        if (frame_length < 4) return -1;
        payload_length = (frame_data[2] << 8) | frame_data[3];
        payload_start = 4;
    } else if (header->payload_len == 127) {
        if (frame_length < 10) return -1;
        payload_length = 0;
        for (int i = 0; i < 8; i++) {
            payload_length = (payload_length << 8) | frame_data[2 + i];
        }
        payload_start = 10;
    }
    
    /* 处理掩码 */
    if (header->mask) {
        if (frame_length < payload_start + 4) return -1;
        memcpy(header->masking_key, frame_data + payload_start, 4);
        payload_start += 4;
    }
    
    /* 检查帧长度 */
    if (frame_length < payload_start + payload_length) {
        return -1;
    }
    
    /* 提取载荷 */
    if (payload_length > 0 && payload != NULL) {
        *payload = (char*)malloc(payload_length + 1);
        if (*payload == NULL) {
            return -1;
        }
        
        memcpy(*payload, frame_data + payload_start, payload_length);
        (*payload)[payload_length] = '\0';
        
        /* 解掩码 */
        if (header->mask) {
            for (int i = 0; i < payload_length; i++) {
                (*payload)[i] ^= header->masking_key[i % 4];
            }
        }
    }
    
    return 1;
}

/* 测试Base64编码/解码 */
void test_base64() {
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

/* 测试WebSocket帧处理 */
void test_frame_handling() {
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

/* 测试不同长度的消息 */
void test_different_message_sizes() {
    printf("=== 不同长度消息测试 ===\n");
    
    const char* test_messages[] = {
        "Short message",
        "This is a medium length message that should still fit in a single WebSocket frame without any issues",
        "This is a very long message that should test the extended length handling capabilities of our WebSocket implementation. It contains more than 125 characters but less than 65536 characters, so it should use the 16-bit extended length field."
    };
    
    const int message_count = sizeof(test_messages) / sizeof(test_messages[0]);
    
    for (int i = 0; i < message_count; i++) {
        printf("测试消息 %d (长度: %zu)...\n", i + 1, strlen(test_messages[i]));
        
        char frame_buffer[8192];
        int frame_length = websocket_frame_create(WS_FRAME_TEXT, test_messages[i], strlen(test_messages[i]), 
                                               frame_buffer, sizeof(frame_buffer));
        
        if (frame_length > 0) {
            printf("✅ 消息 %d 帧创建成功\n", i + 1);
            
            /* 解析验证 */
            WebSocketFrameHeader header;
            char* payload = NULL;
            
            if (websocket_frame_parse(frame_buffer, frame_length, &header, &payload)) {
                if (payload && strcmp(payload, test_messages[i]) == 0) {
                    printf("✅ 消息 %d 解析成功\n", i + 1);
                } else {
                    printf("❌ 消息 %d 解析失败\n", i + 1);
                }
                if (payload) free(payload);
            } else {
                printf("❌ 消息 %d 帧解析失败\n", i + 1);
            }
        } else {
            printf("❌ 消息 %d 帧创建失败\n", i + 1);
        }
    }
    
    printf("\n");
}

/* 测试边界条件 */
void test_boundary_conditions() {
    printf("=== 边界条件测试 ===\n");
    
    /* 测试空消息 */
    printf("测试1: 空消息...\n");
    char frame_buffer[1024];
    int frame_length = websocket_frame_create(WS_FRAME_TEXT, "", 0, frame_buffer, sizeof(frame_buffer));
    
    if (frame_length > 0) {
        printf("✅ 空消息帧创建成功\n");
        
        WebSocketFrameHeader header;
        char* payload = NULL;
        
        if (websocket_frame_parse(frame_buffer, frame_length, &header, &payload)) {
            printf("✅ 空消息帧解析成功\n");
            if (payload) free(payload);
        } else {
            printf("❌ 空消息帧解析失败\n");
        }
    } else {
        printf("❌ 空消息帧创建失败\n");
    }
    
    /* 测试最大载荷 */
    printf("测试2: 最大载荷...\n");
    char large_payload[125];
    memset(large_payload, 'A', 124);
    large_payload[124] = '\0';
    
    frame_length = websocket_frame_create(WS_FRAME_TEXT, large_payload, 125, frame_buffer, sizeof(frame_buffer));
    
    if (frame_length > 0) {
        printf("✅ 最大载荷帧创建成功\n");
        
        WebSocketFrameHeader header;
        char* payload = NULL;
        
        if (websocket_frame_parse(frame_buffer, frame_length, &header, &payload)) {
            printf("✅ 最大载荷帧解析成功\n");
            if (payload && strcmp(payload, large_payload) == 0) {
                printf("✅ 最大载荷内容匹配\n");
            } else {
                printf("❌ 最大载荷内容不匹配\n");
            }
            if (payload) free(payload);
        } else {
            printf("❌ 最大载荷帧解析失败\n");
        }
    } else {
        printf("❌ 最大载荷帧创建失败\n");
    }
    
    printf("\n");
}

/* 主函数 */
int main() {
    printf("WebSocket核心功能测试开始...\n\n");
    
#ifdef _WIN32
    /* 初始化Winsock */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("❌ WSAStartup失败\n");
        return 1;
    }
#endif
    
    /* 运行测试 */
    test_base64();
    test_frame_handling();
    test_different_message_sizes();
    test_boundary_conditions();
    
#ifdef _WIN32
    /* 清理Winsock */
    WSACleanup();
#endif
    
    printf("WebSocket核心功能测试完成\n");
    return 0;
}