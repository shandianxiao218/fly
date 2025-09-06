# CLAUDE-config-variables.md

## 配置变量参考

### 1. 编译配置变量

#### Makefile 配置
```makefile
# 编译器和构建配置
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
CFLAGS_DEBUG = -g -O0 -DDEBUG
INCLUDES = -I./src -I./include -I./lib
LIBDIRS = -L./lib -L./build
LIBS = -lm -lpthread -lculib
BUILD_DIR = ./build
```

#### 条件编译宏
```c
/* 调试和功能开关 */
#define DEBUG              1       /* 调试输出 */
#define DEBUG_MEMORY       0       /* 内存调试 */
#define ENABLE_LOGGING     1       /* 日志系统 */
#define ENABLE_PROFILING   0       /* 性能分析 */
#define ENABLE_WEBSOCKET   0       /* WebSocket支持 */
#define PLATFORM_WINDOWS   1       /* Windows平台 */
```

### 2. 运行时配置

#### 应用程序配置
```c
/**
 * @brief 应用程序配置结构体
 */
typedef struct {
    /* 基础配置 */
    int debug_mode;              /**< 调试模式 (0=关闭, 1=开启) */
    int verbose_level;           /**< 详细程度 (0-3) */
    char log_file[256];          /**< 日志文件路径 */
    char data_dir[256];          /**< 数据目录路径 */
    
    /* 性能配置 */
    int max_threads;             /**< 最大线程数 */
    double timeout;               /**< 超时时间 (秒) */
    int cache_size;              /**< 缓存大小 */
    int buffer_size;             /**< 缓冲区大小 */
    
    /* 网络配置 */
    int server_port;             /**< 服务器端口 */
    int max_connections;         /**< 最大连接数 */
    char server_host[256];       /**< 服务器地址 */
    
    /* 计算配置 */
    int precision_level;         /**< 精度等级 (1-5) */
    int max_iterations;          /**< 最大迭代次数 */
    double convergence_threshold; /**< 收敛阈值 */
    
    /* 文件配置 */
    char input_file[256];        /**< 输入文件路径 */
    char output_file[256];       /**< 输出文件路径 */
    char config_file[256];       /**< 配置文件路径 */
    char temp_dir[256];          /**< 临时目录路径 */
} AppConfig;
```

#### 默认配置值
```c
/**
 * @brief 默认配置值
 */
#define DEFAULT_CONFIG {\
    .debug_mode = 0,\
    .verbose_level = 1,\
    .log_file = "./logs/beidou.log",\
    .data_dir = "./data",\
    .max_threads = 4,\
    .timeout = 30.0,\
    .cache_size = 1024,\
    .buffer_size = 8192,\
    .server_port = 8080,\
    .max_connections = 10,\
    .server_host = "localhost",\
    .precision_level = 3,\
    .max_iterations = 100,\
    .convergence_threshold = 1e-6,\
    .input_file = "",\
    .output_file = "",\
    .config_file = "./config/beidou.conf",\
    .temp_dir = "./temp"\
}
```

### 3. 卫星模块配置

#### 卫星计算配置
```c
/**
 * @brief 卫星计算配置
 */
typedef struct {
    /* 轨道计算参数 */
    double gravitational_constant; /**< 引力常数 */
    double earth_radius;          /**< 地球半径 (米) */
    double earth_rotation_rate;   /**< 地球自转角速度 */
    
    /* 时间参数 */
    double leap_seconds;         /**< 闰秒数 */
    double time_system_offset;   /**< 时间系统偏移 */
    
    /* 计算精度 */
    double position_precision;    /**< 位置计算精度 */
    double velocity_precision;    /**< 速度计算精度 */
    int max_iterations;           /**< 最大迭代次数 */
    
    /* 信号参数 */
    double signal_threshold;      /**< 信号强度阈值 */
    double noise_level;           /**< 噪声水平 */
    double multipath_threshold;   /**< 多路径效应阈值 */
    
    /* 系统参数 */
    int system_type;              /**< 卫星系统类型 */
    int frequency_band;           /**< 频率波段 */
    int constellation_type;       /**< 星座类型 */
} SatelliteConfig;
```

