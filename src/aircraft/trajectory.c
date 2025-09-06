#include "aircraft.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

FlightTrajectory* flight_trajectory_create(int max_points) {
    if (max_points <= 0) return NULL;
    
    FlightTrajectory* trajectory = (FlightTrajectory*)safe_malloc(sizeof(FlightTrajectory));
    if (trajectory == NULL) return NULL;
    
    trajectory->points = (TrajectoryPoint*)safe_calloc(max_points, sizeof(TrajectoryPoint));
    if (trajectory->points == NULL) {
        safe_free((void**)&trajectory);
        return NULL;
    }
    
    trajectory->point_count = 0;
    trajectory->max_points = max_points;
    trajectory->start_time = 0;
    trajectory->end_time = 0;
    trajectory->total_distance = 0.0;
    trajectory->max_altitude = 0.0;
    trajectory->min_altitude = 0.0;
    
    return trajectory;
}

void flight_trajectory_destroy(FlightTrajectory* trajectory) {
    if (trajectory == NULL) return;
    
    if (trajectory->points != NULL) {
        safe_free((void**)&trajectory->points);
    }
    
    safe_free((void**)&trajectory);
}

int flight_trajectory_add_point(FlightTrajectory* trajectory, const TrajectoryPoint* point) {
    if (trajectory == NULL || point == NULL) return 0;
    
    if (trajectory->point_count >= trajectory->max_points) {
        error_set(ERROR_MEMORY, "轨迹点已满", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    memcpy(&trajectory->points[trajectory->point_count], point, sizeof(TrajectoryPoint));
    trajectory->point_count++;
    
    /* 更新轨迹统计信息 */
    if (trajectory->point_count == 1) {
        trajectory->start_time = point->timestamp;
        trajectory->max_altitude = point->state.position.altitude;
        trajectory->min_altitude = point->state.position.altitude;
    } else {
        trajectory->end_time = point->timestamp;
        
        if (point->state.position.altitude > trajectory->max_altitude) {
            trajectory->max_altitude = point->state.position.altitude;
        }
        if (point->state.position.altitude < trajectory->min_altitude) {
            trajectory->min_altitude = point->state.position.altitude;
        }
    }
    
    return 1;
}

int flight_trajectory_clear(FlightTrajectory* trajectory) {
    if (trajectory == NULL) return 0;
    
    trajectory->point_count = 0;
    trajectory->start_time = 0;
    trajectory->end_time = 0;
    trajectory->total_distance = 0.0;
    trajectory->max_altitude = 0.0;
    trajectory->min_altitude = 0.0;
    
    return 1;
}

int flight_trajectory_generate(FlightTrajectory* trajectory, const TrajectoryParams* params) {
    if (trajectory == NULL || params == NULL) return 0;
    
    /* 验证参数 */
    if (!trajectory_params_validate(params)) {
        error_set(ERROR_PARAMETER, "轨迹参数无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 清空现有轨迹 */
    flight_trajectory_clear(trajectory);
    
    /* 根据轨迹类型生成轨迹 */
    switch (params->type) {
        case TRAJECTORY_TYPE_TAKEOFF:
            return flight_trajectory_generate_takeoff(trajectory, &params->start_state, params->end_state.position.altitude);
        
        case TRAJECTORY_TYPE_CRUISE:
            return flight_trajectory_generate_cruise(trajectory, &params->start_state, params->duration, params->start_state.velocity.heading);
        
        case TRAJECTORY_TYPE_LANDING:
            return flight_trajectory_generate_landing(trajectory, &params->start_state, &params->end_state);
        
        case TRAJECTORY_TYPE_MANEUVER:
            return flight_trajectory_generate_maneuver(trajectory, &params->start_state, params->duration, params->max_roll);
        
        default:
            error_set(ERROR_PARAMETER, "不支持的轨迹类型", __func__, __FILE__, __LINE__);
            return 0;
    }
}

int flight_trajectory_generate_takeoff(FlightTrajectory* trajectory, 
                                      const AircraftState* start_state,
                                      double target_altitude) {
    if (trajectory == NULL || start_state == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证开始状态 */
    if (!aircraft_state_validate(start_state)) {
        error_set(ERROR_PARAMETER, "开始状态无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证目标高度 */
    if (target_altitude <= start_state->position.altitude) {
        error_set(ERROR_PARAMETER, "目标高度必须高于开始高度", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 清空现有轨迹 */
    flight_trajectory_clear(trajectory);
    
    /* 起飞参数 */
    const double takeoff_duration = 300.0; /* 5分钟起飞时间 */
    const double ground_speed = 80.0;     /* 地面速度 80 m/s */
    const double climb_rate = 10.0;       /* 爬升率 10 m/s */
    const int point_interval = 5;         /* 5秒间隔 */
    
    int num_points = (int)(takeoff_duration / point_interval) + 1;
    
    /* 生成起飞轨迹点 */
    for (int i = 0; i < num_points; i++) {
        TrajectoryPoint point;
        double t = (double)i / (double)(num_points - 1);
        
        /* 计算时间戳 */
        point.timestamp = start_state->timestamp + (time_t)(i * point_interval);
        
        /* 计算位置 */
        point.state.position.latitude = start_state->position.latitude;
        point.state.position.longitude = start_state->position.longitude;
        
        /* 使用S型曲线计算高度变化 */
        double altitude_progress = 0.5 * (1 - cos(M_PI * t));
        point.state.position.altitude = start_state->position.altitude + 
                                       (target_altitude - start_state->position.altitude) * altitude_progress;
        
        /* 计算速度 */
        if (t < 0.3) {
            /* 加速阶段 */
            point.state.velocity.velocity = ground_speed * (t / 0.3);
        } else if (t < 0.7) {
            /* 稳定阶段 */
            point.state.velocity.velocity = ground_speed;
        } else {
            /* 减速阶段 */
            point.state.velocity.velocity = ground_speed * (1.0 - (t - 0.7) / 0.3);
        }
        
        point.state.velocity.vertical_speed = climb_rate * cos(M_PI * t);
        point.state.velocity.heading = start_state->velocity.heading;
        
        /* 计算姿态角 */
        point.state.attitude.pitch = 15.0 * sin(M_PI * t); /* 最大15度俯仰角 */
        point.state.attitude.roll = 0.0;
        point.state.attitude.yaw = start_state->velocity.heading;
        
        point.state.is_valid = 1;
        
        /* 添加轨迹点 */
        if (!flight_trajectory_add_point(trajectory, &point)) {
            error_set(ERROR_MEMORY, "无法添加轨迹点", __func__, __FILE__, __LINE__);
            return 0;
        }
    }
    
    /* 计算总距离 */
    trajectory->total_distance = 0.0;
    for (int i = 1; i < trajectory->point_count; i++) {
        double distance = aircraft_state_distance(
            &trajectory->points[i-1].state, 
            &trajectory->points[i].state
        );
        trajectory->total_distance += distance;
    }
    
    return 1;
}

int flight_trajectory_generate_cruise(FlightTrajectory* trajectory,
                                     const AircraftState* start_state,
                                     double duration, double heading) {
    if (trajectory == NULL || start_state == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证开始状态 */
    if (!aircraft_state_validate(start_state)) {
        error_set(ERROR_PARAMETER, "开始状态无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证持续时间 */
    if (duration <= 0.0) {
        error_set(ERROR_PARAMETER, "持续时间必须大于0", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 清空现有轨迹 */
    flight_trajectory_clear(trajectory);
    
    /* 巡航参数 */
    const double cruise_speed = 250.0;    /* 巡航速度 250 m/s */
    const int point_interval = 10;         /* 10秒间隔 */
    
    int num_points = (int)(duration / point_interval) + 1;
    
    /* 计算每个时间点的位移 */
    const double earth_radius = 6371000.0; /* 地球半径 */
    double angular_distance = (cruise_speed * point_interval) / earth_radius;
    
    /* 生成巡航轨迹点 */
    for (int i = 0; i < num_points; i++) {
        TrajectoryPoint point;
        
        /* 计算时间戳 */
        point.timestamp = start_state->timestamp + (time_t)(i * point_interval);
        
        /* 计算位置（沿航向直线飞行） */
        double distance = i * cruise_speed * point_interval;
        
        /* 使用大圆航线计算新位置 */
        double lat1 = degrees_to_radians(start_state->position.latitude);
        double lon1 = degrees_to_radians(start_state->position.longitude);
        double bearing = degrees_to_radians(heading);
        double angular_dist = distance / earth_radius;
        
        double lat2 = asin(sin(lat1) * cos(angular_dist) + 
                          cos(lat1) * sin(angular_dist) * cos(bearing));
        double lon2 = lon1 + atan2(sin(bearing) * sin(angular_dist) * cos(lat1),
                                  cos(angular_dist) - sin(lat1) * sin(lat2));
        
        point.state.position.latitude = radians_to_degrees(lat2);
        point.state.position.longitude = radians_to_degrees(lon2);
        point.state.position.altitude = start_state->position.altitude;
        
        /* 计算速度 */
        point.state.velocity.velocity = cruise_speed;
        point.state.velocity.vertical_speed = 0.0;
        point.state.velocity.heading = heading;
        
        /* 计算姿态角 */
        point.state.attitude.pitch = 0.0;
        point.state.attitude.roll = 0.0;
        point.state.attitude.yaw = heading;
        
        point.state.is_valid = 1;
        
        /* 添加轨迹点 */
        if (!flight_trajectory_add_point(trajectory, &point)) {
            error_set(ERROR_MEMORY, "无法添加轨迹点", __func__, __FILE__, __LINE__);
            return 0;
        }
    }
    
    /* 计算总距离 */
    trajectory->total_distance = cruise_speed * duration;
    
    return 1;
}

int flight_trajectory_generate_landing(FlightTrajectory* trajectory,
                                       const AircraftState* start_state,
                                       const AircraftState* end_state) {
    if (trajectory == NULL || start_state == NULL || end_state == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证开始状态和结束状态 */
    if (!aircraft_state_validate(start_state) || !aircraft_state_validate(end_state)) {
        error_set(ERROR_PARAMETER, "开始状态或结束状态无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证高度逻辑 */
    if (start_state->position.altitude <= end_state->position.altitude) {
        error_set(ERROR_PARAMETER, "开始高度必须高于结束高度", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 清空现有轨迹 */
    flight_trajectory_clear(trajectory);
    
    /* 降落参数 */
    const double landing_duration = 300.0;  /* 5分钟降落时间 */
    const double approach_speed = 150.0;    /* 进近速度 150 m/s */
    const double touchdown_speed = 60.0;    /* 着陆速度 60 m/s */
    const int point_interval = 5;           /* 5秒间隔 */
    
    int num_points = (int)(landing_duration / point_interval) + 1;
    
    /* 生成降落轨迹点 */
    for (int i = 0; i < num_points; i++) {
        TrajectoryPoint point;
        double t = (double)i / (double)(num_points - 1);
        
        /* 计算时间戳 */
        point.timestamp = start_state->timestamp + (time_t)(i * point_interval);
        
        /* 计算位置（使用S型曲线进行高度变化） */
        double altitude_progress = 0.5 * (1 - cos(M_PI * t));
        point.state.position.altitude = start_state->position.altitude - 
                                       (start_state->position.altitude - end_state->position.altitude) * altitude_progress;
        
        /* 计算水平位置（直线插值） */
        point.state.position.latitude = interpolate_linear(
            0.0, start_state->position.latitude,
            1.0, end_state->position.latitude,
            t
        );
        
        point.state.position.longitude = interpolate_linear(
            0.0, start_state->position.longitude,
            1.0, end_state->position.longitude,
            t
        );
        
        /* 计算速度（减速过程） */
        point.state.velocity.velocity = approach_speed - (approach_speed - touchdown_speed) * t;
        
        /* 计算垂直速度（下降过程） */
        point.state.velocity.vertical_speed = -(start_state->position.altitude - end_state->position.altitude) / landing_duration * cos(M_PI * t);
        
        /* 计算航向（保持对准跑道） */
        double bearing = aircraft_state_bearing(start_state, end_state);
        point.state.velocity.heading = bearing;
        
        /* 计算姿态角 */
        if (t < 0.8) {
            /* 进近阶段 */
            point.state.attitude.pitch = -5.0 * (1 - t); /* 轻微俯冲 */
        } else {
            /* 拉平阶段 */
            point.state.attitude.pitch = 5.0 * (t - 0.8) / 0.2; /* 拉平姿态 */
        }
        
        point.state.attitude.roll = 0.0;
        point.state.attitude.yaw = bearing;
        
        point.state.is_valid = 1;
        
        /* 添加轨迹点 */
        if (!flight_trajectory_add_point(trajectory, &point)) {
            error_set(ERROR_MEMORY, "无法添加轨迹点", __func__, __FILE__, __LINE__);
            return 0;
        }
    }
    
    /* 计算总距离 */
    trajectory->total_distance = 0.0;
    for (int i = 1; i < trajectory->point_count; i++) {
        double distance = aircraft_state_distance(
            &trajectory->points[i-1].state, 
            &trajectory->points[i].state
        );
        trajectory->total_distance += distance;
    }
    
    return 1;
}

int flight_trajectory_generate_maneuver(FlightTrajectory* trajectory,
                                       const AircraftState* start_state,
                                       double duration, double max_roll) {
    if (trajectory == NULL || start_state == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证开始状态 */
    if (!aircraft_state_validate(start_state)) {
        error_set(ERROR_PARAMETER, "开始状态无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证参数 */
    if (duration <= 0.0) {
        error_set(ERROR_PARAMETER, "持续时间必须大于0", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    if (max_roll < 0.0 || max_roll > 60.0) {
        error_set(ERROR_PARAMETER, "最大横滚角必须在0到60度之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 清空现有轨迹 */
    flight_trajectory_clear(trajectory);
    
    /* 机动参数 */
    const double maneuver_speed = 200.0;   /* 机动速度 200 m/s */
    const int point_interval = 2;           /* 2秒间隔 */
    const double turn_radius = maneuver_speed / (degrees_to_radians(max_roll) * 9.81);
    
    int num_points = (int)(duration / point_interval) + 1;
    
    /* 生成机动轨迹点（执行8字机动） */
    for (int i = 0; i < num_points; i++) {
        TrajectoryPoint point;
        double t = (double)i / (double)(num_points - 1);
        
        /* 计算时间戳 */
        point.timestamp = start_state->timestamp + (time_t)(i * point_interval);
        
        /* 计算机动角度（8字轨迹） */
        double angle = 2 * M_PI * t; /* 完整的8字需要2π */
        
        /* 8字轨迹参数方程 */
        double a = turn_radius * 2;  /* 长轴 */
        double b = turn_radius;      /* 短轴 */
        
        /* 相对于起始位置的偏移 */
        double dx = a * cos(angle) / (1 + sin(angle) * sin(angle));
        double dy = b * sin(angle) * cos(angle) / (1 + sin(angle) * sin(angle));
        
        /* 转换为地理坐标 */
        double lat1 = degrees_to_radians(start_state->position.latitude);
        double lon1 = degrees_to_radians(start_state->position.longitude);
        double bearing = degrees_to_radians(start_state->velocity.heading);
        
        /* 计算新位置 */
        double angular_distance = sqrt(dx * dx + dy * dy) / 6371000.0;
        double angle_to_point = atan2(dy, dx);
        
        double lat2 = asin(sin(lat1) * cos(angular_distance) + 
                          cos(lat1) * sin(angular_distance) * cos(bearing + angle_to_point));
        double lon2 = lon1 + atan2(sin(bearing + angle_to_point) * sin(angular_distance) * cos(lat1),
                                  cos(angular_distance) - sin(lat1) * sin(lat2));
        
        point.state.position.latitude = radians_to_degrees(lat2);
        point.state.position.longitude = radians_to_degrees(lon2);
        point.state.position.altitude = start_state->position.altitude;
        
        /* 计算速度 */
        point.state.velocity.velocity = maneuver_speed;
        point.state.velocity.vertical_speed = 0.0;
        
        /* 计算航向（切线方向） */
        double heading = atan2(dy, dx) + M_PI / 2;
        point.state.velocity.heading = normalize_angle(radians_to_degrees(heading + bearing));
        
        /* 计算姿态角（根据转弯方向） */
        double roll_angle = max_roll * sin(2 * angle); /* 8字机动中的横滚角变化 */
        point.state.attitude.roll = roll_angle;
        
        /* 俯仰角根据横滚角调整（协调转弯） */
        point.state.attitude.pitch = 5.0 * cos(2 * angle); /* 轻微俯仰变化 */
        point.state.attitude.yaw = point.state.velocity.heading;
        
        point.state.is_valid = 1;
        
        /* 添加轨迹点 */
        if (!flight_trajectory_add_point(trajectory, &point)) {
            error_set(ERROR_MEMORY, "无法添加轨迹点", __func__, __FILE__, __LINE__);
            return 0;
        }
    }
    
    /* 计算总距离 */
    trajectory->total_distance = 0.0;
    for (int i = 1; i < trajectory->point_count; i++) {
        double distance = aircraft_state_distance(
            &trajectory->points[i-1].state, 
            &trajectory->points[i].state
        );
        trajectory->total_distance += distance;
    }
    
    return 1;
}

/* CSV文件加载和保存功能实现在csv_parser.c中 */

/**
 * @brief 线性插值角度（处理环绕）
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
 * @brief 飞机状态插值
 * 
 * 在两个飞机状态之间进行线性插值
 * 
 * @param state1 起始状态
 * @param state2 结束状态
 * @param target_time 目标时间
 * @param result 插值结果
 * @return int 成功返回1，失败返回0
 */
int aircraft_state_interpolate(const AircraftState* state1, const AircraftState* state2,
                              time_t target_time, AircraftState* result) {
    if (state1 == NULL || state2 == NULL || result == NULL) {
        error_set(ERROR_PARAMETER, "参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 检查时间戳是否有效 */
    if (state1->timestamp >= state2->timestamp) {
        error_set(ERROR_PARAMETER, "state1的时间戳必须小于state2的时间戳", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    if (target_time < state1->timestamp || target_time > state2->timestamp) {
        error_set(ERROR_PARAMETER, "目标时间必须在两个状态时间戳之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 计算插值参数 */
    double total_time = (double)(state2->timestamp - state1->timestamp);
    double elapsed_time = (double)(target_time - state1->timestamp);
    double t = elapsed_time / total_time;
    
    /* 插值位置 */
    result->position.latitude = interpolate_linear(
        (double)state1->timestamp, state1->position.latitude,
        (double)state2->timestamp, state2->position.latitude,
        (double)target_time
    );
    
    result->position.longitude = interpolate_linear(
        (double)state1->timestamp, state1->position.longitude,
        (double)state2->timestamp, state2->position.longitude,
        (double)target_time
    );
    
    result->position.altitude = interpolate_linear(
        (double)state1->timestamp, state1->position.altitude,
        (double)state2->timestamp, state2->position.altitude,
        (double)target_time
    );
    
    /* 插值速度 */
    result->velocity.velocity = interpolate_linear(
        (double)state1->timestamp, state1->velocity.velocity,
        (double)state2->timestamp, state2->velocity.velocity,
        (double)target_time
    );
    
    result->velocity.vertical_speed = interpolate_linear(
        (double)state1->timestamp, state1->velocity.vertical_speed,
        (double)state2->timestamp, state2->velocity.vertical_speed,
        (double)target_time
    );
    
    /* 插值航向角（需要处理角度环绕） */
    result->velocity.heading = interpolate_angle(
        state1->velocity.heading, state2->velocity.heading, t
    );
    
    /* 插值姿态角 */
    result->attitude.pitch = interpolate_angle(
        state1->attitude.pitch, state2->attitude.pitch, t
    );
    
    result->attitude.roll = interpolate_angle(
        state1->attitude.roll, state2->attitude.roll, t
    );
    
    result->attitude.yaw = interpolate_angle(
        state1->attitude.yaw, state2->attitude.yaw, t
    );
    
    /* 设置时间戳和有效性 */
    result->timestamp = target_time;
    result->is_valid = state1->is_valid && state2->is_valid;
    
    return 1;
}

int aircraft_state_validate(const AircraftState* state) {
    if (state == NULL) {
        error_set(ERROR_PARAMETER, "飞机状态不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证位置信息 */
    if (!validate_latitude(state->position.latitude)) {
        error_set(ERROR_PARAMETER, "纬度无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    if (!validate_longitude(state->position.longitude)) {
        error_set(ERROR_PARAMETER, "经度无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    if (!validate_altitude(state->position.altitude)) {
        error_set(ERROR_PARAMETER, "高度无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证姿态信息 */
    if (!validate_attitude(state->attitude.pitch, state->attitude.roll, state->attitude.yaw)) {
        error_set(ERROR_PARAMETER, "姿态角无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证速度信息 */
    if (!validate_velocity(state->velocity.velocity)) {
        error_set(ERROR_PARAMETER, "速度无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证垂直速度 */
    if (state->velocity.vertical_speed < -200.0 || state->velocity.vertical_speed > 200.0) {
        error_set(ERROR_PARAMETER, "垂直速度超出范围", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证航向角 */
    if (state->velocity.heading < -180.0 || state->velocity.heading > 180.0) {
        error_set(ERROR_PARAMETER, "航向角超出范围", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证时间戳 */
    if (!validate_timestamp(state->timestamp)) {
        error_set(ERROR_PARAMETER, "时间戳无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证有效性标志 */
    if (state->is_valid != 0 && state->is_valid != 1) {
        error_set(ERROR_PARAMETER, "有效性标志必须是0或1", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    return 1;
}

/* CSV轨迹解析功能实现在csv_parser.c中 */
/* CSV示例文件写入功能实现在csv_parser.c中 */

double aircraft_state_distance(const AircraftState* state1, const AircraftState* state2) {
    if (state1 == NULL || state2 == NULL) {
        error_set(ERROR_PARAMETER, "飞机状态不能为NULL", __func__, __FILE__, __LINE__);
        return 0.0;
    }
    
    /* 使用Haversine公式计算水平距离 */
    double horizontal_distance = distance_haversine(
        state1->position.latitude, state1->position.longitude,
        state2->position.latitude, state2->position.longitude
    );
    
    /* 计算高度差 */
    double altitude_diff = state2->position.altitude - state1->position.altitude;
    
    /* 计算三维距离 */
    double distance_3d = sqrt(horizontal_distance * horizontal_distance + altitude_diff * altitude_diff);
    
    return distance_3d;
}

double aircraft_state_bearing(const AircraftState* state1, const AircraftState* state2) {
    if (state1 == NULL || state2 == NULL) {
        error_set(ERROR_PARAMETER, "飞机状态不能为NULL", __func__, __FILE__, __LINE__);
        return 0.0;
    }
    
    /* 使用bearing_calculate函数计算方位角 */
    double bearing = bearing_calculate(
        state1->position.latitude, state1->position.longitude,
        state2->position.latitude, state2->position.longitude
    );
    
    return bearing;
}

const char* trajectory_type_to_string(TrajectoryType type) {
    switch (type) {
        case TRAJECTORY_TYPE_TAKEOFF: return "TAKEOFF";
        case TRAJECTORY_TYPE_CRUISE: return "CRUISE";
        case TRAJECTORY_TYPE_LANDING: return "LANDING";
        case TRAJECTORY_TYPE_MANEUVER: return "MANEUVER";
        case TRAJECTORY_TYPE_CUSTOM: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

int trajectory_params_validate(const TrajectoryParams* params) {
    if (params == NULL) {
        error_set(ERROR_PARAMETER, "轨迹参数不能为NULL", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证轨迹类型 */
    if (params->type < TRAJECTORY_TYPE_TAKEOFF || params->type > TRAJECTORY_TYPE_CUSTOM) {
        error_set(ERROR_PARAMETER, "无效的轨迹类型", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证开始状态 */
    if (!aircraft_state_validate(&params->start_state)) {
        error_set(ERROR_PARAMETER, "开始状态无效", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证结束状态（如果需要） */
    if (params->type == TRAJECTORY_TYPE_LANDING || params->type == TRAJECTORY_TYPE_CUSTOM) {
        if (!aircraft_state_validate(&params->end_state)) {
            error_set(ERROR_PARAMETER, "结束状态无效", __func__, __FILE__, __LINE__);
            return 0;
        }
    }
    
    /* 验证持续时间 */
    if (params->duration <= 0.0) {
        error_set(ERROR_PARAMETER, "持续时间必须大于0", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证点间隔 */
    if (params->point_interval <= 0) {
        error_set(ERROR_PARAMETER, "点间隔必须大于0", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证最大俯仰角 */
    if (params->max_pitch < -90.0 || params->max_pitch > 90.0) {
        error_set(ERROR_PARAMETER, "最大俯仰角必须在-90到90度之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证最大横滚角 */
    if (params->max_roll < -180.0 || params->max_roll > 180.0) {
        error_set(ERROR_PARAMETER, "最大横滚角必须在-180到180度之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证最大转弯率 */
    if (params->max_turn_rate < 0.0 || params->max_turn_rate > 30.0) {
        error_set(ERROR_PARAMETER, "最大转弯率必须在0到30度/秒之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证最大爬升率 */
    if (params->max_climb_rate < -100.0 || params->max_climb_rate > 100.0) {
        error_set(ERROR_PARAMETER, "最大爬升率必须在-100到100米/秒之间", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证轨迹类型特定参数 */
    switch (params->type) {
        case TRAJECTORY_TYPE_TAKEOFF:
            /* 起飞时开始高度应该接近地面 */
            if (params->start_state.position.altitude < -100.0 || params->start_state.position.altitude > 1000.0) {
                error_set(ERROR_PARAMETER, "起飞开始高度不合理", __func__, __FILE__, __LINE__);
                return 0;
            }
            /* 目标高度应该高于开始高度 */
            if (params->end_state.position.altitude <= params->start_state.position.altitude) {
                error_set(ERROR_PARAMETER, "起飞目标高度必须高于开始高度", __func__, __FILE__, __LINE__);
                return 0;
            }
            break;
            
        case TRAJECTORY_TYPE_LANDING:
            /* 降落时结束高度应该接近地面 */
            if (params->end_state.position.altitude < -100.0 || params->end_state.position.altitude > 1000.0) {
                error_set(ERROR_PARAMETER, "降落结束高度不合理", __func__, __FILE__, __LINE__);
                return 0;
            }
            /* 开始高度应该高于结束高度 */
            if (params->start_state.position.altitude <= params->end_state.position.altitude) {
                error_set(ERROR_PARAMETER, "降落开始高度必须高于结束高度", __func__, __FILE__, __LINE__);
                return 0;
            }
            break;
            
        case TRAJECTORY_TYPE_CRUISE:
            /* 巡航时高度应该合理 */
            if (params->start_state.position.altitude < 1000.0 || params->start_state.position.altitude > 20000.0) {
                error_set(ERROR_PARAMETER, "巡航高度不合理", __func__, __FILE__, __LINE__);
                return 0;
            }
            /* 巡航速度应该合理 */
            if (params->start_state.velocity.velocity < 50.0 || params->start_state.velocity.velocity > 500.0) {
                error_set(ERROR_PARAMETER, "巡航速度不合理", __func__, __FILE__, __LINE__);
                return 0;
            }
            break;
            
        case TRAJECTORY_TYPE_MANEUVER:
            /* 机动时高度应该合理 */
            if (params->start_state.position.altitude < 1000.0 || params->start_state.position.altitude > 15000.0) {
                error_set(ERROR_PARAMETER, "机动高度不合理", __func__, __FILE__, __LINE__);
                return 0;
            }
            break;
            
        default:
            break;
    }
    
    return 1;
}
