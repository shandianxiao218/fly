#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <time.h>
#include <stddef.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <pthread.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <pthread.h>
#endif

#include "../satellite/satellite.h"
#include "../aircraft/aircraft.h"
#include "../obstruction/obstruction.h"

/* 前向声明 */
struct WebSocketServer;

/* HTTP方法类型 */
typedef enum {
    HTTP_GET = 1,
    HTTP_POST = 2,
    HTTP_PUT = 3,
    HTTP_DELETE = 4,
    HTTP_HEAD = 5
} HttpMethod;

/* API端点类型 */
typedef enum {
    API_STATUS = 1,
    API_SATELLITE = 2,
    API_TRAJECTORY = 3,
    API_ANALYSIS = 4
} ApiEndpointType;

/* HTTP请求结构 */
typedef struct {
    HttpMethod method;
    char* path;
    char* query_string;
    char** headers;
    int header_count;
    char* body;
    int content_length;
} HttpRequest;

/* HTTP响应结构 */
typedef struct {
    int status_code;
    char* status_message;
    char* headers;
    char* body;
    int content_length;
} HttpResponse;

/* 前向声明 */
struct HttpServer;

/* 函数指针类型定义 */
typedef int (*HttpRequestHandler)(const HttpRequest* request, HttpResponse* response, const struct HttpServer* server);
typedef int (*WebSocketHandler)(const char* message, const struct HttpServer* server);
typedef int (*FileUploadHandler)(const char* filename, const char* data, size_t size, const struct HttpServer* server);

/* 服务器配置 */
typedef struct {
    int port;
    char* host;
    int max_connections;
    int timeout;
    char* static_dir;
} HttpServerConfig;

/* 服务器统计 */
typedef struct {
    int total_requests;
    int active_connections;
    long total_bytes_sent;
    long total_bytes_received;
    time_t start_time;
    double avg_response_time;
} ServerStats;

/* 系统状态 */
typedef struct {
    int is_running;
    time_t start_time;
    int request_count;
    int error_count;
    int active_connections;
    double cpu_usage;
    double memory_usage;
    ServerStats stats;
} SystemStatus;

/* API请求参数 */
typedef struct {
    ApiEndpointType endpoint;
    char* method;
    char* parameters;
    char* body;
    time_t start_time;
    time_t end_time;
    int satellite_prn;
    int trajectory_id;
} ApiRequestParams;

/* API响应数据 */
typedef struct {
    int success;
    char* message;
    char* data;
    int status_code;
    time_t timestamp;
    char error[512];
} ApiResponseData;

/* HTTP服务器 */
typedef struct {
    HttpServerConfig config;
    SystemStatus status;
    SatelliteData* satellite_data;
    FlightTrajectory* trajectory;
    AircraftGeometry* geometry;
    int server_socket;
    int is_running;
    pthread_t server_thread;
    
    /* 回调函数 */
    HttpRequestHandler request_handler;
    WebSocketHandler websocket_handler;
    FileUploadHandler upload_handler;
    
    /* WebSocket支持 */
    struct WebSocketServer* websocket_server;
    int enable_websocket;
} HttpServer;

/* 函数声明 */
HttpServer* http_server_create(const HttpServerConfig* config);
void http_server_destroy(HttpServer* server);
int http_server_start(HttpServer* server);
int http_server_stop(HttpServer* server);
int http_server_restart(HttpServer* server);

int http_server_set_data(HttpServer* server, 
                        SatelliteData* satellite_data,
                        FlightTrajectory* trajectory,
                        AircraftGeometry* geometry);

int http_server_enable_websocket(HttpServer* server, int enable);
int http_server_websocket_broadcast(HttpServer* server, const char* message);
int http_server_websocket_send_status(HttpServer* server);
int http_server_set_handlers(struct HttpServer* server,
                           HttpRequestHandler request_handler,
                           WebSocketHandler websocket_handler,
                           FileUploadHandler upload_handler);

HttpRequest* http_request_create();
void http_request_destroy(HttpRequest* request);
HttpResponse* http_response_create();
void http_response_destroy(HttpResponse* response);
int http_request_parse(const char* raw_request, HttpRequest* request);
int http_response_serialize(const HttpResponse* response, char* buffer, int buffer_size);
int http_response_set_json(HttpResponse* response, const char* json_data);
int http_response_set_error(HttpResponse* response, int status_code, const char* message);
int http_response_set_file(HttpResponse* response, const char* filename);

int api_handle_request(const HttpRequest* request, HttpResponse* response, 
                      const struct HttpServer* server);
int api_handle_status(const ApiRequestParams* params, ApiResponseData* response,
                     const struct HttpServer* server);
int api_handle_satellite(const ApiRequestParams* params, ApiResponseData* response,
                        const struct HttpServer* server);
int api_handle_trajectory(const ApiRequestParams* params, ApiResponseData* response,
                         const struct HttpServer* server);
int api_handle_analysis(const ApiRequestParams* params, ApiResponseData* response,
                       const struct HttpServer* server);

int json_serialize_satellite(const Satellite* satellite, char* buffer, int buffer_size);
int json_serialize_trajectory(const FlightTrajectory* trajectory, char* buffer, int buffer_size);
int json_serialize_analysis(const VisibilityAnalysis* analysis, char* buffer, int buffer_size);
int json_serialize_status(const SystemStatus* status, char* buffer, int buffer_size);

int http_server_config_init(HttpServerConfig* config);
int http_server_config_validate(const HttpServerConfig* config);
int system_status_update(SystemStatus* status, const struct HttpServer* server);
int server_stats_update(ServerStats* stats, const struct HttpServer* server);
const char* http_method_to_string(HttpMethod method);
const char* api_endpoint_to_string(ApiEndpointType endpoint);

#endif /* HTTP_SERVER_H */