#include "satellite.h"
#include "../utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* 地球和轨道常数 */
#define EARTH_RADIUS 6378137.0          /* 地球半长轴 (米) */
#define EARTH_MU 398600441800000.0      /* 地球引力常数 (m^3/s^2) */
#define EARTH_OMEGA 7.2921151467e-5     /* 地球自转角速度 (rad/s) */
#define GPS_PI 3.1415926535898          /* 圆周率 */
#define GPS_C 299792458.0               /* 光速 (m/s) */

/* =================== 卫星数据管理 =================== */

SatelliteData* satellite_data_create(int max_satellites) {
    if (max_satellites <= 0) return NULL;
    
    SatelliteData* data = (SatelliteData*)safe_malloc(sizeof(SatelliteData));
    if (data == NULL) return NULL;
    
    data->satellites = (Satellite*)safe_calloc(max_satellites, sizeof(Satellite));
    if (data->satellites == NULL) {
        safe_free((void**)&data);
        return NULL;
    }
    
    data->satellite_count = 0;
    data->max_satellites = max_satellites;
    data->reference_time = time(NULL);
    
    return data;
}

void satellite_data_destroy(SatelliteData* data) {
    if (data == NULL) return;
    
    if (data->satellites != NULL) {
        safe_free((void**)&data->satellites);
    }
    
    safe_free((void**)&data);
}

