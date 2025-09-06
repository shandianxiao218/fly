#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "src/utils/utils.h"

#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(ms) Sleep(ms)
#else
#include <unistd.h>
#define SLEEP_MS(ms) sleep(ms / 1000)
#endif

#include "src/web/simple_http_server.h"
#include "src/web/simple_api.h"

/* 全局服务器实例 */
static HttpServer* g_server = NULL;

/* 信号处理函数 */
void signal_handler(int signum) {
    printf("\n接收到信号 %d，正在关闭服务器...\n", signum);
    if (g_server) {
        http_server_stop(g_server);
    }
    exit(0);
}

/* 测试数据生成函数 */
void generate_test_data() {
    printf("生成测试数据...\n");
    
    /* 这里可以添加测试数据生成逻辑 */
    /* 例如：创建卫星数据、轨迹数据等 */
    
    printf("测试数据生成完成\n");
}

/* 主函数 */
int main() {
    printf("北斗导航卫星可见性分析系统 - Web服务器测试\n");
    printf("===========================================\n");
    
    /* 初始化日志系统 */
    logger_init("web_server_test.log", LOG_LEVEL_INFO);
    
    /* 创建服务器配置 */
    HttpServerConfig config;
    http_server_config_init(&config);
    config.port = 8080;
    config.host = strdup("localhost");
    config.max_connections = 10;
    config.timeout = 30;
    
    /* 创建服务器 */
    g_server = http_server_create(&config);
    if (g_server == NULL) {
        printf("创建服务器失败\n");
        return 1;
    }
    
    /* 生成测试数据 */
    generate_test_data();
    
    /* 简化版本不需要设置回调函数 */
    
    /* 启动服务器 */
    printf("启动服务器在端口 %d...\n", config.port);
    if (!http_server_start(g_server)) {
        printf("启动服务器失败\n");
        http_server_destroy(g_server);
        return 1;
    }
    
    printf("服务器运行中...\n");
    printf("访问地址: http://localhost:%d\n", config.port);
    printf("API端点:\n");
    printf("  GET  /api/status     - 系统状态\n");
    printf("  GET  /api/satellite  - 卫星数据\n");
    printf("  GET  /api/trajectory - 轨迹数据\n");
    printf("  GET  /api/analysis   - 分析结果\n");
    printf("  POST /api/trajectory - 生成轨迹\n");
    printf("  POST /api/analysis   - 执行分析\n");
    printf("  POST /api/upload     - 文件上传\n");
    printf("\n按 Ctrl+C 停止服务器\n");
    
    /* 主循环 */
    while (g_server->is_running) {
        /* 更新服务器状态 */
        system_status_update(&g_server->status, g_server);
        server_stats_update(&g_server->status.stats, g_server);
        
        /* 简单的状态显示 */
        static time_t last_status_time = 0;
        time_t current_time = time(NULL);
        if (current_time - last_status_time >= 10) {
            time_t uptime = current_time - g_server->status.start_time;
            printf("服务器运行中 - 运行时间: %lld秒, 请求: %d, 错误: %d\n", 
                   (long long)uptime, g_server->status.request_count, g_server->status.error_count);
            last_status_time = current_time;
        }
        
        /* 休眠1秒 */
        SLEEP_MS(1000);
    }
    
    /* 清理资源 */
    printf("关闭服务器...\n");
    http_server_stop(g_server);
    http_server_destroy(g_server);
    
    printf("服务器已停止\n");
    return 0;
}