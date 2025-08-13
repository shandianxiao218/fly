# 北斗导航卫星可见性分析系统

基于C语言开发的专业北斗导航卫星可见性分析系统，采用TDD（测试驱动开发）方法构建。

## 功能特性

- 🛰️ **真实星历数据** - 支持RINEX 3.0格式星历数据解析
- ✈️ **轨迹仿真** - 自动生成飞行轨迹，每秒更新姿态数据
- 🚫 **精确遮挡** - 1度精度的天线遮挡计算
- 📊 **实时可视化** - Chart.js驱动的Web界面显示
- 🔧 **模块化架构** - 基于subagent的模块化设计
- ⚡ **高性能** - C语言实现，满足实时性要求

## 技术架构

### 后端技术栈
- **C语言** - 核心算法和数据处理
- **CuTest** - 单元测试框架
- **libcurl** - HTTP服务器功能
- **jansson** - JSON数据处理

### 前端技术栈
- **Chart.js** - 数据可视化
- **HTML5/CSS3** - 用户界面
- **JavaScript** - 前端逻辑

### 开发方法
- **TDD** - 测试驱动开发
- **Git** - 版本控制
- **模块化** - subagent架构

## 快速开始

### 编译和运行

```bash
# 克隆仓库
git clone https://github.com/your-repo/beidou-analysis.git
cd beidou-analysis

# 编译项目
make

# 运行测试
make test

# 启动服务器
./bin/beidou-server

# 访问Web界面
# 打开浏览器访问 http://localhost:8080
```

### 使用方法

1. **启动系统**
   ```bash
   ./bin/beidou-server
   ```

2. **导入星历数据**
   - 通过Web界面上传RINEX文件
   - 或使用命令行：`./bin/beidou-server --rinex data/rinex/example.rnx`

3. **配置飞行轨迹**
   - 上传CSV轨迹文件
   - 或使用系统自动生成的轨迹

4. **开始分析**
   - 点击"开始分析"按钮
   - 实时查看卫星可见性变化

## 数据格式

### RINEX星历数据
支持RINEX 3.0格式，包含北斗卫星轨道参数。

### CSV轨迹文件
```csv
timestamp,latitude,longitude,altitude,pitch,roll,yaw,velocity
2024-01-01T00:00:00Z,39.9042,116.4074,10000,0.0,0.0,0.0,250.0
2024-01-01T00:00:01Z,39.9045,116.4078,10000,0.5,0.2,0.1,250.5
```

## 项目结构

```
beidou-analysis/
├── src/                    # 源代码
│   ├── satellite/         # 卫星相关模块
│   ├── aircraft/          # 飞机相关模块
│   ├── obstruction/       # 遮挡计算模块
│   ├── web/              # Web服务器模块
│   └── utils/            # 工具函数
├── tests/                 # 测试代码
│   ├── unit/             # 单元测试
│   └── integration/      # 集成测试
├── web/                  # Web前端
│   ├── static/          # 静态文件
│   └── templates/       # HTML模板
├── data/                 # 数据文件
│   ├── rinex/           # RINEX示例
│   └── trajectories/    # 轨迹示例
├── lib/                  # 第三方库
├── build/               # 构建输出
├── docs/                # 文档
├── Makefile            # 构建脚本
└── README.md            # 说明文档
```

## 开发指南

### 环境要求
- **操作系统**: Windows 10/11
- **编译器**: GCC/MinGW
- **构建工具**: Make
- **测试框架**: CuTest

### 开发流程
1. **TDD开发**
   - 编写失败的测试
   - 实现代码使测试通过
   - 重构优化代码

2. **Git提交**
   - 每完成一个功能点立即提交
   - 遵循提交信息规范
   - 确保测试通过

3. **代码审查**
   - 使用code-reviewer agent
   - 确保代码质量
   - 性能优化检查

### 添加新功能
1. 在对应的tests目录编写测试
2. 实现功能代码
3. 运行测试确保通过
4. 提交代码并创建PR

## 系统要求

- **操作系统**: Windows 10/11
- **内存**: 最少4GB RAM
- **存储**: 最少1GB可用空间
- **网络**: 可选（用于下载星历数据）

## 性能指标

- **卫星位置计算**: < 10ms per satellite
- **可见性分析**: < 50ms per time step
- **遮挡计算**: < 100ms per time step
- **更新频率**: 1Hz（每秒更新）
- **内存使用**: < 1GB
- **CPU使用率**: < 50%

## 贡献指南

1. Fork项目
2. 创建功能分支
3. 遵循TDD开发流程
4. 确保所有测试通过
5. 提交Pull Request

## 许可证

MIT License

## 作者

AI助手 - 北斗导航卫星可见性分析系统

## 更新日志

### v2.0.0 (当前版本)
- 迁移到C语言实现
- 采用TDD开发方法
- 添加RINEX格式支持
- 实现精确遮挡计算
- 集成Chart.js可视化

### v1.0.0 (已废弃)
- JavaScript版本
- 基础功能实现