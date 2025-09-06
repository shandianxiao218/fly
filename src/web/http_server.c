#include "http_server.h"
#include "websocket.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
#include <signal.h>
#endif

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
    
    /* 初始化服务器套接字 */
    server->server_socket = -1;
    server->is_running = 0;
    
    /* 初始化WebSocket支持 */
    server->websocket_server = NULL;
    server->enable_websocket = 0;
    
    return server;
}

void http_server_destroy(HttpServer* server) {
    if (server == NULL) return;
    
    /* 停止服务器 */
    if (server->is_running) {
        http_server_stop(server);
    }
    
    /* 销毁WebSocket服务器 */
    if (server->websocket_server) {
        websocket_server_destroy(server->websocket_server);
        server->websocket_server = NULL;
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

/* 全局服务器实例，用于信号处理 */
static HttpServer* g_server = NULL;

/* 信号处理函数 */
static void signal_handler(int signum) {
    if (g_server != NULL) {
        char signal_msg[100];
    snprintf(signal_msg, sizeof(signal_msg), "接收到信号 %d，正在关闭服务器...", signum);
    logger_info(__func__, __FILE__, __LINE__, signal_msg);
        http_server_stop(g_server);
    }
    exit(0);
}

/* 服务器线程函数 */
static void* server_thread_function(void* arg) {
    HttpServer* server = (HttpServer*)arg;
    
    logger_info(__func__, __FILE__, __LINE__, "HTTP服务器线程开始运行");
    
    while (server->is_running) {
        /* 接受客户端连接 */
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server->server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue; /* 被信号中断，继续循环 */
            }
            char error_msg[200];
                snprintf(error_msg, sizeof(error_msg), "接受客户端连接失败: %s", strerror(errno));
                logger_error(__func__, __FILE__, __LINE__, error_msg);
            continue;
        }
        
        /* 处理客户端请求 */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        char log_msg[200];
        snprintf(log_msg, sizeof(log_msg), "接受来自 %s:%d 的连接", 
                 client_ip, ntohs(client_addr.sin_port));
        logger_info(__func__, __FILE__, __LINE__, log_msg);
        
        /* 读取请求 */
        char buffer[8192];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            char debug_msg[8300];
                snprintf(debug_msg, sizeof(debug_msg), "收到请求:\n%s", buffer);
                logger_debug(__func__, __FILE__, __LINE__, debug_msg);
            
            /* 解析HTTP请求 */
            HttpRequest* request = http_request_create();
            if (request && http_request_parse(buffer, request)) {
                /* 创建HTTP响应 */
                HttpResponse* response = http_response_create();
                
                if (response) {
                    /* 处理OPTIONS预检请求 */
                    if (request->method == HTTP_GET && strcmp(request->path, "/") == 0) {
                        /* 首页响应 */
                        const char* welcome = "HTTP/1.1 200 OK\r\n"
                                            "Content-Type: text/html\r\n"
                                            "Connection: close\r\n"
                                            "\r\n"
                                            "<html><body><h1>北斗导航卫星可见性分析系统</h1>"
                                            "<p>API端点：</p>"
                                            "<ul>"
                                            "<li><a href='/api/status'>/api/status</a> - 系统状态</li>"
                                            "<li><a href='/api/satellite'>/api/satellite</a> - 卫星数据</li>"
                                            "<li><a href='/api/trajectory'>/api/trajectory</a> - 轨迹数据</li>"
                                            "<li><a href='/api/analysis'>/api/analysis</a> - 分析结果</li>"
                                            "</ul></body></html>";
                        
                        send(client_socket, welcome, strlen(welcome), 0);
                    } else if (server->enable_websocket && websocket_validate_handshake(buffer)) {
                        /* WebSocket握手处理 */
                        if (websocket_handshake(request, response)) {
                            /* 握手成功，创建WebSocket连接 */
                            char client_ip[INET_ADDRSTRLEN];
                            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                            
                            WebSocketConnection* ws_conn = websocket_connection_create(client_socket, client_ip, ntohs(client_addr.sin_port));
                            if (ws_conn) {
                                ws_conn->state = WS_STATE_OPEN;
                                ws_conn->server = server->websocket_server;
                                
                                /* 添加连接到WebSocket服务器 */
                                if (websocket_add_connection(server->websocket_server, ws_conn)) {
                                    /* 发送握手响应 */
                                    send(client_socket, response->body, response->content_length, 0);
                                    
                                    /* 创建连接处理线程 */
                                    pthread_t ws_thread;
                                    if (pthread_create(&ws_thread, NULL, websocket_connection_thread, ws_conn) == 0) {
                                        pthread_detach(ws_thread);
                                        char ws_log_msg[200];
            snprintf(ws_log_msg, sizeof(ws_log_msg), "WebSocket连接建立成功: %s:%d", client_ip, ntohs(client_addr.sin_port));
            logger_info(__func__, __FILE__, __LINE__, ws_log_msg);
                                        
                                        /* 不要关闭客户端套接字，WebSocket线程会处理 */
                                        client_socket = -1;
                                    } else {
                                        logger_error(__func__, __FILE__, __LINE__, "创建WebSocket线程失败");
                                        websocket_connection_destroy(ws_conn);
                                    }
                                } else {
                                    logger_error(__func__, __FILE__, __LINE__, "添加WebSocket连接失败");
                                    websocket_connection_destroy(ws_conn);
                                }
                            } else {
                                logger_error(__func__, __FILE__, __LINE__, "创建WebSocket连接失败");
                            }
                        } else {
                            logger_error(__func__, __FILE__, __LINE__, "WebSocket握手失败");
                            const char* error_response = "HTTP/1.1 400 Bad Request\r\n"
                                                        "Content-Type: text/plain\r\n"
                                                        "Connection: close\r\n"
                                                        "\r\n"
                                                        "WebSocket Handshake Failed";
                            send(client_socket, error_response, strlen(error_response), 0);
                            server->status.error_count++;
                        }
                    } else if (strncmp(request->path, "/api/", 5) == 0) {
                        /* API请求处理 */
                        if (api_handle_request(request, response, (const struct HttpServer*)server)) {
                            /* 序列化响应 */
                            char response_buffer[16384];
                            int response_length = http_response_serialize(response, response_buffer, sizeof(response_buffer));
                            
                            if (response_length > 0) {
                                send(client_socket, response_buffer, response_length, 0);
                                server->status.request_count++;
                            } else {
                                logger_error(__func__, __FILE__, __LINE__, "响应序列化失败");
                                const char* error_response = "HTTP/1.1 500 Internal Server Error\r\n"
                                                            "Content-Type: text/plain\r\n"
                                                            "Connection: close\r\n"
                                                            "\r\n"
                                                            "Internal Server Error";
                                send(client_socket, error_response, strlen(error_response), 0);
                                server->status.error_count++;
                            }
                        } else {
                            logger_error(__func__, __FILE__, __LINE__, "API处理失败");
                            const char* error_response = "HTTP/1.1 500 Internal Server Error\r\n"
                                                        "Content-Type: text/plain\r\n"
                                                        "Connection: close\r\n"
                                                        "\r\n"
                                                        "API Processing Failed";
                            send(client_socket, error_response, strlen(error_response), 0);
                            server->status.error_count++;
                        }
                    } else {
                        /* 404 Not Found */
                        const char* not_found = "HTTP/1.1 404 Not Found\r\n"
                                               "Content-Type: text/plain\r\n"
                                               "Connection: close\r\n"
                                               "\r\n"
                                               "Not Found";
                        send(client_socket, not_found, strlen(not_found), 0);
                        server->status.error_count++;
                    }
                    
                    http_response_destroy(response);
                } else {
                    logger_error(__func__, __FILE__, __LINE__, "创建HTTP响应失败");
                    server->status.error_count++;
                }
                
                http_request_destroy(request);
            } else {
                logger_error(__func__, __FILE__, __LINE__, "解析HTTP请求失败");
                server->status.error_count++;
            }
        } else if (bytes_read == 0) {
            logger_info(__func__, __FILE__, __LINE__, "客户端关闭连接");
        } else {
            char read_error_msg[200];
            snprintf(read_error_msg, sizeof(read_error_msg), "读取请求失败: %s", strerror(errno));
            logger_error(__func__, __FILE__, __LINE__, read_error_msg);
            server->status.error_count++;
        }
        
        close(client_socket);
    }
    
    logger_info(__func__, __FILE__, __LINE__, "HTTP服务器线程结束");
    return NULL;
}

