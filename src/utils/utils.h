#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <ctype.h>
#include <float.h>

/* 定义M_PI如果不存在 */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* 数学工具函数 */
double degrees_to_radians(double degrees);
double radians_to_degrees(double radians);
double normalize_angle(double degrees);
double normalize_angle_radians(double radians);
double distance_haversine(double lat1, double lon1, double lat2, double lon2);
double bearing_calculate(double lat1, double lon1, double lat2, double lon2);
double altitude_calculate(double distance, double elevation);
double interpolate_linear(double x1, double y1, double x2, double y2, double x);
double interpolate_cubic(double x0, double y0, double x1, double y1, 
                       double x2, double y2, double x3, double y3, double x);

/* 时间工具函数 */
time_t time_parse_iso8601(const char* iso_string);
int time_format_iso8601(time_t time, char* buffer, int buffer_size);
double time_diff_seconds(time_t time1, time_t time2);
time_t time_add_seconds(time_t time, double seconds);
int time_is_valid(time_t time);
double time_to_julian_date(time_t time);
time_t julian_date_to_time(double jd);

/* 地理坐标转换 */
typedef struct {
    double x;    /* X坐标 (米) */
    double y;    /* Y坐标 (米) */
    double z;    /* Z坐标 (米) */
} EcefCoordinate;

typedef struct {
    double latitude;    /* 纬度 (度) */
    double longitude;   /* 经度 (度) */
    double altitude;    /* 高度 (米) */
} GeodeticCoordinate;

EcefCoordinate geodetic_to_ecef(const GeodeticCoordinate* geodetic);
GeodeticCoordinate ecef_to_geodetic(const EcefCoordinate* ecef);
EcefCoordinate geodetic_to_ecef_simple(double lat, double lon, double alt);
GeodeticCoordinate ecef_to_geodetic_simple(double x, double y, double z);

/* 字符串工具 */
int string_is_empty(const char* str);
int string_starts_with(const char* str, const char* prefix);
int string_ends_with(const char* str, const char* suffix);
char* string_trim(char* str);
char* string_to_lower(char* str);
char* string_to_upper(char* str);
int string_split(const char* str, char delimiter, char** tokens, int max_tokens);
char* string_join(const char** tokens, int count, char delimiter);

/* 文件工具 */
int file_exists(const char* filename);
long file_size(const char* filename);
int file_copy(const char* src, const char* dst);
int file_delete(const char* filename);
char* file_read_text(const char* filename);
int file_write_text(const char* filename, const char* content);
int file_append_text(const char* filename, const char* content);
char* file_get_extension(const char* filename);
char* file_get_basename(const char* filename);
char* file_get_dirname(const char* filename);

/* 内存管理 */
void* safe_malloc(size_t size);
void* safe_calloc(size_t count, size_t size);
void* safe_realloc(void* ptr, size_t size);
char* safe_strdup(const char* str);
void safe_free(void** ptr);

/* 错误处理 */
typedef enum {
    ERROR_NONE = 0,
    ERROR_MEMORY = 1,
    ERROR_FILE = 2,
    ERROR_PARSE = 3,
    ERROR_NETWORK = 4,
    ERROR_CALCULATION = 5,
    ERROR_PARAMETER = 6,
    ERROR_TIMEOUT = 7,
    ERROR_SYSTEM = 8
} ErrorCode;

typedef struct {
    ErrorCode code;
    char message[512];
    char function[128];
    char file[128];
    int line;
    time_t timestamp;
} ErrorInfo;

void error_set(ErrorCode code, const char* message, const char* function, 
               const char* file, int line);
ErrorInfo* error_get_last();
void error_clear();
const char* error_to_string(ErrorCode code);

/* 日志工具 */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} LogLevel;

typedef struct {
    LogLevel level;
    char timestamp[32];
    char message[1024];
    char function[128];
    char file[128];
    int line;
    int thread_id;
} LogEntry;

/* 基本日志功能 */
int logger_init(const char* filename, LogLevel level);
void logger_cleanup();
void logger_log(LogLevel level, const char* message, const char* function, 
                const char* file, int line);
void logger_set_level(LogLevel level);
LogLevel logger_get_level();

/* 扩展日志功能 */
void logger_log_format(LogLevel level, const char* function, const char* file, int line, 
                      const char* format, ...);
