#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "satellite/satellite.h"
#include "utils/utils.h"

int main(int argc, char* argv[]) {
    printf("北斗导航卫星可见性分析系统\n");
    printf("Beidou Navigation Satellite Visibility Analysis System\n");
    printf("====================================================\n\n");
    
    /* 创建卫星数据管理器 */
    SatelliteData* satellite_data = satellite_data_create(32);
    if (satellite_data == NULL) {
        printf("错误：无法创建卫星数据管理器\n");
        return 1;
    }
    
    printf("卫星数据管理器创建成功，最大容量：%d颗卫星\n", satellite_data->max_satellites);
    
    /* 添加测试卫星 */
    Satellite test_sat = {0};
    test_sat.prn = 1;
    test_sat.system = SATELLITE_SYSTEM_BEIDOU;
    test_sat.is_valid = 1;
    
    /* 设置轨道参数 */
    test_sat.orbit.sqrt_a = 5153.8;
    test_sat.orbit.e = 0.01;
    test_sat.orbit.i0 = 0.9;
    test_sat.orbit.omega0 = 1.0;
    test_sat.orbit.omega = 2.0;
    test_sat.orbit.m0 = 0.5;
    test_sat.orbit.toe = 1000.0;
    test_sat.orbit.delta_n = 0.0;
    test_sat.orbit.i_dot = 0.0;
    test_sat.orbit.omega_dot = 0.0;
    
    /* 设置调和改正项 */
    test_sat.orbit.cuc = 0.0;
    test_sat.orbit.cus = 0.0;
    test_sat.orbit.crc = 0.0;
    test_sat.orbit.crs = 0.0;
    test_sat.orbit.cic = 0.0;
    test_sat.orbit.cis = 0.0;
    
    /* 设置时钟参数 */
    test_sat.clock.t_oc = time(NULL);
    test_sat.clock.a0 = 0.0;
    test_sat.clock.a1 = 0.0;
    test_sat.clock.a2 = 0.0;
    
    /* 添加卫星 */
    if (satellite_data_add(satellite_data, &test_sat)) {
        printf("成功添加测试卫星 PRN %d\n", test_sat.prn);
    } else {
        printf("添加卫星失败\n");
    }
    
    /* 计算卫星位置 */
    time_t current_time = time(NULL);
    if (satellite_position_calculate(&test_sat, current_time)) {
        printf("卫星位置计算成功：\n");
        printf("  X: %.2f m\n", test_sat.pos.x);
        printf("  Y: %.2f m\n", test_sat.pos.y);
        printf("  Z: %.2f m\n", test_sat.pos.z);
    } else {
        printf("卫星位置计算失败\n");
    }
    
    /* 计算卫星可见性 */
    SatelliteVisibility visibility = {0};
    double lat = 39.9042;  // 北京纬度
    double lon = 116.4074; // 北京经度
    double alt = 50.0;    // 海拔高度
    
    if (satellite_visibility_calculate(&test_sat, lat, lon, alt, &visibility)) {
        printf("卫星可见性分析：\n");
        printf("  高度角：%.2f°\n", visibility.elevation);
        printf("  方位角：%.2f°\n", visibility.azimuth);
        printf("  距离：%.2f km\n", visibility.distance / 1000.0);
        printf("  可见性：%s\n", visibility.is_visible ? "可见" : "不可见");
        printf("  信号强度：%.2f dBm\n", visibility.signal_strength);
    } else {
        printf("卫星可见性计算失败\n");
    }
    
    /* 清理资源 */
    satellite_data_destroy(satellite_data);
    
    printf("\n系统测试完成！\n");
    return 0;
}