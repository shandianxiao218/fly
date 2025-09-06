#include <stdint.h>
#include "websocket.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
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

/* =================== WebSocket服务器管理 =================== */

WebSocketServer* websocket_server_create(HttpServer* http_server) {
    if (http_server == NULL) return NULL;
    
    WebSocketServer* server = (WebSocketServer*)safe_malloc(sizeof(WebSocketServer));
    if (server == NULL) return NULL;
    
    /* 初始化服务器结构 */
    memset(server, 0, sizeof(WebSocketServer));
    server->http_server = http_server;
    server->connections = NULL;
    server->connection_count = 0;
    server->is_running = 0;
    
    /* 初始化互斥锁 */
    if (pthread_mutex_init(&server->connections_mutex, NULL) != 0) {
        safe_free((void**)&server);
        return NULL;
    }
    
    websocket_debug_log("WebSocket服务器创建成功");
    
    return server;
}

void websocket_server_destroy(WebSocketServer* server) {
    if (server == NULL) return;
    
    /* 停止服务器 */
    if (server->is_running) {
        websocket_server_stop(server);
    }
    
    /* 清理所有连接 */
    websocket_cleanup_connections(server);
    
    /* 销毁互斥锁 */
    pthread_mutex_destroy(&server->connections_mutex);
    
    /* 释放服务器结构 */
    safe_free((void**)&server);
    
    websocket_debug_log("WebSocket服务器销毁完成");
}

int websocket_server_start(WebSocketServer* server) {
    if (server == NULL) return 0;
    
    if (server->is_running) {
        websocket_debug_log("WebSocket服务器已经在运行");
        return 1;
    }
    
    /* 创建广播线程 */
    if (pthread_create(&server->broadcast_thread, NULL, websocket_broadcast_thread, server) != 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "创建广播线程失败");
        return 0;
    }
    
    server->is_running = 1;
    
    websocket_debug_log("WebSocket服务器启动成功");
    
    return 1;
}

int websocket_server_stop(WebSocketServer* server) {
    if (server == NULL) return 0;
    
    if (!server->is_running) {
        websocket_debug_log("WebSocket服务器已经停止");
        return 1;
    }
    
    /* 设置停止标志 */
    server->is_running = 0;
    
    /* 关闭所有连接 */
    pthread_mutex_lock(&server->connections_mutex);
    WebSocketConnection* conn = server->connections;
    while (conn != NULL) {
        websocket_connection_send_close(conn, 1000, "Server shutdown");
        conn = conn->next;
    }
    pthread_mutex_unlock(&server->connections_mutex);
    
    /* 等待广播线程结束 */
    if (pthread_join(server->broadcast_thread, NULL) != 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "等待广播线程结束失败");
        return 0;
    }
    
    websocket_debug_log("WebSocket服务器停止成功");
    
    return 1;
}

/* =================== WebSocket连接管理 =================== */

WebSocketConnection* websocket_connection_create(int socket_fd, const char* client_ip, int client_port) {
    if (socket_fd < 0 || client_ip == NULL) return NULL;
    
    WebSocketConnection* connection = (WebSocketConnection*)safe_malloc(sizeof(WebSocketConnection));
    if (connection == NULL) return NULL;
    
    /* 初始化连接结构 */
    memset(connection, 0, sizeof(WebSocketConnection));
    connection->socket_fd = socket_fd;
    connection->state = WS_STATE_CONNECTING;
    strncpy(connection->client_ip, client_ip, sizeof(connection->client_ip) - 1);
    connection->client_port = client_port;
    connection->connect_time = time(NULL);
    connection->last_activity = time(NULL);
    
    /* 初始化缓冲区 */
    connection->recv_buffer_len = 0;
    connection->send_buffer_len = 0;
    connection->fragment_buffer_len = 0;
    
    /* 初始化统计信息 */
    connection->messages_sent = 0;
    connection->messages_received = 0;
    connection->bytes_sent = 0;
    connection->bytes_received = 0;
    
    connection->next = NULL;
    
    websocket_debug_log("WebSocket连接创建成功: %s:%d", client_ip, client_port);
    
    return connection;
}

void websocket_connection_destroy(WebSocketConnection* connection) {
    if (connection == NULL) return;
    
    /* 关闭套接字 */
    if (connection->socket_fd >= 0) {
        close(connection->socket_fd);
        connection->socket_fd = -1;
    }
    
    /* 清理缓冲区 */
    if (connection->fragment_buffer_len > 0) {
        memset(connection->fragment_buffer, 0, sizeof(connection->fragment_buffer));
    }
    
    websocket_debug_log("WebSocket连接销毁完成: %s:%d", connection->client_ip, connection->client_port);
    
    safe_free((void**)&connection);
}