#### 默认卫星配置
```c
#define DEFAULT_SATELLITE_CONFIG {\
    .gravitational_constant = 3.986004418e14,\
    .earth_radius = 6378137.0,\
    .earth_rotation_rate = 7.292115e-5,\
    .leap_seconds = 18.0,\
    .time_system_offset = 0.0,\
    .position_precision = 1e-3,\
    .velocity_precision = 1e-6,\
    .max_iterations = 20,\
    .signal_threshold = -150.0,\
    .noise_level = 2.0,\
    .multipath_threshold = 0.5,\
    .system_type = SATELLITE_SYSTEM_BEIDOU,\
    .frequency_band = FREQUENCY_BAND_B1,\
    .constellation_type = CONSTELLATION_GEO\
}
```

### 4. 飞机模块配置

#### 飞行轨迹配置
```c
/**
 * @brief 飞行轨迹配置
 */
typedef struct {
    /* 轨迹生成参数 */
    double trajectory_resolution; /**< 轨迹分辨率 (秒) */
    double max_altitude;          /**< 最大高度 (米) */
    double min_altitude;          /**< 最小高度 (米) */
    double max_velocity;          /**< 最大速度 (米/秒) */
    double max_acceleration;      /**< 最大加速度 (米/秒²) */
    
    /* 机动参数 */
    double max_roll_rate;         /**< 最大滚转角速度 (度/秒) */
    double max_pitch_rate;        /**< 最大俯仰角速度 (度/秒) */
    double max_yaw_rate;          /**< 最大偏航角速度 (度/秒) */
    double max_bank_angle;        /**< 最大倾斜角 (度) */
    
    /* 航线参数 */
    double cruise_altitude;       /**< 巡航高度 (米) */
    double approach_angle;        /**< 进近角度 (度) */
    double glide_slope;          /**< 下滑道角度 (度) */
    
    /* 性能参数 */
    double climb_rate;           /**< 爬升率 (米/秒) */
    double descent_rate;         /**< 下降率 (米/秒) */
    double turn_radius;          /**< 转弯半径 (米) */
} AircraftConfig;
```

#### 默认飞机配置
```c
#define DEFAULT_AIRCRAFT_CONFIG {\
    .trajectory_resolution = 1.0,\
    .max_altitude = 15000.0,\
    .min_altitude = 0.0,\
    .max_velocity = 300.0,\
    .max_acceleration = 10.0,\
    .max_roll_rate = 30.0,\
    .max_pitch_rate = 20.0,\
    .max_yaw_rate = 15.0,\
    .max_bank_angle = 45.0,\
    .cruise_altitude = 10000.0,\
    .approach_angle = 3.0,\
    .glide_slope = 3.0,\
    .climb_rate = 10.0,\
    .descent_rate = -5.0,\
    .turn_radius = 1000.0\
}
```

### 5. 遮挡模块配置

#### 遮挡计算配置
```c
typedef struct {
    double angular_resolution;    /**< 角度分辨率 (度) */
    double distance_threshold;    /**< 距离阈值 (米) */
    double intersection_tolerance; /**< 相交容差 */
    int model_detail_level;        /**< 模型细节等级 (1-5) */
    int mesh_density;             /**< 网格密度 */
    double smoothing_factor;      /**< 平滑因子 */
    double material_density;       /**< 材质密度 */
    double material_thickness;    /**< 材料厚度 */
    double attenuation_factor;    /**< 衰减因子 */
    int ray_count;               /**< 射线数量 */
    int max_reflections;         /**< 最大反射次数 */
    double min_obstruction_angle; /**< 最小遮挡角度 */
    int use_spatial_index;       /**< 使用空间索引 */
    int use_occlusion_culling;   /**< 使用遮挡剔除 */
    int use_level_of_detail;     /**< 使用LOD */
} ObstructionConfig;

#define DEFAULT_OBSTRUCTION_CONFIG {\
    .angular_resolution = 1.0, .distance_threshold = 0.1,\
    .intersection_tolerance = 1e-6, .model_detail_level = 3,\
    .mesh_density = 100, .smoothing_factor = 0.1,\
    .material_density = 2700.0, .material_thickness = 0.01,\
    .attenuation_factor = 0.8, .ray_count = 360,\
    .max_reflections = 3, .min_obstruction_angle = 1.0,\
    .use_spatial_index = 1, .use_occlusion_culling = 1,\
    .use_level_of_detail = 1\
}
```

