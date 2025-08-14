#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

/* 地球常数 */
#define EARTH_RADIUS 6378137.0          /* 地球半长轴 (米) */
#define EARTH_FLATTENING 1.0/298.257223563  /* 地球扁率 */
#define EARTH_SEMI_MINOR 6356752.3142   /* 地球半短轴 (米) */
#define EARTH_ECCENTRICITY 0.0818191908426 /* 地球偏心率 */

/* =================== 数学工具函数 =================== */

double degrees_to_radians(double degrees) {
    return degrees * M_PI / 180.0;
}

double radians_to_degrees(double radians) {
    return radians * 180.0 / M_PI;
}

double normalize_angle(double degrees) {
    while (degrees > 180.0) degrees -= 360.0;
    while (degrees < -180.0) degrees += 360.0;
    return degrees;
}

double normalize_angle_radians(double radians) {
    while (radians > M_PI) radians -= 2.0 * M_PI;
    while (radians < -M_PI) radians += 2.0 * M_PI;
    return radians;
}

double distance_haversine(double lat1, double lon1, double lat2, double lon2) {
    double lat1_rad = degrees_to_radians(lat1);
    double lat2_rad = degrees_to_radians(lat2);
    double delta_lat = degrees_to_radians(lat2 - lat1);
    double delta_lon = degrees_to_radians(lon2 - lon1);
    
    double a = sin(delta_lat / 2) * sin(delta_lat / 2) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(delta_lon / 2) * sin(delta_lon / 2);
    
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    
    return EARTH_RADIUS * c;
}

double bearing_calculate(double lat1, double lon1, double lat2, double lon2) {
    double lat1_rad = degrees_to_radians(lat1);
    double lat2_rad = degrees_to_radians(lat2);
    double delta_lon = degrees_to_radians(lon2 - lon1);
    
    double y = sin(delta_lon) * cos(lat2_rad);
    double x = cos(lat1_rad) * sin(lat2_rad) -
               sin(lat1_rad) * cos(lat2_rad) * cos(delta_lon);
    
    double bearing = atan2(y, x);
    return normalize_angle(radians_to_degrees(bearing));
}

double altitude_calculate(double distance, double elevation) {
    return distance * tan(degrees_to_radians(elevation));
}

double interpolate_linear(double x1, double y1, double x2, double y2, double x) {
    if (x2 == x1) return y1;
    return y1 + (y2 - y1) * (x - x1) / (x2 - x1);
}

double interpolate_cubic(double x0, double y0, double x1, double y1, 
                       double x2, double y2, double x3, double y3, double x) {
    double h0 = x1 - x0;
    double h1 = x2 - x1;
    double h2 = x3 - x2;
    
    if (h0 == 0 || h1 == 0 || h2 == 0) return interpolate_linear(x1, y1, x2, y2, x);
    
    double t0 = (x - x0) / (x3 - x0);
    double t1 = (x - x1) / (x3 - x0);
    double t2 = (x - x2) / (x3 - x0);
    double t3 = (x - x3) / (x3 - x0);
    
    return y0 * (1 - 3*t0 + 2*t0*t0) +
           y1 * (3*t1 - 2*t1*t1) +
           y2 * (3*t2 - 2*t2*t2) +
           y3 * (1 - 3*t3 + 2*t3*t3);
}

/* =================== 时间工具函数 =================== */

time_t time_parse_iso8601(const char* iso_string) {
    if (iso_string == NULL) return (time_t)-1;
    
    struct tm tm = {0};
    int year, month, day, hour, minute, second;
    
    if (sscanf(iso_string, "%d-%d-%dT%d:%d:%d", 
               &year, &month, &day, &hour, &minute, &second) == 6) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        return mktime(&tm);
    }
    
    /* 尝试其他格式 */
    if (sscanf(iso_string, "%d-%d-%d %d:%d:%d", 
               &year, &month, &day, &hour, &minute, &second) == 6) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        return mktime(&tm);
    }
    
    return (time_t)-1;
}

int time_format_iso8601(time_t time, char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return 0;
    
    struct tm* tm_info = localtime(&time);
    if (tm_info == NULL) return 0;
    
    return strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S", tm_info);
}

double time_diff_seconds(time_t time1, time_t time2) {
    return difftime(time2, time1);
}

time_t time_add_seconds(time_t time, double seconds) {
    return time + (time_t)seconds;
}

int time_is_valid(time_t time) {
    return time != (time_t)-1;
}

