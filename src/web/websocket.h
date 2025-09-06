#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 前向声明以避免循环依赖 */
typedef struct HttpServer HttpServer;
typedef struct HttpRequest HttpRequest;
typedef struct HttpResponse HttpResponse;

/* WebSocket消息类型枚举 */
typedef enum {
    WS_MESSAGE_STATUS = 1,         /* 状态消息 */
    WS_MESSAGE_DATA = 2,          /* 数据消息 */
    WS_MESSAGE_ERROR = 3,         /* 错误消息 */
    WS_MESSAGE_COMMAND = 4        /* 命令消息 */
} WebSocketMessageType;

/* WebSocket回调函数类型 */
typedef int (*WebSocketHandler)(const struct WebSocketMessage* message);

/* WebSocket消息结构前向声明 */
struct WebSocketMessage;

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pthread.h>
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

/* =================== WebSocket常量定义 =================== */

#define WebSocket_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WebSocket_MAX_FRAME_SIZE 65536
#define WebSocket_MAX_MESSAGE_SIZE 1048576
#define WebSocket_BUFFER_SIZE 8192
#define WebSocket_TIMEOUT_SECONDS 30

/* =================== WebSocket帧类型 =================== */

typedef enum {
    WS_FRAME_CONTINUATION = 0x0,
    WS_FRAME_TEXT = 0x1,
    WS_FRAME_BINARY = 0x2,
    WS_FRAME_CLOSE = 0x8,
    WS_FRAME_PING = 0x9,
    WS_FRAME_PONG = 0xA
} WebSocketFrameType;

/* =================== WebSocket连接状态 =================== */

typedef enum {
    WS_STATE_CONNECTING = 0,
    WS_STATE_OPEN = 1,
    WS_STATE_CLOSING = 2,
    WS_STATE_CLOSED = 3
} WebSocketConnectionState;

/* =================== WebSocket帧头 =================== */

typedef struct {
    unsigned char fin : 1;          /* FIN位 */
    unsigned char rsv1 : 1;         /* RSV1位 */
    unsigned char rsv2 : 1;         /* RSV2位 */
    unsigned char rsv3 : 1;         /* RSV3位 */
    unsigned char opcode : 4;       /* 操作码 */
    unsigned char mask : 1;         /* 掩码位 */
    unsigned char payload_len : 7;  /* 载荷长度 */
    unsigned char extended_len[8];  /* 扩展长度 */
    unsigned char masking_key[4];   /* 掩码密钥 */
} WebSocketFrameHeader;

/* =================== WebSocket连接 =================== */

typedef struct WebSocketConnection {
    int socket_fd;                 /* 套接字描述符 */
    WebSocketConnectionState state; /* 连接状态 */
    char client_ip[64];            /* 客户端IP */
    int client_port;               /* 客户端端口 */
    time_t connect_time;           /* 连接时间 */
    time_t last_activity;           /* 最后活动时间 */
    struct WebSocketServer* server; /* 所属服务器 */
    
    /* 接收缓冲区 */
    char recv_buffer[WebSocket_BUFFER_SIZE];
    int recv_buffer_len;
    
    /* 发送缓冲区 */
    char send_buffer[WebSocket_BUFFER_SIZE];
    int send_buffer_len;
    
    /* 消息分片处理 */
    WebSocketFrameType fragment_opcode;
    char fragment_buffer[WebSocket_MAX_MESSAGE_SIZE];
    int fragment_buffer_len;
    
    /* 统计信息 */
    int messages_sent;
    int messages_received;
    int bytes_sent;
    int bytes_received;
    
    /* 链表节点 */
    struct WebSocketConnection* next;
} WebSocketConnection;

/* =================== WebSocket服务器 =================== */

typedef struct WebSocketServer {
    HttpServer* http_server;        /* 关联的HTTP服务器 */
    WebSocketConnection* connections; /* 连接链表 */
    int connection_count;          /* 连接计数 */
    pthread_mutex_t connections_mutex; /* 连接互斥锁 */
    pthread_t broadcast_thread;     /* 广播线程 */
    int is_running;                /* 是否运行中 */
    
    /* 回调函数 */
    WebSocketHandler message_handler;
    WebSocketHandler connect_handler;
    WebSocketHandler disconnect_handler;
    
    /* 统计信息 */
    int total_connections;
    int total_messages_sent;
    int total_messages_received;
} WebSocketServer;