int websocket_connection_send(WebSocketConnection* connection, const char* data, int length) {
    if (connection == NULL || data == NULL || length <= 0) return 0;
    
    /* 创建WebSocket帧 */
    char frame_buffer[WebSocket_MAX_FRAME_SIZE + 14]; /* 帧头最大14字节 */
    int frame_length = websocket_frame_create(WS_FRAME_TEXT, data, length, frame_buffer, sizeof(frame_buffer));
    
    if (frame_length <= 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "创建WebSocket帧失败");
        return 0;
    }
    
    /* 发送帧数据 */
    int bytes_sent = send(connection->socket_fd, frame_buffer, frame_length, 0);
    if (bytes_sent != frame_length) {
        websocket_log_error(__func__, __FILE__, __LINE__, "发送WebSocket帧失败");
        return 0;
    }
    
    /* 更新统计信息 */
    connection->messages_sent++;
    connection->bytes_sent += bytes_sent;
    connection->last_activity = time(NULL);
    
    websocket_debug_log("WebSocket消息发送成功: %d 字节", bytes_sent);
    
    return 1;
}

int websocket_connection_send_text(WebSocketConnection* connection, const char* text) {
    if (connection == NULL || text == NULL) return 0;
    
    return websocket_connection_send(connection, text, strlen(text));
}

int websocket_connection_send_binary(WebSocketConnection* connection, const char* data, int length) {
    if (connection == NULL || data == NULL || length <= 0) return 0;
    
    /* 创建WebSocket帧 */
    char frame_buffer[WebSocket_MAX_FRAME_SIZE + 14];
    int frame_length = websocket_frame_create(WS_FRAME_BINARY, data, length, frame_buffer, sizeof(frame_buffer));
    
    if (frame_length <= 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "创建WebSocket帧失败");
        return 0;
    }
    
    /* 发送帧数据 */
    int bytes_sent = send(connection->socket_fd, frame_buffer, frame_length, 0);
    if (bytes_sent != frame_length) {
        websocket_log_error(__func__, __FILE__, __LINE__, "发送WebSocket帧失败");
        return 0;
    }
    
    /* 更新统计信息 */
    connection->messages_sent++;
    connection->bytes_sent += bytes_sent;
    connection->last_activity = time(NULL);
    
    websocket_debug_log("WebSocket二进制消息发送成功: %d 字节", bytes_sent);
    
    return 1;
}

int websocket_connection_send_close(WebSocketConnection* connection, int code, const char* reason) {
    if (connection == NULL) return 0;
    
    /* 创建关闭帧载荷 */
    char payload[125];
    int payload_length = 0;
    
    if (code > 0) {
        payload[0] = (code >> 8) & 0xFF;
        payload[1] = code & 0xFF;
        payload_length = 2;
        
        if (reason != NULL) {
            int reason_length = strlen(reason);
            if (reason_length > 0) {
                int max_reason_length = sizeof(payload) - payload_length;
                if (reason_length > max_reason_length) {
                    reason_length = max_reason_length;
                }
                memcpy(payload + payload_length, reason, reason_length);
                payload_length += reason_length;
            }
        }
    }
    
    /* 创建关闭帧 */
    char frame_buffer[WebSocket_MAX_FRAME_SIZE + 14];
    int frame_length = websocket_frame_create(WS_FRAME_CLOSE, payload, payload_length, frame_buffer, sizeof(frame_buffer));
    
    if (frame_length <= 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "创建WebSocket关闭帧失败");
        return 0;
    }
    
    /* 发送关闭帧 */
    int bytes_sent = send(connection->socket_fd, frame_buffer, frame_length, 0);
    if (bytes_sent != frame_length) {
        websocket_log_error(__func__, __FILE__, __LINE__, "发送WebSocket关闭帧失败");
        return 0;
    }
    
    /* 更新连接状态 */
    connection->state = WS_STATE_CLOSING;
    connection->last_activity = time(NULL);
    
    websocket_debug_log("WebSocket关闭帧发送成功: code=%d, reason=%s", code, reason ? reason : "null");
    
    return 1;
}

int websocket_connection_send_ping(WebSocketConnection* connection) {
    if (connection == NULL) return 0;
    
    /* 创建ping帧 */
    char frame_buffer[WebSocket_MAX_FRAME_SIZE + 14];
    int frame_length = websocket_frame_create(WS_FRAME_PING, NULL, 0, frame_buffer, sizeof(frame_buffer));
    
    if (frame_length <= 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "创建WebSocket ping帧失败");
        return 0;
    }
    
    /* 发送ping帧 */
    int bytes_sent = send(connection->socket_fd, frame_buffer, frame_length, 0);
    if (bytes_sent != frame_length) {
        websocket_log_error(__func__, __FILE__, __LINE__, "发送WebSocket ping帧失败");
        return 0;
    }
    
    connection->last_activity = time(NULL);
    
    websocket_debug_log("WebSocket ping帧发送成功");
    
    return 1;
}