double time_to_julian_date(time_t time) {
    if (!time_is_valid(time)) return -1.0;
    
    /* Unix时间戳 (1970-01-01) 对应的儒略日 */
    const double unix_epoch_jd = 2440587.5;
    
    /* 将秒转换为儒略日 */
    return unix_epoch_jd + (double)time / 86400.0;
}

time_t julian_date_to_time(double jd) {
    if (jd < 0) return (time_t)-1;
    
    /* 儒略日转换为Unix时间戳 */
    const double unix_epoch_jd = 2440587.5;
    double seconds_since_epoch = (jd - unix_epoch_jd) * 86400.0;
    
    return (time_t)seconds_since_epoch;
}

/* =================== 地理坐标转换 =================== */

EcefCoordinate geodetic_to_ecef(const GeodeticCoordinate* geodetic) {
    EcefCoordinate ecef = {0};
    
    if (geodetic == NULL) return ecef;
    
    double lat_rad = degrees_to_radians(geodetic->latitude);
    double lon_rad = degrees_to_radians(geodetic->longitude);
    double alt = geodetic->altitude;
    
    double sin_lat = sin(lat_rad);
    double cos_lat = cos(lat_rad);
    double sin_lon = sin(lon_rad);
    double cos_lon = cos(lon_rad);
    
    double N = EARTH_RADIUS / sqrt(1.0 - EARTH_ECCENTRICITY * EARTH_ECCENTRICITY * sin_lat * sin_lat);
    
    ecef.x = (N + alt) * cos_lat * cos_lon;
    ecef.y = (N + alt) * cos_lat * sin_lon;
    ecef.z = (N * (1.0 - EARTH_ECCENTRICITY * EARTH_ECCENTRICITY) + alt) * sin_lat;
    
    return ecef;
}

GeodeticCoordinate ecef_to_geodetic(const EcefCoordinate* ecef) {
    GeodeticCoordinate geodetic = {0};
    
    if (ecef == NULL) return geodetic;
    
    double x = ecef->x;
    double y = ecef->y;
    double z = ecef->z;
    
    double p = sqrt(x * x + y * y);
    double theta = atan2(z * EARTH_RADIUS, p * EARTH_SEMI_MINOR);
    
    double sin_theta = sin(theta);
    double cos_theta = cos(theta);
    
    double lat_rad = atan2(z + EARTH_ECCENTRICITY * EARTH_ECCENTRICITY * EARTH_SEMI_MINOR * sin_theta * sin_theta * sin_theta,
                           p - EARTH_ECCENTRICITY * EARTH_ECCENTRICITY * EARTH_RADIUS * cos_theta * cos_theta * cos_theta);
    
    double lon_rad = atan2(y, x);
    
    double N = EARTH_RADIUS / sqrt(1.0 - EARTH_ECCENTRICITY * EARTH_ECCENTRICITY * sin(lat_rad) * sin(lat_rad));
    double alt = p / cos(lat_rad) - N;
    
    geodetic.latitude = radians_to_degrees(lat_rad);
    geodetic.longitude = radians_to_degrees(lon_rad);
    geodetic.altitude = alt;
    
    return geodetic;
}

EcefCoordinate geodetic_to_ecef_simple(double lat, double lon, double alt) {
    GeodeticCoordinate geodetic = {lat, lon, alt};
    return geodetic_to_ecef(&geodetic);
}

GeodeticCoordinate ecef_to_geodetic_simple(double x, double y, double z) {
    EcefCoordinate ecef = {x, y, z};
    return ecef_to_geodetic(&ecef);
}

/* =================== 字符串工具 =================== */

int string_is_empty(const char* str) {
    return str == NULL || str[0] == '\0';
}

