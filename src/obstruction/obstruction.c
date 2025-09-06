#include "obstruction.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief 计算卫星到天线的射线
 * @param satellite_pos 卫星位置
 * @param antenna_pos 天线位置
 * @return 射线结构
 */
static Ray create_satellite_ray(const SatellitePosition* satellite_pos, const Vector3D* antenna_pos) {
    Ray ray;
    ray.origin = *antenna_pos;
    
    // 计算方向向量
    Vector3D direction = vector3d_subtract(&satellite_pos->position, antenna_pos);
    ray.direction = vector3d_normalize(&direction);
    ray.length = vector3d_length(&direction);
    
    return ray;
}

/**
 * @brief 计算信号衰减
 * @param obstruction_distance 遮挡距离
 * @param part_type 遮挡部件类型
 * @return 信号衰减 (dB)
 */
static double calculate_signal_loss(double obstruction_distance, AircraftPart part_type) {
    // 基础衰减系数
    double base_loss = 0.0;
    
    // 根据部件类型设置不同的衰减系数
    switch (part_type) {
        case AIRCRAFT_PART_FUSELAGE:
            base_loss = 20.0;  // 机身衰减较大
            break;
        case AIRCRAFT_PART_WING_LEFT:
        case AIRCRAFT_PART_WING_RIGHT:
            base_loss = 15.0;  // 机翼衰减中等
            break;
        case AIRCRAFT_PART_TAIL:
            base_loss = 12.0;  // 尾翼衰减较小
            break;
        case AIRCRAFT_PART_ENGINE:
            base_loss = 25.0;  // 发动机衰减最大
            break;
        default:
            base_loss = 10.0;
    }
    
    // 距离衰减 (简化模型)
    double distance_loss = 0.1 * obstruction_distance / 1000.0; // 每公里0.1dB
    
    return base_loss + distance_loss;
}

/**
 * @brief 计算遮挡角度
 * @param intersection 交点
 * @param antenna_pos 天线位置
 * @param satellite_pos 卫星位置
 * @return 遮挡角度 (度)
 */
static double calculate_obstruction_angle(const Vector3D* intersection, 
                                         const Vector3D* antenna_pos,
                                         const SatellitePosition* satellite_pos) {
    // 计算天线到交点的向量
    Vector3D to_intersection = vector3d_subtract(intersection, antenna_pos);
    
    // 计算天线到卫星的向量
    Vector3D to_satellite = vector3d_subtract(&satellite_pos->position, antenna_pos);
    
    // 归一化向量
    Vector3D v1 = vector3d_normalize(&to_intersection);
    Vector3D v2 = vector3d_normalize(&to_satellite);
    
    // 计算点积
    double dot_product = vector3d_dot(&v1, &v2);
    
    // 避免数值误差
    if (dot_product > 1.0) dot_product = 1.0;
    if (dot_product < -1.0) dot_product = -1.0;
    
    // 计算角度
    double angle_rad = acos(dot_product);
    return angle_rad * 180.0 / M_PI;
}

/**
 * @brief 遮挡计算核心函数
 * @param geometry 飞机几何模型
 * @param satellite_pos 卫星位置
 * @param aircraft_state 飞机状态
 * @param params 计算参数
 * @param result 计算结果
 * @return 成功返回1，失败返回0
 */
int obstruction_calculate(const AircraftGeometry* geometry,
                         const SatellitePosition* satellite_pos,
                         const AircraftState* aircraft_state,
                         const ObstructionParams* params,
                         ObstructionResult* result) {
    if (!geometry || !satellite_pos || !aircraft_state || !params || !result) {
        return 0;
    }
    
    // 初始化结果
    memset(result, 0, sizeof(ObstructionResult));
    result->is_obstructed = 0;
    
    // 更新飞机几何模型的位置和姿态
    AircraftGeometry* updated_geometry = aircraft_geometry_create(geometry->model_type);
    if (!updated_geometry) {
        return 0;
    }
    
    // 复制部件信息
    updated_geometry->component_count = geometry->component_count;
    updated_geometry->components = (AircraftComponent*)malloc(
        sizeof(AircraftComponent) * geometry->component_count);
    
    if (!updated_geometry->components) {
        aircraft_geometry_destroy(updated_geometry);
        return 0;
    }
    
    memcpy(updated_geometry->components, geometry->components, 
           sizeof(AircraftComponent) * geometry->component_count);
    
    // 更新几何变换
    if (!aircraft_geometry_update_transform(updated_geometry, &aircraft_state->attitude)) {
        aircraft_geometry_destroy(updated_geometry);
        return 0;
    }
    
    // 计算天线位置
    Vector3D antenna_pos = updated_geometry->antenna_position;
    
    // 创建卫星射线
    Ray satellite_ray = create_satellite_ray(satellite_pos, &antenna_pos);
    
    // 检测与所有部件的相交
    double min_distance = satellite_ray.length;
    int found_obstruction = 0;
    Vector3D closest_intersection;
    AircraftPart closest_part = AIRCRAFT_PART_FUSELAGE;
    
    for (int i = 0; i < updated_geometry->component_count; i++) {
        AircraftComponent* component = &updated_geometry->components[i];
        
        if (!component->is_obstructing) {
            continue; // 跳过不产生遮挡的部件
        }
        
        Vector3D intersection;
        double distance;
        
        if (ray_component_intersection(&satellite_ray, component, &intersection, &distance)) {
            if (distance < min_distance) {
                min_distance = distance;
                closest_intersection = intersection;
                closest_part = component->part_type;
                found_obstruction = 1;
            }
        }
    }
    
    // 填充结果
    if (found_obstruction) {
        result->is_obstructed = 1;
        result->obstruction_distance = min_distance;
        result->intersection_point = closest_intersection;
        result->obstruction_part = closest_part;
        
        // 计算遮挡角度
        result->obstruction_angle = calculate_obstruction_angle(
            &closest_intersection, &antenna_pos, satellite_pos);
        
        // 计算信号衰减
        result->signal_loss = calculate_signal_loss(min_distance, closest_part);
        
        // 检查是否超过最小遮挡角度阈值
        if (result->obstruction_angle < params->min_obstruction_angle) {
            result->is_obstructed = 0;
        }
    }
    
    aircraft_geometry_destroy(updated_geometry);
    return 1;
}