void logger_debug(const char* function, const char* file, int line, const char* message);
void logger_info(const char* function, const char* file, int line, const char* message);
void logger_warning(const char* function, const char* file, int line, const char* message);
void logger_error(const char* function, const char* file, int line, const char* message);
void logger_fatal(const char* function, const char* file, int line, const char* message);

/* 日志轮转 */
int logger_rotate();

/* 日志统计 */
typedef struct {
    int debug_count;
    int info_count;
    int warning_count;
    int error_count;
    int fatal_count;
    time_t start_time;
} LogStats;

void logger_stats_init();
void logger_stats_get(LogStats* stats);
void logger_stats_print();
void logger_log_with_stats(LogLevel level, const char* message, const char* function, 
                          const char* file, int line);

/* 日志缓冲区 */
void logger_buffer_init();
void logger_buffer_add(LogLevel level, const char* message);
void logger_buffer_print(int count);

/* 日志过滤 */
void logger_set_filter(const char* file_filter, const char* function_filter, LogLevel min_level);
int logger_should_log(LogLevel level, const char* function, const char* file);

/* 多目标日志输出 */
int logger_add_target(FILE* file, LogLevel level);
void logger_remove_target(FILE* file);
void logger_write_to_targets(LogLevel level, const char* message, const char* function, 
                            const char* file, int line);

/* 日志宏定义 */
#define LOG_DEBUG(msg) logger_log_with_stats(LOG_LEVEL_DEBUG, msg, __func__, __FILE__, __LINE__)
#define LOG_INFO(msg) logger_log_with_stats(LOG_LEVEL_INFO, msg, __func__, __FILE__, __LINE__)
#define LOG_WARNING(msg) logger_log_with_stats(LOG_LEVEL_WARNING, msg, __func__, __FILE__, __LINE__)
#define LOG_ERROR(msg) logger_log_with_stats(LOG_LEVEL_ERROR, msg, __func__, __FILE__, __LINE__)
#define LOG_FATAL(msg) logger_log_with_stats(LOG_LEVEL_FATAL, msg, __func__, __FILE__, __LINE__)

#define LOG_DEBUG_FMT(fmt, ...) logger_log_format(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define LOG_INFO_FMT(fmt, ...) logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define LOG_WARNING_FMT(fmt, ...) logger_log_format(LOG_LEVEL_WARNING, __func__, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define LOG_ERROR_FMT(fmt, ...) logger_log_format(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define LOG_FATAL_FMT(fmt, ...) logger_log_format(LOG_LEVEL_FATAL, __func__, __FILE__, __LINE__, fmt, __VA_ARGS__)

/* 配置管理 */
typedef struct {
    char config_file[512];
    char log_file[512];
    char data_dir[512];
    int server_port;
    int max_connections;
    int log_level;
    int enable_debug;
    char version[64];
} AppConfig;

int config_load(const char* filename, AppConfig* config);
int config_save(const char* filename, const AppConfig* config);
int config_validate(const AppConfig* config);
void config_set_defaults(AppConfig* config);

/* 数据验证 */
int validate_latitude(double lat);
int validate_longitude(double lon);
int validate_altitude(double alt);
int validate_attitude(double pitch, double roll, double yaw);
int validate_velocity(double velocity);
int validate_timestamp(time_t time);
int validate_prn(int prn);
int validate_signal_strength(double signal_strength);

/* 性能监控 */
typedef struct {
    double start_time;
    double end_time;
    double duration;
    char name[128];
} PerformanceTimer;

void performance_timer_start(PerformanceTimer* timer, const char* name);
void performance_timer_stop(PerformanceTimer* timer);
double performance_timer_elapsed(const PerformanceTimer* timer);

typedef struct {
    int call_count;
    double total_time;
    double min_time;
    double max_time;
    double avg_time;
    char name[128];
} PerformanceCounter;

void performance_counter_init(PerformanceCounter* counter, const char* name);
void performance_counter_add(PerformanceCounter* counter, double duration);
void performance_counter_reset(PerformanceCounter* counter);

/* 线程安全 (简单版本) */
typedef struct {
    int is_initialized;
    int lock_count;
} SimpleMutex;

int simple_mutex_init(SimpleMutex* mutex);
int simple_mutex_lock(SimpleMutex* mutex);
int simple_mutex_unlock(SimpleMutex* mutex);
void simple_mutex_destroy(SimpleMutex* mutex);

#endif /* UTILS_H */