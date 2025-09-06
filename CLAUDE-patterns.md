# CLAUDE-patterns.md

## 代码模式和规范

### 1. 头文件模式

#### 标准头文件结构
```c
/**
 * @file module_name.h
 * @brief 模块功能描述
 * @author 开发者
 * @date 创建日期
 * @version 版本号
 * 
 * 详细功能描述...
 */

#ifndef MODULE_NAME_H
#define MODULE_NAME_H

#include <标准头文件>
#include "项目内部头文件"

/* 常量定义 */
#define CONSTANT_NAME value

/* 枚举定义 */
typedef enum {
    ENUM_VALUE_1,
    ENUM_VALUE_2,
    ENUM_VALUE_COUNT
} EnumName;

/* 结构体定义 */
typedef struct {
    Type member1;
    Type member2;
    // 成员说明
} StructName;

/* 函数声明 */
ReturnType function_name(ParameterType param1, ParameterType param2);

#endif /* MODULE_NAME_H */
```

#### 头文件包含顺序
1. 系统头文件 (标准C库)
2. 项目内部头文件
3. 模块内部头文件

### 2. 源文件模式

#### 标准源文件结构
```c
/**
 * @file module_name.c
 * @brief 模块功能实现
 * @author 开发者
 * @date 创建日期
 * @version 版本号
 */

#include "module_name.h"
#include <标准头文件>
#include "其他项目头文件"

/* 私有宏定义 */
#define PRIVATE_MACRO value

/* 私有结构体定义 */
typedef struct {
    Type member1;
    Type member2;
} PrivateStruct;

/* 私有函数声明 */
static ReturnType private_function(ParameterType param);

/* 全局变量 */
static GlobalType global_variable;

/* 函数实现 */
ReturnType function_name(ParameterType param1, ParameterType param2) {
    /* 参数验证 */
    if (param1 == NULL || param2 == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* 函数实现 */
    ReturnType result = SUCCESS;
    
    /* 清理资源 */
    return result;
}
```

### 3. 错误处理模式

#### 错误码定义
```c
/**
 * @brief 错误码枚举
 */
typedef enum {
    ERROR_SUCCESS = 0,           /**< 成功 */
    ERROR_INVALID_PARAMETER,     /**< 无效参数 */
    ERROR_MEMORY_ALLOC,          /**< 内存分配失败 */
    ERROR_FILE_NOT_FOUND,        /**< 文件未找到 */
    ERROR_PARSE_FAILED,          /**< 解析失败 */
    ERROR_CALCULATION_FAILED,    /**< 计算失败 */
    ERROR_SYSTEM_ERROR,          /**< 系统错误 */
    ERROR_UNKNOWN = -1           /**< 未知错误 */
} ErrorCode;

/**
 * @brief 错误信息结构体
 */
typedef struct {
    ErrorCode code;              /**< 错误码 */
    char message[256];           /**< 错误消息 */
    const char* function;        /**< 函数名 */
    const char* file;            /**< 文件名 */
    int line;                    /**< 行号 */
} ErrorInfo;
```

#### 错误处理函数
```c
/**
 * @brief 设置错误信息
 * @param code 错误码
 * @param message 错误消息
 * @param function 函数名
 * @param file 文件名
 * @param line 行号
 */
static void error_set(ErrorCode code, const char* message, 
                      const char* function, const char* file, int line) {
    last_error.code = code;
    strncpy(last_error.message, message, sizeof(last_error.message) - 1);
    last_error.function = function;
    last_error.file = file;
    last_error.line = line;
}
```

### 4. 内存管理模式

#### 安全内存分配
```c
/**
 * @brief 安全的内存分配
 * @param size 分配大小
 * @return 分配的内存指针，失败返回NULL
 */
void* safe_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (ptr == NULL) {
        error_set(ERROR_MEMORY_ALLOC, "内存分配失败", __func__, __FILE__, __LINE__);
        return NULL;
    }
    
    memset(ptr, 0, size);
    return ptr;
}

/**
 * @brief 安全的内存释放
 * @param ptr 指向指针的指针
 */
void safe_free(void** ptr) {
    if (ptr != NULL && *ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}
```

#### 对象创建和销毁模式
```c
/**
 * @brief 创建对象
 * @param config 配置参数
 * @return 对象指针，失败返回NULL
 */
ObjectName* object_name_create(const ConfigType* config) {
    ObjectName* obj = safe_malloc(sizeof(ObjectName));
    if (obj == NULL) {
        return NULL;
    }
    
    /* 初始化成员 */
    obj->member1 = value1;
    obj->member2 = value2;
    
    /* 分配子对象 */
    obj->sub_object = sub_object_create();
    if (obj->sub_object == NULL) {
        object_name_destroy(obj);
        return NULL;
    }
    
    return obj;
}

/**
 * @brief 销毁对象
 * @param obj 对象指针
 */
void object_name_destroy(ObjectName* obj) {
    if (obj == NULL) {
        return;
    }
    
    /* 销毁子对象 */
    if (obj->sub_object != NULL) {
        sub_object_destroy(obj->sub_object);
    }
    
    /* 释放对象 */
    safe_free((void**)&obj);
}
```

