#ifndef API_H
#define API_H

#include "http_server.h"

/* 主要API处理函数声明 */
int api_process_request(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_handle_get(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_handle_post(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_handle_put(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_handle_delete(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_handle_head(HttpRequest* request, HttpResponse* response, struct HttpServer* server);

/* 具体的API端点处理函数声明 */
int api_get_status(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_get_satellite(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_get_trajectory(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_get_analysis(HttpRequest* request, HttpResponse* response, struct HttpServer* server);

int api_post_trajectory(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_post_analysis(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_post_upload(HttpRequest* request, HttpResponse* response, struct HttpServer* server);

#endif /* API_H */