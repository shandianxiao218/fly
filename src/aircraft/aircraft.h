#ifndef AIRCRAFT_H
#define AIRCRAFT_H

#include <time.h>

/* 飞机姿态角 */
typedef struct {
    double pitch;         /* 俯仰角 (度) */
    double roll;          /* 横滚角 (度) */
    double yaw;           /* 偏航角 (度) */
} AircraftAttitude;

/* 飞机位置 */
typedef struct {
    double latitude;      /* 纬度 (度) */
    double longitude;     /* 经度 (度) */
    double altitude;      /* 高度 (米) */
} AircraftPosition;

/* 飞机速度 */
typedef struct {
    double velocity;      /* 速度 (米/秒) */
    double vertical_speed; /* 垂直速度 (米/秒) */
    double heading;       /* 航向角 (度) */
} AircraftVelocity;

/* 飞机状态 */
typedef struct {
    AircraftPosition position;   /* 位置 */
    AircraftAttitude attitude;  /* 姿态 */
    AircraftVelocity velocity;  /* 速度 */
    time_t timestamp;           /* 时间戳 */
    int is_valid;               /* 是否有效 */
} AircraftState;

/* 轨迹点 */
typedef struct {
    time_t timestamp;           /* 时间戳 */
    AircraftState state;        /* 飞机状态 */
} TrajectoryPoint;

/* 飞行轨迹 */
typedef struct {
    int trajectory_id;          /* 轨迹ID */
    TrajectoryPoint* points;    /* 轨迹点数组 */
    int point_count;           /* 轨迹点数量 */
    int max_points;            /* 最大轨迹点数量 */
    time_t start_time;         /* 开始时间 */
    time_t end_time;           /* 结束时间 */
    double total_distance;      /* 总距离 (米) */
    double max_altitude;       /* 最大高度 (米) */
    double min_altitude;       /* 最小高度 (米) */
} FlightTrajectory;

/* 轨迹类型 */
typedef enum {
    TRAJECTORY_TYPE_TAKEOFF = 1,   /* 起飞轨迹 */
    TRAJECTORY_TYPE_CRUISE = 2,    /* 巡航轨迹 */
    TRAJECTORY_TYPE_LANDING = 3,   /* 降落轨迹 */
    TRAJECTORY_TYPE_MANEUVER = 4,  /* 机动轨迹 */
    TRAJECTORY_TYPE_CUSTOM = 5     /* 自定义轨迹 */
} TrajectoryType;

/* 轨迹生成参数 */
typedef struct {
    TrajectoryType type;           /* 轨迹类型 */
    AircraftState start_state;     /* 开始状态 */
    AircraftState end_state;       /* 结束状态 */
    double duration;               /* 持续时间 (秒) */
    int point_interval;           /* 点间隔 (秒) */
    double max_pitch;             /* 最大俯仰角 (度) */
    double max_roll;              /* 最大横滚角 (度) */
    double max_turn_rate;         /* 最大转弯率 (度/秒) */
    double max_climb_rate;        /* 最大爬升率 (米/秒) */
} TrajectoryParams;

/* CSV解析状态 */
typedef struct {
    int line_number;              /* 当前行号 */
    int total_lines;              /* 总行数 */
    int valid_points;             /* 有效点数 */
    int error_count;              /* 错误数量 */
    char last_error[256];         /* 最后错误信息 */
} CsvParseStatus;

/* 函数声明 */
FlightTrajectory* flight_trajectory_create(int max_points);
void flight_trajectory_destroy(FlightTrajectory* trajectory);
int flight_trajectory_add_point(FlightTrajectory* trajectory, const TrajectoryPoint* point);
int flight_trajectory_clear(FlightTrajectory* trajectory);

int flight_trajectory_generate(FlightTrajectory* trajectory, const TrajectoryParams* params);
int flight_trajectory_generate_takeoff(FlightTrajectory* trajectory, 
                                      const AircraftState* start_state,
                                      double target_altitude);
int flight_trajectory_generate_cruise(FlightTrajectory* trajectory,
                                     const AircraftState* start_state,
                                     double duration, double heading);
int flight_trajectory_generate_landing(FlightTrajectory* trajectory,
                                       const AircraftState* start_state,
                                       const AircraftState* end_state);
int flight_trajectory_generate_maneuver(FlightTrajectory* trajectory,
                                       const AircraftState* start_state,
                                       double duration, double max_roll);

int flight_trajectory_load_csv(FlightTrajectory* trajectory, const char* filename);
int flight_trajectory_save_csv(const FlightTrajectory* trajectory, const char* filename);

int aircraft_state_interpolate(const AircraftState* state1, const AircraftState* state2,
                              time_t target_time, AircraftState* result);
int aircraft_state_validate(const AircraftState* state);

int csv_trajectory_parse(const char* filename, FlightTrajectory* trajectory, 
                        CsvParseStatus* status);
int csv_trajectory_write_example(const char* filename);

double aircraft_state_distance(const AircraftState* state1, const AircraftState* state2);
double aircraft_state_bearing(const AircraftState* state1, const AircraftState* state2);

const char* trajectory_type_to_string(TrajectoryType type);
int trajectory_params_validate(const TrajectoryParams* params);

/* 姿态计算相关函数 */
int aircraft_attitude_calculate(const AircraftVelocity* velocity, 
                              const double acceleration[3],
                              AircraftAttitude* attitude);
int aircraft_attitude_extreme(const AircraftAttitude* base_attitude,
                             int maneuver_type, double intensity,
                             AircraftAttitude* result);
int aircraft_attitude_interpolate(const AircraftAttitude* attitude1,
                                 const AircraftAttitude* attitude2,
                                 double t, AircraftAttitude* result);
int aircraft_attitude_validate(const AircraftAttitude* attitude);

#endif /* AIRCRAFT_H */