#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

/* =================== 日志工具实现 =================== */

static FILE* log_file = NULL;
static LogLevel current_log_level = LOG_LEVEL_INFO;
static int log_initialized = 0;

int logger_init(const char* filename, LogLevel level) {
    if (filename == NULL) return 0;
    
    /* 如果已经初始化，先清理 */
    if (log_initialized) {
        logger_cleanup();
    }
    
    log_file = fopen(filename, "a");
    if (log_file == NULL) return 0;
    
    current_log_level = level;
    log_initialized = 1;
    
    /* 记录初始化信息 */
    LOG_INFO("日志系统初始化完成");
    
    return 1;
}

void logger_cleanup() {
    if (!log_initialized) return;
    
    if (log_file != NULL) {
        LOG_INFO("日志系统关闭");
        fclose(log_file);
        log_file = NULL;
    }
    
    log_initialized = 0;
}

void logger_log(LogLevel level, const char* message, const char* function, 
                const char* file, int line) {
    if (!log_initialized || level < current_log_level || log_file == NULL) return;
    
    const char* level_str;
    switch (level) {
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO: level_str = "INFO"; break;
        case LOG_LEVEL_WARNING: level_str = "WARNING"; break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        case LOG_LEVEL_FATAL: level_str = "FATAL"; break;
        default: level_str = "UNKNOWN"; break;
    }
    
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_file, "[%s] [%s] [%s:%d] %s: %s\n", 
            time_str, level_str, file, line, function, message);
    fflush(log_file);
    
    /* 对于ERROR和FATAL级别的日志，同时输出到stderr */
    if (level >= LOG_LEVEL_ERROR) {
        fprintf(stderr, "[%s] [%s] [%s:%d] %s: %s\n", 
                time_str, level_str, file, line, function, message);
    }
}

void logger_set_level(LogLevel level) {
    if (!log_initialized) return;
    current_log_level = level;
}

LogLevel logger_get_level() {
    return current_log_level;
}

/* =================== 扩展日志功能 =================== */

void logger_log_format(LogLevel level, const char* function, const char* file, int line, 
                      const char* format, ...) {
    if (!log_initialized || level < current_log_level || log_file == NULL) return;
    
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    logger_log(level, message, function, file, line);
}

/* 定义宏的底层实现 */
void logger_debug(const char* function, const char* file, int line, const char* message) {
    logger_log(LOG_LEVEL_DEBUG, message, function, file, line);
}

void logger_info(const char* function, const char* file, int line, const char* message) {
    logger_log(LOG_LEVEL_INFO, message, function, file, line);
}

void logger_warning(const char* function, const char* file, int line, const char* message) {
    logger_log(LOG_LEVEL_WARNING, message, function, file, line);
}

void logger_error(const char* function, const char* file, int line, const char* message) {
    logger_log(LOG_LEVEL_ERROR, message, function, file, line);
}

void logger_fatal(const char* function, const char* file, int line, const char* message) {
    logger_log(LOG_LEVEL_FATAL, message, function, file, line);
}

/* =================== 日志轮转 =================== */

#define MAX_LOG_SIZE (10 * 1024 * 1024) /* 10MB */
#define MAX_LOG_FILES 5

int logger_rotate() {
    if (!log_initialized || log_file == NULL) return 0;
    
    /* 检查当前日志文件大小 */
    long current_size = ftell(log_file);
    if (current_size < MAX_LOG_SIZE) return 1;
    
    /* 关闭当前文件 */
    fclose(log_file);
    
    /* 使用默认日志文件名 */
    const char* log_filename = "beidou_server.log";
    
    /* 轮转日志文件 */
    for (int i = MAX_LOG_FILES - 1; i > 0; i--) {
        char old_name[512];
        char new_name[512];
        
        if (i == 1) {
            snprintf(old_name, sizeof(old_name), "%s", log_filename);
        } else {
            snprintf(old_name, sizeof(old_name), "%s.%d", log_filename, i - 1);
        }
        
        snprintf(new_name, sizeof(new_name), "%s.%d", log_filename, i);
        
        /* 如果旧文件存在，重命名它 */
        if (file_exists(old_name)) {
            remove(new_name);
            rename(old_name, new_name);
        }
    }
    
    /* 重新打开日志文件 */
    log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        log_initialized = 0;
        return 0;
    }
    
    LOG_INFO("日志文件轮转完成");
    return 1;
}

/* =================== 日志统计 =================== */

static LogStats log_stats = {0};

void logger_stats_init() {
    memset(&log_stats, 0, sizeof(log_stats));
    log_stats.start_time = time(NULL);
}

void logger_stats_get(LogStats* stats) {
    if (stats != NULL) {
        memcpy(stats, &log_stats, sizeof(LogStats));
    }
}

void logger_stats_print() {
    time_t now = time(NULL);
    double uptime = difftime(now, log_stats.start_time);
    
    printf("=== 日志统计 ===\n");
    printf("运行时间: %.2f 秒\n", uptime);
    printf("DEBUG: %d\n", log_stats.debug_count);
    printf("INFO: %d\n", log_stats.info_count);
    printf("WARNING: %d\n", log_stats.warning_count);
    printf("ERROR: %d\n", log_stats.error_count);
    printf("FATAL: %d\n", log_stats.fatal_count);
    printf("总计: %d\n", log_stats.debug_count + log_stats.info_count + 
                          log_stats.warning_count + log_stats.error_count + log_stats.fatal_count);
    printf("==============\n");
}