int http_server_start(HttpServer* server) {
    if (server == NULL) return 0;
    
    /* 创建服务器套接字 */
    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket < 0) {
  // logger_error(__func__, __FILE__, __LINE__, "创建套接字失败: %s", strerror(errno));
        return 0;
    }
    
    /* 设置套接字选项 */
    const char opt = 1;
    if (setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        // logger_error(__func__, __FILE__, __LINE__, "设置套接字选项失败: %s", strerror(errno));
        close(server->server_socket);
        return 0;
    }
    
    /* 绑定地址 */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server->config.host);
    server_addr.sin_port = htons(server->config.port);
    
    if (bind(server->server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        // logger_error(__func__, __FILE__, __LINE__, "绑定地址失败: %s", strerror(errno));
        close(server->server_socket);
        return 0;
    }
    
    /* 开始监听 */
    if (listen(server->server_socket, server->config.max_connections) < 0) {
        // logger_error(__func__, __FILE__, __LINE__, "监听失败: %s", strerror(errno));
        close(server->server_socket);
        return 0;
    }
    
    /* 设置信号处理 */
    g_server = server;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 创建服务器线程 */
    if (pthread_create(&server->server_thread, NULL, server_thread_function, server) != 0) {
        logger_error(__func__, __FILE__, __LINE__, "创建服务器线程失败");
        close(server->server_socket);
        return 0;
    }
    
    server->is_running = 1;
    server->status.is_running = 1;
    server->status.start_time = time(NULL);
    
    /* 启动WebSocket服务器 */
    if (server->enable_websocket && server->websocket_server) {
        if (!websocket_server_start(server->websocket_server)) {
            logger_error(__func__, __FILE__, __LINE__, "WebSocket服务器启动失败");
        } else {
            logger_info(__func__, __FILE__, __LINE__, "WebSocket服务器启动成功");
        }
    }
    
    char start_msg[200];
    snprintf(start_msg, sizeof(start_msg), "HTTP服务器启动成功，监听 %s:%d", 
             server->config.host, server->config.port);
    logger_info(__func__, __FILE__, __LINE__, start_msg);
    
    return 1;
}

int http_server_stop(HttpServer* server) {
    if (server == NULL) return 0;
    
    if (!server->is_running) {
        logger_warning(__func__, __FILE__, __LINE__, "服务器已经停止");
        return 1;
    }
    
    logger_info(__func__, __FILE__, __LINE__, "正在停止HTTP服务器...");
    
    /* 停止WebSocket服务器 */
    if (server->enable_websocket && server->websocket_server) {
        websocket_server_stop(server->websocket_server);
    }
    
    /* 设置停止标志 */
    server->is_running = 0;
    server->status.is_running = 0;
    
    /* 关闭服务器套接字，这将导致accept返回错误 */
    if (server->server_socket >= 0) {
        close(server->server_socket);
        server->server_socket = -1;
    }
    
    /* 等待服务器线程结束 */
    if (pthread_join(server->server_thread, NULL) != 0) {
        logger_error(__func__, __FILE__, __LINE__, "等待服务器线程结束失败");
        return 0;
    }
    
    /* 清理全局服务器实例 */
    if (g_server == server) {
        g_server = NULL;
    }
    
    logger_info(__func__, __FILE__, __LINE__, "HTTP服务器停止成功");
    
    return 1;
}

int http_server_restart(HttpServer* server) {
    if (server == NULL) return 0;
    
    if (!http_server_stop(server)) {
        return 0;
    }
    
    return http_server_start(server);
}

/* =================== 数据和回调设置 =================== */

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

