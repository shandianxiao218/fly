# 北斗导航卫星可见性分析系统 - Makefile
# 基于C语言和TDD开发方法

# 编译器和标志
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -g
INCLUDES = -I./lib -I./src
LIBS = -lm -lpthread

# 源文件目录
SRC_DIR = src
TEST_DIR = tests
LIB_DIR = lib
BUILD_DIR = build

# 源文件
SATELLITE_SRC = $(SRC_DIR)/satellite/satellite.c
AIRCRAFT_SRC = $(SRC_DIR)/aircraft/trajectory.c $(SRC_DIR)/aircraft/attitude.c $(SRC_DIR)/aircraft/csv_parser.c
OBSTRUCTION_SRC = $(SRC_DIR)/obstruction/geometry.c $(SRC_DIR)/obstruction/obstruction.c $(SRC_DIR)/obstruction/aircraft_model.c
WEB_SRC = $(SRC_DIR)/web/http_server.c $(SRC_DIR)/web/api.c $(SRC_DIR)/web/json_utils.c $(SRC_DIR)/web/websocket.c
UTILS_SRC = $(SRC_DIR)/utils/utils.c $(SRC_DIR)/utils/logger.c

# 主程序文件
MAIN_SRC = $(SRC_DIR)/main.c

# 模块源文件（不包括main.c）
MODULE_SRC = $(SATELLITE_SRC) $(AIRCRAFT_SRC) $(OBSTRUCTION_SRC) $(WEB_SRC) $(UTILS_SRC)

# 所有源文件
ALL_SRC = $(MAIN_SRC) $(MODULE_SRC)

# 测试文件
TEST_FILES = $(TEST_DIR)/unit/test_all.c

# 目标文件
TARGET = $(BUILD_DIR)/beidou-server
TEST_TARGET = $(BUILD_DIR)/test_runner
WEB_TEST_TARGET = $(BUILD_DIR)/web_server_test

# 默认目标
all: setup build test

# 构建utils模块
utils: $(UTILS_SRC)
	@echo "编译utils模块..."
	$(CC) $(CFLAGS) $(INCLUDES) -c $(UTILS_SRC)
	@echo "utils模块编译完成"

# 设置环境
setup:
	@echo "设置开发环境..."
	@mkdir -p $(BUILD_DIR)
	@echo "环境设置完成"

# 编译主程序
build: $(TARGET)

$(TARGET): $(ALL_SRC)
	@echo "编译主程序..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

# 编译测试程序
test: $(TEST_TARGET)
	@echo "运行测试..."
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_FILES) $(LIB_DIR)/CuTest.c
	@echo "编译测试程序..."
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(MODULE_SRC) $(LIBS)
	@echo "测试编译完成: $(TEST_TARGET)"

# 清理
clean:
	@echo "清理构建文件..."
	rm -rf $(BUILD_DIR)/*.o $(TARGET) $(TEST_TARGET)
	@echo "清理完成"

# 运行程序
run: $(TARGET)
	@echo "启动北斗导航卫星可见性分析系统..."
	./$(TARGET)

# 安装依赖
install:
	@echo "检查并安装依赖..."
	@echo "CuTest已包含在项目中"
	@echo "依赖检查完成"

# 开发帮助
help:
	@echo "北斗导航卫星可见性分析系统 - 开发帮助"
	@echo ""
	@echo "可用命令:"
	@echo "  make setup    - 设置开发环境"
	@echo "  make build    - 编译主程序"
	@echo "  make test     - 编译并运行测试"
	@echo "  make run      - 运行程序"
	@echo "  make clean    - 清理构建文件"
	@echo "  make install  - 安装依赖"
	@echo "  make help     - 显示帮助信息"
	@echo ""
	@echo "TDD开发流程:"
	@echo "  1. 编写失败的测试"
	@echo "  2. 实现代码使测试通过"
	@echo "  3. 重构优化代码"
	@echo "  4. 提交代码"

# 编译web服务器测试程序
web_test: $(WEB_TEST_TARGET)

$(WEB_TEST_TARGET): $(UTILS_SRC) $(WEB_SRC) web_server_test.c
	@echo "编译web服务器测试程序..."
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) -lpthread
	@echo "web服务器测试程序编译完成: $@"

# 运行web服务器测试程序
run_web_test: $(WEB_TEST_TARGET)
	@echo "运行web服务器测试程序..."
	./$(WEB_TEST_TARGET) 8080 localhost

.PHONY: all setup build test clean run install help web_test run_web_test