int string_starts_with(const char* str, const char* prefix) {
    if (str == NULL || prefix == NULL) return 0;
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int string_ends_with(const char* str, const char* suffix) {
    if (str == NULL || suffix == NULL) return 0;
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char* string_trim(char* str) {
    if (str == NULL) return NULL;
    
    /* 去除前导空格 */
    char* start = str;
    while (*start && isspace(*start)) start++;
    
    /* 去除尾部空格 */
    char* end = str + strlen(str) - 1;
    while (end > start && isspace(*end)) end--;
    
    *(end + 1) = '\0';
    
    return start;
}

char* string_to_lower(char* str) {
    if (str == NULL) return NULL;
    
    for (char* p = str; *p; p++) {
        *p = tolower(*p);
    }
    
    return str;
}

char* string_to_upper(char* str) {
    if (str == NULL) return NULL;
    
    for (char* p = str; *p; p++) {
        *p = toupper(*p);
    }
    
    return str;
}

int string_split(const char* str, char delimiter, char** tokens, int max_tokens) {
    if (str == NULL || tokens == NULL || max_tokens <= 0) return 0;
    
    char* copy = strdup(str);
    if (copy == NULL) return 0;
    
    int count = 0;
    char* token = strtok(copy, &delimiter);
    
    while (token != NULL && count < max_tokens) {
        tokens[count] = strdup(token);
        if (tokens[count] == NULL) {
            for (int i = 0; i < count; i++) free(tokens[i]);
            free(copy);
            return 0;
        }
        count++;
        token = strtok(NULL, &delimiter);
    }
    
    free(copy);
    return count;
}

char* string_join(const char** tokens, int count, char delimiter) {
    if (tokens == NULL || count <= 0) return NULL;
    
    size_t total_length = 0;
    for (int i = 0; i < count; i++) {
        if (tokens[i] != NULL) {
            total_length += strlen(tokens[i]);
        }
    }
    total_length += count; /* 分隔符 */
    
    char* result = malloc(total_length + 1);
    if (result == NULL) return NULL;
    
    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (tokens[i] != NULL) {
            strcat(result, tokens[i]);
            if (i < count - 1) {
                char delim_str[2] = {delimiter, '\0'};
                strcat(result, delim_str);
            }
        }
    }
    
    return result;
}

/* =================== 文件工具 =================== */

int file_exists(const char* filename) {
    if (filename == NULL) return 0;
    FILE* file = fopen(filename, "r");
    if (file == NULL) return 0;
    fclose(file);
    return 1;
}

long file_size(const char* filename) {
    if (filename == NULL) return -1;
    
    FILE* file = fopen(filename, "rb");
    if (file == NULL) return -1;
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    
    return size;
}

int file_copy(const char* src, const char* dst) {
    if (src == NULL || dst == NULL) return 0;
    
    FILE* source = fopen(src, "rb");
    if (source == NULL) return 0;
    
    FILE* dest = fopen(dst, "wb");
    if (dest == NULL) {
        fclose(source);
        return 0;
    }
    
    char buffer[4096];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dest) != bytes_read) {
            fclose(source);
            fclose(dest);
            return 0;
        }
    }
    
    fclose(source);
    fclose(dest);
    return 1;
}

int file_delete(const char* filename) {
    if (filename == NULL) return 0;
    return remove(filename) == 0;
}

char* file_read_text(const char* filename) {
    if (filename == NULL) return NULL;
    
    long size = file_size(filename);
    if (size < 0) return NULL;
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) return NULL;
    
    char* content = malloc(size + 1);
    if (content == NULL) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(content, 1, size, file);
    content[bytes_read] = '\0';
    
    fclose(file);
    return content;
}

int file_write_text(const char* filename, const char* content) {
    if (filename == NULL || content == NULL) return 0;
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) return 0;
    
    int result = fputs(content, file) != EOF;
    fclose(file);
    
    return result;
}

int file_append_text(const char* filename, const char* content) {
    if (filename == NULL || content == NULL) return 0;
    
    FILE* file = fopen(filename, "a");
    if (file == NULL) return 0;
    
    int result = fputs(content, file) != EOF;
    fclose(file);
    
    return result;
}

char* file_get_extension(const char* filename) {
    if (filename == NULL) return NULL;
    
    char* last_dot = strrchr(filename, '.');
    if (last_dot == NULL) return "";
    
    return last_dot + 1;
}

char* file_get_basename(const char* filename) {
    if (filename == NULL) return NULL;
    
    char* last_slash = strrchr(filename, '/');
    char* last_backslash = strrchr(filename, '\\');
    
    char* last_separator = last_slash > last_backslash ? last_slash : last_backslash;
    if (last_separator == NULL) return (char*)filename;
    
    return last_separator + 1;
}

char* file_get_dirname(const char* filename) {
    if (filename == NULL) return NULL;
    
    char* last_slash = strrchr(filename, '/');
    char* last_backslash = strrchr(filename, '\\');
    
    char* last_separator = last_slash > last_backslash ? last_slash : last_backslash;
    if (last_separator == NULL) return ".";
    
    size_t length = last_separator - filename;
    char* dirname = malloc(length + 1);
    if (dirname == NULL) return NULL;
    
    strncpy(dirname, filename, length);
    dirname[length] = '\0';
    
    return dirname;
}