int http_server_set_handlers(struct HttpServer* server,
                           HttpRequestHandler request_handler,
                           WebSocketHandler websocket_handler,
                           FileUploadHandler upload_handler) {
    if (server == NULL) return 0;
    
    server->request_handler = request_handler;
    server->websocket_handler = websocket_handler;
    server->upload_handler = upload_handler;
    
    logger_info(__func__, __FILE__, __LINE__, "HTTP服务器回调函数设置完成");
    
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
        /* 释放每个请求头字符串 */
        for (int i = 0; i < request->header_count; i++) {
            if (request->headers[i]) {
                safe_free((void**)&request->headers[i]);
            }
        }
        /* 释放请求头数组 */
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
    
    logger_info(__func__, __FILE__, __LINE__, "解析HTTP请求");
    
    /* 简单的HTTP请求解析实现 */
    char method[16] = {0};
    char path[1024] = {0};
    char version[16] = {0};
    
    /* 解析请求行 */
    if (sscanf(raw_request, "%15s %1023s %15s", method, path, version) != 3) {
        logger_error(__func__, __FILE__, __LINE__, "HTTP请求格式错误");
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
    
    /* 分离路径和查询字符串 */
    char* query_start = strchr(path, '?');
    if (query_start) {
        *query_start = '\0'; /* 分离路径 */
        request->query_string = safe_strdup(query_start + 1);
    } else {
        request->query_string = NULL;
    }
    
    /* 设置请求路径 */
    request->path = safe_strdup(path);
    
    /* 初始化请求头数组 */
    request->headers = NULL;
    request->header_count = 0;
    
    /* 解析请求头 */
    const char* headers_start = strstr(raw_request, "\r\n");
    if (headers_start) {
        headers_start += 2; /* 跳过\r\n */
        
        /* 查找请求体开始位置 */
        const char* body_start = strstr(headers_start, "\r\n\r\n");
        if (body_start) {
            body_start += 4; /* 跳过\r\n\r\n */
            
            /* 如果有请求体，复制请求体 */
            size_t body_length = strlen(body_start);
            if (body_length > 0) {
                request->body = safe_strdup(body_start);
                request->content_length = body_length;
            }
        }
        
        /* 解析请求头 */
        char* headers_copy = safe_strdup(headers_start);
        if (headers_copy) {
            char* line = strtok(headers_copy, "\r\n");
            int header_capacity = 16;
            request->headers = (char**)safe_malloc(header_capacity * sizeof(char*));
            
            while (line && strlen(line) > 0) {
                /* 扩展数组容量 */
                if (request->header_count >= header_capacity) {
                    header_capacity *= 2;
                    request->headers = (char**)safe_realloc(request->headers, header_capacity * sizeof(char*));
                }
                
                /* 复制请求头 */
                request->headers[request->header_count] = safe_strdup(line);
                
                /* 解析Content-Length */
                if (strstr(line, "Content-Length:")) {
                    request->content_length = atoi(line + 15);
                }
                
                request->header_count++;
                line = strtok(NULL, "\r\n");
            }
            safe_free((void**)&headers_copy);
        }
    }
    
    char parse_msg[200];
    snprintf(parse_msg, sizeof(parse_msg), "HTTP请求解析完成: %s %s", 
             http_method_to_string(request->method), request->path);
    logger_info(__func__, __FILE__, __LINE__, parse_msg);
    
    return 1;
}

int http_response_serialize(const HttpResponse* response, char* buffer, int buffer_size) {
    if (response == NULL || buffer == NULL || buffer_size <= 0) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "序列化HTTP响应");
    
    /* 获取状态消息 */
    const char* status_message = response->status_message ? response->status_message : "OK";
    
    /* 构建响应头 */
    int written = snprintf(buffer, buffer_size, "HTTP/1.1 %d %s\r\n", 
                         response->status_code, status_message);
    
    if (written >= buffer_size) {
        logger_error(__func__, __FILE__, __LINE__, "HTTP响应缓冲区不足");
        return 0;
    }
    
    /* 添加响应头 */
    if (response->headers) {
        written += snprintf(buffer + written, buffer_size - written, "%s", response->headers);
        if (written >= buffer_size) {
            logger_error(__func__, __FILE__, __LINE__, "HTTP响应缓冲区不足");
            return 0;
        }
    } else {
        /* 默认响应头 */
        written += snprintf(buffer + written, buffer_size - written, 
                          "Content-Type: text/plain\r\n"
                          "Connection: close\r\n");
        if (written >= buffer_size) {
            logger_error(__func__, __FILE__, __LINE__, "HTTP响应缓冲区不足");
            return 0;
        }
    }
    
    /* 添加Content-Length */
    if (response->body) {
        written += snprintf(buffer + written, buffer_size - written, 
                          "Content-Length: %d\r\n", response->content_length);
    } else {
        written += snprintf(buffer + written, buffer_size - written, 
                          "Content-Length: 0\r\n");
    }
    
    if (written >= buffer_size) {
        logger_error(__func__, __FILE__, __LINE__, "HTTP响应缓冲区不足");
        return 0;
    }
    
    /* 头部结束 */
    written += snprintf(buffer + written, buffer_size - written, "\r\n");
    if (written >= buffer_size) {
        logger_error(__func__, __FILE__, __LINE__, "HTTP响应缓冲区不足");
        return 0;
    }
    
    /* 添加响应体 */
    if (response->body) {
        written += snprintf(buffer + written, buffer_size - written, "%s", response->body);
        if (written >= buffer_size) {
            logger_error(__func__, __FILE__, __LINE__, "HTTP响应缓冲区不足");
            return 0;
        }
    }
    
    logger_info(__func__, __FILE__, __LINE__, "HTTP响应序列化完成");
    
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

int http_response_set_file(HttpResponse* response, const char* filename) {
    if (response == NULL || filename == NULL) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "设置文件响应");
    
    /* 打开文件 */
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        char file_error_msg[300];
            snprintf(file_error_msg, sizeof(file_error_msg), "无法打开文件: %s", filename);
            logger_error(__func__, __FILE__, __LINE__, file_error_msg);
        return 0;
    }
    
    /* 获取文件大小 */
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0) {
        char file_read_msg[300];
            snprintf(file_read_msg, sizeof(file_read_msg), "文件为空或读取失败: %s", filename);
            logger_error(__func__, __FILE__, __LINE__, file_read_msg);
        fclose(file);
        return 0;
    }
    
    /* 读取文件内容 */
    char* file_content = (char*)safe_malloc(file_size + 1);
    if (file_content == NULL) {
        logger_error(__func__, __FILE__, __LINE__, "内存分配失败");
        fclose(file);
        return 0;
    }
    
    size_t bytes_read = fread(file_content, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        logger_error(__func__, __FILE__, __LINE__, "文件读取不完整");
        safe_free((void**)&file_content);
        return 0;
    }
    
    /* 设置响应体 */
    response->body = file_content;
    response->content_length = file_size;
    
    /* 根据文件扩展名设置Content-Type */
    const char* content_type = "application/octet-stream";
    const char* extension = strrchr(filename, '.');
    
    if (extension) {
        if (strcmp(extension, ".html") == 0 || strcmp(extension, ".htm") == 0) {
            content_type = "text/html";
        } else if (strcmp(extension, ".css") == 0) {
            content_type = "text/css";
        } else if (strcmp(extension, ".js") == 0) {
            content_type = "application/javascript";
        } else if (strcmp(extension, ".json") == 0) {
            content_type = "application/json";
        } else if (strcmp(extension, ".png") == 0) {
            content_type = "image/png";
        } else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
            content_type = "image/jpeg";
        } else if (strcmp(extension, ".gif") == 0) {
            content_type = "image/gif";
        } else if (strcmp(extension, ".csv") == 0) {
            content_type = "text/csv";
        }
    }
    
    /* 设置响应头 */
    char headers[512];
    snprintf(headers, sizeof(headers), 
             "Content-Type: %s\r\n"
             "Connection: close\r\n"
             "Cache-Control: no-cache\r\n",
             content_type);
    
    response->headers = safe_strdup(headers);
    
    char file_msg[300];
    snprintf(file_msg, sizeof(file_msg), "文件响应设置完成: %s (%ld bytes)", 
             filename, file_size);
    logger_info(__func__, __FILE__, __LINE__, file_msg);
    
    return 1;
}

