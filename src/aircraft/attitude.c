#include "aircraft.h"
#include "../utils/utils.h"
#include <math.h>

/* 常量定义 */
#define GRAVITY 9.81
#define MIN_HORIZONTAL_VELOCITY 0.1

/**
 * @brief 计算飞机姿态角
 * 
 * 根据飞机的速度和加速度计算姿态角
 * 
 * @param velocity 速度向量
 * @param acceleration 加速度向量
 * @param attitude 计算得到的姿态角
 * @return int 成功返回1，失败返回0
 */
int aircraft_attitude_calculate(const AircraftVelocity* velocity, 
                              const double acceleration[3],
                              AircraftAttitude* attitude) {
    if (velocity == NULL || acceleration == NULL || attitude == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 计算俯仰角 */
    double vx = velocity->velocity * cos(degrees_to_radians(velocity->heading));
    double vy = velocity->velocity * sin(degrees_to_radians(velocity->heading));
    double vz = velocity->vertical_speed;
    
    /* 俯仰角基于垂直速度和水平速度计算 */
    double horizontal_velocity = sqrt(vx * vx + vy * vy);
    if (horizontal_velocity > MIN_HORIZONTAL_VELOCITY) {
        attitude->pitch = radians_to_degrees(atan2(vz, horizontal_velocity));
    } else {
        attitude->pitch = 0.0;
    }
    
    /* 横滚角基于侧向加速度计算 */
    double centripetal_accel = acceleration[0] * cos(degrees_to_radians(velocity->heading)) + 
                              acceleration[1] * sin(degrees_to_radians(velocity->heading));
    attitude->roll = radians_to_degrees(atan2(centripetal_accel, GRAVITY));
    
    /* 偏航角直接使用航向角 */
    attitude->yaw = velocity->heading;
    
    /* 归一化角度到[-180, 180]范围 */
    attitude->pitch = normalize_angle(attitude->pitch);
    attitude->roll = normalize_angle(attitude->roll);
    attitude->yaw = normalize_angle(attitude->yaw);
    
    return 1;
}

/**
 * @brief 计算极端姿态角
 * 
 * 模拟飞机在极端条件下的姿态角
 * 
 * @param base_attitude 基础姿态角
 * @param maneuver_type 机动类型 (1: 急转弯, 2: 急爬升, 3: 急俯冲, 4: 筋斗)
 * @param intensity 机动强度 (0.0 - 1.0)
 * @param result 计算得到的极端姿态角
 * @return int 成功返回1，失败返回0
 */
int aircraft_attitude_extreme(const AircraftAttitude* base_attitude,
                             int maneuver_type, double intensity,
                             AircraftAttitude* result) {
    if (base_attitude == NULL || result == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    if (intensity < 0.0 || intensity > 1.0) {
        error_set(ERROR_PARAMETER, "机动强度必须在0.0到1.0之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 复制基础姿态 */
    *result = *base_attitude;
    
    /* 根据机动类型调整姿态角 */
    switch (maneuver_type) {
        case 1: /* 急转弯 */
            result->roll += 60.0 * intensity; /* 最大60度横滚 */
            result->pitch += 15.0 * intensity; /* 轻微抬头 */
            break;
            
        case 2: /* 急爬升 */
            result->pitch += 45.0 * intensity; /* 最大45度俯仰 */
            result->roll += 5.0 * intensity; /* 轻微横滚 */
            break;
            
        case 3: /* 急俯冲 */
            result->pitch -= 60.0 * intensity; /* 最大60度俯冲 */
            result->roll -= 5.0 * intensity; /* 轻微横滚 */
            break;
            
        case 4: /* 筋斗 */
            result->pitch += 180.0 * intensity; /* 180度翻转 */
            result->roll += 30.0 * intensity; /* 伴随横滚 */
            break;
            
        default:
            error_set(ERROR_PARAMETER, "未知的机动类型", __func__, __FILE__, __LINE__);
            return 0;
    }
    
    /* 归一化角度 */
    result->pitch = normalize_angle(result->pitch);
    result->roll = normalize_angle(result->roll);
    result->yaw = normalize_angle(result->yaw);
    
    return 1;
}

/**
 * @brief 姿态角插值
 * 
 * 在两个姿态角之间进行线性插值
 * 
 * @param attitude1 起始姿态角
 * @param attitude2 结束姿态角
 * @param t 插值参数 (0.0 - 1.0)
 * @param result 插值结果
 * @return int 成功返回1，失败返回0
 */
/**
 * @brief 角度插值辅助函数
 * 
 * 处理角度环绕的插值
 * 
 * @param angle1 起始角度
 * @param angle2 结束角度
 * @param t 插值参数
 * @return double 插值结果
 */
static double interpolate_angle(double angle1, double angle2, double t) {
    /* 计算角度差，考虑环绕 */
    double diff = angle2 - angle1;
    
    /* 如果差值大于180度，选择另一条路径 */
    if (diff > 180.0) {
        diff -= 360.0;
    } else if (diff < -180.0) {
        diff += 360.0;
    }
    
    /* 线性插值 */
    double result = angle1 + diff * t;
    
    /* 归一化到[-180, 180]范围 */
    return normalize_angle(result);
}

/**
 * @brief 姿态角插值
 * 
 * 在两个姿态角之间进行线性插值
 * 
 * @param attitude1 起始姿态角
 * @param attitude2 结束姿态角
 * @param t 插值参数 (0.0 - 1.0)
 * @param result 插值结果
 * @return int 成功返回1，失败返回0
 */
int aircraft_attitude_interpolate(const AircraftAttitude* attitude1,
                                 const AircraftAttitude* attitude2,
                                 double t, AircraftAttitude* result) {
    if (attitude1 == NULL || attitude2 == NULL || result == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    if (t < 0.0 || t > 1.0) {
        error_set(ERROR_PARAMETER, "插值参数必须在0.0到1.0之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 对角度进行插值，需要处理角度环绕 */
    result->pitch = interpolate_angle(attitude1->pitch, attitude2->pitch, t);
    result->roll = interpolate_angle(attitude1->roll, attitude2->roll, t);
    result->yaw = interpolate_angle(attitude1->yaw, attitude2->yaw, t);
    
    return 1;
}

/**
 * @brief 验证姿态角
 * 
 * 检查姿态角是否在合理范围内
 * 
 * @param attitude 姿态角
 * @return int 有效返回1，无效返回0
 */
int aircraft_attitude_validate(const AircraftAttitude* attitude) {
    if (attitude == NULL) {
        error_set(ERROR_PARAMETER, "姿态角不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 检查俯仰角范围 */
    if (attitude->pitch < -90.0 || attitude->pitch > 90.0) {
        error_set(ERROR_PARAMETER, "俯仰角超出范围", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 检查横滚角范围 */
    if (attitude->roll < -180.0 || attitude->roll > 180.0) {
        error_set(ERROR_PARAMETER, "横滚角超出范围", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 检查偏航角范围 */
    if (attitude->yaw < -180.0 || attitude->yaw > 180.0) {
        error_set(ERROR_PARAMETER, "偏航角超出范围", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    return 1;
}
