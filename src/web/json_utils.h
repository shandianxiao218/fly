#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "http_server.h"

/* 基本JSON工具函数声明 */
int json_parse(const char* json_string, void* data_structure);
int json_serialize(const void* data_structure, char* buffer, int buffer_size);
int json_validate(const char* json_string);

/* JSON辅助函数声明 */
int json_escape_string(const char* input, char* output, int output_size);
int json_extract_string(const char* json_string, const char* key, char* output, int output_size);
int json_extract_int(const char* json_string, const char* key, int* output);
int json_extract_double(const char* json_string, const char* key, double* output);

#endif /* JSON_UTILS_H */