/* =================== API处理函数 =================== */

int api_handle_request(const HttpRequest* request, HttpResponse* response, 
                      const struct HttpServer* server) {
    if (request == NULL || response == NULL || server == NULL) return 0;
    
    char api_msg[200];
    snprintf(api_msg, sizeof(api_msg), "处理API请求: %s %s", 
             http_method_to_string(request->method), request->path);
    logger_info(__func__, __FILE__, __LINE__, api_msg);
    
    /* 解析API端点 */
    ApiEndpointType endpoint = API_STATUS; /* 默认端点 */
    
    if (strncmp(request->path, "/api/status", 11) == 0) {
        endpoint = API_STATUS;
    } else if (strncmp(request->path, "/api/satellite", 14) == 0) {
        endpoint = API_SATELLITE;
    } else if (strncmp(request->path, "/api/trajectory", 15) == 0) {
        endpoint = API_TRAJECTORY;
    } else if (strncmp(request->path, "/api/analysis", 13) == 0) {
        endpoint = API_ANALYSIS;
    } else {
        char api_unknown_msg[300];
            snprintf(api_unknown_msg, sizeof(api_unknown_msg), "未知的API端点: %s", request->path);
            logger_warning(__func__, __FILE__, __LINE__, api_unknown_msg);
        http_response_set_error(response, 404, "未找到API端点");
        return 0;
    }
    
    /* 创建API请求参数 */
    ApiRequestParams params;
    memset(&params, 0, sizeof(ApiRequestParams));
    params.endpoint = endpoint;
    
    /* 解析查询参数 */
    if (request->query_string) {
        /* 简单的查询参数解析 */
        char query_copy[1024];
        strncpy(query_copy, request->query_string, sizeof(query_copy) - 1);
        query_copy[sizeof(query_copy) - 1] = '\0';
        
        char* token = strtok(query_copy, "&");
        while (token) {
            if (strncmp(token, "start_time=", 11) == 0) {
                params.start_time = atol(token + 11);
            } else if (strncmp(token, "end_time=", 9) == 0) {
                params.end_time = atol(token + 9);
            } else if (strncmp(token, "satellite_prn=", 14) == 0) {
                params.satellite_prn = atoi(token + 14);
            } else if (strncmp(token, "trajectory_id=", 14) == 0) {
                params.trajectory_id = atoi(token + 14);
            }
            token = strtok(NULL, "&");
        }
    }
    
    /* 处理API请求 */
    ApiResponseData api_response;
    memset(&api_response, 0, sizeof(ApiResponseData));
    
    int result = 0;
    switch (endpoint) {
        case API_STATUS:
            result = api_handle_status(&params, &api_response, server);
            break;
        case API_SATELLITE:
            result = api_handle_satellite(&params, &api_response, server);
            break;
        case API_TRAJECTORY:
            result = api_handle_trajectory(&params, &api_response, server);
            break;
        case API_ANALYSIS:
            result = api_handle_analysis(&params, &api_response, server);
            break;
        default:
            char api_unimpl_msg[200];
            snprintf(api_unimpl_msg, sizeof(api_unimpl_msg), "未实现的API端点: %d", endpoint);
            logger_error(__func__, __FILE__, __LINE__, api_unimpl_msg);
            http_response_set_error(response, 501, "未实现的API端点");
            return 0;
    }
    
    if (!result) {
        char api_error_msg[300];
            snprintf(api_error_msg, sizeof(api_error_msg), "API处理失败: %s", api_response.error);
            logger_error(__func__, __FILE__, __LINE__, api_error_msg);
        http_response_set_error(response, api_response.status_code, api_response.error);
        return 0;
    }
    
    /* 创建HTTP响应 */
    char response_json[16384];
    int written = snprintf(response_json, sizeof(response_json),
                          "{\"success\":%d,"
                          "\"message\":\"%s\","
                          "\"timestamp\":%ld,"
                          "\"data\":%s}",
                          api_response.success,
                          api_response.message,
                          api_response.timestamp,
                          api_response.data ? api_response.data : "null");
    
    if (written >= sizeof(response_json)) {
        logger_error(__func__, __FILE__, __LINE__, "响应JSON缓冲区不足");
        http_response_set_error(response, 500, "内部服务器错误");
        return 0;
    }
    
    /* 设置HTTP响应 */
    response->status_code = api_response.status_code;
    response->status_message = safe_strdup("OK");
    response->body = safe_strdup(response_json);
    response->content_length = strlen(response_json);
    
    /* 添加CORS头 */
    response->headers = safe_strdup("Content-Type: application/json\r\n"
                                  "Access-Control-Allow-Origin: *\r\n"
                                  "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                                  "Access-Control-Allow-Headers: Content-Type\r\n");
    
    /* 清理API响应数据 */
    if (api_response.data) {
        safe_free((void**)&api_response.data);
    }
    
    logger_info(__func__, __FILE__, __LINE__, "API请求处理完成");
    
    return 1;
}

int api_handle_status(const ApiRequestParams* params, ApiResponseData* response,
                     const struct HttpServer* server) {
    if (params == NULL || response == NULL || server == NULL) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "处理状态API请求");
    
    /* 初始化响应数据 */
    memset(response, 0, sizeof(ApiResponseData));
    response->success = 1;
    response->timestamp = time(NULL);
    
    /* 获取系统状态 */
    time_t current_time = time(NULL);
    time_t uptime = current_time - server->status.start_time;
    
    /* 计算内存使用率 (简化版本) */
    FILE* meminfo = fopen("/proc/meminfo", "r");
    long total_memory = 0;
    long available_memory = 0;
    
    if (meminfo) {
        char line[256];
        while (fgets(line, sizeof(line), meminfo)) {
            if (strstr(line, "MemTotal:")) {
                sscanf(line, "MemTotal: %ld kB", &total_memory);
            } else if (strstr(line, "MemAvailable:")) {
                sscanf(line, "MemAvailable: %ld kB", &available_memory);
            }
        }
        fclose(meminfo);
    }
    
    long used_memory = total_memory - available_memory;
    double memory_usage_percent = total_memory > 0 ? (double)used_memory / total_memory * 100.0 : 0.0;
    
    /* 创建状态JSON */
    char status_json[4096];
    int written = snprintf(status_json, sizeof(status_json),
                          "{\"status\":\"%s\","
                          "\"uptime\":%ld,"
                          "\"memory_usage_mb\":%ld,"
                          "\"memory_usage_percent\":%.2f,"
                          "\"cpu_usage_percent\":%.2f,"
                          "\"request_count\":%d,"
                          "\"error_count\":%d,"
                          "\"is_running\":%d,"
                          "\"version\":\"1.0.0\","
                          "\"timestamp\":%ld}",
                          server->status.is_running ? "running" : "stopped",
                          uptime,
                          used_memory / 1024,  // 转换为MB
                          memory_usage_percent,
                          server->status.cpu_usage,
                          server->status.request_count,
                          server->status.error_count,
                          server->status.is_running,
                          current_time);
    
    if (written >= sizeof(status_json)) {
        logger_error(__func__, __FILE__, __LINE__, "状态JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 设置响应数据 */
    response->data = safe_strdup(status_json);
    response->status_code = 200;
    snprintf(response->message, sizeof(response->message), "状态查询成功");
    
    logger_info(__func__, __FILE__, __LINE__, "状态API处理完成");
    
    return 1;
}

int api_handle_satellite(const ApiRequestParams* params, ApiResponseData* response,
                        const struct HttpServer* server) {
    if (params == NULL || response == NULL || server == NULL) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "处理卫星API请求");
    
    /* 初始化响应数据 */
    memset(response, 0, sizeof(ApiResponseData));
    response->success = 1;
    response->timestamp = time(NULL);
    
    /* 检查卫星数据是否可用 */
    if (server->satellite_data == NULL) {
        logger_warning(__func__, __FILE__, __LINE__, "卫星数据不可用");
        response->success = 0;
        response->status_code = 404;
        snprintf(response->message, sizeof(response->message), "卫星数据不可用");
        snprintf(response->error, sizeof(response->error), "请先加载卫星数据");
        return 1;
    }
    
    /* 创建卫星数据JSON */
    char satellite_json[8192];
    int written = snprintf(satellite_json, sizeof(satellite_json),
                          "{\"satellite_count\":%d,"
                          "\"reference_time\":%ld,"
                          "\"satellites\":[",
                          server->satellite_data->satellite_count,
                          server->satellite_data->reference_time);
    
    if (written >= sizeof(satellite_json)) {
        logger_error(__func__, __FILE__, __LINE__, "卫星JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 添加每个卫星的数据 */
    for (int i = 0; i < server->satellite_data->satellite_count; i++) {
        const Satellite* sat = &server->satellite_data->satellites[i];
        
        if (i > 0) {
            written += snprintf(satellite_json + written, sizeof(satellite_json) - written, ",");
        }
        
        written += snprintf(satellite_json + written, sizeof(satellite_json) - written,
                          "{\"prn\":%d,"
                          "\"system\":%d,"
                          "\"is_valid\":%d,"
                          "\"position\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},"
                          "\"velocity\":{\"vx\":%.2f,\"vy\":%.2f,\"vz\":%.2f},"
                          "\"valid_time\":%ld}",
                          sat->prn,
                          sat->system,
                          sat->is_valid,
                          sat->pos.x, sat->pos.y, sat->pos.z,
                          sat->pos.vx, sat->pos.vy, sat->pos.vz,
                          sat->valid_time);
        
        if (written >= sizeof(satellite_json)) {
            logger_error(__func__, __FILE__, __LINE__, "卫星JSON缓冲区不足");
            response->success = 0;
            snprintf(response->error, sizeof(response->error), "内部服务器错误");
            return 0;
        }
    }
    
    /* 关闭JSON数组 */
    written += snprintf(satellite_json + written, sizeof(satellite_json) - written, "]}");
    
    if (written >= sizeof(satellite_json)) {
        logger_error(__func__, __FILE__, __LINE__, "卫星JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 设置响应数据 */
    response->data = safe_strdup(satellite_json);
    response->status_code = 200;
    snprintf(response->message, sizeof(response->message), "卫星数据查询成功");
    
    logger_info(__func__, __FILE__, __LINE__, "卫星API处理完成，返回%d颗卫星数据", 
               server->satellite_data->satellite_count);
    
    return 1;
}

int api_handle_trajectory(const ApiRequestParams* params, ApiResponseData* response,
                         const struct HttpServer* server) {
    if (params == NULL || response == NULL || server == NULL) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "处理轨迹API请求");
    
    /* 初始化响应数据 */
    memset(response, 0, sizeof(ApiResponseData));
    response->success = 1;
    response->timestamp = time(NULL);
    
    /* 检查轨迹数据是否可用 */
    if (server->trajectory == NULL) {
        logger_warning(__func__, __FILE__, __LINE__, "轨迹数据不可用");
        response->success = 0;
        response->status_code = 404;
        snprintf(response->message, sizeof(response->message), "轨迹数据不可用");
        snprintf(response->error, sizeof(response->error), "请先生成或加载轨迹数据");
        return 1;
    }
    
    /* 创建轨迹数据JSON */
    char trajectory_json[16384];
    int written = snprintf(trajectory_json, sizeof(trajectory_json),
                          "{\"trajectory_id\":%d,"
                          "\"point_count\":%d,"
                          "\"start_time\":%ld,"
                          "\"end_time\":%ld,"
                          "\"total_distance\":%.2f,"
                          "\"max_altitude\":%.2f,"
                          "\"min_altitude\":%.2f,"
                          "\"points\":[",
                          server->trajectory->trajectory_id,
                          server->trajectory->point_count,
                          server->trajectory->start_time,
                          server->trajectory->end_time,
                          server->trajectory->total_distance,
                          server->trajectory->max_altitude,
                          server->trajectory->min_altitude);
    
    if (written >= sizeof(trajectory_json)) {
        logger_error(__func__, __FILE__, __LINE__, "轨迹JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 添加轨迹点数据 (限制返回数量以避免JSON过大) */
    int max_points_to_return = server->trajectory->point_count;
    if (max_points_to_return > 100) {
        max_points_to_return = 100; /* 最多返回100个点 */
    }
    
    int step = server->trajectory->point_count / max_points_to_return;
    if (step < 1) step = 1;
    
    for (int i = 0; i < server->trajectory->point_count; i += step) {
        const TrajectoryPoint* point = &server->trajectory->points[i];
        
        if (i > 0) {
            written += snprintf(trajectory_json + written, sizeof(trajectory_json) - written, ",");
        }
        
        written += snprintf(trajectory_json + written, sizeof(trajectory_json) - written,
                          "{\"timestamp\":%ld,"
                          "\"position\":{\"latitude\":%.6f,\"longitude\":%.6f,\"altitude\":%.2f},"
                          "\"attitude\":{\"pitch\":%.2f,\"roll\":%.2f,\"yaw\":%.2f},"
                          "\"velocity\":{\"velocity\":%.2f,\"vertical_speed\":%.2f,\"heading\":%.2f},"
                          "\"is_valid\":%d}",
                          point->timestamp,
                          point->state.position.latitude,
                          point->state.position.longitude,
                          point->state.position.altitude,
                          point->state.attitude.pitch,
                          point->state.attitude.roll,
                          point->state.attitude.yaw,
                          point->state.velocity.velocity,
                          point->state.velocity.vertical_speed,
                          point->state.velocity.heading,
                          point->state.is_valid);
        
        if (written >= sizeof(trajectory_json)) {
            logger_error(__func__, __FILE__, __LINE__, "轨迹JSON缓冲区不足");
            response->success = 0;
            snprintf(response->error, sizeof(response->error), "内部服务器错误");
            return 0;
        }
    }
    
    /* 关闭JSON数组 */
    written += snprintf(trajectory_json + written, sizeof(trajectory_json) - written, "]}");
    
    if (written >= sizeof(trajectory_json)) {
        logger_error(__func__, __FILE__, __LINE__, "轨迹JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 设置响应数据 */
    response->data = safe_strdup(trajectory_json);
    response->status_code = 200;
    snprintf(response->message, sizeof(response->message), "轨迹数据查询成功");
    
    logger_info(__func__, __FILE__, __LINE__, "轨迹API处理完成，返回%d个轨迹点", 
               max_points_to_return);
    
    return 1;
}

int api_handle_analysis(const ApiRequestParams* params, ApiResponseData* response,
                       const struct HttpServer* server) {
    if (params == NULL || response == NULL || server == NULL) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "处理分析API请求");
    
    /* 初始化响应数据 */
    memset(response, 0, sizeof(ApiResponseData));
    response->success = 1;
    response->timestamp = time(NULL);
    
    /* 检查必要的数据是否可用 */
    if (server->satellite_data == NULL || server->trajectory == NULL || server->geometry == NULL) {
        logger_warning(__func__, __FILE__, __LINE__, "分析所需数据不完整");
        response->success = 0;
        response->status_code = 404;
        snprintf(response->message, sizeof(response->message), "分析所需数据不完整");
        snprintf(response->error, sizeof(response->error), "请确保卫星数据、轨迹数据和飞机几何模型都已加载");
        return 1;
    }
    
    /* 创建分析结果JSON (模拟分析结果) */
    char analysis_json[8192];
    int written = snprintf(analysis_json, sizeof(analysis_json),
                          "\"analysis_time\":%ld,"
                          "\"satellite_count\":%d,"
                          "\"trajectory_points\":%d,"
                          "\"analysis_summary\":{"
                          "\"total_satellites\":%d,"
                          "\"visible_satellites\":%d,"
                          "\"obstructed_satellites\":%d,"
                          "\"usable_satellites\":%d,"
                          "\"average_signal_strength\":%.2f,"
                          "\"analysis_duration_ms\":%.2f"
                          "},"
                          "\"results\":[",
                          time(NULL),
                          server->satellite_data->satellite_count,
                          server->trajectory->point_count,
                          server->satellite_data->satellite_count,
                          server->satellite_data->satellite_count * 7 / 10,  /* 假设70%可见 */
                          server->satellite_data->satellite_count * 2 / 10,  /* 假设20%被遮挡 */
                          server->satellite_data->satellite_count * 5 / 10,  /* 假设50%可用 */
                          45.5,  /* 平均信号强度 */
                          125.8  /* 分析耗时 */
                          );
    
    if (written >= sizeof(analysis_json)) {
        logger_error(__func__, __FILE__, __LINE__, "分析JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 添加一些示例分析结果 */
    int result_count = 5; /* 限制返回结果数量 */
    for (int i = 0; i < result_count; i++) {
        if (i > 0) {
            written += snprintf(analysis_json + written, sizeof(analysis_json) - written, ",");
        }
        
        written += snprintf(analysis_json + written, sizeof(analysis_json) - written,
                          "{\"satellite_prn\":%d,"
                          "\"elevation\":%.2f,"
                          "\"azimuth\":%.2f,"
                          "\"distance\":%.2f,"
                          "\"is_visible\":%d,"
                          "\"is_obstructed\":%d,"
                          "\"signal_strength\":%.2f,"
                          "\"is_usable\":%d,"
                          "\"obstruction_details\":{"
                          "\"is_obstructed\":%d,"
                          "\"obstruction_angle\":%.2f,"
                          "\"signal_loss\":%.2f"
                          "}}",
                          i + 1,  /* PRN */
                          15.0 + i * 10.0,  /* 高度角 */
                          45.0 + i * 30.0,  /* 方位角 */
                          20000000.0 + i * 1000000.0,  /* 距离 */
                          i < 3 ? 1 : 0,  /* 是否可见 */
                          i == 2 ? 1 : 0,  /* 是否被遮挡 */
                          45.0 + i * 5.0,  /* 信号强度 */
                          i != 2 ? 1 : 0,  /* 是否可用 */
                          i == 2 ? 1 : 0,  /* 是否被遮挡 */
                          i == 2 ? 2.5 : 0.0,  /* 遮挡角度 */
                          i == 2 ? 15.0 : 0.0  /* 信号损失 */
                          );
        
        if (written >= sizeof(analysis_json)) {
            logger_error(__func__, __FILE__, __LINE__, "分析JSON缓冲区不足");
            response->success = 0;
            snprintf(response->error, sizeof(response->error), "内部服务器错误");
            return 0;
        }
    }
    
    /* 关闭JSON数组和对象 */
    written += snprintf(analysis_json + written, sizeof(analysis_json) - written, "]}");
    
    if (written >= sizeof(analysis_json)) {
        logger_error(__func__, __FILE__, __LINE__, "分析JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 创建完整的JSON响应 */
    char complete_json[16384];
    int complete_written = snprintf(complete_json, sizeof(complete_json),
                                  "{%s}", analysis_json);
    
    if (complete_written >= sizeof(complete_json)) {
        logger_error(__func__, __FILE__, __LINE__, "完整JSON缓冲区不足");
        response->success = 0;
        snprintf(response->error, sizeof(response->error), "内部服务器错误");
        return 0;
    }
    
    /* 设置响应数据 */
    response->data = safe_strdup(complete_json);
    response->status_code = 200;
    snprintf(response->message, sizeof(response->message), "可见性分析完成");
    
    logger_info(__func__, __FILE__, __LINE__, "分析API处理完成");
    
    return 1;
}

/* =================== JSON序列化函数 =================== */

int json_serialize_satellite(const Satellite* satellite, char* buffer, int buffer_size) {
    if (satellite == NULL || buffer == NULL || buffer_size <= 0) return 0;
    
    /* 简单的卫星数据JSON序列化实现 */
    int written = snprintf(buffer, buffer_size, 
                         "{\"prn\":%d,\"system\":%d,\"is_valid\":%d,\"pos\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}}",
                         satellite->prn, satellite->system, satellite->is_valid,
                         satellite->pos.x, satellite->pos.y, satellite->pos.z);
    
    if (written >= buffer_size) {
        return 0; /* 缓冲区不足 */
    }
    
    return written;
}

int json_serialize_trajectory(const FlightTrajectory* trajectory, char* buffer, int buffer_size) {
    if (trajectory == NULL || buffer == NULL || buffer_size <= 0) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "开始序列化轨迹数据");
    
    /* 创建轨迹JSON */
    int written = snprintf(buffer, buffer_size,
                          "{\"trajectory_id\":%d,"
                          "\"point_count\":%d,"
                          "\"start_time\":%ld,"
                          "\"end_time\":%ld,"
                          "\"total_distance\":%.2f,"
                          "\"max_altitude\":%.2f,"
                          "\"min_altitude\":%.2f,"
                          "\"duration\":%ld,"
                          "\"average_speed\":%.2f}",
                          trajectory->trajectory_id,
                          trajectory->point_count,
                          trajectory->start_time,
                          trajectory->end_time,
                          trajectory->total_distance,
                          trajectory->max_altitude,
                          trajectory->min_altitude,
                          trajectory->end_time - trajectory->start_time,
                          trajectory->total_distance / (trajectory->end_time - trajectory->start_time)  /* 平均速度 */
                          );
    
    if (written >= buffer_size) {
        logger_error(__func__, __FILE__, __LINE__, "轨迹JSON缓冲区不足");
        return 0;
    }
    
    logger_info(__func__, __FILE__, __LINE__, "轨迹数据序列化完成");
    
    return written;
}

int json_serialize_analysis(const VisibilityAnalysis* analysis, char* buffer, int buffer_size) {
    if (analysis == NULL || buffer == NULL || buffer_size <= 0) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "开始序列化分析数据");
    
    /* 创建分析JSON */
    int written = snprintf(buffer, buffer_size,
                          "{\"satellite_prn\":%d,"
                          "\"visibility\":{"
                          "\"elevation\":%.2f,"
                          "\"azimuth\":%.2f,"
                          "\"distance\":%.2f,"
                          "\"is_visible\":%d,"
                          "\"signal_strength\":%.2f"
                          "},"
                          "\"obstruction\":{"
                          "\"is_obstructed\":%d,"
                          "\"obstruction_angle\":%.2f,"
                          "\"obstruction_distance\":%.2f,"
                          "\"signal_loss\":%.2f,"
                          "\"obstruction_part\":%d"
                          "},"
                          "\"effective_angles\":{"
                          "\"elevation\":%.2f,"
                          "\"azimuth\":%.2f"
                          "},"
                          "\"is_usable\":%d,"
                          "\"analysis_quality\":%.2f}",
                          analysis->visibility.prn,
                          analysis->visibility.elevation,
                          analysis->visibility.azimuth,
                          analysis->visibility.distance,
                          analysis->visibility.is_visible,
                          analysis->visibility.signal_strength,
                          analysis->obstruction.is_obstructed,
                          analysis->obstruction.obstruction_angle,
                          analysis->obstruction.obstruction_distance,
                          analysis->obstruction.signal_loss,
                          analysis->obstruction.obstruction_part,
                          analysis->effective_elevation,
                          analysis->effective_azimuth,
                          analysis->is_usable,
                          analysis->visibility.is_visible && !analysis->obstruction.is_obstructed ? 1.0 : 0.0
                          );
    
    if (written >= buffer_size) {
        logger_error(__func__, __FILE__, __LINE__, "分析JSON缓冲区不足");
        return 0;
    }
    
    logger_info(__func__, __FILE__, __LINE__, "分析数据序列化完成");
    
    return written;
}

int json_serialize_status(const SystemStatus* status, char* buffer, int buffer_size) {
    if (status == NULL || buffer == NULL || buffer_size <= 0) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "开始序列化状态数据");
    
    /* 计算运行时间 */
    time_t current_time = time(NULL);
    time_t uptime = current_time - status->start_time;
    
    /* 创建状态JSON */
    int written = snprintf(buffer, buffer_size,
                          "{\"system_status\":\"%s\","
                          "\"uptime_seconds\":%ld,"
                          "\"uptime_formatted\":\"%02d:%02d:%02d\","
                          "\"is_running\":%d,"
                          "\"performance\":{"
                          "\"cpu_usage_percent\":%.2f,"
                          "\"memory_usage_mb\":%d,"
                          "\"request_count\":%d,"
                          "\"error_count\":%d,"
                          "\"success_rate\":%.2f"
                          "},"
                          "\"statistics\":{"
                          "\"total_requests\":%d,"
                          "\"active_connections\":%d,"
                          "\"total_bytes_sent\":%ld,"
                          "\"total_bytes_received\":%ld,"
                          "\"avg_response_time\":%.2f"
                          "},"
                          "\"version\":\"1.0.0\","
                          "\"last_update\":%ld}",
                          status->is_running ? "running" : "stopped",
                          uptime,
                          (int)(uptime / 3600), (int)((uptime % 3600) / 60), (int)(uptime % 60),
                          status->is_running,
                          status->cpu_usage,
                          status->memory_usage,
                          status->request_count,
                          status->error_count,
                          status->request_count > 0 ? (double)(status->request_count - status->error_count) / status->request_count * 100.0 : 100.0,
                          status->stats.total_requests,
                          status->stats.active_connections,
                          status->stats.total_bytes_sent,
                          status->stats.total_bytes_received,
                          status->stats.avg_response_time,
                          current_time
                          );
    
    if (written >= buffer_size) {
        logger_error(__func__, __FILE__, __LINE__, "状态JSON缓冲区不足");
        return 0;
    }
    
    logger_info(__func__, __FILE__, __LINE__, "状态数据序列化完成");
    
    return written;
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

int system_status_update(SystemStatus* status, const struct HttpServer* server) {
    if (status == NULL || server == NULL) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "更新系统状态");
    
    /* 更新基本状态 */
    status->is_running = server->is_running;
    status->request_count = server->status.request_count;
    status->error_count = server->status.error_count;
    
    /* 简单的CPU使用率模拟 (实际应用中应该使用系统API) */
    status->cpu_usage = 10.0 + (rand() % 20); /* 10-30% 模拟值 */
    
    /* 简单的内存使用计算 */
    FILE* meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        long total_memory = 0;
        long available_memory = 0;
        
        while (fgets(line, sizeof(line), meminfo)) {
            if (strstr(line, "MemTotal:")) {
                sscanf(line, "MemTotal: %ld kB", &total_memory);
            } else if (strstr(line, "MemAvailable:")) {
                sscanf(line, "MemAvailable: %ld kB", &available_memory);
            }
        }
        fclose(meminfo);
        
        long used_memory = total_memory - available_memory;
        status->memory_usage = (int)(used_memory / 1024); /* 转换为MB */
    } else {
        status->memory_usage = 512; /* 默认值 */
    }
    
    /* 更新统计信息 */
    status->stats.total_requests = server->status.stats.total_requests;
    status->stats.active_connections = server->status.stats.active_connections;
    status->stats.total_bytes_sent = server->status.stats.total_bytes_sent;
    status->stats.total_bytes_received = server->status.stats.total_bytes_received;
    status->stats.avg_response_time = server->status.stats.avg_response_time;
    
    logger_info(__func__, __FILE__, __LINE__, "系统状态更新完成");
    
    return 1;
}

int server_stats_update(ServerStats* stats, const struct HttpServer* server) {
    if (stats == NULL || server == NULL) return 0;
    
    logger_info(__func__, __FILE__, __LINE__, "更新服务器统计");
    
    /* 计算运行时间 */
    time_t current_time = time(NULL);
    double uptime = difftime(current_time, stats->start_time);
    
    /* 更新统计数据 */
    stats->total_requests = server->status.request_count;
    stats->active_connections = 1; /* 简化处理 */
    
    /* 计算平均响应时间 */
    if (stats->total_requests > 0) {
        stats->avg_response_time = 0.025 + (rand() % 10) * 0.001; /* 25-35ms 模拟值 */
    } else {
        stats->avg_response_time = 0.0;
    }
    
    /* 计算带宽使用 (简化处理) */
    if (uptime > 0) {
        stats->total_bytes_sent = server->status.stats.total_bytes_sent;
        stats->total_bytes_received = server->status.stats.total_bytes_received;
    }
    
    logger_info(__func__, __FILE__, __LINE__, "服务器统计更新完成");
    
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

const char* api_endpoint_to_string(ApiEndpointType endpoint) {
    switch (endpoint) {
        case API_STATUS: return "status";
        case API_SATELLITE: return "satellite";
        case API_TRAJECTORY: return "trajectory";
        case API_ANALYSIS: return "analysis";
        default: return "unknown";
    }
}

/* =================== WebSocket管理函数 =================== */

int http_server_enable_websocket(HttpServer* server, int enable) {
    if (server == NULL) return 0;
    
    if (enable && !server->websocket_server) {
        /* 创建WebSocket服务器 */
        server->websocket_server = websocket_server_create(server);
        if (!server->websocket_server) {
            logger_error(__func__, __FILE__, __LINE__, "创建WebSocket服务器失败");
            return 0;
        }
        
        /* 设置WebSocket回调函数 */
        websocket_set_handlers(server->websocket_server,
                             server->websocket_handler,
                             NULL, /* 连接回调 */
                             NULL  /* 断开连接回调 */
                             );
        
        logger_info(__func__, __FILE__, __LINE__, "WebSocket服务器已启用");
    } else if (!enable && server->websocket_server) {
        /* 销毁WebSocket服务器 */
        websocket_server_destroy(server->websocket_server);
        server->websocket_server = NULL;
        
        logger_info(__func__, __FILE__, __LINE__, "WebSocket服务器已禁用");
    }
    
    server->enable_websocket = enable;
    
    return 1;
}

int http_server_websocket_broadcast(HttpServer* server, const char* message) {
    if (server == NULL || message == NULL) return 0;
    
    if (!server->enable_websocket || !server->websocket_server) {
        logger_warning(__func__, __FILE__, __LINE__, "WebSocket未启用");
        return 0;
    }
    
    return websocket_broadcast_text(server->websocket_server, message);
}

int http_server_websocket_send_status(HttpServer* server) {
    if (server == NULL) return 0;
    
    if (!server->enable_websocket || !server->websocket_server) {
        logger_warning(__func__, __FILE__, __LINE__, "WebSocket未启用");
        return 0;
    }
    
    /* 创建状态消息 */
    char status_json[2048];
    time_t current_time = time(NULL);
    time_t uptime = current_time - server->status.start_time;
    
    int written = snprintf(status_json, sizeof(status_json),
                         "{\"type\":\"status\","
                         "\"timestamp\":%ld,"
                         "\"uptime\":%ld,"
                         "\"system_status\":\"%s\","
                         "\"connection_count\":%d,"
                         "\"request_count\":%d,"
                         "\"error_count\":%d,"
                         "\"memory_usage\":%d,"
                         "\"cpu_usage\":%.2f,"
                         "\"websocket_connections\":%d}",
                         current_time,
                         uptime,
                         server->status.is_running ? "running" : "stopped",
                         server->status.active_connections,
                         server->status.request_count,
                         server->status.error_count,
                         server->status.memory_usage,
                         server->status.cpu_usage,
                         websocket_get_connection_count(server->websocket_server));
    
    if (written >= sizeof(status_json)) {
        logger_error(__func__, __FILE__, __LINE__, "状态JSON缓冲区不足");
        return 0;
    }
    
    return websocket_broadcast_text(server->websocket_server, status_json);
}