/* =================== WebSocket消息 =================== */

typedef struct {
    WebSocketFrameType frame_type; /* 帧类型 */
    WebSocketMessageType msg_type; /* 消息类型 */
    char* data;                   /* 消息数据 */
    int data_length;              /* 数据长度 */
    time_t timestamp;             /* 时间戳 */
    WebSocketConnection* connection; /* 来源连接 */
} WebSocketMessage;

/* =================== 函数声明 =================== */

/* WebSocket服务器管理 */
WebSocketServer* websocket_server_create(HttpServer* http_server);
void websocket_server_destroy(WebSocketServer* server);

int websocket_server_start(WebSocketServer* server);
int websocket_server_stop(WebSocketServer* server);

/* WebSocket连接管理 */
WebSocketConnection* websocket_connection_create(int socket_fd, const char* client_ip, int client_port);
void websocket_connection_destroy(WebSocketConnection* connection);

int websocket_connection_send(WebSocketConnection* connection, const char* data, int length);
int websocket_connection_send_text(WebSocketConnection* connection, const char* text);
int websocket_connection_send_binary(WebSocketConnection* connection, const char* data, int length);
int websocket_connection_send_close(WebSocketConnection* connection, int code, const char* reason);
int websocket_connection_send_ping(WebSocketConnection* connection);
int websocket_connection_send_pong(WebSocketConnection* connection);

/* WebSocket握手处理 */
int websocket_handshake(const HttpRequest* request, HttpResponse* response);
int websocket_validate_handshake(const char* handshake_data);

/* WebSocket帧处理 */
int websocket_frame_parse(const char* frame_data, int frame_length, WebSocketFrameHeader* header, char** payload);
int websocket_frame_create(WebSocketFrameType frame_type, const char* payload, int payload_length, char* frame_buffer, int frame_buffer_size);

/* WebSocket消息处理 */
int websocket_message_create(WebSocketMessage* message, WebSocketFrameType frame_type, const char* data, int length);
void websocket_message_destroy(WebSocketMessage* message);

/* WebSocket广播功能 */
int websocket_broadcast(WebSocketServer* server, const char* data, int length);
int websocket_broadcast_text(WebSocketServer* server, const char* text);
int websocket_broadcast_binary(WebSocketServer* server, const char* data, int length);
int websocket_broadcast_to_connections(WebSocketServer* server, WebSocketConnection** connections, int count, const char* data, int length);

/* WebSocket工具函数 */
const char* websocket_frame_type_to_string(WebSocketFrameType type);
const char* websocket_connection_state_to_string(WebSocketConnectionState state);
const char* websocket_message_type_to_string(WebSocketMessageType type);

int websocket_base64_encode(const unsigned char* input, int input_length, char* output, int output_size);
int websocket_base64_decode(const char* input, int input_length, unsigned char* output, int output_size);
int websocket_sha1_hash(const char* input, int input_length, unsigned char* output);

/* WebSocket统计信息 */
int websocket_get_connection_count(WebSocketServer* server);
int websocket_get_total_connections(WebSocketServer* server);
int websocket_get_total_messages(WebSocketServer* server, int* sent, int* received);

/* WebSocket回调设置 */
int websocket_set_handlers(WebSocketServer* server,
                         WebSocketHandler message_handler,
                         WebSocketHandler connect_handler,
                         WebSocketHandler disconnect_handler);

/* WebSocket连接管理 */
int websocket_add_connection(WebSocketServer* server, WebSocketConnection* connection);
int websocket_remove_connection(WebSocketServer* server, WebSocketConnection* connection);
WebSocketConnection* websocket_find_connection(WebSocketServer* server, int socket_fd);
int websocket_cleanup_connections(WebSocketServer* server);

/* WebSocket消息处理线程 */
void* websocket_connection_thread(void* arg);
void* websocket_broadcast_thread(void* arg);

/* WebSocket心跳检测 */
int websocket_send_heartbeat(WebSocketServer* server);
int websocket_check_timeouts(WebSocketServer* server);

/* WebSocket错误处理 */
int websocket_handle_error(WebSocketConnection* connection, const char* error_message);
int websocket_log_error(const char* function, const char* file, int line, const char* format, ...);

/* WebSocket调试功能 */
#ifdef DEBUG_WEBSOCKET
#define websocket_debug_log(...) websocket_log_error(__func__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define websocket_debug_log(...)
#endif

#endif /* WEBSOCKET_H */