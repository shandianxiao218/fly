#include "obstruction.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    Vector3D satellite_vector = vector3d_create(satellite_pos->x, satellite_pos->y, satellite_pos->z);
    Vector3D direction = vector3d_subtract(&satellite_vector, antenna_pos);
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
    Vector3D satellite_vector = vector3d_create(satellite_pos->x, satellite_pos->y, satellite_pos->z);
    Vector3D to_satellite = vector3d_subtract(&satellite_vector, antenna_pos);
    
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

/**
 * @brief 创建飞机几何模型
 * @param model_type 飞机模型类型
 * @return 创建的几何模型
 */
AircraftGeometry* aircraft_geometry_create(AircraftModelType model_type) {
    AircraftGeometry* geometry = (AircraftGeometry*)malloc(sizeof(AircraftGeometry));
    if (!geometry) {
        return NULL;
    }
    
    memset(geometry, 0, sizeof(AircraftGeometry));
    geometry->model_type = model_type;
    geometry->component_count = 0;
    geometry->components = NULL;
    geometry->scale_factor = 1.0;
    
    // 设置默认天线位置（在飞机顶部）
    geometry->antenna_position = vector3d_create(0.0, 0.0, 2.0);
    
    return geometry;
}

/**
 * @brief 销毁飞机几何模型
 * @param geometry 几何模型
 */
void aircraft_geometry_destroy(AircraftGeometry* geometry) {
    if (!geometry) {
        return;
    }
    
    if (geometry->components) {
        free(geometry->components);
    }
    
    free(geometry);
}

/**
 * @brief 添加飞机部件
 * @param geometry 几何模型
 * @param component 部件
 * @return 成功返回1，失败返回0
 */
int aircraft_geometry_add_component(AircraftGeometry* geometry, 
                                   const AircraftComponent* component) {
    if (!geometry || !component) {
        return 0;
    }
    
    // 重新分配内存
    AircraftComponent* new_components = (AircraftComponent*)realloc(
        geometry->components, 
        sizeof(AircraftComponent) * (geometry->component_count + 1)
    );
    
    if (!new_components) {
        return 0;
    }
    
    geometry->components = new_components;
    geometry->components[geometry->component_count] = *component;
    geometry->component_count++;
    
    return 1;
}

/**
 * @brief 设置天线位置
 * @param geometry 几何模型
 * @param position 天线位置
 * @return 成功返回1，失败返回0
 */
int aircraft_geometry_set_antenna_position(AircraftGeometry* geometry, 
                                         const Vector3D* position) {
    if (!geometry || !position) {
        return 0;
    }
    
    geometry->antenna_position = *position;
    return 1;
}

/**
 * @brief 更新几何变换
 * @param geometry 几何模型
 * @param attitude 飞机姿态
 * @return 成功返回1，失败返回0
 */
int aircraft_geometry_update_transform(AircraftGeometry* geometry, 
                                     const AircraftAttitude* attitude) {
    if (!geometry || !attitude) {
        return 0;
    }
    
    // 为每个部件应用姿态变换
    for (int i = 0; i < geometry->component_count; i++) {
        AircraftComponent* component = &geometry->components[i];
        
        // 更新部件的旋转角度
        component->rotation.x = attitude->pitch;
        component->rotation.y = attitude->roll;
        component->rotation.z = attitude->yaw;
    }
    
    return 1;
}

/**
 * @brief 可见性分析
 * @param geometry 飞机几何模型
 * @param satellite 卫星数据
 * @param aircraft_state 飞机状态
 * @param params 计算参数
 * @param analysis 分析结果
 * @return 成功返回1，失败返回0
 */
int visibility_analyze(const AircraftGeometry* geometry,
                      const Satellite* satellite,
                      const AircraftState* aircraft_state,
                      const ObstructionParams* params,
                      VisibilityAnalysis* analysis) {
    if (!geometry || !satellite || !aircraft_state || !params || !analysis) {
        return 0;
    }
    
    // 首先计算卫星可见性
    SatelliteVisibility visibility;
    int result = satellite_visibility_calculate(satellite, 
                                               aircraft_state->position.latitude,
                                               aircraft_state->position.longitude,
                                               aircraft_state->position.altitude,
                                               &visibility);
    
    if (!result) {
        return 0;
    }
    
    // 如果卫星本身不可见，直接返回
    if (!visibility.is_visible) {
        memset(analysis, 0, sizeof(VisibilityAnalysis));
        analysis->visibility = visibility;
        analysis->is_usable = 0;
        return 1;
    }
    
    // 创建卫星位置结构
    SatellitePosition sat_pos = satellite->pos;
    
    // 计算遮挡
    ObstructionResult obstruction;
    result = obstruction_calculate(geometry, &sat_pos, aircraft_state, params, &obstruction);
    
    if (!result) {
        return 0;
    }
    
    // 填充分析结果
    analysis->visibility = visibility;
    analysis->obstruction = obstruction;
    
    // 计算有效高度角和方位角
    if (obstruction.is_obstructed) {
        analysis->effective_elevation = visibility.elevation - obstruction.obstruction_angle;
        analysis->effective_azimuth = visibility.azimuth;
        analysis->is_usable = (analysis->effective_elevation > params->min_obstruction_angle);
    } else {
        analysis->effective_elevation = visibility.elevation;
        analysis->effective_azimuth = visibility.azimuth;
        analysis->is_usable = 1;
    }
    
    return 1;
}