### 6. Web模块配置

#### HTTP服务器配置
```c
/**
 * @brief HTTP服务器配置
 */
typedef struct {
    /* 服务器参数 */
    int port;                    /**< 服务器端口 */
    int max_connections;         /**< 最大连接数 */
    int request_timeout;         /**< 请求超时 (秒) */
    int keep_alive_timeout;      /**< 保持连接超时 (秒) */
    
    /* 缓冲区参数 */
    int buffer_size;             /**< 缓冲区大小 */
    int max_request_size;        /**< 最大请求大小 */
    int max_response_size;       /**< 最大响应大小 */
    
    /* 线程参数 */
    int thread_pool_size;        /**< 线程池大小 */
    int max_threads_per_client;   /**< 每个客户端最大线程数 */
    
    /* 静态文件参数 */
    char static_dir[256];        /**< 静态文件目录 */
    char index_file[256];        /**< 默认索引文件 */
    int enable_directory_listing; /**< 启用目录列表 */
    
    /* API参数 */
    int enable_cors;             /**< 启用CORS */
    int enable_compression;      /**< 启用压缩 */
    int enable_websocket;        /**< 启用WebSocket */
    
    /* 安全参数 */
    int enable_ssl;              /**< 启用SSL */
    char ssl_cert_file[256];     /**< SSL证书文件 */
    char ssl_key_file[256];      /**< SSL密钥文件 */
    int max_request_rate;        /**< 最大请求率 */
} HttpServerConfig;
```

#### 默认Web配置
```c
#define DEFAULT_HTTP_SERVER_CONFIG {\
    .port = 8080,\
    .max_connections = 100,\
    .request_timeout = 30,\
    .keep_alive_timeout = 5,\
    .buffer_size = 8192,\
    .max_request_size = 1048576,\
    .max_response_size = 10485760,\
    .thread_pool_size = 10,\
    .max_threads_per_client = 2,\
    .static_dir = "./web/static",\
    .index_file = "index.html",\
    .enable_directory_listing = 0,\
    .enable_cors = 1,\
    .enable_compression = 1,\
    .enable_websocket = 0,\
    .enable_ssl = 0,\
    .ssl_cert_file = "",\
    .ssl_key_file = "",\
    .max_request_rate = 100\
}
```

### 7. 日志系统配置

#### 日志配置
```c
/**
 * @brief 日志配置
 */
typedef struct {
    /* 日志级别 */
    int log_level;               /**< 日志级别 */
    int file_log_level;          /**< 文件日志级别 */
    int console_log_level;       /**< 控制台日志级别 */
    
    /* 文件配置 */
    char log_file[256];          /**< 日志文件路径 */
    int max_file_size;           /**< 最大文件大小 (字节) */
    int max_file_count;          /**< 最大文件数量 */
    int enable_rotation;         /**< 启用日志轮转 */
    
    /* 格式配置 */
    int enable_timestamp;        /**< 启用时间戳 */
    int enable_function_name;    /**< 启用函数名 */
    int enable_line_number;      /**< 启用行号 */
    int enable_thread_id;        /**< 启用线程ID */
    
    /* 输出配置 */
    int enable_console_output;   /**< 启用控制台输出 */
    int enable_file_output;      /**< 启用文件输出 */
    int enable_syslog;           /**< 启用系统日志 */
    
    /* 缓冲配置 */
    int enable_buffering;        /**< 启用缓冲 */
    int buffer_size;             /**< 缓冲区大小 */
    int flush_interval;          /**< 刷新间隔 (秒) */
} LogConfig;
```

