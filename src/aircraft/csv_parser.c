#include "aircraft.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief 解析CSV轨迹文件
 * 
 * CSV文件格式：
 * timestamp,latitude,longitude,altitude,velocity,vertical_speed,heading,pitch,roll,yaw,is_valid
 * 
 * @param filename CSV文件名
 * @param trajectory 轨迹结构体
 * @param status 解析状态
 * @return int 成功返回1，失败返回0
 */
int csv_trajectory_parse(const char* filename, FlightTrajectory* trajectory, 
                        CsvParseStatus* status) {
    if (filename == NULL || trajectory == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 初始化状态 */
    if (status != NULL) {
        memset(status, 0, sizeof(CsvParseStatus));
        status->line_number = 1;
    }
    
    /* 打开文件 */
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        error_set(ERROR_FILE, "无法打开CSV文件", __func__, __FILE__, __LINE__);
        if (status != NULL) {
            strncpy(status->last_error, "无法打开文件", sizeof(status->last_error) - 1);
            status->error_count++;
        }
        return 0;
    }
    
    /* 清空现有轨迹 */
    flight_trajectory_clear(trajectory);
    
    char line[1024];
    int is_header = 1; /* 第一行是标题行 */
    
    while (fgets(line, sizeof(line), file) != NULL) {
        /* 去除换行符 */
        line[strcspn(line, "\r\n")] = '\0';
        
        /* 跳过空行 */
        if (strlen(line) == 0) {
            if (status != NULL) status->line_number++;
            continue;
        }
        
        /* 跳过注释行 */
        if (line[0] == '#') {
            if (status != NULL) status->line_number++;
            continue;
        }
        
        /* 处理标题行 */
        if (is_header) {
            is_header = 0;
            if (status != NULL) status->line_number++;
            continue;
        }
        
        /* 解析数据行 */
        TrajectoryPoint point;
        memset(&point, 0, sizeof(TrajectoryPoint));
        
        char* token;
        char* rest = line;
        int field_count = 0;
        int parse_error = 0;
        
        /* 解析各个字段 */
        while ((token = strtok_r(rest, ",", &rest)) != NULL && field_count < 11) {
            /* 检查字段是否为空 */
            if (token == NULL || strlen(token) == 0) {
                parse_error = 1;
                break;
            }
            
            /* 安全的数字转换 */
            char* endptr;
            switch (field_count) {
                case 0: /* timestamp */
                    point.timestamp = (time_t)strtoll(token, &endptr, 10);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 1: /* latitude */
                    point.state.position.latitude = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 2: /* longitude */
                    point.state.position.longitude = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 3: /* altitude */
                    point.state.position.altitude = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 4: /* velocity */
                    point.state.velocity.velocity = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 5: /* vertical_speed */
                    point.state.velocity.vertical_speed = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 6: /* heading */
                    point.state.velocity.heading = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 7: /* pitch */
                    point.state.attitude.pitch = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 8: /* roll */
                    point.state.attitude.roll = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 9: /* yaw */
                    point.state.attitude.yaw = strtod(token, &endptr);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                case 10: /* is_valid */
                    point.state.is_valid = (int)strtol(token, &endptr, 10);
                    if (*endptr != '\0' && *endptr != '\n' && *endptr != '\r') {
                        parse_error = 1;
                    }
                    break;
                    
                default:
                    break;
            }
            
            if (parse_error) {
                break;
            }
            
            field_count++;
        }
        
        /* 检查是否还有多余的字段 */
        if (strtok_r(rest, ",", &rest) != NULL) {
            parse_error = 1; /* 字段过多 */
        }
        
        /* 检查字段数量 */
        if (field_count < 11) {
            parse_error = 1;
            if (status != NULL) {
                snprintf(status->last_error, sizeof(status->last_error), 
                        "第%d行: 字段数量不足", status->line_number);
                status->error_count++;
            }
        }
        
        /* 验证数据 */
        if (!parse_error && !aircraft_state_validate(&point.state)) {
            parse_error = 1;
            if (status != NULL) {
                snprintf(status->last_error, sizeof(status->last_error), 
                        "第%d行: 数据无效", status->line_number);
                status->error_count++;
            }
        }
        
        /* 添加到轨迹 */
        if (!parse_error) {
            if (flight_trajectory_add_point(trajectory, &point)) {
                if (status != NULL) {
                    status->valid_points++;
                }
            } else {
                parse_error = 1;
                if (status != NULL) {
                    snprintf(status->last_error, sizeof(status->last_error), 
                            "第%d行: 无法添加轨迹点", status->line_number);
                    status->error_count++;
                }
            }
        }
        
        if (status != NULL) {
            status->line_number++;
        }
    }
    
    /* 关闭文件 */
    fclose(file);
    
    /* 更新状态 */
    if (status != NULL) {
        status->total_lines = status->line_number - 1;
    }
    
    /* 检查是否有有效数据 */
    if (trajectory->point_count == 0) {
        error_set(ERROR_PARSE, "CSV文件中没有有效数据", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    return 1;
}

/**
 * @brief 写入CSV示例文件
 * 
 * 生成一个CSV示例文件，包含示例轨迹数据
 * 
 * @param filename 文件名
 * @return int 成功返回1，失败返回0
 */
int csv_trajectory_write_example(const char* filename) {
    if (filename == NULL) {
        error_set(ERROR_PARAMETER, "文件名不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        error_set(ERROR_FILE, "无法创建CSV文件", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 写入标题行 */
    fprintf(file, "timestamp,latitude,longitude,altitude,velocity,vertical_speed,heading,pitch,roll,yaw,is_valid\n");
    
    /* 写入示例数据 */
    time_t base_time = time(NULL);
    
    for (int i = 0; i < 10; i++) {
        time_t timestamp = base_time + i * 10;
        double latitude = 39.9042 + i * 0.001;
        double longitude = 116.4074 + i * 0.001;
        double altitude = 1000.0 + i * 100.0;
        double velocity = 250.0;
        double vertical_speed = 5.0;
        double heading = 45.0;
        double pitch = 2.0;
        double roll = 0.0;
        double yaw = 45.0;
        int is_valid = 1;
        
        fprintf(file, "%ld,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d\n",
                timestamp, latitude, longitude, altitude, velocity, vertical_speed,
                heading, pitch, roll, yaw, is_valid);
    }
    
    fclose(file);
    return 1;
}

/**
 * @brief 保存轨迹到CSV文件
 * 
 * @param trajectory 轨迹结构体
 * @param filename 文件名
 * @return int 成功返回1，失败返回0
 */
int flight_trajectory_save_csv(const FlightTrajectory* trajectory, const char* filename) {
    if (trajectory == NULL || filename == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        error_set(ERROR_FILE, "无法创建CSV文件", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 写入标题行 */
    fprintf(file, "timestamp,latitude,longitude,altitude,velocity,vertical_speed,heading,pitch,roll,yaw,is_valid\n");
    
    /* 写入轨迹数据 */
    for (int i = 0; i < trajectory->point_count; i++) {
        const TrajectoryPoint* point = &trajectory->points[i];
        
        fprintf(file, "%ld,%.6f,%.6f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d\n",
                point->timestamp,
                point->state.position.latitude,
                point->state.position.longitude,
                point->state.position.altitude,
                point->state.velocity.velocity,
                point->state.velocity.vertical_speed,
                point->state.velocity.heading,
                point->state.attitude.pitch,
                point->state.attitude.roll,
                point->state.attitude.yaw,
                point->state.is_valid);
    }
    
    fclose(file);
    return 1;
}

/**
 * @brief 从CSV文件加载轨迹
 * 
 * @param trajectory 轨迹结构体
 * @param filename 文件名
 * @return int 成功返回1，失败返回0
 */
int flight_trajectory_load_csv(FlightTrajectory* trajectory, const char* filename) {
    return csv_trajectory_parse(filename, trajectory, NULL);
}
