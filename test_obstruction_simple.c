#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../src/obstruction/obstruction.h"
#include "../src/satellite/satellite.h"
#include "../src/aircraft/aircraft.h"

int main() {
    printf("=== Obstruction模块测试 ===\n");
    
    // 创建飞机几何模型
    AircraftGeometry* geometry = aircraft_geometry_create(AIRCRAFT_MODEL_COMMERCIAL);
    if (!geometry) {
        printf("飞机几何模型创建失败\n");
        return 1;
    }
    printf("飞机几何模型创建成功\n");
    
    // 添加机身部件
    AircraftComponent fuselage = {0};
    fuselage.part_type = AIRCRAFT_PART_FUSELAGE;
    fuselage.position = vector3d_create(0.0, 0.0, 0.0);
    fuselage.size = vector3d_create(30.0, 3.0, 3.0);
    fuselage.is_obstructing = 1;
    
    if (aircraft_geometry_add_component(geometry, &fuselage)) {
        printf("机身部件添加成功\n");
    } else {
        printf("机身部件添加失败\n");
    }
    
    // 添加机翼部件
    AircraftComponent wing = {0};
    wing.part_type = AIRCRAFT_PART_WING_LEFT;
    wing.position = vector3d_create(0.0, -15.0, 0.0);
    wing.size = vector3d_create(5.0, 25.0, 1.0);
    wing.is_obstructing = 1;
    
    if (aircraft_geometry_add_component(geometry, &wing)) {
        printf("机翼部件添加成功\n");
    } else {
        printf("机翼部件添加失败\n");
    }
    
    // 测试向量运算
    Vector3D v1 = vector3d_create(1.0, 2.0, 3.0);
    Vector3D v2 = vector3d_create(4.0, 5.0, 6.0);
    Vector3D sum = vector3d_add(&v1, &v2);
    printf("向量加法测试: (%.1f, %.1f, %.1f) + (%.1f, %.1f, %.1f) = (%.1f, %.1f, %.1f)\n",
           v1.x, v1.y, v1.z, v2.x, v2.y, v2.z, sum.x, sum.y, sum.z);
    
    // 测试射线相交
    Ray ray = {0};
    ray.origin = vector3d_create(-10.0, 0.0, 0.0);
    ray.direction = vector3d_create(1.0, 0.0, 0.0);
    ray.length = 50.0;
    
    ObstructionBody box = {0};
    box.center = vector3d_create(0.0, 0.0, 0.0);
    box.size = vector3d_create(10.0, 5.0, 5.0);
    
    Vector3D intersection;
    double distance;
    if (ray_box_intersection(&ray, &box, &intersection, &distance)) {
        printf("射线相交测试成功: 交点(%.1f, %.1f, %.1f), 距离=%.1f\n",
               intersection.x, intersection.y, intersection.z, distance);
    } else {
        printf("射线相交测试失败\n");
    }
    
    // 测试旋转矩阵
    RotationMatrix matrix = rotation_matrix_create_from_euler(90.0, 0.0, 0.0);
    Vector3D rotated = vector3d_rotate(&v1, &matrix);
    printf("旋转矩阵测试: (%.1f, %.1f, %.1f) 旋转90度后 = (%.1f, %.1f, %.1f)\n",
           v1.x, v1.y, v1.z, rotated.x, rotated.y, rotated.z);
    
    // 测试遮挡计算参数
    ObstructionParams params;
    if (obstruction_params_init(&params)) {
        printf("遮挡计算参数初始化成功\n");
        printf("精度: %.1f度, 最小遮挡角度: %.1f度\n", params.precision, params.min_obstruction_angle);
    } else {
        printf("遮挡计算参数初始化失败\n");
    }
    
    // 测试卫星和飞机状态
    SatellitePosition sat_pos = {0};
    sat_pos.x = 1000000.0;
    sat_pos.y = 2000000.0;
    sat_pos.z = 3000000.0;
    
    AircraftState aircraft_state = {0};
    aircraft_state.position.latitude = 39.9;
    aircraft_state.position.longitude = 116.4;
    aircraft_state.position.altitude = 1000.0;
    aircraft_state.is_valid = 1;
    
    // 测试遮挡计算
    ObstructionResult result = {0};
    if (obstruction_calculate(geometry, &sat_pos, &aircraft_state, &params, &result)) {
        printf("遮挡计算成功: %s\n", result.is_obstructed ? "被遮挡" : "未被遮挡");
        if (result.is_obstructed) {
            printf("  遮挡角度: %.1f度\n", result.obstruction_angle);
            printf("  遮挡距离: %.1f米\n", result.obstruction_distance);
            printf("  信号损失: %.1fdB\n", result.signal_loss);
            printf("  遮挡部件: %s\n", aircraft_part_to_string(result.obstruction_part));
        }
    } else {
        printf("遮挡计算失败\n");
    }
    
    // 清理
    aircraft_geometry_destroy(geometry);
    
    printf("=== Obstruction模块测试完成 ===\n");
    return 0;
}