int websocket_connection_send_pong(WebSocketConnection* connection) {
    if (connection == NULL) return 0;
    
    /* 创建pong帧 */
    char frame_buffer[WebSocket_MAX_FRAME_SIZE + 14];
    int frame_length = websocket_frame_create(WS_FRAME_PONG, NULL, 0, frame_buffer, sizeof(frame_buffer));
    
    if (frame_length <= 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "创建WebSocket pong帧失败");
        return 0;
    }
    
    /* 发送pong帧 */
    int bytes_sent = send(connection->socket_fd, frame_buffer, frame_length, 0);
    if (bytes_sent != frame_length) {
        websocket_log_error(__func__, __FILE__, __LINE__, "发送WebSocket pong帧失败");
        return 0;
    }
    
    connection->last_activity = time(NULL);
    
    websocket_debug_log("WebSocket pong帧发送成功");
    
    return 1;
}

/* =================== WebSocket握手处理 =================== */

int websocket_handshake(const HttpRequest* request, HttpResponse* response) {
    if (request == NULL || response == NULL) return 0;
    
    /* 检查Upgrade头 */
    int has_upgrade = 0;
    int has_connection = 0;
    char websocket_key[256] = {0};
    
    /* 解析请求头 */
    if (request->headers) {
        for (int i = 0; i < request->header_count; i++) {
            if (strcmp(request->headers[i], "Upgrade: websocket") == 0) {
                has_upgrade = 1;
            } else if (strcmp(request->headers[i], "Connection: Upgrade") == 0) {
                has_connection = 1;
            } else if (strncmp(request->headers[i], "Sec-WebSocket-Key:", 18) == 0) {
                strncpy(websocket_key, request->headers[i] + 19, sizeof(websocket_key) - 1);
            }
        }
    }
    
    if (!has_upgrade || !has_connection || strlen(websocket_key) == 0) {
        websocket_log_error(__func__, __FILE__, __LINE__, "WebSocket握手请求无效");
        return 0;
    }
    
    /* 计算Sec-WebSocket-Accept */
    char accept_key[256];
    char combined[512];
    snprintf(combined, sizeof(combined), "%s%s", websocket_key, WebSocket_MAGIC_STRING);
    
    unsigned char sha1_hash[20];
    websocket_sha1_hash(combined, strlen(combined), sha1_hash);
    
    websocket_base64_encode(sha1_hash, 20, accept_key, sizeof(accept_key));
    
    /* 构建握手响应 */
    char response_headers[1024];
    snprintf(response_headers, sizeof(response_headers),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n"
             "Sec-WebSocket-Protocol: chat\r\n"
             "\r\n",
             accept_key);
    
    /* 设置响应 */
    response->status_code = 101;
    strncpy(response->status_message, "Switching Protocols", sizeof(response->status_message) - 1);
    response->body = safe_strdup(response_headers);
    response->content_length = strlen(response_headers);
    
    websocket_debug_log("WebSocket握手成功");
    
    return 1;
}

int websocket_validate_handshake(const char* handshake_data) {
    if (handshake_data == NULL) return 0;
    
    /* 检查是否包含必要的WebSocket握手字段 */
    if (strstr(handshake_data, "Upgrade: websocket") == NULL) {
        return 0;
    }
    
    if (strstr(handshake_data, "Connection: Upgrade") == NULL) {
        return 0;
    }
    
    if (strstr(handshake_data, "Sec-WebSocket-Key:") == NULL) {
        return 0;
    }
    
    return 1;
}

/* =================== WebSocket帧处理 =================== */

