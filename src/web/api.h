#ifndef API_H
#define API_H

#include "http_server.h"

/* API处理函数声明 */
int api_process_request(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_handle_get(HttpRequest* request, HttpResponse* response, struct HttpServer* server);
int api_handle_post(HttpRequest* request, HttpResponse* response, struct HttpServer* server);

#endif /* API_H */