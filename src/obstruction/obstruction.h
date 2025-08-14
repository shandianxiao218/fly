#ifndef OBSTRUCTION_H
#define OBSTRUCTION_H

#include "../satellite/satellite.h"
#include "../aircraft/aircraft.h"

/* 飞机几何模型类型 */
typedef enum {
    AIRCRAFT_MODEL_COMMERCIAL = 1,   /* 商用飞机 */
    AIRCRAFT_MODEL_MILITARY = 2,     /* 军用飞机 */
    AIRCRAFT_MODEL_GENERAL = 3,      /* 通用飞机 */
    AIRCRAFT_MODEL_DRONE = 4         /* 无人机 */
} AircraftModelType;

/* 3D向量 */
typedef struct {
    double x;    /* X坐标 (米) */
    double y;    /* Y坐标 (米) */
    double z;    /* Z坐标 (米) */
} Vector3D;

/* 3D旋转矩阵 */
typedef struct {
    double m[3][3];  /* 3x3旋转矩阵 */
} RotationMatrix;

/* 飞机部件 */
typedef enum {
    AIRCRAFT_PART_FUSELAGE = 1,     /* 机身 */
    AIRCRAFT_PART_WING_LEFT = 2,    /* 左翼 */
    AIRCRAFT_PART_WING_RIGHT = 3,   /* 右翼 */
    AIRCRAFT_PART_TAIL = 4,         /* 尾翼 */
    AIRCRAFT_PART_ENGINE = 5        /* 发动机 */
} AircraftPart;

/* 飞机部件几何参数 */
typedef struct {
    AircraftPart part_type;          /* 部件类型 */
    Vector3D position;               /* 部件位置 (相对于飞机中心) */
    Vector3D size;                  /* 部件尺寸 (长宽高) */
    Vector3D rotation;              /* 部件旋转 (俯仰,横滚,偏航) */
    int is_obstructing;             /* 是否产生遮挡 */
} AircraftComponent;

/* 飞机几何模型 */
typedef struct {
    AircraftModelType model_type;   /* 飞机类型 */
    AircraftComponent* components;  /* 部件数组 */
    int component_count;           /* 部件数量 */
    Vector3D antenna_position;      /* 天线位置 (相对于飞机中心) */
    double scale_factor;           /* 缩放因子 */
} AircraftGeometry;

/* 遮挡体 */
typedef struct {
    Vector3D center;               /* 中心点 */
    Vector3D size;                 /* 尺寸 (长宽高) */
    RotationMatrix rotation;       /* 旋转矩阵 */
    AircraftPart part_type;        /* 部件类型 */
} ObstructionBody;

/* 射线 */
typedef struct {
    Vector3D origin;               /* 原点 */
    Vector3D direction;            /* 方向向量 */
    double length;                 /* 长度 */
} Ray;

/* 遮挡计算结果 */
typedef struct {
    int is_obstructed;             /* 是否被遮挡 */
    double obstruction_angle;      /* 遮挡角度 (度) */
    double obstruction_distance;   /* 遮挡距离 (米) */
    AircraftPart obstruction_part; /* 遮挡部件 */
    Vector3D intersection_point;   /* 交点 */
    double signal_loss;             /* 信号损失 (dB) */
} ObstructionResult;

/* 遮挡分析结果 */
typedef struct {
    SatelliteVisibility visibility; /* 卫星可见性信息 */
    ObstructionResult obstruction;  /* 遮挡计算结果 */
    double effective_elevation;     /* 有效高度角 (度) */
    double effective_azimuth;       /* 有效方位角 (度) */
    int is_usable;                 /* 是否可用 */
} VisibilityAnalysis;

/* 遮挡计算参数 */
typedef struct {
    double precision;               /* 计算精度 (度) */
    int max_iterations;            /* 最大迭代次数 */
    double min_obstruction_angle;   /* 最小遮挡角度 (度) */
    double signal_threshold;       /* 信号阈值 (dBHz) */
    int consider_multipath;         /* 是否考虑多径效应 */
    int consider_diffraction;       /* 是否考虑衍射效应 */
} ObstructionParams;

/* 批量遮挡计算结果 */
typedef struct {
    VisibilityAnalysis* analyses;   /* 分析结果数组 */
    int analysis_count;            /* 分析结果数量 */
    time_t calculation_time;        /* 计算时间 */
    double total_calculation_time;  /* 总计算时间 (秒) */
    int visible_satellites;        /* 可见卫星数量 */
    int obstructed_satellites;     /* 被遮挡卫星数量 */
    int usable_satellites;          /* 可用卫星数量 */
} BatchObstructionResult;

/* 函数声明 */
AircraftGeometry* aircraft_geometry_create(AircraftModelType model_type);
void aircraft_geometry_destroy(AircraftGeometry* geometry);
int aircraft_geometry_add_component(AircraftGeometry* geometry, 
                                   const AircraftComponent* component);
int aircraft_geometry_set_antenna_position(AircraftGeometry* geometry, 
                                         const Vector3D* position);

int aircraft_geometry_update_transform(AircraftGeometry* geometry, 
                                     const AircraftAttitude* attitude);

Vector3D vector3d_create(double x, double y, double z);
Vector3D vector3d_add(const Vector3D* v1, const Vector3D* v2);
Vector3D vector3d_subtract(const Vector3D* v1, const Vector3D* v2);
Vector3D vector3d_multiply(const Vector3D* v, double scalar);
double vector3d_dot(const Vector3D* v1, const Vector3D* v2);
Vector3D vector3d_cross(const Vector3D* v1, const Vector3D* v2);
double vector3d_length(const Vector3D* v);
Vector3D vector3d_normalize(const Vector3D* v);
Vector3D vector3d_rotate(const Vector3D* v, const RotationMatrix* matrix);

RotationMatrix rotation_matrix_create_from_euler(double pitch, double roll, double yaw);
RotationMatrix rotation_matrix_multiply(const RotationMatrix* m1, const RotationMatrix* m2);

int ray_box_intersection(const Ray* ray, const ObstructionBody* box, 
                         Vector3D* intersection, double* distance);
int ray_component_intersection(const Ray* ray, const AircraftComponent* component,
                               Vector3D* intersection, double* distance);

int obstruction_calculate(const AircraftGeometry* geometry,
                         const SatellitePosition* satellite_pos,
                         const AircraftState* aircraft_state,
                         const ObstructionParams* params,
                         ObstructionResult* result);

int visibility_analyze(const AircraftGeometry* geometry,
                      const Satellite* satellite,
                      const AircraftState* aircraft_state,
                      const ObstructionParams* params,
                      VisibilityAnalysis* analysis);

int batch_obstruction_calculate(const AircraftGeometry* geometry,
                               const SatelliteData* satellite_data,
                               const AircraftState* aircraft_state,
                               const ObstructionParams* params,
                               BatchObstructionResult* result);

int obstruction_params_init(ObstructionParams* params);
int obstruction_params_validate(const ObstructionParams* params);

const char* aircraft_model_type_to_string(AircraftModelType type);
const char* aircraft_part_to_string(AircraftPart part);

#endif /* OBSTRUCTION_H */