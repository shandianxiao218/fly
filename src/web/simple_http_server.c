#include "simple_http_server.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* =================== HTTP服务器管理 =================== */

HttpServer* http_server_create(const HttpServerConfig* config) {
    if (config == NULL) return NULL;
    
    HttpServer* server = (HttpServer*)safe_malloc(sizeof(HttpServer));
    if (server == NULL) return NULL;
    
    /* 复制配置 */
    memcpy(&server->config, config, sizeof(HttpServerConfig));
    
    /* 初始化状态 */
    memset(&server->status, 0, sizeof(SystemStatus));
    server->status.is_running = 0;
    server->status.start_time = time(NULL);
    
    /* 初始化数据指针 */
    server->satellite_data = NULL;
    server->trajectory = NULL;
    server->geometry = NULL;
    
    /* 初始化服务器状态 */
    server->is_running = 0;
    
    return server;
}

void http_server_destroy(HttpServer* server) {
    if (server == NULL) return;
    
    /* 停止服务器 */
    if (server->is_running) {
        http_server_stop(server);
    }
    
    /* 释放配置字符串 */
    if (server->config.host) {
        safe_free((void**)&server->config.host);
    }
    if (server->config.static_dir) {
        safe_free((void**)&server->config.static_dir);
    }
    
    /* 释放服务器结构 */
    safe_free((void**)&server);
}

int http_server_start(HttpServer* server) {
    if (server == NULL) return 0;
    
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "HTTP服务器启动");
    
    server->is_running = 1;
    server->status.is_running = 1;
    server->status.start_time = time(NULL);
    
    return 1;
}

int http_server_stop(HttpServer* server) {
    if (server == NULL) return 0;
    
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "HTTP服务器停止");
    
    server->is_running = 0;
    server->status.is_running = 0;
    
    return 1;
}

int http_server_restart(HttpServer* server) {
    if (server == NULL) return 0;
    
    if (!http_server_stop(server)) {
        return 0;
    }
    
    return http_server_start(server);
}

/* =================== 数据设置 =================== */

int http_server_set_data(HttpServer* server, 
                        SatelliteData* satellite_data,
                        FlightTrajectory* trajectory,
                        AircraftGeometry* geometry) {
    if (server == NULL) return 0;
    
    server->satellite_data = satellite_data;
    server->trajectory = trajectory;
    server->geometry = geometry;
    
    return 1;
}

/* =================== HTTP请求/响应处理 =================== */

HttpRequest* http_request_create() {
    HttpRequest* request = (HttpRequest*)safe_malloc(sizeof(HttpRequest));
    if (request == NULL) return NULL;
    
    memset(request, 0, sizeof(HttpRequest));
    request->method = HTTP_GET;
    
    return request;
}

void http_request_destroy(HttpRequest* request) {
    if (request == NULL) return;
    
    if (request->path) {
        safe_free((void**)&request->path);
    }
    if (request->query_string) {
        safe_free((void**)&request->query_string);
    }
    if (request->headers) {
        safe_free((void**)&request->headers);
    }
    if (request->body) {
        safe_free((void**)&request->body);
    }
    
    safe_free((void**)&request);
}

HttpResponse* http_response_create() {
    HttpResponse* response = (HttpResponse*)safe_malloc(sizeof(HttpResponse));
    if (response == NULL) return NULL;
    
    memset(response, 0, sizeof(HttpResponse));
    response->status_code = 200;
    
    return response;
}

void http_response_destroy(HttpResponse* response) {
    if (response == NULL) return;
    
    if (response->status_message) {
        safe_free((void**)&response->status_message);
    }
    if (response->headers) {
        safe_free((void**)&response->headers);
    }
    if (response->body) {
        safe_free((void**)&response->body);
    }
    
    safe_free((void**)&response);
}

int http_request_parse(const char* raw_request, HttpRequest* request) {
    if (raw_request == NULL || request == NULL) return 0;
    
    /* 简单的HTTP请求解析实现 */
    char method[16] = {0};
    char path[256] = {0};
    char version[16] = {0};
    
    /* 解析请求行 */
    if (sscanf(raw_request, "%15s %255s %15s", method, path, version) != 3) {
        return 0;
    }
    
    /* 设置请求方法 */
    if (strcmp(method, "GET") == 0) {
        request->method = HTTP_GET;
    } else if (strcmp(method, "POST") == 0) {
        request->method = HTTP_POST;
    } else if (strcmp(method, "PUT") == 0) {
        request->method = HTTP_PUT;
    } else if (strcmp(method, "DELETE") == 0) {
        request->method = HTTP_DELETE;
    } else if (strcmp(method, "HEAD") == 0) {
        request->method = HTTP_HEAD;
    } else {
        request->method = HTTP_GET; /* 默认为GET */
    }
    
    /* 设置请求路径 */
    request->path = safe_strdup(path);
    
    return 1;
}

int http_response_serialize(const HttpResponse* response, char* buffer, int buffer_size) {
    if (response == NULL || buffer == NULL || buffer_size <= 0) return 0;
    
    /* 获取状态消息 */
    const char* status_message = response->status_message ? response->status_message : "OK";
    
    /* 简单的HTTP响应序列化实现 */
    int written = snprintf(buffer, buffer_size, "HTTP/1.1 %d %s\r\n\r\n", 
                         response->status_code, status_message);
    
    if (written >= buffer_size) {
        return 0; /* 缓冲区不足 */
    }
    
    return written;
}

int http_response_set_json(HttpResponse* response, const char* json_data) {
    if (response == NULL || json_data == NULL) return 0;
    
    response->body = safe_strdup(json_data);
    response->content_length = strlen(json_data);
    
    return 1;
}

int http_response_set_error(HttpResponse* response, int status_code, const char* message) {
    if (response == NULL) return 0;
    
    response->status_code = status_code;
    if (message) {
        response->status_message = safe_strdup(message);
    }
    
    return 1;
}

/* =================== 工具函数 =================== */

int http_server_config_init(HttpServerConfig* config) {
    if (config == NULL) return 0;
    
    memset(config, 0, sizeof(HttpServerConfig));
    config->port = 8080;
    config->host = safe_strdup("localhost");
    config->max_connections = 10;
    config->timeout = 30;
    config->static_dir = safe_strdup("./static");
    
    return 1;
}

int http_server_config_validate(const HttpServerConfig* config) {
    if (config == NULL) return 0;
    
    if (config->port <= 0 || config->port > 65535) {
        return 0;
    }
    
    if (config->host == NULL || strlen(config->host) == 0) {
        return 0;
    }
    
    if (config->max_connections <= 0) {
        return 0;
    }
    
    return 1;
}

int system_status_update(SystemStatus* status, const HttpServer* server) {
    if (status == NULL || server == NULL) return 0;
    
    /* 更新运行时间 */
    if (server->is_running) {
        time_t current_time = time(NULL);
        status->uptime = current_time - status->start_time;
    }
    
    return 1;
}

int server_stats_update(ServerStats* stats, const HttpServer* server) {
    if (stats == NULL || server == NULL) return 0;
    
    /* 更新统计信息 */
    stats->start_time = server->status.start_time;
    stats->total_requests = server->status.request_count;
    
    return 1;
}

const char* http_method_to_string(HttpMethod method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        case HTTP_HEAD: return "HEAD";
        default: return "UNKNOWN";
    }
}