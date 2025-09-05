#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "http_server.h"

/* JSON工具函数声明 */
int json_parse(const char* json_string, void* data_structure);
int json_serialize(const void* data_structure, char* buffer, int buffer_size);
int json_validate(const char* json_string);

#endif /* JSON_UTILS_H */