int websocket_frame_parse(const char* frame_data, int frame_length, WebSocketFrameHeader* header, char** payload) {
    if (frame_data == NULL || frame_length < 2 || header == NULL) return 0;
    
    /* 解析帧头第一个字节 */
    header->fin = (frame_data[0] & 0x80) != 0;
    header->rsv1 = (frame_data[0] & 0x40) != 0;
    header->rsv2 = (frame_data[0] & 0x20) != 0;
    header->rsv3 = (frame_data[0] & 0x10) != 0;
    header->opcode = frame_data[0] & 0x0F;
    
    /* 解析帧头第二个字节 */
    header->mask = (frame_data[1] & 0x80) != 0;
    header->payload_len = frame_data[1] & 0x7F;
    
    int payload_offset = 2;
    int payload_length = header->payload_len;
    
    /* 处理扩展载荷长度 */
    if (header->payload_len == 126) {
        if (frame_length < 4) return 0;
        payload_length = (frame_data[2] << 8) | frame_data[3];
        payload_offset = 4;
    } else if (header->payload_len == 127) {
        if (frame_length < 10) return 0;
        payload_length = 0;
        for (int i = 0; i < 8; i++) {
            payload_length = (payload_length << 8) | frame_data[2 + i];
        }
        payload_offset = 10;
    }
    
    /* 处理掩码 */
    if (header->mask) {
        if (frame_length < payload_offset + 4) return 0;
        memcpy(header->masking_key, frame_data + payload_offset, 4);
        payload_offset += 4;
    }
    
    /* 检查载荷长度 */
    if (frame_length < payload_offset + payload_length) {
        return 0;
    }
    
    /* 提取载荷数据 */
    if (payload_length > 0 && payload != NULL) {
        *payload = (char*)safe_malloc(payload_length + 1);
        if (*payload == NULL) return 0;
        
        memcpy(*payload, frame_data + payload_offset, payload_length);
        (*payload)[payload_length] = '\0';
        
        /* 应用掩码 */
        if (header->mask) {
            for (int i = 0; i < payload_length; i++) {
                (*payload)[i] ^= header->masking_key[i % 4];
            }
        }
    }
    
    websocket_debug_log("WebSocket帧解析成功: opcode=%d, payload_len=%d", header->opcode, payload_length);
    
    return 1;
}

int websocket_frame_create(WebSocketFrameType frame_type, const char* payload, int payload_length, char* frame_buffer, int frame_buffer_size) {
    if (frame_buffer == NULL || frame_buffer_size < 2) return 0;
    
    int offset = 0;
    
    /* 构建第一个字节 */
    frame_buffer[0] = 0x80; /* FIN位设置为1 */
    frame_buffer[0] |= frame_type & 0x0F;
    offset = 1;
    
    /* 构建第二个字节 */
    if (payload_length <= 125) {
        frame_buffer[1] = payload_length & 0x7F;
        offset = 2;
    } else if (payload_length <= 65535) {
        frame_buffer[1] = 126;
        frame_buffer[2] = (payload_length >> 8) & 0xFF;
        frame_buffer[3] = payload_length & 0xFF;
        offset = 4;
    } else {
        frame_buffer[1] = 127;
        for (int i = 0; i < 8; i++) {
            frame_buffer[2 + i] = (payload_length >> (56 - i * 8)) & 0xFF;
        }
        offset = 10;
    }
    
    /* 添加载荷数据 */
    if (payload != NULL && payload_length > 0) {
        if (offset + payload_length > frame_buffer_size) {
            return 0;
        }
        memcpy(frame_buffer + offset, payload, payload_length);
        offset += payload_length;
    }
    
    websocket_debug_log("WebSocket帧创建成功: type=%d, total_length=%d", frame_type, offset);
    
    return offset;
}

/* =================== WebSocket消息处理 =================== */

int websocket_message_create(WebSocketMessage* message, WebSocketFrameType frame_type, const char* data, int length) {
    if (message == NULL || data == NULL || length < 0) return 0;
    
    memset(message, 0, sizeof(WebSocketMessage));
    message->frame_type = frame_type;
    message->timestamp = time(NULL);
    
    /* 根据帧类型设置消息类型 */
    switch (frame_type) {
        case WS_FRAME_TEXT:
            message->msg_type = WS_MESSAGE_DATA;
            break;
        case WS_FRAME_BINARY:
            message->msg_type = WS_MESSAGE_DATA;
            break;
        case WS_FRAME_CLOSE:
            message->msg_type = WS_MESSAGE_ERROR;
            break;
        case WS_FRAME_PING:
        case WS_FRAME_PONG:
            message->msg_type = WS_MESSAGE_STATUS;
            break;
        default:
            message->msg_type = WS_MESSAGE_ERROR;
            break;
    }
    
    /* 复制数据 */
    if (length > 0) {
        message->data = (char*)safe_malloc(length + 1);
        if (message->data == NULL) return 0;
        
        memcpy(message->data, data, length);
        message->data[length] = '\0';
        message->data_length = length;
    }
    
    return 1;
}

void websocket_message_destroy(WebSocketMessage* message) {
    if (message == NULL) return;
    
    if (message->data) {
        safe_free((void**)&message->data);
    }
    
    memset(message, 0, sizeof(WebSocketMessage));
}

/* =================== WebSocket广播功能 =================== */

int websocket_broadcast(WebSocketServer* server, const char* data, int length) {
    if (server == NULL || data == NULL || length <= 0) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    WebSocketConnection* conn = server->connections;
    int sent_count = 0;
    
    while (conn != NULL) {
        if (conn->state == WS_STATE_OPEN) {
            if (websocket_connection_send(conn, data, length)) {
                sent_count++;
            }
        }
        conn = conn->next;
    }
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    /* 更新统计信息 */
    server->total_messages_sent += sent_count;
    
    websocket_debug_log("WebSocket广播完成: 发送给%d个连接", sent_count);
    
    return sent_count;
}

