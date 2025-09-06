#ifndef SIMPLE_HTTP_SERVER_H
#define SIMPLE_HTTP_SERVER_H

#include <time.h>
#include <stddef.h>
#include "../satellite/satellite.h"
#include "../aircraft/aircraft.h"
#include "../obstruction/obstruction.h"

/* HTTP方法类型 */
typedef enum {
    HTTP_GET = 1,
    HTTP_POST = 2,
    HTTP_PUT = 3,
    HTTP_DELETE = 4,
    HTTP_HEAD = 5
} HttpMethod;

/* HTTP请求结构 */
typedef struct {
    HttpMethod method;
    char* path;
    char* query_string;
    char* headers;
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
    time_t uptime;
    int request_count;
    int error_count;
    double cpu_usage;
    double memory_usage;
    ServerStats stats;
} SystemStatus;

/* HTTP服务器 */
typedef struct {
    HttpServerConfig config;
    SystemStatus status;
    SatelliteData* satellite_data;
    FlightTrajectory* trajectory;
    AircraftGeometry* geometry;
    int is_running;
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

HttpRequest* http_request_create();
void http_request_destroy(HttpRequest* request);
HttpResponse* http_response_create();
void http_response_destroy(HttpResponse* response);

int http_request_parse(const char* raw_request, HttpRequest* request);
int http_response_serialize(const HttpResponse* response, char* buffer, int buffer_size);
int http_response_set_json(HttpResponse* response, const char* json_data);
int http_response_set_error(HttpResponse* response, int status_code, const char* message);

int http_server_config_init(HttpServerConfig* config);
int http_server_config_validate(const HttpServerConfig* config);
int system_status_update(SystemStatus* status, const HttpServer* server);
int server_stats_update(ServerStats* stats, const HttpServer* server);

const char* http_method_to_string(HttpMethod method);

#endif /* SIMPLE_HTTP_SERVER_H */