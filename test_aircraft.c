#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "../src/aircraft/aircraft.h"
#include "../src/utils/utils.h"

/**
 * @brief 测试飞机姿态计算
 */
void test_aircraft_attitude() {
    printf("=== 测试飞机姿态计算 ===\n");
    
    AircraftVelocity velocity = {250.0, 5.0, 45.0};  /* 速度250m/s，垂直速度5m/s，航向45° */
    double acceleration[3] = {2.0, 0.0, 0.0};      /* 加速度向量 */
    AircraftAttitude attitude;
    
    int result = aircraft_attitude_calculate(&velocity, acceleration, &attitude);
    
    if (result) {
        printf("姿态计算成功：\n");
        printf("  俯仰角: %.2f°\n", attitude.pitch);
        printf("  横滚角: %.2f°\n", attitude.roll);
        printf("  偏航角: %.2f°\n", attitude.yaw);
    } else {
        printf("姿态计算失败\n");
    }
    printf("\n");
}

/**
 * @brief 测试轨迹生成
 */
void test_trajectory_generation() {
    printf("=== 测试轨迹生成 ===\n");
    
    FlightTrajectory* trajectory = flight_trajectory_create(100);
    if (trajectory == NULL) {
        printf("轨迹创建失败\n");
        return;
    }
    
    /* 创建开始状态 */
    AircraftState start_state = {
        .position = {39.9042, 116.4074, 0.0},    /* 北京坐标 */
        .velocity = {0.0, 0.0, 90.0},           /* 初始速度0，航向90° */
        .attitude = {0.0, 0.0, 90.0},           /* 初始姿态 */
        .timestamp = time(NULL),
        .is_valid = 1
    };
    
    /* 生成起飞轨迹 */
    printf("生成起飞轨迹...\n");
    int result = flight_trajectory_generate_takeoff(trajectory, &start_state, 10000.0);
    
    if (result) {
        printf("起飞轨迹生成成功，共%d个点\n", trajectory->point_count);
        
        /* 显示前几个点 */
        for (int i = 0; i < 5 && i < trajectory->point_count; i++) {
            TrajectoryPoint* point = &trajectory->points[i];
            printf("  点%d: 时间=%ld, 高度=%.2f, 速度=%.2f\n", 
                   i, point->timestamp, point->state.position.altitude, 
                   point->state.velocity.velocity);
        }
    } else {
        printf("起飞轨迹生成失败\n");
    }
    
    flight_trajectory_destroy(trajectory);
    printf("\n");
}

/**
 * @brief 测试CSV解析
 */
void test_csv_parsing() {
    printf("=== 测试CSV解析 ===\n");
    
    /* 生成示例CSV文件 */
    const char* filename = "test_trajectory.csv";
    int result = csv_trajectory_write_example(filename);
    
    if (result) {
        printf("示例CSV文件生成成功: %s\n", filename);
        
        /* 解析CSV文件 */
        FlightTrajectory* trajectory = flight_trajectory_create(100);
        if (trajectory == NULL) {
            printf("轨迹创建失败\n");
            return;
        }
        
        CsvParseStatus status;
        result = csv_trajectory_parse(filename, trajectory, &status);
        
        if (result) {
            printf("CSV解析成功，共%d个点\n", trajectory->point_count);
            printf("解析状态: %d\n", status);
        } else {
            printf("CSV解析失败，状态: %d\n", status);
        }
        
        flight_trajectory_destroy(trajectory);
        
        /* 清理测试文件 */
        remove(filename);
    } else {
        printf("示例CSV文件生成失败\n");
    }
    printf("\n");
}

/**
 * @brief 测试极端姿态
 */
void test_extreme_attitude() {
    printf("=== 测试极端姿态 ===\n");
    
    AircraftVelocity velocity = {300.0, 0.0, 0.0};
    AircraftAttitude attitude;
    
    /* 测试急转弯 */
    printf("急转弯姿态：\n");
    double acceleration_turn[3] = {15.0, 0.0, 0.0};  /* 1.5g转弯 */
    aircraft_attitude_calculate(&velocity, acceleration_turn, &attitude);
    printf("  横滚角: %.2f°\n", attitude.roll);
    
    /* 测试急爬升 */
    printf("急爬升姿态：\n");
    velocity.vertical_speed = 20.0;
    double acceleration_climb[3] = {0.0, 0.0, 5.0};
    aircraft_attitude_calculate(&velocity, acceleration_climb, &attitude);
    printf("  俯仰角: %.2f°\n", attitude.pitch);
    
    printf("\n");
}

/**
 * @brief 测试状态插值
 */
void test_state_interpolation() {
    printf("=== 测试状态插值 ===\n");
    
    /* 创建两个状态 */
    AircraftState state1 = {
        .position = {39.9042, 116.4074, 1000.0},
        .velocity = {100.0, 5.0, 45.0},
        .attitude = {5.0, 0.0, 45.0},
        .timestamp = 1000,
        .is_valid = 1
    };
    
    AircraftState state2 = {
        .position = {39.9142, 116.4174, 2000.0},
        .velocity = {150.0, 10.0, 50.0},
        .attitude = {10.0, 5.0, 50.0},
        .timestamp = 1100,
        .is_valid = 1
    };
    
    /* 插值计算 */
    AircraftState result;
    time_t target_time = 1050;  /* 中间时间点 */
    
    int success = aircraft_state_interpolate(&state1, &state2, target_time, &result);
    
    if (success) {
        printf("状态插值成功：\n");
        printf("  位置: %.6f, %.6f, %.2f\n", result.position.latitude, 
               result.position.longitude, result.position.altitude);
        printf("  速度: %.2f, %.2f, %.2f\n", result.velocity.velocity, 
               result.velocity.vertical_speed, result.velocity.heading);
        printf("  姿态: %.2f, %.2f, %.2f\n", result.attitude.pitch, 
               result.attitude.roll, result.attitude.yaw);
        printf("  时间: %ld\n", result.timestamp);
    } else {
        printf("状态插值失败\n");
    }
    printf("\n");
}

/**
 * @brief 主函数
 */
int main() {
    printf("北斗导航卫星可见性分析系统 - Aircraft模块测试\n");
    printf("Beidou Navigation Satellite Visibility Analysis System - Aircraft Module Test\n");
    printf("================================================================================\n\n");
    
    /* 初始化日志系统 */
    logger_init("aircraft_test.log", LOG_LEVEL_INFO);
    
    /* 运行测试 */
    test_aircraft_attitude();
    test_trajectory_generation();
    test_csv_parsing();
    test_extreme_attitude();
    test_state_interpolation();
    
    printf("所有测试完成！\n");
    
    /* 清理日志系统 */
    logger_cleanup();
    
    return 0;
}