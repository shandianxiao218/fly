# 项目目录结构设计

## 整体结构

```
北斗导航卫星可见性分析系统/
├── .claude/                    # Claude AI 配置目录
├── .git/                       # Git 版本控制
├── .vscode/                    # VS Code 配置
├── archive/                    # 归档文件
├── build/                      # 构建输出目录
├── data/                       # 数据文件
├── docs/                       # 文档目录
├── lib/                        # 第三方库
├── specs/                      # 规格说明
├── src/                        # 源代码目录
│   ├── aircraft/               # 飞机模块
│   ├── obstruction/            # 遮挡检测模块
│   ├── satellite/              # 卫星模块
│   ├── utils/                  # 工具模块
│   ├── web/                    # Web 模块
│   └── main.c                  # 主程序入口
├── tests/                      # 测试目录
│   ├── unit/                   # 单元测试
│   ├── integration/            # 集成测试
│   ├── examples/               # 示例代码
│   └── temporary/              # 临时测试文件
├── web/                        # Web 资源
│   └── static/                 # 静态文件
├── CuTest/                     # 测试框架
├── CLAUDE.md                   # Claude 项目配置
├── CLAUDE-*.md                 # Claude 内存银行系统
├── Makefile                    # 构建脚本
├── README.md                   # 项目说明
└── LICENSE                     # 许可证
```

## 文件分类和移动计划

### 1. 根目录临时文件 -> tests/temporary/
- `api_test.c` -> `tests/temporary/api_test.c`
- `test_aircraft.c` -> `tests/temporary/test_aircraft.c`
- `test_obstruction_simple.c` -> `tests/temporary/test_obstruction_simple.c`
- `web_server_simple_test.c` -> `tests/temporary/web_server_simple_test.c`
- `web_server_test.c` -> `tests/temporary/web_server_test.c`

### 2. WebSocket 测试文件 -> tests/examples/websocket/
- `test_websocket_*.c` -> `tests/examples/websocket/`

### 3. 编译产物清理
- 删除根目录的所有 `.o` 文件
- 删除根目录的所有 `.exe` 文件
- 删除根目录的所有 `.log` 文件

### 4. 重复文件处理
- `CuTest/` 和 `lib/CuTest.*` 重复，保留 `CuTest/` 目录

### 5. 配置文件保持位置
- 所有 `CLAUDE-*.md` 文件保持在根目录
- `Makefile` 保持在根目录
- `README.md` 和 `LICENSE` 保持在根目录

## 目录结构说明

### src/ - 源代码目录
每个模块都有自己的子目录，包含对应的 `.c` 和 `.h` 文件。

### tests/ - 测试目录
- `unit/` - 单元测试
- `integration/` - 集成测试
- `examples/` - 示例代码
- `temporary/` - 临时测试文件（可随时删除）

### build/ - 构建输出
所有编译产物（`.o`, `.exe`, `.log`）都应该在这里。

### docs/ - 项目文档
项目相关的文档和说明。

### specs/ - 规格说明
需求文档和追踪信息。

### data/ - 数据文件
测试数据和配置数据。

### web/ - Web 资源
Web 模块的静态资源。

### archive/ - 归档文件
不再活跃的文档和计划。

## 维护规则

1. 新文件必须按照结构放置在对应目录
2. 根目录只保留必要的配置和说明文件
3. 编译产物必须放在 `build/` 目录
4. 临时测试文件放在 `tests/temporary/` 目录
5. 定期清理 `build/` 和 `tests/temporary/` 目录
6. 更新目录结构时同步更新相关文档