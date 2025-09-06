# CLAUDE-troubleshooting.md

## 故障排除指南

### 1. 编译错误

#### 问题1: 头文件包含错误
**错误信息**: `fatal error: module_name.h: No such file or directory`  
**常见原因**: 
- 头文件路径不正确
- 头文件不存在
- 构建系统配置错误

**解决方案**:
```bash
# 检查头文件是否存在
find . -name "module_name.h"

# 检查编译命令中的包含路径
gcc -I./src -I./include ...

# 更新Makefile中的INCLUDES变量
INCLUDES = -I./src -I./include
```

#### 问题2: 链接错误
**错误信息**: `undefined reference to 'function_name'`  
**常见原因**:
- 函数定义不存在
- 函数声明与定义不匹配
- 目标文件未正确链接

**解决方案**:
```bash
# 检查函数是否定义
grep -r "function_name" src/

# 检查Makefile中的链接顺序
OBJS = main.o satellite.o aircraft.o obstruction.o utils.o

# 确保所有源文件都被编译
$(CC) $(CFLAGS) -c $< -o $@
```

#### 问题3: 类型不匹配错误
**错误信息**: `incompatible pointer type` 或 `type mismatch`  
**常见原因**:
- 指针类型不匹配
- 函数参数类型不正确
- 结构体成员类型错误

**解决方案**:
```c
// 检查函数声明和实现
ReturnType function_name(ParameterType* param);

// 确保参数类型匹配
function_name((ParameterType*)ptr);

// 使用类型转换时要注意安全性
```

### 2. 运行时错误

#### 问题4: 段错误 (Segmentation Fault)
**错误信息**: `Segmentation fault (core dumped)`  
**常见原因**:
- 空指针解引用
- 数组越界访问
- 内存释放后访问
- 栈溢出

**解决方案**:
```bash
# 使用gdb调试
gcc -g -o program program.c
gdb ./program
(gdb) run
(gdb) bt  # 查看调用栈

# 使用valgrind检测内存错误
valgrind --leak-check=full ./program
```

**预防措施**:
```c
// 始终检查指针是否为NULL
if (ptr == NULL) {
    return ERROR_INVALID_PARAMETER;
}

// 使用安全的内存访问函数
if (index < array_size) {
    value = array[index];
}
```

#### 问题5: 内存泄漏
**症状**: 程序运行时间越长，内存使用量越大  
**检测方法**:
```bash
# 使用valgrind检测内存泄漏
valgrind --leak-check=full --show-leak-kinds=all ./program

# 在代码中添加内存跟踪
#ifdef DEBUG_MEMORY
#define DEBUG_MALLOC(size) debug_malloc(__FILE__, __LINE__, size)
#define DEBUG_FREE(ptr) debug_free(__FILE__, __LINE__, ptr)
#else
#define DEBUG_MALLOC(size) malloc(size)
#define DEBUG_FREE(ptr) free(ptr)
#endif
```

**解决方案**:
```c
// 确保每个malloc都有对应的free
void* ptr = malloc(size);
// ... 使用ptr
free(ptr);

// 使用RAII模式管理资源
typedef struct {
    void* data;
    void (*cleanup)(void*);
} Resource;

Resource* resource_create(void* data, void (*cleanup)(void*)) {
    Resource* res = malloc(sizeof(Resource));
    res->data = data;
    res->cleanup = cleanup;
    return res;
}

void resource_destroy(Resource* res) {
    if (res && res->cleanup) {
        res->cleanup(res->data);
    }
    free(res);
}
```

#### 问题6: 死锁
**症状**: 程序挂起，无法继续执行  
**常见原因**:
- 线程间互相等待资源
- 锁的获取顺序不一致
- 未正确释放锁

**解决方案**:
```c
// 使用一致的锁获取顺序
void function1() {
    pthread_mutex_lock(&mutex1);
    pthread_mutex_lock(&mutex2);
    // ... 临界区
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
}

void function2() {
    pthread_mutex_lock(&mutex1);  // 相同的顺序
    pthread_mutex_lock(&mutex2);
    // ... 临界区
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
}

// 使用try_lock避免死锁
if (pthread_mutex_trylock(&mutex1) == 0) {
    if (pthread_mutex_trylock(&mutex2) == 0) {
        // 成功获取两个锁
        pthread_mutex_unlock(&mutex2);
    }
    pthread_mutex_unlock(&mutex1);
}
```

### 3. 性能问题

#### 问题7: 计算性能差
**症状**: 卫星位置计算超过10ms，遮挡计算超过100ms  
**诊断方法**:
```c
// 使用性能计数器
#include <time.h>

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void profile_function() {
    double start = get_time_ms();
    
    // 被测试的函数
    satellite_position_calculate(satellite, time);
    
    double end = get_time_ms();
    printf("Function took %.2f ms\n", end - start);
}
```