### 5. 数据结构管理模式

#### 动态数组模式
```c
/**
 * @brief 动态数组结构体
 */
typedef struct {
    Type* data;                  /**< 数据数组 */
    size_t size;                 /**< 当前大小 */
    size_t capacity;             /**< 容量 */
} DynamicArray;

/**
 * @brief 动态数组初始化
 * @param array 数组指针
 * @param initial_capacity 初始容量
 * @return 成功返回0，失败返回错误码
 */
int dynamic_array_init(DynamicArray* array, size_t initial_capacity) {
    if (array == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    array->data = safe_malloc(initial_capacity * sizeof(Type));
    if (array->data == NULL) {
        return ERROR_MEMORY_ALLOC;
    }
    
    array->size = 0;
    array->capacity = initial_capacity;
    return ERROR_SUCCESS;
}

/**
 * @brief 动态数组添加元素
 * @param array 数组指针
 * @param element 元素
 * @return 成功返回0，失败返回错误码
 */
int dynamic_array_add(DynamicArray* array, Type element) {
    if (array == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* 检查是否需要扩容 */
    if (array->size >= array->capacity) {
        size_t new_capacity = array->capacity * 2;
        Type* new_data = safe_realloc(array->data, new_capacity * sizeof(Type));
        if (new_data == NULL) {
            return ERROR_MEMORY_ALLOC;
        }
        
        array->data = new_data;
        array->capacity = new_capacity;
    }
    
    array->data[array->size++] = element;
    return ERROR_SUCCESS;
}
```

### 6. 函数命名和接口模式

#### 函数命名约定
- **创建函数**: `module_object_create()`
- **销毁函数**: `module_object_destroy()`
- **初始化函数**: `module_object_init()`
- **清理函数**: `module_object_cleanup()`
- **添加函数**: `module_object_add()`
- **删除函数**: `module_object_remove()`
- **查找函数**: `module_object_find()`
- **计算函数**: `module_object_calculate()`
- **验证函数**: `module_object_validate()`
- **序列化函数**: `module_object_serialize()`
- **反序列化函数**: `module_object_deserialize()`

#### 函数参数模式
```c
/**
 * @brief 标准函数接口
 * @param object 对象指针（输入参数）
 * @param input 输入参数
 * @param output 输出参数
 * @return 成功返回0，失败返回错误码
 */
int module_function(ObjectType* object, InputType input, OutputType* output) {
    /* 参数验证 */
    if (object == NULL || output == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* 函数实现 */
    int result = ERROR_SUCCESS;
    
    /* 返回结果 */
    return result;
}
```

### 7. 日志记录模式

#### 日志级别定义
```c
typedef enum {
    LOG_LEVEL_DEBUG,     /**< 调试信息 */
    LOG_LEVEL_INFO,      /**< 一般信息 */
    LOG_LEVEL_WARNING,   /**< 警告信息 */
    LOG_LEVEL_ERROR,     /**< 错误信息 */
    LOG_LEVEL_FATAL      /**< 致命错误 */
} LogLevel;
```

#### 日志记录函数
```c
/**
 * @brief 记录日志
 * @param level 日志级别
 * @param message 日志消息
 * @param function 函数名
 * @param file 文件名
 * @param line 行号
 */
void logger_log(LogLevel level, const char* message, 
                const char* function, const char* file, int line) {
    if (level < current_log_level) {
        return;
    }
    
    const char* level_str = "UNKNOWN";
    switch (level) {
        case LOG_LEVEL_DEBUG:   level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:    level_str = "INFO"; break;
        case LOG_LEVEL_WARNING: level_str = "WARNING"; break;
        case LOG_LEVEL_ERROR:   level_str = "ERROR"; break;
        case LOG_LEVEL_FATAL:   level_str = "FATAL"; break;
    }
    
    fprintf(log_file, "[%s] %s:%d %s(): %s\n", 
            level_str, file, line, function, message);
    fflush(log_file);
}
```

#### 日志宏定义
```c
#define LOG_DEBUG(message) \
    logger_log(LOG_LEVEL_DEBUG, message, __func__, __FILE__, __LINE__)
#define LOG_INFO(message) \
    logger_log(LOG_LEVEL_INFO, message, __func__, __FILE__, __LINE__)
#define LOG_WARNING(message) \
    logger_log(LOG_LEVEL_WARNING, message, __func__, __FILE__, __LINE__)
#define LOG_ERROR(message) \
    logger_log(LOG_LEVEL_ERROR, message, __func__, __FILE__, __LINE__)
#define LOG_FATAL(message) \
    logger_log(LOG_LEVEL_FATAL, message, __func__, __FILE__, __LINE__)
```

### 8. 测试驱动开发模式