/**
 * @brief 批量遮挡计算
 * @param geometry 飞机几何模型
 * @param satellite_data 卫星数据
 * @param aircraft_state 飞机状态
 * @param params 计算参数
 * @param result 计算结果
 * @return 成功返回1，失败返回0
 */
int batch_obstruction_calculate(const AircraftGeometry* geometry,
                               const SatelliteData* satellite_data,
                               const AircraftState* aircraft_state,
                               const ObstructionParams* params,
                               BatchObstructionResult* result) {
    if (!geometry || !satellite_data || !aircraft_state || !params || !result) {
        return 0;
    }
    
    // 初始化结果
    memset(result, 0, sizeof(BatchObstructionResult));
    result->calculation_time = time(NULL);
    result->visible_satellites = 0;
    result->obstructed_satellites = 0;
    result->usable_satellites = 0;
    
    // 分配分析结果数组
    result->analyses = (VisibilityAnalysis*)malloc(
        sizeof(VisibilityAnalysis) * satellite_data->satellite_count
    );
    
    if (!result->analyses) {
        return 0;
    }
    
    clock_t start_time = clock();
    
    // 对每颗卫星进行分析
    for (int i = 0; i < satellite_data->satellite_count; i++) {
        const Satellite* satellite = &satellite_data->satellites[i];
        
        if (!satellite->is_valid) {
            continue;
        }
        
        VisibilityAnalysis analysis;
        int analysis_result = visibility_analyze(geometry, satellite, aircraft_state, params, &analysis);
        
        if (analysis_result) {
            result->analyses[result->analysis_count] = analysis;
            result->analysis_count++;
            
            // 统计结果
            if (analysis.visibility.is_visible) {
                result->visible_satellites++;
            }
            
            if (analysis.obstruction.is_obstructed) {
                result->obstructed_satellites++;
            }
            
            if (analysis.is_usable) {
                result->usable_satellites++;
            }
        }
    }
    
    clock_t end_time = clock();
    result->total_calculation_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    return 1;
}

/**
 * @brief 初始化遮挡计算参数
 * @param params 参数结构
 * @return 成功返回1，失败返回0
 */
int obstruction_params_init(ObstructionParams* params) {
    if (!params) {
        return 0;
    }
    
    memset(params, 0, sizeof(ObstructionParams));
    params->precision = 1.0;           // 1度精度
    params->max_iterations = 100;      // 最大迭代次数
    params->min_obstruction_angle = 5.0; // 最小遮挡角度
    params->signal_threshold = -150.0;   // 信号阈值
    params->consider_multipath = 0;     // 不考虑多径效应
    params->consider_diffraction = 0;   // 不考虑衍射效应
    
    return 1;
}

/**
 * @brief 验证遮挡计算参数
 * @param params 参数结构
 * @return 有效返回1，无效返回0
 */
int obstruction_params_validate(const ObstructionParams* params) {
    if (!params) {
        return 0;
    }
    
    if (params->precision <= 0.0 || params->precision > 10.0) {
        return 0;
    }
    
    if (params->max_iterations <= 0 || params->max_iterations > 1000) {
        return 0;
    }
    
    if (params->min_obstruction_angle < 0.0 || params->min_obstruction_angle > 90.0) {
        return 0;
    }
    
    if (params->signal_threshold < -200.0 || params->signal_threshold > -100.0) {
        return 0;
    }
    
    return 1;
}

/**
 * @brief 飞机模型类型转字符串
 * @param type 模型类型
 * @return 字符串描述
 */
const char* aircraft_model_type_to_string(AircraftModelType type) {
    switch (type) {
        case AIRCRAFT_MODEL_COMMERCIAL:
            return "商用飞机";
        case AIRCRAFT_MODEL_MILITARY:
            return "军用飞机";
        case AIRCRAFT_MODEL_GENERAL:
            return "通用飞机";
        case AIRCRAFT_MODEL_DRONE:
            return "无人机";
        default:
            return "未知类型";
    }
}

/**
 * @brief 飞机部件转字符串
 * @param part 部件类型
 * @return 字符串描述
 */
const char* aircraft_part_to_string(AircraftPart part) {
    switch (part) {
        case AIRCRAFT_PART_FUSELAGE:
            return "机身";
        case AIRCRAFT_PART_WING_LEFT:
            return "左翼";
        case AIRCRAFT_PART_WING_RIGHT:
            return "右翼";
        case AIRCRAFT_PART_TAIL:
            return "尾翼";
        case AIRCRAFT_PART_ENGINE:
            return "发动机";
        default:
            return "未知部件";
    }
}
