#ifndef SIMPLE_API_H
#define SIMPLE_API_H

#include "simple_http_server.h"

/* API处理函数声明 */
int api_process_request(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_handle_get(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_handle_post(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_handle_put(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_handle_delete(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_handle_head(HttpRequest* request, HttpResponse* response, HttpServer* server);

/* 具体API实现 */
int api_get_status(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_get_satellite(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_get_trajectory(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_get_analysis(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_post_trajectory(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_post_analysis(HttpRequest* request, HttpResponse* response, HttpServer* server);
int api_post_upload(HttpRequest* request, HttpResponse* response, HttpServer* server);

#endif /* SIMPLE_API_H */