**优化策略**:
```c
// 使用查找表优化重复计算
static const double sin_table[360] = { /* 预计算的sin值 */ };

// 使用SIMD指令优化向量化计算
#include <immintrin.h>

// 缓存友好数据结构
typedef struct {
    double data[64];  // 缓存行对齐
} CacheAlignedData;
```

#### 问题8: 内存使用过高
**症状**: 程序内存使用超过1GB限制  
**诊断方法**:
```bash
# 使用top或htop监控内存使用
top -p $(pidof program)

# 使用pmap查看内存映射
pmap -x $(pidof program)
```

**解决方案**:
```c
// 使用内存池
typedef struct {
    void* pool;
    size_t block_size;
    size_t total_size;
    size_t used_size;
} MemoryPool;

MemoryPool* memory_pool_create(size_t block_size, size_t count) {
    MemoryPool* pool = malloc(sizeof(MemoryPool));
    pool->pool = malloc(block_size * count);
    pool->block_size = block_size;
    pool->total_size = block_size * count;
    pool->used_size = 0;
    return pool;
}

void* memory_pool_alloc(MemoryPool* pool) {
    if (pool->used_size + pool->block_size > pool->total_size) {
        return NULL;
    }
    void* ptr = (char*)pool->pool + pool->used_size;
    pool->used_size += pool->block_size;
    return ptr;
}

// 使用对象复用
typedef struct {
    ObjectType objects[MAX_OBJECTS];
    int used[MAX_OBJECTS];
    int count;
} ObjectPool;
```

### 4. 算法问题

#### 问题9: 卫星位置计算精度不足
**症状**: 计算结果与预期值偏差较大  
**可能原因**:
- 坐标系转换错误
- 时间计算不准确
- 轨道参数解析错误

**调试方法**:
```c
// 添加调试输出
void debug_satellite_position(const Satellite* satellite) {
    printf("卫星PRN: %d\n", satellite->prn);
    printf("位置: (%.6f, %.6f, %.6f)\n", 
           satellite->position.x, 
           satellite->position.y, 
           satellite->position.z);
    printf("时间: %ld\n", satellite->time);
}

// 使用已知测试数据验证
int test_satellite_position() {
    Satellite sat = {
        .prn = 1,
        .time = known_time,
        .orbital_elements = known_elements
    };
    
    int result = satellite_position_calculate(&sat, known_time);
    
    // 验证计算结果
    double error = distance(sat.position, known_position);
    if (error > MAX_ALLOWED_ERROR) {
        printf("位置计算误差过大: %.6f\n", error);
        return -1;
    }
    
    return 0;
}
```

#### 问题10: 遮挡计算结果不正确
**症状**: 遮挡角度计算结果不符合预期  
**可能原因**:
- 3D几何变换错误
- 射线相交检测算法错误
- 飞机几何模型不准确

**调试方法**:
```c
// 可视化调试
void debug_obstruction_calculation(const AircraftGeometry* geometry,
                                   const SatellitePosition* satellite_pos,
                                   const AircraftState* aircraft_state) {
    printf("飞机位置: (%.6f, %.6f, %.6f)\n",
           aircraft_state->position.x,
           aircraft_state->position.y,
           aircraft_state->position.z);
    
    printf("卫星位置: (%.6f, %.6f, %.6f)\n",
           satellite_pos->x,
           satellite_pos->y,
           satellite_pos->z);
    
    printf("飞机姿态: pitch=%.2f, roll=%.2f, yaw=%.2f\n",
           aircraft_state->attitude.pitch,
           aircraft_state->attitude.roll,
           aircraft_state->attitude.yaw);
}

// 简化测试用例
int test_simple_obstruction() {
    // 使用简单的几何形状进行测试
    AircraftGeometry* geometry = create_simple_geometry();
    SatellitePosition satellite_pos = {0, 0, 1000};  // 正上方
    AircraftState aircraft_state = {0, 0, 0, {0, 0, 0}};
    
    ObstructionResult result;
    int ret = obstruction_calculate(geometry, &satellite_pos, 
                                  &aircraft_state, NULL, &result);
    
    // 对于正上方的卫星，应该没有遮挡
    if (result.is_obstructed) {
        printf("错误: 正上方的卫星被错误地标记为遮挡\n");
        return -1;
    }
    
    return 0;
}
```

### 5. 网络问题

#### 问题11: HTTP服务器无法启动
**症状**: 服务器启动失败，端口被占用  
**解决方案**:
```c
// 检查端口是否被占用
int is_port_available(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return 0;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    int result = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    
    return result == 0;
}

// 使用动态端口分配
int find_available_port(int start_port) {
    for (int port = start_port; port < start_port + 100; port++) {
        if (is_port_available(port)) {
            return port;
        }
    }
    return -1;
}
```

