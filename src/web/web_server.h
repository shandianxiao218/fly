#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "../satellite/satellite.h"
#include "../aircraft/aircraft.h"
#include "../obstruction/obstruction.h"
#include "websocket.h"

/* HTTP服务器配置 */
typedef struct {
    int port;                     /* 端口号 */
    char host[256];               /* 主机地址 */
    int max_connections;          /* 最大连接数 */
    int timeout;                  /* 超时时间 (秒) */
    char document_root[512];      /* 文档根目录 */
    int enable_cors;              /* 是否启用CORS */
    int enable_logging;           /* 是否启用日志 */
} HttpServerConfig;

/* HTTP请求方法 */
typedef enum {
    HTTP_METHOD_GET = 1,
    HTTP_METHOD_POST = 2,
    HTTP_METHOD_PUT = 3,
    HTTP_METHOD_DELETE = 4,
    HTTP_METHOD_HEAD = 5
} HttpMethod;

/* HTTP请求 */
typedef struct {
    HttpMethod method;            /* 请求方法 */
    char path[1024];              /* 请求路径 */
    char query[1024];             /* 查询字符串 */
    char version[32];             /* HTTP版本 */
    char** headers;               /* 请求头 */
    int header_count;             /* 请求头数量 */
    char* body;                   /* 请求体 */
    int body_length;              /* 请求体长度 */
    char client_ip[64];           /* 客户端IP */
    int client_port;              /* 客户端端口 */
} HttpRequest;

/* HTTP响应 */
typedef struct {
    int status_code;              /* 状态码 */
    char status_message[256];     /* 状态消息 */
    char** headers;               /* 响应头 */
    int header_count;             /* 响应头数量 */
    char* body;                   /* 响应体 */
    int body_length;              /* 响应体长度 */
} HttpResponse;

/* API端点类型 */
typedef enum {
    API_ENDPOINT_STATUS = 1,      /* 状态查询 */
    API_ENDPOINT_SATELLITE = 2,   /* 卫星数据 */
    API_ENDPOINT_TRAJECTORY = 3,   /* 轨迹数据 */
    API_ENDPOINT_ANALYSIS = 4,    /* 分析结果 */
    API_ENDPOINT_CONFIG = 5,      /* 配置管理 */
    API_ENDPOINT_UPLOAD = 6       /* 文件上传 */
} ApiEndpointType;

/* API请求参数 */
typedef struct {
    ApiEndpointType endpoint;     /* 端点类型 */
    char action[64];             /* 操作类型 */
    time_t start_time;            /* 开始时间 */
    time_t end_time;              /* 结束时间 */
    int satellite_prn;            /* 卫星PRN */
    int trajectory_id;            /* 轨迹ID */
    double latitude;              /* 纬度 */
    double longitude;             /* 经度 */
    double altitude;              /* 高度 */
    int include_visibility;       /* 包含可见性 */
    int include_obstruction;      /* 包含遮挡分析 */
    int format_json;              /* JSON格式 */
    char filename[512];           /* 文件名 */
} ApiRequestParams;

/* API响应数据 */
typedef struct {
    int success;                  /* 是否成功 */
    char message[256];            /* 响应消息 */
    char error[256];              /* 错误信息 */
    int data_count;               /* 数据数量 */
    void* data;                   /* 数据指针 */
    time_t timestamp;             /* 时间戳 */
    double processing_time;       /* 处理时间 (秒) */
} ApiResponseData;

/* 系统状态 */
typedef struct {
    int is_running;               /* 是否运行中 */
    time_t start_time;             /* 启动时间 */
    int uptime;                   /* 运行时间 (秒) */
    int memory_usage;             /* 内存使用 (MB) */
    double cpu_usage;             /* CPU使用率 (%) */
    int active_connections;        /* 活动连接数 */
    int total_requests;           /* 总请求数 */
    int error_count;              /* 错误数量 */
    char version[64];             /* 版本信息 */
} SystemStatus;

/* 服务器统计数据 */
typedef struct {
    int requests_per_second;      /* 每秒请求数 */
    double avg_response_time;     /* 平均响应时间 (秒) */
    int max_response_time;        /* 最大响应时间 (秒) */
    int min_response_time;        /* 最小响应时间 (秒) */
    int bandwidth_in;             /* 入站带宽 (KB/s) */
    int bandwidth_out;            /* 出站带宽 (KB/s) */
} ServerStats;

/* WebSocket消息类型 */
typedef enum {
    WS_MESSAGE_STATUS = 1,         /* 状态消息 */
    WS_MESSAGE_DATA = 2,          /* 数据消息 */
    WS_MESSAGE_ERROR = 3,         /* 错误消息 */
    WS_MESSAGE_COMMAND = 4        /* 命令消息 */
} WebSocketMessageType;