#### 默认日志配置
```c
#define DEFAULT_LOG_CONFIG {\
    .log_level = LOG_LEVEL_INFO,\
    .file_log_level = LOG_LEVEL_DEBUG,\
    .console_log_level = LOG_LEVEL_INFO,\
    .log_file = "./logs/beidou.log",\
    .max_file_size = 10485760,\
    .max_file_count = 5,\
    .enable_rotation = 1,\
    .enable_timestamp = 1,\
    .enable_function_name = 1,\
    .enable_line_number = 1,\
    .enable_thread_id = 0,\
    .enable_console_output = 1,\
    .enable_file_output = 1,\
    .enable_syslog = 0,\
    .enable_buffering = 1,\
    .buffer_size = 8192,\
    .flush_interval = 5\
}
```

### 8. 性能监控配置

#### 性能配置
```c
/**
 * @brief 性能监控配置
 */
typedef struct {
    /* 计时器配置 */
    int enable_performance_timer; /**< 启用性能计时器 */
    int timer_precision;          /**< 计时器精度 */
    int max_timers;              /**< 最大计时器数量 */
    
    /* 计数器配置 */
    int enable_performance_counter; /**< 启用性能计数器 */
    int max_counters;             /**< 最大计数器数量 */
    int counter_reset_interval;   /**< 计数器重置间隔 */
    
    /* 内存监控配置 */
    int enable_memory_monitor;    /**< 启用内存监控 */
    int memory_check_interval;    /**< 内存检查间隔 */
    int memory_threshold;         /**< 内存阈值 */
    
    /* CPU监控配置 */
    int enable_cpu_monitor;       /**< 启用CPU监控 */
    int cpu_check_interval;       /**< CPU检查间隔 */
    int cpu_threshold;            /**< CPU阈值 */
    
    /* 统计配置 */
    int enable_statistics;        /**< 启用统计 */
    int statistics_interval;      /**< 统计间隔 */
    int enable_profiling;         /**< 启用性能分析 */
} PerformanceConfig;
```

#### 默认性能配置
```c
#define DEFAULT_PERFORMANCE_CONFIG {\
    .enable_performance_timer = 1,\
    .timer_precision = TIMER_PRECISION_MILLISECONDS,\
    .max_timers = 100,\
    .enable_performance_counter = 1,\
    .max_counters = 50,\
    .counter_reset_interval = 60,\
    .enable_memory_monitor = 1,\
    .memory_check_interval = 10,\
    .memory_threshold = 80,\
    .enable_cpu_monitor = 1,\
    .cpu_check_interval = 5,\
    .cpu_threshold = 90,\
    .enable_statistics = 1,\
    .statistics_interval = 30,\
    .enable_profiling = 0\
}
```

### 9. 测试配置

#### 测试配置
```c
/**
 * @brief 测试配置
 */
typedef struct {
    /* 测试用例配置 */
    int enable_unit_tests;        /**< 启用单元测试 */
    int enable_integration_tests; /**< 启用集成测试 */
    int enable_performance_tests; /**< 启用性能测试 */
    
    /* 测试数据配置 */
    char test_data_dir[256];     /**< 测试数据目录 */
    char expected_results_dir[256]; /**< 期望结果目录 */
    
    /* 测试运行配置 */
    int test_timeout;            /**< 测试超时 (秒) */
    int max_test_iterations;     /**< 最大测试迭代次数 */
    int enable_test_coverage;    /**< 启用测试覆盖率 */
    
    /* 测试输出配置 */
    int enable_detailed_output;  /**< 启用详细输出 */
    int enable_xml_output;       /**< 启用XML输出 */
    char output_file[256];       /**< 输出文件路径 */
    
    /* 测试环境配置 */
    int enable_mock_objects;     /**< 启用模拟对象 */
    int enable_test_doubles;     /**< 启用测试替身 */
    int enable_test_isolation;   /**< 启用测试隔离 */
} TestConfig;
```

