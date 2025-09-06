#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../src/web/http_server.h"
#include "../src/web/api.h"
#include "../src/utils/utils.h"

/* 全局服务器实例 */
static HttpServer* g_server = NULL;

/* 信号处理函数 */
static void signal_handler(int signum) {
    printf("\n接收到信号 %d，正在关闭服务器...\n", signum);
    if (g_server) {
        http_server_stop(g_server);
        http_server_destroy(g_server);
        g_server = NULL;
    }
    exit(0);
}

/* 主函数 */
int main(int argc, char* argv[]) {
    printf("北斗导航卫星可见性分析系统 - Web服务器测试程序\n");
    printf("================================================\n");
    
    /* 初始化日志系统 */
    logger_init("web_server.log", LOG_LEVEL_INFO);
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 创建服务器配置 */
    HttpServerConfig config;
    http_server_config_init(&config);
    
    /* 解析命令行参数 */
    if (argc > 1) {
        config.port = atoi(argv[1]);
        if (config.port <= 0 || config.port > 65535) {
            printf("错误：端口号必须在1-65535之间\n");
            return 1;
        }
    }
    
    if (argc > 2) {
        strncpy(config.host, argv[2], sizeof(config.host) - 1);
        config.host[sizeof(config.host) - 1] = '\0';
    }
    
    /* 验证配置 */
    if (!http_server_config_validate(&config)) {
        printf("错误：服务器配置无效\n");
        return 1;
    }
    
    printf("服务器配置：\n");
    printf("  监听地址：%s:%d\n", config.host, config.port);
    printf("  最大连接数：%d\n", config.max_connections);
    printf("  超时时间：%d秒\n", config.timeout);
    printf("  静态文件目录：%s\n", config.static_dir);
    printf("\n");
    
    /* 创建HTTP服务器 */
    g_server = http_server_create(&config);
    if (g_server == NULL) {
        printf("错误：创建HTTP服务器失败\n");
        return 1;
    }
    
    /* 设置回调函数 */
    http_server_set_handlers(g_server, 
                           api_process_request, 
                           NULL,  /* WebSocket处理器 */
                           NULL); /* 文件上传处理器 */
    
    /* 启动服务器 */
    if (!http_server_start(g_server)) {
        printf("错误：启动HTTP服务器失败\n");
        http_server_destroy(g_server);
        return 1;
    }
    
    printf("HTTP服务器启动成功！\n");
    printf("访问地址：http://%s:%d\n", config.host, config.port);
    printf("API端点：\n");
    printf("  GET  /               - 服务器首页\n");
    printf("  GET  /api/status     - 系统状态\n");
    printf("  GET  /api/satellite  - 卫星数据\n");
    printf("  GET  /api/trajectory - 轨迹数据\n");
    printf("  GET  /api/analysis   - 分析结果\n");
    printf("  POST /api/trajectory - 生成轨迹\n");
    printf("  POST /api/analysis   - 执行分析\n");
    printf("  POST /api/upload     - 文件上传\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    
    /* 主循环 */
    while (g_server && g_server->is_running) {
        sleep(1);
        
        /* 定期更新服务器状态 */
        static int counter = 0;
        if (++counter % 10 == 0) {
            system_status_update(&g_server->status, g_server);
            server_stats_update(&g_server->status.stats, g_server);
        }
    }
    
    /* 清理资源 */
    if (g_server) {
        http_server_stop(g_server);
        http_server_destroy(g_server);
        g_server = NULL;
    }
    
    logger_cleanup();
    
    printf("服务器已关闭\n");
    return 0;
}