/* WebSocket消息 */
typedef struct {
    WebSocketMessageType type;    /* 消息类型 */
    char* data;                   /* 消息数据 */
    int data_length;              /* 数据长度 */
    time_t timestamp;             /* 时间戳 */
} WebSocketMessage;

/* 文件上传信息 */
typedef struct {
    char filename[512];           /* 文件名 */
    char temp_path[512];          /* 临时路径 */
    long file_size;               /* 文件大小 */
    char content_type[128];       /* 内容类型 */
    int upload_progress;          /* 上传进度 (%) */
    int is_complete;              /* 是否完成 */
    char error[256];              /* 错误信息 */
} FileUploadInfo;

/* 回调函数类型 */
typedef int (*HttpRequestHandler)(const HttpRequest* request, HttpResponse* response);
typedef int (*WebSocketHandler)(const WebSocketMessage* message);
typedef int (*FileUploadHandler)(const FileUploadInfo* upload_info);

/* HTTP服务器 */
typedef struct {
    HttpServerConfig config;      /* 服务器配置 */
    int server_socket;            /* 服务器套接字 */
    int is_running;               /* 是否运行中 */
    pthread_t server_thread;       /* 服务器线程 */
    SystemStatus status;          /* 系统状态 */
    ServerStats stats;            /* 服务器统计 */
    
    /* 数据引用 */
    SatelliteData* satellite_data; /* 卫星数据 */
    FlightTrajectory* trajectory;  /* 飞行轨迹 */
    AircraftGeometry* geometry;    /* 飞机几何模型 */
    
    /* WebSocket支持 */
    WebSocketServer* websocket_server; /* WebSocket服务器 */
    int enable_websocket;         /* 是否启用WebSocket */
    
    /* 回调函数 */
    HttpRequestHandler request_handler;
    WebSocketHandler websocket_handler;
    FileUploadHandler upload_handler;
} HttpServer;

/* 函数声明 */
HttpServer* http_server_create(const HttpServerConfig* config);
void http_server_destroy(HttpServer* server);

int http_server_start(HttpServer* server);
int http_server_stop(HttpServer* server);
int http_server_restart(HttpServer* server);

/* WebSocket管理 */
int http_server_enable_websocket(HttpServer* server, int enable);
int http_server_websocket_broadcast(HttpServer* server, const char* message);
int http_server_websocket_send_status(HttpServer* server);

int http_server_set_data(HttpServer* server, 
                        SatelliteData* satellite_data,
                        FlightTrajectory* trajectory,
                        AircraftGeometry* geometry);

int http_server_set_handlers(HttpServer* server,
                           HttpRequestHandler request_handler,
                           WebSocketHandler websocket_handler,
                           FileUploadHandler upload_handler);

/* HTTP请求/响应处理 */
HttpRequest* http_request_create();
void http_request_destroy(HttpRequest* request);
HttpResponse* http_response_create();
void http_response_destroy(HttpResponse* response);

int http_request_parse(const char* raw_request, HttpRequest* request);
int http_response_serialize(const HttpResponse* response, char* buffer, int buffer_size);

int http_response_set_json(HttpResponse* response, const char* json_data);
int http_response_set_error(HttpResponse* response, int status_code, const char* message);
int http_response_set_file(HttpResponse* response, const char* filename);

/* API处理 */
int api_handle_request(const HttpRequest* request, HttpResponse* response, 
                      const HttpServer* server);
int api_handle_status(const ApiRequestParams* params, ApiResponseData* response,
                     const HttpServer* server);
int api_handle_satellite(const ApiRequestParams* params, ApiResponseData* response,
                        const HttpServer* server);
int api_handle_trajectory(const ApiRequestParams* params, ApiResponseData* response,
                         const HttpServer* server);
int api_handle_analysis(const ApiRequestParams* params, ApiResponseData* response,
                       const HttpServer* server);

/* JSON工具函数 */
int json_serialize_satellite(const Satellite* satellite, char* buffer, int buffer_size);
int json_serialize_trajectory(const FlightTrajectory* trajectory, char* buffer, int buffer_size);
int json_serialize_analysis(const VisibilityAnalysis* analysis, char* buffer, int buffer_size);
int json_serialize_status(const SystemStatus* status, char* buffer, int buffer_size);

/* 工具函数 */
int http_server_config_init(HttpServerConfig* config);
int http_server_config_validate(const HttpServerConfig* config);
int system_status_update(SystemStatus* status, const HttpServer* server);
int server_stats_update(ServerStats* stats, const HttpServer* server);

const char* http_method_to_string(HttpMethod method);
const char* api_endpoint_to_string(ApiEndpointType endpoint);

#endif /* WEB_SERVER_H */