/* =================== 内存管理 =================== */

void* safe_malloc(size_t size) {
    if (size == 0) return NULL;
    
    void* ptr = malloc(size);
    if (ptr == NULL) {
        error_set(ERROR_MEMORY, "内存分配失败", __func__, __FILE__, __LINE__);
        return NULL;
    }
    
    return ptr;
}

void* safe_calloc(size_t count, size_t size) {
    if (count == 0 || size == 0) return NULL;
    
    void* ptr = calloc(count, size);
    if (ptr == NULL) {
        error_set(ERROR_MEMORY, "内存分配失败", __func__, __FILE__, __LINE__);
        return NULL;
    }
    
    return ptr;
}

void* safe_realloc(void* ptr, size_t size) {
    if (size == 0) {
        safe_free(&ptr);
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        error_set(ERROR_MEMORY, "内存重新分配失败", __func__, __FILE__, __LINE__);
        return NULL;
    }
    
    return new_ptr;
}

char* safe_strdup(const char* str) {
    if (str == NULL) return NULL;
    
    char* copy = strdup(str);
    if (copy == NULL) {
        error_set(ERROR_MEMORY, "字符串复制失败", __func__, __FILE__, __LINE__);
        return NULL;
    }
    
    return copy;
}

void safe_free(void** ptr) {
    if (ptr != NULL && *ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}

/* =================== 错误处理 =================== */

static ErrorInfo last_error = {0};

void error_set(ErrorCode code, const char* message, const char* function, 
               const char* file, int line) {
    last_error.code = code;
    last_error.timestamp = time(NULL);
    
    if (message != NULL) {
        strncpy(last_error.message, message, sizeof(last_error.message) - 1);
        last_error.message[sizeof(last_error.message) - 1] = '\0';
    }
    
    if (function != NULL) {
        strncpy(last_error.function, function, sizeof(last_error.function) - 1);
        last_error.function[sizeof(last_error.function) - 1] = '\0';
    }
    
    if (file != NULL) {
        strncpy(last_error.file, file, sizeof(last_error.file) - 1);
        last_error.file[sizeof(last_error.file) - 1] = '\0';
    }
    
    last_error.line = line;
}

ErrorInfo* error_get_last() {
    return &last_error;
}

void error_clear() {
    memset(&last_error, 0, sizeof(last_error));
}

const char* error_to_string(ErrorCode code) {
    switch (code) {
        case ERROR_NONE: return "无错误";
        case ERROR_MEMORY: return "内存错误";
        case ERROR_FILE: return "文件错误";
        case ERROR_PARSE: return "解析错误";
        case ERROR_NETWORK: return "网络错误";
        case ERROR_CALCULATION: return "计算错误";
        case ERROR_PARAMETER: return "参数错误";
        case ERROR_TIMEOUT: return "超时错误";
        case ERROR_SYSTEM: return "系统错误";
        default: return "未知错误";
    }
}

/* =================== 日志工具 (调用logger.c中的实现) =================== */

/* 基本日志功能由logger.c提供，这里只提供兼容性声明 */

/* =================== 配置管理 =================== */

void config_set_defaults(AppConfig* config) {
    if (config == NULL) return;
    
    strcpy(config->config_file, "config.ini");
    strcpy(config->log_file, "beidou_server.log");
    strcpy(config->data_dir, "data");
    config->server_port = 8080;
    config->max_connections = 10;
    config->log_level = LOG_LEVEL_INFO;
    config->enable_debug = 0;
    strcpy(config->version, "1.0.0");
}

int config_load(const char* filename, AppConfig* config) {
    if (filename == NULL || config == NULL) return 0;
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) return 0;
    
    config_set_defaults(config);
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char key[128], value[256];
        if (sscanf(line, "%127[^=]=%255[^\n]", key, value) == 2) {
            /* 去除空格 */
            char* trimmed_key = string_trim(key);
            char* trimmed_value = string_trim(value);
            
            if (strcmp(trimmed_key, "server_port") == 0) {
                config->server_port = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "max_connections") == 0) {
                config->max_connections = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "log_level") == 0) {
                config->log_level = atoi(trimmed_value);
            } else if (strcmp(trimmed_key, "enable_debug") == 0) {
                config->enable_debug = atoi(trimmed_value);
            }
        }
    }
    
    fclose(file);
    return 1;
}