#### 问题12: 客户端连接超时
**症状**: 客户端无法连接到服务器  
**解决方案**:
```c
// 设置合理的超时时间
void set_socket_timeout(int sock, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

// 增加重试机制
int connect_with_retry(const char* host, int port, int max_retries) {
    for (int i = 0; i < max_retries; i++) {
        int sock = create_socket_and_connect(host, port);
        if (sock >= 0) {
            return sock;
        }
        sleep(1);  // 等待1秒后重试
    }
    return -1;
}
```

### 6. 测试问题

#### 问题13: 测试用例失败
**症状**: 单元测试失败，但功能看起来正常  
**解决方案**:
```c
// 检查测试环境设置
void test_setup() {
    // 初始化测试数据
    memset(test_buffer, 0, sizeof(test_buffer));
    
    // 设置已知的初始状态
    test_satellite.prn = 1;
    test_satellite.time = test_time;
    
    // 重置全局状态
    reset_global_state();
}

// 使用断言检查预期结果
void test_satellite_position() {
    Satellite sat = create_test_satellite();
    time_t test_time = 1234567890;
    
    int result = satellite_position_calculate(&sat, test_time);
    
    CuAssertIntEquals(tc, ERROR_SUCCESS, result);
    CuAssertDblEquals(tc, expected_x, sat.position.x, 0.001);
    CuAssertDblEquals(tc, expected_y, sat.position.y, 0.001);
    CuAssertDblEquals(tc, expected_z, sat.position.z, 0.001);
}
```

#### 问题14: 测试覆盖率不足
**症状**: 某些代码路径没有被测试覆盖  
**解决方案**:
```bash
# 使用gcov生成覆盖率报告
gcc -fprofile-arcs -ftest-coverage -o test_program test_program.c
./test_program
gcov test_program.c

# 查看覆盖率报告
cat test_program.c.gcov
```

**补充测试用例**:
```c
// 测试错误处理路径
void test_error_handling() {
    // 测试NULL指针
    int result = satellite_position_calculate(NULL, time);
    CuAssertIntEquals(tc, ERROR_INVALID_PARAMETER, result);
    
    // 测试无效时间
    result = satellite_position_calculate(&satellite, -1);
    CuAssertIntEquals(tc, ERROR_INVALID_PARAMETER, result);
    
    // 测试边界条件
    result = satellite_position_calculate(&satellite, 0);
    CuAssertIntEquals(tc, ERROR_SUCCESS, result);
}
```

### 7. 系统集成问题

#### 问题15: 模块间接口不匹配
**症状**: 模块单独测试通过，但集成测试失败  
**解决方案**:
```c
// 定义接口契约
typedef struct {
    int (*init)(void* config);
    int (*process)(void* input, void* output);
    int (*cleanup)(void);
} ModuleInterface;

// 实现接口适配器
typedef struct {
    ModuleInterface* satellite_module;
    ModuleInterface* aircraft_module;
    ModuleInterface* obstruction_module;
} SystemContext;

// 集成测试
int test_system_integration() {
    SystemContext ctx;
    
    // 初始化所有模块
    ctx.satellite_module = get_satellite_interface();
    ctx.aircraft_module = get_aircraft_interface();
    ctx.obstruction_module = get_obstruction_interface();
    
    // 模拟系统工作流程
    SatelliteData* sat_data = create_satellite_data();
    FlightTrajectory* trajectory = create_trajectory();
    
    // 测试完整的数据流
    int result = process_system_workflow(&ctx, sat_data, trajectory);
    
    // 验证结果
    CuAssertIntEquals(tc, ERROR_SUCCESS, result);
    
    // 清理资源
    destroy_satellite_data(sat_data);
    destroy_trajectory(trajectory);
    
    return 0;
}
```

### 8. 环境问题

#### 问题16: 依赖库缺失
**症状**: 编译时找不到所需的库文件  
**解决方案**:
```bash
# 检查库是否安装
ldconfig -p | grep library_name

# 安装缺失的库
sudo apt-get install liblibrary-name-dev  # Ubuntu/Debian
sudo yum install library-name-devel       # CentOS/RHEL

# 在Makefile中正确配置库路径
LIBS = -lm -lpthread -lculib
LIBDIRS = -L./lib -L/usr/local/lib
```

#### 问题17: 权限问题
**症状**: 程序无法访问某些文件或端口  
**解决方案**:
```bash
# 检查文件权限
ls -la filename

# 修改文件权限
chmod 644 filename
chmod 755 directory

# 检查端口权限
sudo netstat -tlnp | grep :port

# 使用setuid/setgid（谨慎使用）
chmod u+s program  # 设置setuid位
```