#### 测试用例结构
```c
/**
 * @brief 测试用例函数
 */
void TestFunctionName(CuTest* tc) {
    /* 测试数据准备 */
    InputType input = create_test_input();
    OutputType expected = create_expected_output();
    OutputType actual;
    
    /* 执行被测函数 */
    int result = module_function(&input, &actual);
    
    /* 验证结果 */
    CuAssertIntEquals(tc, ERROR_SUCCESS, result);
    CuAssertTrue(tc, output_equals(&expected, &actual));
    
    /* 清理测试数据 */
    cleanup_test_data(&input, &actual);
}
```

#### 测试套件注册
```c
/**
 * @brief 测试套件注册
 */
CuSuite* get_module_test_suite(void) {
    CuSuite* suite = CuSuiteNew();
    
    SUITE_ADD_TEST(suite, TestFunction1);
    SUITE_ADD_TEST(suite, TestFunction2);
    SUITE_ADD_TEST(suite, TestFunction3);
    
    return suite;
}
```

### 9. 配置管理模式

#### 配置结构体
```c
/**
 * @brief 应用配置结构体
 */
typedef struct {
    int debug_mode;              /**< 调试模式 */
    char log_file[256];          /**< 日志文件路径 */
    int max_connections;         /**< 最大连接数 */
    double timeout;               /**< 超时时间 */
    /* 其他配置项 */
} AppConfig;
```

#### 配置文件操作
```c
/**
 * @brief 加载配置文件
 * @param filename 配置文件路径
 * @param config 配置结构体
 * @return 成功返回0，失败返回错误码
 */
int config_load(const char* filename, AppConfig* config) {
    if (filename == NULL || config == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    /* 设置默认值 */
    config_set_defaults(config);
    
    /* 解析配置文件 */
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return ERROR_FILE_NOT_FOUND;
    }
    
    /* 读取和解析配置项 */
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        parse_config_line(line, config);
    }
    
    fclose(file);
    return ERROR_SUCCESS;
}
```

### 10. 模块接口模式

#### 模块公共接口
```c
/**
 * @defgroup module_name 模块名称
 * @brief 模块功能描述
 * 
 * 详细功能描述...
 * 
 * @{
 */

/* 类型定义 */
typedef struct ModuleName ModuleName;

/* 创建和销毁 */
ModuleName* module_name_create(const ModuleConfig* config);
void module_name_destroy(ModuleName* module);

/* 核心功能 */
int module_name_process(ModuleName* module, const InputType* input, OutputType* output);
int module_name_configure(ModuleName* module, const ModuleConfig* config);

/* 状态查询 */
int module_name_get_status(const ModuleName* module, ModuleStatus* status);
int module_name_is_ready(const ModuleName* module);

/** @} */
```

### 11. 性能优化模式

#### 性能计数器
```c
/**
 * @brief 性能计数器结构体
 */
typedef struct {
    const char* name;            /**< 计数器名称 */
    double total_time;            /**< 总时间 */
    long call_count;             /**< 调用次数 */
    double min_time;             /**< 最小时间 */
    double max_time;             /**< 最大时间 */
} PerformanceCounter;

/**
 * @brief 性能计时器结构体
 */
typedef struct {
    const char* name;            /**< 计时器名称 */
    clock_t start_time;          /**< 开始时间 */
    PerformanceCounter* counter;  /**< 关联的计数器 */
} PerformanceTimer;
```

#### 性能监控宏
```c
#define PERFORMANCE_TIMER_START(name) \
    PerformanceTimer timer_##name = {name, clock(), &counter_##name}
#define PERFORMANCE_TIMER_STOP(name) \
    do { \
        double elapsed = (double)(clock() - timer_##name.start_time) / CLOCKS_PER_SEC; \
        performance_counter_add(timer_##name.counter, elapsed); \
    } while(0)
```

### 12. 线程安全模式

#### 简单互斥锁
```c
/**
 * @brief 简单互斥锁结构体
 */
typedef struct {
    CRITICAL_SECTION cs;         /**< Windows临界区 */
    int initialized;             /**< 初始化标志 */
} SimpleMutex;

/**
 * @brief 初始化互斥锁
 * @param mutex 互斥锁指针
 * @return 成功返回0，失败返回错误码
 */
int simple_mutex_init(SimpleMutex* mutex) {
    if (mutex == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    InitializeCriticalSection(&mutex->cs);
    mutex->initialized = 1;
    return ERROR_SUCCESS;
}

/**
 * @brief 锁定互斥锁
 * @param mutex 互斥锁指针
 * @return 成功返回0，失败返回错误码
 */
int simple_mutex_lock(SimpleMutex* mutex) {
    if (mutex == NULL || !mutex->initialized) {
        return ERROR_INVALID_PARAMETER;
    }
    
    EnterCriticalSection(&mutex->cs);
    return ERROR_SUCCESS;
}
```

---

**文档版本**: 1.0  
**创建日期**: 2025-09-06  
**最后更新**: 2025-09-06  
**状态**: 代码模式文档已创建  
**下次更新**: 新的模式建立时