int websocket_broadcast_text(WebSocketServer* server, const char* text) {
    if (server == NULL || text == NULL) return 0;
    
    return websocket_broadcast(server, text, strlen(text));
}

int websocket_broadcast_binary(WebSocketServer* server, const char* data, int length) {
    if (server == NULL || data == NULL || length <= 0) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    WebSocketConnection* conn = server->connections;
    int sent_count = 0;
    
    while (conn != NULL) {
        if (conn->state == WS_STATE_OPEN) {
            if (websocket_connection_send_binary(conn, data, length)) {
                sent_count++;
            }
        }
        conn = conn->next;
    }
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    /* 更新统计信息 */
    server->total_messages_sent += sent_count;
    
    websocket_debug_log("WebSocket二进制广播完成: 发送给%d个连接", sent_count);
    
    return sent_count;
}

int websocket_broadcast_to_connections(WebSocketServer* server, WebSocketConnection** connections, int count, const char* data, int length) {
    if (server == NULL || connections == NULL || count <= 0 || data == NULL || length <= 0) return 0;
    
    int sent_count = 0;
    
    for (int i = 0; i < count; i++) {
        if (connections[i] != NULL && connections[i]->state == WS_STATE_OPEN) {
            if (websocket_connection_send(connections[i], data, length)) {
                sent_count++;
            }
        }
    }
    
    websocket_debug_log("WebSocket定向广播完成: 发送给%d个连接", sent_count);
    
    return sent_count;
}

/* =================== WebSocket工具函数 =================== */

const char* websocket_frame_type_to_string(WebSocketFrameType type) {
    switch (type) {
        case WS_FRAME_CONTINUATION: return "CONTINUATION";
        case WS_FRAME_TEXT: return "TEXT";
        case WS_FRAME_BINARY: return "BINARY";
        case WS_FRAME_CLOSE: return "CLOSE";
        case WS_FRAME_PING: return "PING";
        case WS_FRAME_PONG: return "PONG";
        default: return "UNKNOWN";
    }
}

const char* websocket_connection_state_to_string(WebSocketConnectionState state) {
    switch (state) {
        case WS_STATE_CONNECTING: return "CONNECTING";
        case WS_STATE_OPEN: return "OPEN";
        case WS_STATE_CLOSING: return "CLOSING";
        case WS_STATE_CLOSED: return "CLOSED";
        default: return "UNKNOWN";
    }
}

const char* websocket_message_type_to_string(WebSocketMessageType type) {
    switch (type) {
        case WS_MESSAGE_STATUS: return "STATUS";
        case WS_MESSAGE_DATA: return "DATA";
        case WS_MESSAGE_ERROR: return "ERROR";
        case WS_MESSAGE_COMMAND: return "COMMAND";
        default: return "UNKNOWN";
    }
}

/* =================== Base64编码/解码 =================== */

int websocket_base64_encode(const unsigned char* input, int input_length, char* output, int output_size) {
    if (input == NULL || input_length <= 0 || output == NULL || output_size <= 0) return 0;
    
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    int output_length = 0;
    int i = 0;
    
    while (i < input_length) {
        unsigned int value = 0;
        int padding = 0;
        
        /* 读取3个字节 */
        for (int j = 0; j < 3; j++) {
            if (i + j < input_length) {
                value = (value << 8) | input[i + j];
            } else {
                value = value << 8;
                padding++;
            }
        }
        
        /* 编码为4个Base64字符 */
        for (int j = 0; j < 4 - padding; j++) {
            int index = (value >> (18 - j * 6)) & 0x3F;
            if (output_length < output_size - 1) {
                output[output_length++] = base64_chars[index];
            }
        }
        
        /* 添加填充字符 */
        for (int j = 0; j < padding; j++) {
            if (output_length < output_size - 1) {
                output[output_length++] = '=';
            }
        }
        
        i += 3;
    }
    
    output[output_length] = '\0';
    
    return output_length;
}

int websocket_base64_decode(const char* input, int input_length, unsigned char* output, int output_size) {
    if (input == NULL || input_length <= 0 || output == NULL || output_size <= 0) return 0;
    
    static const int base64_decode_table[256] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
    };
    
    int output_length = 0;
    int i = 0;
    
    while (i < input_length && input[i] != '=') {
        unsigned int value = 0;
        int padding = 0;
        
        /* 读取4个Base64字符 */
        for (int j = 0; j < 4; j++) {
            if (i + j < input_length && input[i + j] != '=') {
                int index = base64_decode_table[(unsigned char)input[i + j]];
                if (index == -1) {
                    return 0; /* 无效的Base64字符 */
                }
                value = (value << 6) | index;
            } else {
                value = value << 6;
                padding++;
            }
        }
        
        /* 解码为3个字节 */
        for (int j = 0; j < 3 - padding; j++) {
            if (output_length < output_size) {
                output[output_length++] = (value >> (16 - j * 8)) & 0xFF;
            }
        }
        
        i += 4;
    }
    
    return output_length;
}

