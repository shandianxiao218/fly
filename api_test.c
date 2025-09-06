#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "src/web/simple_http_server.h"
#include "src/web/simple_api.h"
#include "src/utils/utils.h"

/* 测试API函数 */
void test_api_functions() {
    printf("测试API函数...\n");
    
    /* 创建测试服务器 */
    HttpServerConfig config;
    http_server_config_init(&config);
    HttpServer* server = http_server_create(&config);
    if (server == NULL) {
        printf("创建服务器失败\n");
        return;
    }
    
    /* 启动服务器 */
    http_server_start(server);
    
    /* 测试请求和响应 */
    HttpRequest* request = http_request_create();
    HttpResponse* response = http_response_create();
    
    if (request == NULL || response == NULL) {
        printf("创建请求/响应失败\n");
        return;
    }
    
    /* 测试1: 获取系统状态 */
    printf("\n=== 测试1: 获取系统状态 ===\n");
    request->method = HTTP_GET;
    request->path = strdup("/api/status");
    
    if (api_process_request(request, response, server)) {
        printf("状态API调用成功\n");
        printf("响应状态码: %d\n", response->status_code);
        if (response->body) {
            printf("响应体: %s\n", response->body);
        }
    } else {
        printf("状态API调用失败\n");
    }
    
    /* 重置响应 */
    http_response_destroy(response);
    response = http_response_create();
    
    /* 测试2: 获取卫星数据 */
    printf("\n=== 测试2: 获取卫星数据 ===\n");
    request->method = HTTP_GET;
    free(request->path);
    request->path = strdup("/api/satellite");
    
    if (api_process_request(request, response, server)) {
        printf("卫星API调用成功\n");
        printf("响应状态码: %d\n", response->status_code);
        if (response->body) {
            printf("响应体: %s\n", response->body);
        }
    } else {
        printf("卫星API调用失败\n");
    }
    
    /* 重置响应 */
    http_response_destroy(response);
    response = http_response_create();
    
    /* 测试3: 获取轨迹数据 */
    printf("\n=== 测试3: 获取轨迹数据 ===\n");
    request->method = HTTP_GET;
    free(request->path);
    request->path = strdup("/api/trajectory");
    
    if (api_process_request(request, response, server)) {
        printf("轨迹API调用成功\n");
        printf("响应状态码: %d\n", response->status_code);
        if (response->body) {
            printf("响应体: %s\n", response->body);
        }
    } else {
        printf("轨迹API调用失败\n");
    }
    
    /* 重置响应 */
    http_response_destroy(response);
    response = http_response_create();
    
    /* 测试4: 获取分析结果 */
    printf("\n=== 测试4: 获取分析结果 ===\n");
    request->method = HTTP_GET;
    free(request->path);
    request->path = strdup("/api/analysis");
    
    if (api_process_request(request, response, server)) {
        printf("分析API调用成功\n");
        printf("响应状态码: %d\n", response->status_code);
        if (response->body) {
            printf("响应体: %s\n", response->body);
        }
    } else {
        printf("分析API调用失败\n");
    }
    
    /* 重置响应 */
    http_response_destroy(response);
    response = http_response_create();
    
    /* 测试5: POST请求 - 生成轨迹 */
    printf("\n=== 测试5: POST生成轨迹 ===\n");
    request->method = HTTP_POST;
    free(request->path);
    request->path = strdup("/api/trajectory");
    request->body = strdup("{\"action\":\"generate\",\"duration\":3600}");
    request->content_length = strlen(request->body);
    
    if (api_process_request(request, response, server)) {
        printf("轨迹生成API调用成功\n");
        printf("响应状态码: %d\n", response->status_code);
        if (response->body) {
            printf("响应体: %s\n", response->body);
        }
    } else {
        printf("轨迹生成API调用失败\n");
    }
    
    /* 重置响应 */
    http_response_destroy(response);
    response = http_response_create();
    
    /* 测试6: POST请求 - 执行分析 */
    printf("\n=== 测试6: POST执行分析 ===\n");
    request->method = HTTP_POST;
    free(request->path);
    request->path = strdup("/api/analysis");
    free(request->body);
    request->body = strdup("{\"action\":\"analyze\",\"satellite_prn\":1}");
    request->content_length = strlen(request->body);
    
    if (api_process_request(request, response, server)) {
        printf("分析API调用成功\n");
        printf("响应状态码: %d\n", response->status_code);
        if (response->body) {
            printf("响应体: %s\n", response->body);
        }
    } else {
        printf("分析API调用失败\n");
    }
    
    /* 重置响应 */
    http_response_destroy(response);
    response = http_response_create();
    
    /* 测试7: 不存在的端点 */
    printf("\n=== 测试7: 不存在的端点 ===\n");
    request->method = HTTP_GET;
    free(request->path);
    request->path = strdup("/api/unknown");
    
    if (api_process_request(request, response, server)) {
        printf("未知API调用成功\n");
        printf("响应状态码: %d\n", response->status_code);
        if (response->body) {
            printf("响应体: %s\n", response->body);
        }
    } else {
        printf("未知API调用失败\n");
    }
    
    /* 清理资源 */
    http_request_destroy(request);
    http_response_destroy(response);
    http_server_stop(server);
    http_server_destroy(server);
    
    printf("\nAPI函数测试完成！\n");
}

/* 主函数 */
int main() {
    printf("北斗导航卫星可见性分析系统 - API功能测试\n");
    printf("==========================================\n");
    
    /* 初始化日志系统 */
    logger_init("api_test.log", LOG_LEVEL_INFO);
    
    /* 测试API函数 */
    test_api_functions();
    
    printf("\n测试完成！\n");
    return 0;
}