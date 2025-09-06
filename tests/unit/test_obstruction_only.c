#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../lib/CuTest.h"
#include "../src/obstruction/obstruction.h"
#include "../src/satellite/satellite.h"
#include "../src/aircraft/aircraft.h"

void TestObstructionBasic(CuTest* tc) {
    printf("=== Obstruction模块基础测试 ===\n");
    
    // 创建飞机几何模型
    AircraftGeometry* geometry = aircraft_geometry_create(AIRCRAFT_MODEL_COMMERCIAL);
    CuAssertPtrNotNull(tc, geometry);
    
    // 添加机身部件
    AircraftComponent fuselage = {0};
    fuselage.part_type = AIRCRAFT_PART_FUSELAGE;
    fuselage.position = vector3d_create(0.0, 0.0, 0.0);
    fuselage.size = vector3d_create(10.0, 3.0, 3.0);
    fuselage.is_obstructing = 1;
    
    int result = aircraft_geometry_add_component(geometry, &fuselage);
    CuAssertIntEquals(tc, 1, result);
    
    // 测试向量运算
    Vector3D v1 = vector3d_create(1.0, 2.0, 3.0);
    Vector3D v2 = vector3d_create(4.0, 5.0, 6.0);
    Vector3D sum = vector3d_add(&v1, &v2);
    
    CuAssertDblEquals(tc, 5.0, sum.x, 0.001);
    CuAssertDblEquals(tc, 7.0, sum.y, 0.001);
    CuAssertDblEquals(tc, 9.0, sum.z, 0.001);
    
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
    result = ray_box_intersection(&ray, &box, &intersection, &distance);
    CuAssertIntEquals(tc, 1, result);
    CuAssertTrue(tc, distance > 0);
    
    // 测试遮挡计算参数
    ObstructionParams params;
    result = obstruction_params_init(&params);
    CuAssertIntEquals(tc, 1, result);
    CuAssertDblEquals(tc, 1.0, params.precision, 0.001);
    CuAssertDblEquals(tc, 5.0, params.min_obstruction_angle, 0.001);
    
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
    ObstructionResult obstruction_result = {0};
    result = obstruction_calculate(geometry, &sat_pos, &aircraft_state, &params, &obstruction_result);
    CuAssertIntEquals(tc, 1, result);
    CuAssertTrue(tc, obstruction_result.is_obstructed >= 0 && obstruction_result.is_obstructed <= 1);
    
    // 清理
    aircraft_geometry_destroy(geometry);
    
    printf("=== Obstruction模块基础测试完成 ===\n");
}

CuSuite* CuGetSuite() {
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestObstructionBasic);
    return suite;
}

int main() {
    CuString* output = CuStringNew();
    CuSuite* suite = CuGetSuite();
    
    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    
    printf("%s\n", output->buffer);
    
    int failed = suite->failCount;
    
    CuStringDelete(output);
    CuSuiteDelete(suite);
    
    return failed;
}