int satellite_data_add(SatelliteData* data, const Satellite* satellite) {
    if (data == NULL || satellite == NULL) return 0;
    
    if (data->satellite_count >= data->max_satellites) {
        error_set(ERROR_MEMORY, "卫星数据已满", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 验证卫星数据 */
    if (!validate_prn(satellite->prn)) {
        error_set(ERROR_PARAMETER, "无效的PRN号", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 检查是否已存在相同PRN的卫星 */
    for (int i = 0; i < data->satellite_count; i++) {
        if (data->satellites[i].prn == satellite->prn) {
            /* 更新现有卫星数据 */
            memcpy(&data->satellites[i], satellite, sizeof(Satellite));
            return 1;
        }
    }
    
    /* 添加新卫星 */
    memcpy(&data->satellites[data->satellite_count], satellite, sizeof(Satellite));
    data->satellite_count++;
    
    return 1;
}

Satellite* satellite_data_find(SatelliteData* data, int prn) {
    if (data == NULL || !validate_prn(prn)) return NULL;
    
    for (int i = 0; i < data->satellite_count; i++) {
        if (data->satellites[i].prn == prn) {
            return &data->satellites[i];
        }
    }
    
    return NULL;
}

/* =================== 卫星位置计算 =================== */

/* 解开普勒方程 */
static double solve_kepler_equation(double M, double e, double tolerance) {
    double E = M;  /* 初始猜测 */
    double delta_E;
    
    for (int i = 0; i < 50; i++) {  /* 最多迭代50次 */
        delta_E = (E - e * sin(E) - M) / (1 - e * cos(E));
        E = E - delta_E;
        
        if (fabs(delta_E) < tolerance) {
            return E;
        }
    }
    
    return E;  /* 返回最佳估计 */
}

/* 计算卫星位置 */
int satellite_position_calculate(Satellite* satellite, time_t time) {
    if (satellite == NULL || !time_is_valid(time)) return 0;
    
    /* 计算时间差 */
    double dt = difftime(time, satellite->valid_time);
    if (dt < 0) dt = 0;  /* 不能使用未来的数据 */
    
    /* 获取轨道参数 */
    SatelliteOrbit* orbit = &satellite->orbit;
    
    /* 计算平近点角 */
    double n = sqrt(EARTH_MU) / (orbit->sqrt_a * orbit->sqrt_a * orbit->sqrt_a) + orbit->delta_n;
    double M = orbit->m0 + n * dt;
    
    /* 归一化平近点角到 [0, 2π] */
    M = fmod(M, 2 * GPS_PI);
    if (M < 0) M += 2 * GPS_PI;
    
    /* 解开普勒方程得到偏近点角 */
    double E = solve_kepler_equation(M, orbit->e, 1e-12);
    
    /* 计算真近点角 */
    double nu = 2 * atan2(sqrt(1 + orbit->e) * sin(E / 2), 
                          sqrt(1 - orbit->e) * cos(E / 2));
    
    /* 计算轨道半径 */
    double r = orbit->sqrt_a * orbit->sqrt_a * (1 - orbit->e * cos(E));
    
    /* 计算轨道平面内的位置 */
    double x_orbital = r * cos(nu);
    double y_orbital = r * sin(nu);
    
    /* 计算经度修正 */
    double omega = orbit->omega + orbit->omega_dot * dt;
    
    /* 计算纬度修正 */
    double phi = nu + orbit->omega;
    
    /* 应用调和改正项 */
    double delta_u = orbit->cus * sin(2 * phi) + orbit->cuc * cos(2 * phi);
    double delta_r = orbit->crs * sin(2 * phi) + orbit->crc * cos(2 * phi);
    double delta_i = orbit->cis * sin(2 * phi) + orbit->cic * cos(2 * phi);
    
    /* 修正后的轨道参数 */
    double u = phi + delta_u;
    double r_corrected = r + delta_r;
    double i_corrected = orbit->i0 + orbit->i_dot * dt + delta_i;
    
    /* 计算升交点赤经 */
    double lambda = orbit->omega0 + (orbit->omega_dot - EARTH_OMEGA) * dt - EARTH_OMEGA * orbit->toe;
    
    """    /* TODO: 实现相对论效应修正 */
    /* 计算ECEF坐标 */
    double x = r_corrected * (cos(u) * cos(lambda) - sin(u) * cos(i_corrected) * sin(lambda));
    double y = r_corrected * (cos(u) * sin(lambda) + sin(u) * cos(i_corrected) * cos(lambda));
    double z = r_corrected * sin(u) * sin(i_corrected);""
    
    /* 计算速度 (简化版本) */
    double v = sqrt(EARTH_MU / (orbit->sqrt_a * orbit->sqrt_a * orbit->sqrt_a));
    double vx = -v * sin(u) * cos(lambda) - v * cos(u) * cos(i_corrected) * sin(lambda);
    double vy = -v * sin(u) * sin(lambda) + v * cos(u) * cos(i_corrected) * cos(lambda);
    double vz = v * cos(u) * sin(i_corrected);
    
    /* 存储结果 */
    satellite->pos.x = x;
    satellite->pos.y = y;
    satellite->pos.z = z;
    satellite->pos.vx = vx;
    satellite->pos.vy = vy;
    satellite->pos.vz = vz;
    satellite->valid_time = time;
    satellite->is_valid = 1;
    
    return 1;
}

/* =================== 卫星可见性计算 =================== */

int satellite_visibility_calculate(const Satellite* satellite, 
                                   double lat, double lon, double alt,
                                   SatelliteVisibility* visibility) {
    if (satellite == NULL || visibility == NULL || 
        !validate_latitude(lat) || !validate_longitude(lon) || !validate_altitude(alt)) {
        return 0;
    }
    
    if (!satellite->is_valid) {
        visibility->is_visible = 0;
        visibility->is_obstructed = 1;
        return 0;
    }
    
    /* 计算接收机位置 */
    GeodeticCoordinate receiver_pos = {lat, lon, alt};
    EcefCoordinate receiver_ecef = geodetic_to_ecef(&receiver_pos);
    
    /* 计算卫星到接收机的向量 */
    double dx = satellite->pos.x - receiver_ecef.x;
    double dy = satellite->pos.y - receiver_ecef.y;
    double dz = satellite->pos.z - receiver_ecef.z;
    
    /* 计算距离 */
    visibility->distance = sqrt(dx * dx + dy * dy + dz * dz);
    
    /* 计算高度角和方位角 */
    double receiver_lat_rad = degrees_to_radians(lat);
    double receiver_lon_rad = degrees_to_radians(lon);
    
    /* 转换到接收机坐标系 */
    double cos_lat = cos(receiver_lat_rad);
    double sin_lat = sin(receiver_lat_rad);
    double cos_lon = cos(receiver_lon_rad);
    double sin_lon = sin(receiver_lon_rad);
    
    /* 接收机坐标系中的卫星位置 */
    double x_local = -sin_lat * cos_lon * dx - sin_lat * sin_lon * dy + cos_lat * dz;
    double y_local = -sin_lon * dx + cos_lon * dy;
    double z_local = cos_lat * cos_lon * dx + cos_lat * sin_lon * dy + sin_lat * dz;
    
    /* 计算高度角 */
    double elevation_rad = atan2(z_local, sqrt(x_local * x_local + y_local * y_local));
    visibility->elevation = radians_to_degrees(elevation_rad);
    
    /* 计算方位角 */
    double azimuth_rad = atan2(y_local, x_local);
    visibility->azimuth = radians_to_degrees(azimuth_rad);
    if (visibility->azimuth < 0) {
        visibility->azimuth += 360.0;
    }
    
    /* 判断可见性 */
    visibility->is_visible = (visibility->elevation > 5.0);  /* 5度截止角 */
    visibility->is_obstructed = 0;  /* 简化版本，不考虑遮挡 */
    
    /* 计算信号强度 (简化模型) */
    if (visibility->is_visible) {
        /* 自由空间路径损耗 */
        double path_loss = 20 * log10(visibility->distance) + 20 * log10(1575.42e6) - 147.55;
        visibility->signal_strength = -130.0 - path_loss;  /* 典型GPS信号强度 */
    } else {
        visibility->signal_strength = -200.0;  /* 不可见 */
    }
    
    visibility->prn = satellite->prn;
    
    return 1;
}

/* =================== RINEX文件处理 =================== */

int rinex_header_parse(const char* filename, RinexHeader* header) {
    if (filename == NULL || header == NULL) return 0;
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        error_set(ERROR_FILE, "无法打开RINEX文件", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 初始化头信息 */
    memset(header, 0, sizeof(RinexHeader));
    
    char line[256];
    int line_number = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        char* trimmed = string_trim(line);
        
        /* 解析版本 */
        if (string_starts_with(trimmed, "RINEX VERSION / TYPE")) {
            sscanf(trimmed, "%s", header->version);
        }
        /* 解析卫星系统 */
        else if (string_starts_with(trimmed, "PGM / RUN BY / DATE")) {
            strncpy(header->satellite_system, "BEIDOU", sizeof(header->satellite_system) - 1);
        }
        /* 解析开始时间 */
        else if (string_starts_with(trimmed, "TIME OF FIRST OBS")) {
            int year, month, day, hour, minute, second;
            if (sscanf(trimmed, " %d %d %d %d %d %d", 
                      &year, &month, &day, &hour, &minute, &second) == 6) {
                struct tm tm = {0};
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                tm.tm_hour = hour;
                tm.tm_min = minute;
                tm.tm_sec = second;
                header->start_time = mktime(&tm);
            }
        }
        /* 解析结束标记 */
        else if (string_starts_with(trimmed, "END OF HEADER")) {
            break;
        }
    }
    
    fclose(file);
    return 1;
}

int rinex_data_parse(const char* filename, SatelliteData* data) {
    if (filename == NULL || data == NULL) return 0;
    
    /* 这是一个简化的RINEX解析器 */
    /* 实际实现需要更复杂的解析逻辑 */
    
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        error_set(ERROR_FILE, "无法打开RINEX文件", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    char line[256];
    int in_data_section = 0;
    
    while (fgets(line, sizeof(line), file)) {
        char* trimmed = string_trim(line);
        
        /* 跳过头信息 */
        if (string_starts_with(trimmed, "END OF HEADER")) {
            in_data_section = 1;
            continue;
        }
        
        if (!in_data_section) continue;
        
        /* 简化的数据解析 */
        if (strlen(trimmed) > 0) {
            /* 这里应该解析实际的卫星数据 */
            /* 为了演示，我们创建一个模拟卫星 */
            if (data->satellite_count < data->max_satellites) {
                Satellite sat = {0};
                sat.prn = data->satellite_count + 1;
                sat.system = SATELLITE_SYSTEM_BEIDOU;
                sat.is_valid = 1;
                sat.valid_time = time(NULL);
                
                /* 设置默认轨道参数 */
                sat.orbit.sqrt_a = 5153.8;  /* 典型的北斗卫星轨道参数 */
                sat.orbit.e = 0.001;
                sat.orbit.i0 = 55.0 * GPS_PI / 180.0;
                sat.orbit.omega0 = 0.0;
                sat.orbit.omega = 0.0;
                sat.orbit.m0 = 0.0;
                sat.orbit.toe = 0.0;
                
                /* 计算位置 */
                satellite_position_calculate(&sat, sat.valid_time);
                
                /* 添加到数据中 */
                satellite_data_add(data, &sat);
            }
        }
    }
    
    fclose(file);
    return 1;
}

int rinex_write_example(const char* filename) {
    if (filename == NULL) return 0;
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        error_set(ERROR_FILE, "无法创建RINEX文件", __func__, __FILE__, __LINE__);
        return 0;
    }
    
    /* 写入RINEX头信息 */
    fprintf(file, "     2.11           OBSERVATION DATA    M (Mix)             RINEX VERSION / TYPE\n");
    fprintf(file, "BEIDOU                                   PGM / RUN BY / DATE\n");
    fprintf(file, "                                                            COMMENT\n");
    fprintf(file, "                                                            MARKER NAME\n");
    fprintf(file, "                                                            MARKER NUMBER\n");
    fprintf(file, "                                                            MARKER TYPE\n");
    fprintf(file, "                                                            OBSERVER / AGENCY\n");
    fprintf(file, "                                                            REC # / TYPE / VERS\n");
    fprintf(file, "                                                            ANT # / TYPE\n");
    fprintf(file, "        0.0000        0.0000        0.0000                 APPROX POSITION XYZ\n");
    fprintf(file, "        0.0000        0.0000        0.0000                 ANTENNA: DELTA H/E/N\n");
    fprintf(file, "     1     0                                                WAVELENGTH FACT L1/2\n");
    fprintf(file, "     4    C1C    L1C    D1C    S1C                          # / TYPES OF OBSERV\n");
    fprintf(file, "    12                                                        SYS / # / OBS TYPES\n");
    fprintf(file, "                                                            SYS / # / OBS TYPES\n");
    fprintf(file, "                                                            SYS / # / OBS TYPES\n");
    fprintf(file, "                                                            SYS / # / OBS TYPES\n");
    fprintf(file, "     1     1                                                INTERVAL\n");
    fprintf(file, "                                                            TIME OF FIRST OBS\n");
    fprintf(file, "     1     1                                                TIME OF LAST OBS\n");
    fprintf(file, "     1                                                        LEAP SECONDS\n");
    fprintf(file, "                                                            # OF SATELLITES\n");
    fprintf(file, "                                                            PRN / # OF OBS\n");
    fprintf(file, "                                                            END OF HEADER\n");
    
    fclose(file);
    return 1;
}

/* =================== 数据验证 =================== */

int satellite_data_validate(const SatelliteData* data) {
    if (data == NULL) return 0;
    
    if (data->satellites == NULL || data->max_satellites <= 0) {
        return 0;
    }
    
    if (data->satellite_count > data->max_satellites) {
        return 0;
    }
    
    for (int i = 0; i < data->satellite_count; i++) {
        Satellite* sat = &data->satellites[i];
        
        if (!validate_prn(sat->prn)) {
            return 0;
        }
        
        if (sat->system < SATELLITE_SYSTEM_BEIDOU || 
            sat->system > SATELLITE_SYSTEM_GALILEO) {
            return 0;
        }
    }
    
    return 1;
}

const char* satellite_system_to_string(SatelliteSystem system) {
    switch (system) {
        case SATELLITE_SYSTEM_BEIDOU: return "BEIDOU";
        case SATELLITE_SYSTEM_GPS: return "GPS";
        case SATELLITE_SYSTEM_GLONASS: return "GLONASS";
        case SATELLITE_SYSTEM_GALILEO: return "GALILEO";
        default: return "UNKNOWN";
    }
}