int config_save(const char* filename, const AppConfig* config) {
    if (filename == NULL || config == NULL) return 0;
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) return 0;
    
    fprintf(file, "server_port=%d\n", config->server_port);
    fprintf(file, "max_connections=%d\n", config->max_connections);
    fprintf(file, "log_level=%d\n", config->log_level);
    fprintf(file, "enable_debug=%d\n", config->enable_debug);
    fprintf(file, "version=%s\n", config->version);
    
    fclose(file);
    return 1;
}

int config_validate(const AppConfig* config) {
    if (config == NULL) return 0;
    
    if (config->server_port <= 0 || config->server_port > 65535) return 0;
    if (config->max_connections <= 0 || config->max_connections > 1000) return 0;
    if (config->log_level < LOG_LEVEL_DEBUG || config->log_level > LOG_LEVEL_FATAL) return 0;
    
    return 1;
}

/* =================== 数据验证 =================== */

int validate_latitude(double lat) {
    return lat >= -90.0 && lat <= 90.0;
}

int validate_longitude(double lon) {
    return lon >= -180.0 && lon <= 180.0;
}

int validate_altitude(double alt) {
    return alt >= -500.0 && alt <= 50000.0;
}

int validate_attitude(double pitch, double roll, double yaw) {
    return pitch >= -90.0 && pitch <= 90.0 &&
           roll >= -180.0 && roll <= 180.0 &&
           yaw >= -180.0 && yaw <= 180.0;
}

int validate_velocity(double velocity) {
    return velocity >= 0.0 && velocity <= 1000.0;
}

int validate_timestamp(time_t time) {
    return time_is_valid(time);
}

int validate_prn(int prn) {
    return prn >= 1 && prn <= 99;
}

int validate_signal_strength(double signal_strength) {
    return signal_strength >= -200.0 && signal_strength <= 0.0;
}

/* =================== 性能监控 =================== */

void performance_timer_start(PerformanceTimer* timer, const char* name) {
    if (timer == NULL) return;
    
    timer->start_time = (double)clock() / CLOCKS_PER_SEC;
    timer->end_time = 0.0;
    timer->duration = 0.0;
    strncpy(timer->name, name, sizeof(timer->name) - 1);
    timer->name[sizeof(timer->name) - 1] = '\0';
}

void performance_timer_stop(PerformanceTimer* timer) {
    if (timer == NULL) return;
    
    timer->end_time = (double)clock() / CLOCKS_PER_SEC;
    timer->duration = timer->end_time - timer->start_time;
}

double performance_timer_elapsed(const PerformanceTimer* timer) {
    if (timer == NULL) return 0.0;
    return timer->duration;
}

void performance_counter_init(PerformanceCounter* counter, const char* name) {
    if (counter == NULL) return;
    
    counter->call_count = 0;
    counter->total_time = 0.0;
    counter->min_time = DBL_MAX;
    counter->max_time = 0.0;
    counter->avg_time = 0.0;
    strncpy(counter->name, name, sizeof(counter->name) - 1);
    counter->name[sizeof(counter->name) - 1] = '\0';
}

void performance_counter_add(PerformanceCounter* counter, double duration) {
    if (counter == NULL) return;
    
    counter->call_count++;
    counter->total_time += duration;
    
    if (duration < counter->min_time) {
        counter->min_time = duration;
    }
    
    if (duration > counter->max_time) {
        counter->max_time = duration;
    }
    
    counter->avg_time = counter->total_time / counter->call_count;
}

void performance_counter_reset(PerformanceCounter* counter) {
    if (counter == NULL) return;
    
    counter->call_count = 0;
    counter->total_time = 0.0;
    counter->min_time = DBL_MAX;
    counter->max_time = 0.0;
    counter->avg_time = 0.0;
}

/* =================== 线程安全 (简单版本) =================== */

int simple_mutex_init(SimpleMutex* mutex) {
    if (mutex == NULL) return 0;
    
    mutex->is_initialized = 1;
    mutex->lock_count = 0;
    return 1;
}

int simple_mutex_lock(SimpleMutex* mutex) {
    if (mutex == NULL || !mutex->is_initialized) return 0;
    
    /* 简单版本，实际应用中应使用真正的互斥锁 */
    mutex->lock_count++;
    return 1;
}

int simple_mutex_unlock(SimpleMutex* mutex) {
    if (mutex == NULL || !mutex->is_initialized || mutex->lock_count <= 0) return 0;
    
    mutex->lock_count--;
    return 1;
}

void simple_mutex_destroy(SimpleMutex* mutex) {
    if (mutex == NULL) return;
    
    mutex->is_initialized = 0;
    mutex->lock_count = 0;
}