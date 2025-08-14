#ifndef SATELLITE_H
#define SATELLITE_H

#include <time.h>

/* 卫星系统类型 */
typedef enum {
    SATELLITE_SYSTEM_BEIDOU = 1,
    SATELLITE_SYSTEM_GPS = 2,
    SATELLITE_SYSTEM_GLONASS = 3,
    SATELLITE_SYSTEM_GALILEO = 4
} SatelliteSystem;

/* 卫星轨道参数 */
typedef struct {
    double toe;          /* 星历参考时间 (秒) */
    double sqrt_a;       /* 轨道长半轴平方根 (m^0.5) */
    double e;            /* 离心率 */
    double i0;           /* 轨道倾角 (弧度) */
    double omega0;       /* 升交点赤经 (弧度) */
    double omega;        /* 近地点幅角 (弧度) */
    double m0;           /* 平近点角 (弧度) */
    double delta_n;      /* 平均运动角速度修正 (弧度/秒) */
    double i_dot;        /* 轨道倾角变化率 (弧度/秒) */
    double omega_dot;    /* 升交点赤经变化率 (弧度/秒) */
    double cuc;          /* 纬度余弦调和改正项 */
    double cus;          /* 纬度正弦调和改正项 */
    double crc;          /* 轨道半径余弦调和改正项 */
    double crs;          /* 轨道半径正弦调和改正项 */
    double cic;          /* 轨道倾角余弦调和改正项 */
    double cis;          /* 轨道倾角正弦调和改正项 */
} SatelliteOrbit;

/* 卫星时钟参数 */
typedef struct {
    double t_oc;         /* 时钟参考时间 (秒) */
    double a0;           /* 时钟偏差 (秒) */
    double a1;           /* 时钟漂移 (秒/秒) */
    double a2;           /* 时钟漂移率 (秒/秒^2) */
} SatelliteClock;

/* 卫星位置和速度 */
typedef struct {
    double x;            /* X坐标 (米) */
    double y;            /* Y坐标 (米) */
    double z;            /* Z坐标 (米) */
    double vx;           /* X方向速度 (米/秒) */
    double vy;           /* Y方向速度 (米/秒) */
    double vz;           /* Z方向速度 (米/秒) */
} SatellitePosition;

/* 卫星状态 */
typedef struct {
    int prn;             /* 卫星PRN号 */
    SatelliteSystem system; /* 卫星系统 */
    SatelliteOrbit orbit;   /* 轨道参数 */
    SatelliteClock clock;   /* 时钟参数 */
    SatellitePosition pos;  /* 位置和速度 */
    time_t valid_time;   /* 有效时间 */
    int is_valid;        /* 是否有效 */
} Satellite;

/* 卫星可见性状态 */
typedef struct {
    int prn;             /* 卫星PRN号 */
    double elevation;    /* 高度角 (度) */
    double azimuth;      /* 方位角 (度) */
    double distance;     /* 距离 (米) */
    int is_visible;     /* 是否可见 */
    double signal_strength; /* 信号强度 (dBHz) */
    int is_obstructed;  /* 是否被遮挡 */
} SatelliteVisibility;

/* 卫星数据管理 */
typedef struct {
    Satellite* satellites;      /* 卫星数组 */
    int satellite_count;        /* 卫星数量 */
    int max_satellites;        /* 最大卫星数量 */
    time_t reference_time;      /* 参考时间 */
} SatelliteData;

/* RINEX文件头信息 */
typedef struct {
    char version[16];          /* RINEX版本 */
    char file_type[16];        /* 文件类型 */
    char satellite_system[16]; /* 卫星系统 */
    char observation_type[64];  /* 观测类型 */
    time_t start_time;         /* 开始时间 */
    time_t end_time;           /* 结束时间 */
    int interval;              /* 时间间隔 (秒) */
    int satellite_count;       /* 卫星数量 */
    int prn_list[64];         /* PRN列表 */
} RinexHeader;

/* 函数声明 */
SatelliteData* satellite_data_create(int max_satellites);
void satellite_data_destroy(SatelliteData* data);
int satellite_data_add(SatelliteData* data, const Satellite* satellite);
Satellite* satellite_data_find(SatelliteData* data, int prn);

int satellite_position_calculate(Satellite* satellite, time_t time);
int satellite_visibility_calculate(const Satellite* satellite, 
                                   double lat, double lon, double alt,
                                   SatelliteVisibility* visibility);

int rinex_header_parse(const char* filename, RinexHeader* header);
int rinex_data_parse(const char* filename, SatelliteData* data);
int rinex_write_example(const char* filename);

int satellite_data_validate(const SatelliteData* data);
const char* satellite_system_to_string(SatelliteSystem system);

#endif /* SATELLITE_H */