### 9. 调试工具

#### 常用调试命令
```bash
# 编译调试版本
gcc -g -O0 -DDEBUG -o program program.c

# 使用gdb调试
gdb ./program
(gdb) break function_name
(gdb) run
(gdb) print variable_name
(gdb) bt
(gdb) continue

# 使用strace跟踪系统调用
strace -o trace.log ./program

# 使用ltrace跟踪库函数调用
ltrace -o ltrace.log ./program

# 使用valgrind检测内存错误
valgrind --tool=memcheck --leak-check=full ./program
```

#### 性能分析工具
```bash
# 使用gprof进行性能分析
gcc -pg -o program program.c
./program
gprof ./program gmon.out > analysis.txt

# 使用perf进行性能分析
perf record ./program
perf report

# 使用time命令测量运行时间
time ./program
```

### 10. 测试套件集成问题

#### 问题18: 完整测试套件运行失败
**症状**: `make test` 命令执行失败，各模块独立测试通过但集成测试失败  
**错误信息**: 测试程序编译成功但运行时出现错误  
**常见原因**:
- 模块间依赖关系冲突
- 全局状态初始化问题
- 内存分配/释放冲突
- 静态变量初始化顺序问题

**诊断方法**:
```bash
# 检查编译警告
make clean && make test 2>&1 | grep -i warning

# 检查模块间的符号冲突
nm build/test_runner.exe | grep ' T ' | sort

# 检查静态初始化问题
gdb -ex run -ex bt --batch ./build/test_runner.exe

# 检查内存问题
valgrind --tool=memcheck --leak-check=full ./build/test_runner.exe
```

**解决方案**:
```c
// 1. 检查模块初始化顺序
void test_module_initialization() {
    // 确保按正确顺序初始化模块
    CuAssertIntEquals(tc, ERROR_SUCCESS, logger_init("test.log", LOG_LEVEL_DEBUG));
    CuAssertIntEquals(tc, ERROR_SUCCESS, satellite_module_init());
    CuAssertIntEquals(tc, ERROR_SUCCESS, aircraft_module_init());
    CuAssertIntEquals(tc, ERROR_SUCCESS, obstruction_module_init());
}

// 2. 隔离测试环境
void test_with_clean_state() {
    // 清理全局状态
    reset_global_state();
    
    // 重新初始化
    init_test_environment();
    
    // 执行测试
    int result = function_under_test();
    
    // 清理
    cleanup_test_environment();
}

// 3. 使用测试替身隔离依赖
typedef struct {
    int (*mock_init)(void);
    int (*mock_process)(void* input, void* output);
    void (*mock_cleanup)(void);
} MockInterface;

int test_with_mock(MockInterface* mock) {
    mock->mock_init();
    int result = mock->mock_process(input, output);
    mock->mock_cleanup();
    return result;
}
```

**预防措施**:
```c
// 模块初始化状态管理
typedef struct {
    int logger_initialized;
    int satellite_initialized;
    int aircraft_initialized;
    int obstruction_initialized;
} ModuleState;

static ModuleState global_module_state = {0};

int ensure_module_initialized(ModuleType type) {
    switch (type) {
        case MODULE_LOGGER:
            if (!global_module_state.logger_initialized) {
                logger_init("test.log", LOG_LEVEL_DEBUG);
                global_module_state.logger_initialized = 1;
            }
            break;
        // 其他模块...
    }
    return ERROR_SUCCESS;
}

// 测试前清理
void test_setup() {
    memset(&global_module_state, 0, sizeof(global_module_state));
    cleanup_all_resources();
}
```

#### 问题19: 模块间接口不匹配
**症状**: 模块单独测试通过，但集成时出现接口错误  
**可能原因**:
- 头文件版本不一致
- 结构体定义差异
- 函数签名不匹配
- 宏定义冲突

**解决方案**:
```c
// 统一接口定义
// 在公共头文件中定义
typedef struct {
    int version;
    int (*init)(const void* config);
    int (*process)(const void* input, void* output);
    int (*cleanup)(void);
} ModuleInterface;

// 版本检查
int check_interface_compatibility(const ModuleInterface* iface) {
    if (iface->version != MODULE_INTERFACE_VERSION) {
        LOG_ERROR("接口版本不匹配: 期望 %d, 实际 %d", 
                 MODULE_INTERFACE_VERSION, iface->version);
        return ERROR_INTERFACE_MISMATCH;
    }
    return ERROR_SUCCESS;
}
```

---

**文档版本**: 1.1  
**创建日期**: 2025-09-06  
**最后更新**: 2025-09-06 19:30:00  
**状态**: 添加了测试套件集成问题解决方案  
**下次更新**: 新的问题解决方案时