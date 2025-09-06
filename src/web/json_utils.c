#include "json_utils.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int json_parse(const char* json_string, void* data_structure) {
    if (json_string == NULL || data_structure == NULL) return 0;
    
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "解析JSON字符串");
    
    /* 简单的JSON验证 */
    if (!json_validate(json_string)) {
        logger_log_format(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__, "JSON格式无效");
        return 0;
    }
    
    /* 这里应该实现具体的JSON解析逻辑 */
    /* 由于这是一个简单的实现，我们只是记录一下信息 */
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "JSON解析完成（占位实现）");
    
    return 1;
}

int json_serialize(const void* data_structure, char* buffer, int buffer_size) {
    if (data_structure == NULL || buffer == NULL || buffer_size <= 0) return 0;
    
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "序列化数据结构为JSON");
    
    /* 这里应该实现具体的JSON序列化逻辑 */
    /* 由于这是一个简单的实现，我们创建一个基本的JSON结构 */
    int written = snprintf(buffer, buffer_size, 
                          "{\"type\":\"data_structure\","
                          "\"serialized\":true,"
                          "\"timestamp\":%lld}",
                          time(NULL));
    
    if (written >= buffer_size) {
        logger_log_format(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__, "JSON序列化缓冲区不足");
        return 0;
    }
    
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "JSON序列化完成");
    
    return written;
}

int json_validate(const char* json_string) {
    if (json_string == NULL) return 0;
    
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "验证JSON格式");
    
    int len = strlen(json_string);
    if (len == 0) return 0;
    
    /* 基本的JSON格式验证 */
    int brace_count = 0;
    int bracket_count = 0;
    int in_string = 0;
    int escape = 0;
    
    for (int i = 0; i < len; i++) {
        char c = json_string[i];
        
        if (escape) {
            escape = 0;
            continue;
        }
        
        if (c == '\\') {
            escape = 1;
            continue;
        }
        
        if (c == '"' && !escape) {
            in_string = !in_string;
            continue;
        }
        
        if (in_string) continue;
        
        switch (c) {
            case '{':
                brace_count++;
                break;
            case '}':
                brace_count--;
                if (brace_count < 0) return 0;
                break;
            case '[':
                bracket_count++;
                break;
            case ']':
                bracket_count--;
                if (bracket_count < 0) return 0;
                break;
        }
    }
    
    /* 检查括号是否匹配 */
    if (brace_count != 0 || bracket_count != 0) {
        logger_log_format(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__, "JSON括号不匹配");
        return 0;
    }
    
    /* 检查是否以正确的开头和结尾 */
    if (json_string[0] != '{' && json_string[0] != '[') {
        logger_log_format(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__, "JSON格式错误：应该以{或[开头");
        return 0;
    }
    
    char last_char = json_string[len - 1];
    while (isspace(last_char) && len > 1) {
        len--;
        last_char = json_string[len - 1];
    }
    
    if (last_char != '}' && last_char != ']') {
        logger_log_format(LOG_LEVEL_ERROR, __func__, __FILE__, __LINE__, "JSON格式错误：应该以}或]结尾");
        return 0;
    }
    
    logger_log_format(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__, "JSON格式验证通过");
    
    return 1;
}

/* 辅助函数：安全的JSON字符串转义 */
int json_escape_string(const char* input, char* output, int output_size) {
    if (input == NULL || output == NULL || output_size <= 0) return 0;
    
    int input_len = strlen(input);
    int output_pos = 0;
    
    for (int i = 0; i < input_len && output_pos < output_size - 1; i++) {
        char c = input[i];
        
        switch (c) {
            case '"':
                if (output_pos + 1 < output_size) {
                    output[output_pos++] = '\\';
                    output[output_pos++] = '"';
                }
                break;
            case '\\':
                if (output_pos + 1 < output_size) {
                    output[output_pos++] = '\\';
                    output[output_pos++] = '\\';
                }
                break;
            case '\n':
                if (output_pos + 1 < output_size) {
                    output[output_pos++] = '\\';
                    output[output_pos++] = 'n';
                }
                break;
            case '\r':
                if (output_pos + 1 < output_size) {
                    output[output_pos++] = '\\';
                    output[output_pos++] = 'r';
                }
                break;
            case '\t':
                if (output_pos + 1 < output_size) {
                    output[output_pos++] = '\\';
                    output[output_pos++] = 't';
                }
                break;
            default:
                if (output_pos < output_size) {
                    output[output_pos++] = c;
                }
                break;
        }
    }
    
    output[output_pos] = '\0';
    return output_pos;
}

/* 辅助函数：从JSON中提取字符串值 */
int json_extract_string(const char* json_string, const char* key, char* output, int output_size) {
    if (json_string == NULL || key == NULL || output == NULL || output_size <= 0) return 0;
    
    /* 构建搜索模式 */
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    
    char* key_pos = strstr(json_string, search_pattern);
    if (key_pos == NULL) return 0;
    
    /* 找到值开始位置 */
    char* value_start = strchr(key_pos + strlen(search_pattern), '"');
    if (value_start == NULL) return 0;
    
    value_start++; /* 跳过开始的引号 */
    
    /* 找到值结束位置 */
    char* value_end = value_start;
    while (*value_end != '\0' && *value_end != '"') {
        if (*value_end == '\\' && *(value_end + 1) == '"') {
            value_end += 2; /* 跳过转义的引号 */
        } else {
            value_end++;
        }
    }
    
    if (*value_end != '"') return 0;
    
    /* 计算值长度 */
    int value_len = value_end - value_start;
    if (value_len >= output_size) value_len = output_size - 1;
    
    /* 复制值 */
    strncpy(output, value_start, value_len);
    output[value_len] = '\0';
    
    return 1;
}

/* 辅助函数：从JSON中提取整数值 */
int json_extract_int(const char* json_string, const char* key, int* output) {
    if (json_string == NULL || key == NULL || output == NULL) return 0;
    
    /* 构建搜索模式 */
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    
    char* key_pos = strstr(json_string, search_pattern);
    if (key_pos == NULL) return 0;
    
    /* 找到值开始位置 */
    char* value_start = key_pos + strlen(search_pattern);
    while (*value_start && isspace(*value_start)) value_start++;
    
    /* 解析整数值 */
    *output = atoi(value_start);
    
    return 1;
}

/* 辅助函数：从JSON中提取浮点数值 */
int json_extract_double(const char* json_string, const char* key, double* output) {
    if (json_string == NULL || key == NULL || output == NULL) return 0;
    
    /* 构建搜索模式 */
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    
    char* key_pos = strstr(json_string, search_pattern);
    if (key_pos == NULL) return 0;
    
    /* 找到值开始位置 */
    char* value_start = key_pos + strlen(search_pattern);
    while (*value_start && isspace(*value_start)) value_start++;
    
    /* 解析浮点数值 */
    *output = atof(value_start);
    
    return 1;
}