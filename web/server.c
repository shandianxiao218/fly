#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "../src/web/http_server.h"
#include "../src/web/api.h"
#include "../src/web/websocket.h"
#include "../src/satellite/satellite.h"
#include "../src/aircraft/aircraft.h"
#include "../src/obstruction/obstruction.h"
#include "../src/utils/utils.h"

/* 全局变量 */
static struct HttpServer* g_server = NULL;
static int g_running = 0;

/* 信号处理函数 */
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        printf("\n接收到终止信号，正在关闭服务器...\n");
        g_running = 0;
        if (g_server) {
            http_server_stop(g_server);
        }
    }
}

/* 打印帮助信息 */
static void print_help(const char* program_name) {
    printf("北斗导航卫星可见性分析系统 - Web服务器\n\n");
    printf("用法: %s [选项]\n", program_name);
    printf("选项:\n");
    printf("  -p <端口>      指定HTTP服务器端口 (默认: 8080)\n");
    printf("  -w <端口>      指定WebSocket端口 (默认: 8081)\n");
    printf("  -d <目录>      指定Web根目录 (默认: ./web)\n");
    printf("  -s <星历文件>  指定RINEX星历文件路径\n");
    printf("  -t <轨迹文件>  指定轨迹文件路径\n");
    printf("  -v             启用详细日志\n");
    printf("  -h             显示帮助信息\n\n");
    printf("示例:\n");
    printf("  %s                    # 使用默认配置启动\n", program_name);
    printf("  %s -p 8080 -d ./web  # 指定端口和Web根目录\n", program_name);
    printf("  %s -s data.rnx -t flight.csv  # 指定数据文件\n", program_name);
}

/* 静态文件处理函数 */
static int serve_static_file(const HttpRequest* request, HttpResponse* response, const struct HttpServer* server) {
    char file_path[1024];
    const char* web_root = "./web"; // 简化处理
    
    // 构建文件路径
    if (strcmp(request->path, "/") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/index.html", web_root);
    } else {
        // 安全检查：防止路径遍历攻击
        if (strstr(request->path, "..")) {
            http_response_set_error(response, 403, "Forbidden");
            return -1;
        }
        snprintf(file_path, sizeof(file_path), "%s%s", web_root, request->path);
    }
    
    // 检查文件是否存在
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        http_response_set_error(response, 404, "File not found");
        return -1;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 读取文件内容
    char* file_content = malloc(file_size + 1);
    if (!file_content) {
        fclose(file);
        http_response_set_error(response, 500, "Internal server error");
        return -1;
    }
    
    size_t bytes_read = fread(file_content, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        free(file_content);
        http_response_set_error(response, 500, "Internal server error");
        return -1;
    }
    
    file_content[bytes_read] = '\0';
    
    // 根据文件扩展名设置Content-Type
    const char* content_type = "text/plain";
    const char* extension = strrchr(file_path, '.');
    if (extension) {
        if (strcmp(extension, ".html") == 0) {
            content_type = "text/html; charset=utf-8";
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
        }
    }
    
    // 设置响应
    response->status_code = 200;
    response->status_message = strdup("OK");
    
    // 创建响应头
    char headers[1024];
    snprintf(headers, sizeof(headers), 
        "Content-Type: %s\r\nContent-Length: %ld\r\nCache-Control: no-cache\r\n", 
        content_type, file_size);
    response->headers = strdup(headers);
    
    response->body = file_content;
    response->content_length = file_size;
    
    return 0;
}

/* 主请求处理函数 */
static int request_handler(const HttpRequest* request, HttpResponse* response, const struct HttpServer* server) {
    printf("收到请求: %s %s\n", 
           request->method == HTTP_GET ? "GET" : 
           request->method == HTTP_POST ? "POST" : "OTHER", 
           request->path);
    
    // 检查是否为API请求
    if (strncmp(request->path, "/api/", 5) == 0) {
        return api_process_request((HttpRequest*)request, response, (struct HttpServer*)server);
    }
    
    // 静态文件服务
    return serve_static_file(request, response, server);
}

/* WebSocket消息处理函数 */
static int websocket_handler(const char* message, const struct HttpServer* server) {
    printf("收到WebSocket消息: %s\n", message);
    
    // 简单回显
    if (server && server->websocket_server) {
        websocket_broadcast(server->websocket_server, message, strlen(message));
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    int websocket_port = 8081;
    char web_root[512] = "./web";
    char ephemeris_file[512] = "";
    char trajectory_file[512] = "";
    int verbose = 0;
    int opt;
    
    // 解析命令行参数
    while ((opt = getopt(argc, argv, "p:w:d:s:t:vh")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'w':
                websocket_port = atoi(optarg);
                break;
            case 'd':
                strncpy(web_root, optarg, sizeof(web_root) - 1);
                break;
            case 's':
                strncpy(ephemeris_file, optarg, sizeof(ephemeris_file) - 1);
                break;
            case 't':
                strncpy(trajectory_file, optarg, sizeof(trajectory_file) - 1);
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                print_help(argv[0]);
                return 0;
            default:
                fprintf(stderr, "未知选项: -%c\n", opt);
                print_help(argv[0]);
                return 1;
        }
    }
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化日志系统
    logger_init(verbose ? "web_server.log" : NULL, verbose ? LOG_LEVEL_DEBUG : LOG_LEVEL_INFO);
    
    printf("北斗导航卫星可见性分析系统 - Web服务器\n");
    printf("=========================================\n");
    printf("HTTP端口: %d\n", port);
    printf("WebSocket端口: %d\n", websocket_port);
    printf("Web根目录: %s\n", web_root);
    
    // 检查Web根目录是否存在
    FILE* test_file = fopen(web_root, "r");
    if (!test_file) {
        printf("错误: Web根目录 '%s' 不存在\n", web_root);
        return 1;
    }
    fclose(test_file);
    
    // 创建HTTP服务器配置
    HttpServerConfig config = {
        .port = port,
        .max_connections = 100,
        .timeout = 30
    };
    
    // 创建HTTP服务器
    g_server = http_server_create(&config);
    if (!g_server) {
        printf("错误: 无法创建HTTP服务器\n");
        return 1;
    }
    
    // 设置请求处理器
    http_server_set_handlers(g_server, request_handler, websocket_handler, NULL);
    
    // 加载数据
    if (strlen(ephemeris_file) > 0) {
        printf("正在加载星历文件: %s\n", ephemeris_file);
        // 这里应该调用satellite模块的函数加载数据
    }
    
    if (strlen(trajectory_file) > 0) {
        printf("正在加载轨迹文件: %s\n", trajectory_file);
        // 这里应该调用aircraft模块的函数加载数据
    }
    
    // 启动服务器
    printf("正在启动HTTP服务器...\n");
    if (http_server_start(g_server) != 0) {
        printf("错误: 无法启动HTTP服务器\n");
        http_server_destroy(g_server);
        return 1;
    }
    
    printf("服务器启动成功!\n");
    printf("HTTP服务地址: http://localhost:%d\n", port);
    printf("WebSocket服务地址: ws://localhost:%d\n", websocket_port);
    printf("按Ctrl+C停止服务器\n");
    
    g_running = 1;
    
    // 主循环
    while (g_running) {
        sleep(1);
        
        // 更新服务器状态
        if (g_server->is_running) {
            // 可以在这里添加定期状态更新逻辑
        }
    }
    
    // 清理资源
    printf("正在停止服务器...\n");
    http_server_stop(g_server);
    http_server_destroy(g_server);
    
    logger_cleanup();
    
    printf("服务器已停止\n");
    return 0;
}