#### 默认测试配置
```c
#define DEFAULT_TEST_CONFIG {\
    .enable_unit_tests = 1,\
    .enable_integration_tests = 1,\
    .enable_performance_tests = 0,\
    .test_data_dir = "./tests/data",\
    .expected_results_dir = "./tests/expected",\
    .test_timeout = 30,\
    .max_test_iterations = 1000,\
    .enable_test_coverage = 1,\
    .enable_detailed_output = 1,\
    .enable_xml_output = 0,\
    .output_file = "./test_results.txt",\
    .enable_mock_objects = 1,\
    .enable_test_doubles = 1,\
    .enable_test_isolation = 1\
}
```

### 10. 环境变量配置

#### 环境变量列表
```bash
# 基础配置
export BEIDOU_DEBUG=1                    # 调试模式
export BEIDOU_LOG_LEVEL=DEBUG             # 日志级别
export BEIDOU_CONFIG_FILE=/path/to/config # 配置文件路径
export BEIDOU_DATA_DIR=/path/to/data      # 数据目录路径

# 性能配置
export BEIDOU_MAX_THREADS=8               # 最大线程数
export BEIDOU_CACHE_SIZE=2048             # 缓存大小
export BEIDOU_TIMEOUT=60                  # 超时时间

# 网络配置
export BEIDOU_SERVER_PORT=8080            # 服务器端口
export BEIDOU_MAX_CONNECTIONS=100          # 最大连接数
export BEIDOU_SERVER_HOST=localhost       # 服务器地址

# 模块配置
export BEIDOU_SATELLITE_PRECISION=3       # 卫星计算精度
export BEIDOU_AIRCRAFT_MAX_ALT=15000      # 最大飞行高度
export BEIDOU_OBSTRUCTION_RESOLUTION=1.0   # 遮挡计算分辨率

# 测试配置
export BEIDOU_TEST_MODE=1                 # 测试模式
export BEIDOU_TEST_DATA_DIR=./tests/data  # 测试数据目录
export BEIDOU_TEST_TIMEOUT=30             # 测试超时
```

### 11. 配置文件格式

#### INI格式示例
```ini
[application]
debug_mode = 1
verbose_level = 2
log_file = ./logs/beidou.log
data_dir = ./data

[performance]
max_threads = 4
timeout = 30.0
cache_size = 1024

[network]
server_port = 8080
max_connections = 10
server_host = localhost

[satellite]
precision_level = 3
max_iterations = 100
signal_threshold = -150.0

[aircraft]
max_altitude = 15000.0
max_velocity = 300.0
trajectory_resolution = 1.0

[obstruction]
angular_resolution = 1.0
ray_count = 360
use_spatial_index = 1
```

#### JSON格式示例
```json
{
  "application": {
    "debug_mode": true,
    "verbose_level": 2,
    "log_file": "./logs/beidou.log",
    "data_dir": "./data"
  },
  "performance": {
    "max_threads": 4,
    "timeout": 30.0,
    "cache_size": 1024
  },
  "network": {
    "server_port": 8080,
    "max_connections": 10,
    "server_host": "localhost"
  },
  "satellite": {
    "precision_level": 3,
    "max_iterations": 100,
    "signal_threshold": -150.0
  },
  "aircraft": {
    "max_altitude": 15000.0,
    "max_velocity": 300.0,
    "trajectory_resolution": 1.0
  },
  "obstruction": {
    "angular_resolution": 1.0,
    "ray_count": 360,
    "use_spatial_index": true
  }
}
```

---

**文档版本**: 1.0  
**创建日期**: 2025-09-06  
**最后更新**: 2025-09-06  
**状态**: 配置变量参考已创建  
**下次更新**: 新的配置参数时