/* 重写logger_log函数以支持统计 */
void logger_log_with_stats(LogLevel level, const char* message, const char* function, 
                          const char* file, int line) {
    /* 更新统计信息 */
    switch (level) {
        case LOG_LEVEL_DEBUG: log_stats.debug_count++; break;
        case LOG_LEVEL_INFO: log_stats.info_count++; break;
        case LOG_LEVEL_WARNING: log_stats.warning_count++; break;
        case LOG_LEVEL_ERROR: log_stats.error_count++; break;
        case LOG_LEVEL_FATAL: log_stats.fatal_count++; break;
        default: break;
    }
    
    /* 调用原始日志函数 */
    logger_log(level, message, function, file, line);
}

/* =================== 日志缓冲区 =================== */

#define LOG_BUFFER_SIZE 1000

typedef struct {
    char messages[LOG_BUFFER_SIZE][1024];
    LogLevel levels[LOG_BUFFER_SIZE];
    int head;
    int tail;
    int count;
} LogBuffer;

static LogBuffer log_buffer = {0};

void logger_buffer_init() {
    memset(&log_buffer, 0, sizeof(LogBuffer));
    log_buffer.head = 0;
    log_buffer.tail = 0;
    log_buffer.count = 0;
}

void logger_buffer_add(LogLevel level, const char* message) {
    if (log_buffer.count >= LOG_BUFFER_SIZE) {
        /* 缓冲区满，移除最旧的日志 */
        log_buffer.tail = (log_buffer.tail + 1) % LOG_BUFFER_SIZE;
        log_buffer.count--;
    }
    
    strncpy(log_buffer.messages[log_buffer.head], message, sizeof(log_buffer.messages[0]) - 1);
    log_buffer.messages[log_buffer.head][sizeof(log_buffer.messages[0]) - 1] = '\0';
    log_buffer.levels[log_buffer.head] = level;
    
    log_buffer.head = (log_buffer.head + 1) % LOG_BUFFER_SIZE;
    log_buffer.count++;
}

void logger_buffer_print(int count) {
    int start = log_buffer.tail;
    
    if (log_buffer.count == 0) {
        printf("日志缓冲区为空\n");
        return;
    }
    
    printf("=== 最近 %d 条日志 ===\n", count > log_buffer.count ? log_buffer.count : count);
    
    for (int i = 0; i < count && i < log_buffer.count; i++) {
        int index = (start + i) % LOG_BUFFER_SIZE;
        const char* level_str;
        
        switch (log_buffer.levels[index]) {
            case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
            case LOG_LEVEL_INFO: level_str = "INFO"; break;
            case LOG_LEVEL_WARNING: level_str = "WARNING"; break;
            case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
            case LOG_LEVEL_FATAL: level_str = "FATAL"; break;
            default: level_str = "UNKNOWN"; break;
        }
        
        printf("[%s] %s\n", level_str, log_buffer.messages[index]);
    }
    
    printf("==================\n");
}

/* =================== 日志过滤 =================== */

typedef struct {
    const char* file_filter;
    const char* function_filter;
    LogLevel min_level;
} LogFilter;

static LogFilter log_filter = {NULL, NULL, LOG_LEVEL_DEBUG};

void logger_set_filter(const char* file_filter, const char* function_filter, LogLevel min_level) {
    log_filter.file_filter = file_filter;
    log_filter.function_filter = function_filter;
    log_filter.min_level = min_level;
}

int logger_should_log(LogLevel level, const char* function, const char* file) {
    /* 检查日志级别 */
    if (level < log_filter.min_level) return 0;
    
    /* 检查文件过滤 */
    if (log_filter.file_filter != NULL && file != NULL) {
        if (strstr(file, log_filter.file_filter) == NULL) return 0;
    }
    
    /* 检查函数过滤 */
    if (log_filter.function_filter != NULL && function != NULL) {
        if (strstr(function, log_filter.function_filter) == NULL) return 0;
    }
    
    return 1;
}

/* =================== 日志输出到多个目标 =================== */

typedef struct {
    FILE* file;
    LogLevel level;
    int enabled;
} LogTarget;

#define MAX_LOG_TARGETS 5

static LogTarget log_targets[MAX_LOG_TARGETS] = {0};
static int log_target_count = 0;

int logger_add_target(FILE* file, LogLevel level) {
    if (log_target_count >= MAX_LOG_TARGETS) return 0;
    
    log_targets[log_target_count].file = file;
    log_targets[log_target_count].level = level;
    log_targets[log_target_count].enabled = 1;
    log_target_count++;
    
    return 1;
}

void logger_remove_target(FILE* file) {
    for (int i = 0; i < log_target_count; i++) {
        if (log_targets[i].file == file) {
            log_targets[i].enabled = 0;
            break;
        }
    }
}

void logger_write_to_targets(LogLevel level, const char* message, const char* function, 
                            const char* file, int line) {
    for (int i = 0; i < log_target_count; i++) {
        if (log_targets[i].enabled && log_targets[i].file != NULL && 
            level >= log_targets[i].level) {
            
            const char* level_str;
            switch (level) {
                case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
                case LOG_LEVEL_INFO: level_str = "INFO"; break;
                case LOG_LEVEL_WARNING: level_str = "WARNING"; break;
                case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
                case LOG_LEVEL_FATAL: level_str = "FATAL"; break;
                default: level_str = "UNKNOWN"; break;
            }
            
            time_t now = time(NULL);
            struct tm* tm_info = localtime(&now);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
            
            fprintf(log_targets[i].file, "[%s] [%s] [%s:%d] %s: %s\n", 
                    time_str, level_str, file, line, function, message);
            fflush(log_targets[i].file);
        }
    }
}