/* =================== SHA1哈希计算 =================== */

int websocket_sha1_hash(const char* input, int input_length, unsigned char* output) {
    if (input == NULL || input_length <= 0 || output == NULL) return 0;
    
    /* 简化的SHA1实现 */
    /* 注意：在实际应用中应该使用加密库如OpenSSL */
    
    unsigned int h0 = 0x67452301;
    unsigned int h1 = 0xEFCDAB89;
    unsigned int h2 = 0x98BADCFE;
    unsigned int h3 = 0x10325476;
    unsigned int h4 = 0xC3D2E1F0;
    
    /* 填充消息 */
    unsigned char* message = (unsigned char*)safe_malloc(input_length + 64 + 8);
    if (message == NULL) return 0;
    
    memcpy(message, input, input_length);
    message[input_length] = 0x80; /* 填充位 */
    
    /* 填充长度 */
    uint64_t bit_length = (uint64_t)input_length * 8;
    for (int i = 0; i < 8; i++) {
        message[input_length + 1 + i] = (bit_length >> (56 - i * 8)) & 0xFF;
    }
    
    /* 处理消息块 */
    int message_length = input_length + 1 + 8;
    if (message_length % 64 != 0) {
        message_length += 64 - (message_length % 64);
    }
    
    for (int i = 0; i < message_length; i += 64) {
        unsigned int w[80];
        
        /* 扩展消息 */
        for (int j = 0; j < 16; j++) {
            w[j] = (message[i + j * 4] << 24) | (message[i + j * 4 + 1] << 16) |
                   (message[i + j * 4 + 2] << 8) | message[i + j * 4 + 3];
        }
        
        for (int j = 16; j < 80; j++) {
            w[j] = (w[j - 3] ^ w[j - 8] ^ w[j - 14] ^ w[j - 16]);
            w[j] = (w[j] << 1) | (w[j] >> 31);
        }
        
        /* 初始化哈希值 */
        unsigned int a = h0;
        unsigned int b = h1;
        unsigned int c = h2;
        unsigned int d = h3;
        unsigned int e = h4;
        
        /* 主循环 */
        for (int j = 0; j < 80; j++) {
            unsigned int f, k;
            
            if (j < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (j < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (j < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        
        /* 更新哈希值 */
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
    
    /* 输出哈希值 */
    for (int i = 0; i < 5; i++) {
        output[i * 4] = (h0 >> (24 - i * 8)) & 0xFF;
        output[i * 4 + 1] = (h1 >> (24 - i * 8)) & 0xFF;
        output[i * 4 + 2] = (h2 >> (24 - i * 8)) & 0xFF;
        output[i * 4 + 3] = (h3 >> (24 - i * 8)) & 0xFF;
    }
    
    safe_free((void**)&message);
    
    return 1;
}

/* =================== WebSocket统计信息 =================== */

int websocket_get_connection_count(WebSocketServer* server) {
    if (server == NULL) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    int count = server->connection_count;
    pthread_mutex_unlock(&server->connections_mutex);
    
    return count;
}

int websocket_get_total_connections(WebSocketServer* server) {
    if (server == NULL) return 0;
    
    return server->total_connections;
}

int websocket_get_total_messages(WebSocketServer* server, int* sent, int* received) {
    if (server == NULL) return 0;
    
    if (sent) *sent = server->total_messages_sent;
    if (received) *received = server->total_messages_received;
    
    return 1;
}

/* =================== WebSocket回调设置 =================== */

int websocket_set_handlers(WebSocketServer* server,
                         WebSocketHandler message_handler,
                         WebSocketHandler connect_handler,
                         WebSocketHandler disconnect_handler) {
    if (server == NULL) return 0;
    
    server->message_handler = message_handler;
    server->connect_handler = connect_handler;
    server->disconnect_handler = disconnect_handler;
    
    websocket_debug_log("WebSocket回调函数设置完成");
    
    return 1;
}

/* =================== WebSocket连接管理 =================== */

int websocket_add_connection(WebSocketServer* server, WebSocketConnection* connection) {
    if (server == NULL || connection == NULL) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    /* 添加到连接链表头部 */
    connection->next = server->connections;
    server->connections = connection;
    server->connection_count++;
    server->total_connections++;
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    websocket_debug_log("WebSocket连接添加成功: %s:%d", connection->client_ip, connection->client_port);
    
    return 1;
}

int websocket_remove_connection(WebSocketServer* server, WebSocketConnection* connection) {
    if (server == NULL || connection == NULL) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    WebSocketConnection* prev = NULL;
    WebSocketConnection* curr = server->connections;
    
    while (curr != NULL) {
        if (curr == connection) {
            if (prev == NULL) {
                server->connections = curr->next;
            } else {
                prev->next = curr->next;
            }
            server->connection_count--;
            break;
        }
        prev = curr;
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    websocket_debug_log("WebSocket连接移除成功: %s:%d", connection->client_ip, connection->client_port);
    
    return 1;
}

WebSocketConnection* websocket_find_connection(WebSocketServer* server, int socket_fd) {
    if (server == NULL || socket_fd < 0) return NULL;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    WebSocketConnection* conn = server->connections;
    while (conn != NULL) {
        if (conn->socket_fd == socket_fd) {
            pthread_mutex_unlock(&server->connections_mutex);
            return conn;
        }
        conn = conn->next;
    }
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    return NULL;
}

int websocket_cleanup_connections(WebSocketServer* server) {
    if (server == NULL) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    WebSocketConnection* curr = server->connections;
    while (curr != NULL) {
        WebSocketConnection* next = curr->next;
        websocket_connection_destroy(curr);
        curr = next;
    }
    
    server->connections = NULL;
    server->connection_count = 0;
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    websocket_debug_log("WebSocket连接清理完成");
    
    return 1;
}

/* =================== WebSocket消息处理线程 =================== */

void* websocket_connection_thread(void* arg) {
    WebSocketConnection* connection = (WebSocketConnection*)arg;
    if (connection == NULL) return NULL;
    
    websocket_debug_log("WebSocket连接线程启动: %s:%d", connection->client_ip, connection->client_port);
    
    while (connection->state == WS_STATE_OPEN) {
        /* 接收数据 */
        char buffer[WebSocket_BUFFER_SIZE];
        int bytes_received = recv(connection->socket_fd, buffer, sizeof(buffer), 0);
        
        if (bytes_received > 0) {
            /* 处理接收到的数据 */
            connection->bytes_received += bytes_received;
            connection->last_activity = time(NULL);
            
            /* 解析WebSocket帧 */
            WebSocketFrameHeader header;
            char* payload = NULL;
            
            if (websocket_frame_parse(buffer, bytes_received, &header, &payload)) {
                /* 处理不同类型的帧 */
                switch (header.opcode) {
                    case WS_FRAME_TEXT:
                    case WS_FRAME_BINARY:
                        /* 处理消息帧 */
                        if (header.fin) {
                            /* 完整消息 */
                            WebSocketMessage message;
                            if (websocket_message_create(&message, header.opcode, payload, header.payload_len)) {
                                /* 调用消息处理回调 */
                                if (connection->server && connection->server->message_handler) {
                                    connection->server->message_handler(&message);
                                }
                                websocket_message_destroy(&message);
                            }
                        } else {
                            /* 消息分片处理 */
                            if (connection->fragment_buffer_len + header.payload_len <= sizeof(connection->fragment_buffer)) {
                                memcpy(connection->fragment_buffer + connection->fragment_buffer_len, payload, header.payload_len);
                                connection->fragment_buffer_len += header.payload_len;
                                connection->fragment_opcode = header.opcode;
                            }
                        }
                        break;
                        
                    case WS_FRAME_CONTINUATION:
                        /* 处理续帧 */
                        if (connection->fragment_buffer_len > 0) {
                            if (connection->fragment_buffer_len + header.payload_len <= sizeof(connection->fragment_buffer)) {
                                memcpy(connection->fragment_buffer + connection->fragment_buffer_len, payload, header.payload_len);
                                connection->fragment_buffer_len += header.payload_len;
                                
                                if (header.fin) {
                                    /* 完整消息接收完成 */
                                    WebSocketMessage message;
                                    if (websocket_message_create(&message, connection->fragment_opcode, connection->fragment_buffer, connection->fragment_buffer_len)) {
                                        /* 调用消息处理回调 */
                                        if (connection->server && connection->server->message_handler) {
                                            connection->server->message_handler(&message);
                                        }
                                        websocket_message_destroy(&message);
                                    }
                                    
                                    /* 重置分片缓冲区 */
                                    connection->fragment_buffer_len = 0;
                                }
                            }
                        }
                        break;
                        
                    case WS_FRAME_PING:
                        /* 响应ping帧 */
                        websocket_connection_send_pong(connection);
                        break;
                        
                    case WS_FRAME_PONG:
                        /* 处理pong帧 */
                        break;
                        
                    case WS_FRAME_CLOSE:
                        /* 处理关闭帧 */
                        connection->state = WS_STATE_CLOSING;
                        websocket_connection_send_close(connection, 1000, "Normal closure");
                        break;
                        
                    default:
                        /* 未知帧类型 */
                        websocket_log_error(__func__, __FILE__, __LINE__, "未知的WebSocket帧类型: %d", header.opcode);
                        break;
                }
                
                if (payload) {
                    safe_free((void**)&payload);
                }
            }
            
            connection->messages_received++;
            
        } else if (bytes_received == 0) {
            /* 连接关闭 */
            websocket_debug_log("WebSocket连接关闭: %s:%d", connection->client_ip, connection->client_port);
            break;
        } else {
            /* 接收错误 */
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                websocket_log_error(__func__, __FILE__, __LINE__, "WebSocket接收错误: %s", strerror(errno));
                break;
            }
        }
        
        /* 检查超时 */
        time_t current_time = time(NULL);
        if (current_time - connection->last_activity > WebSocket_TIMEOUT_SECONDS) {
            websocket_debug_log("WebSocket连接超时: %s:%d", connection->client_ip, connection->client_port);
            break;
        }
        
        /* 短暂休眠以避免CPU占用过高 */
        usleep(10000); /* 10ms */
    }
    
    /* 清理连接 */
    if (connection->server) {
        websocket_remove_connection(connection->server, connection);
        
        /* 调用断开连接回调 */
        if (connection->server->disconnect_handler) {
            WebSocketMessage message;
            memset(&message, 0, sizeof(message));
            message.msg_type = WS_MESSAGE_ERROR;
            message.connection = connection;
            connection->server->disconnect_handler(&message);
        }
    }
    
    websocket_connection_destroy(connection);
    
    websocket_debug_log("WebSocket连接线程结束: %s:%d", connection->client_ip, connection->client_port);
    
    return NULL;
}

void* websocket_broadcast_thread(void* arg) {
    WebSocketServer* server = (WebSocketServer*)arg;
    if (server == NULL) return NULL;
    
    websocket_debug_log("WebSocket广播线程启动");
    
    while (server->is_running) {
        /* 发送心跳 */
        websocket_send_heartbeat(server);
        
        /* 检查超时连接 */
        websocket_check_timeouts(server);
        
        /* 休眠1秒 */
        sleep(1);
    }
    
    websocket_debug_log("WebSocket广播线程结束");
    
    return NULL;
}

/* =================== WebSocket心跳检测 =================== */

int websocket_send_heartbeat(WebSocketServer* server) {
    if (server == NULL) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    WebSocketConnection* conn = server->connections;
    int ping_count = 0;
    
    while (conn != NULL) {
        if (conn->state == WS_STATE_OPEN) {
            time_t current_time = time(NULL);
            if (current_time - conn->last_activity > WebSocket_TIMEOUT_SECONDS / 2) {
                if (websocket_connection_send_ping(conn)) {
                    ping_count++;
                }
            }
        }
        conn = conn->next;
    }
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    websocket_debug_log("WebSocket心跳发送完成: %d个ping", ping_count);
    
    return ping_count;
}

int websocket_check_timeouts(WebSocketServer* server) {
    if (server == NULL) return 0;
    
    pthread_mutex_lock(&server->connections_mutex);
    
    WebSocketConnection* conn = server->connections;
    int timeout_count = 0;
    
    while (conn != NULL) {
        if (conn->state == WS_STATE_OPEN) {
            time_t current_time = time(NULL);
            if (current_time - conn->last_activity > WebSocket_TIMEOUT_SECONDS) {
                websocket_debug_log("WebSocket连接超时: %s:%d", conn->client_ip, conn->client_port);
                conn->state = WS_STATE_CLOSING;
                timeout_count++;
            }
        }
        conn = conn->next;
    }
    
    pthread_mutex_unlock(&server->connections_mutex);
    
    websocket_debug_log("WebSocket超时检查完成: %d个超时", timeout_count);
    
    return timeout_count;
}

/* =================== WebSocket错误处理 =================== */

int websocket_handle_error(WebSocketConnection* connection, const char* error_message) {
    if (connection == NULL || error_message == NULL) return 0;
    
    websocket_log_error(__func__, __FILE__, __LINE__, "WebSocket错误: %s", error_message);
    
    /* 发送错误消息 */
    char error_json[512];
    snprintf(error_json, sizeof(error_json),
             "{\"type\":\"error\",\"message\":\"%s\",\"timestamp\":%lld}",
             error_message, (long long)time(NULL));
    
    websocket_connection_send_text(connection, error_json);
    
    return 1;
}

int websocket_log_error(const char* function, const char* file, int line, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    va_end(args);
    
    logger_error(function, file, line, buffer);
    
    return 1;
}