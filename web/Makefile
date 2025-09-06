# Web服务器Makefile
# 用于编译Web前端界面和HTTP服务器

# 编译器和选项
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -I./src
LDFLAGS = -lpthread -lm

# 目录
SRC_DIR = src
WEB_DIR = web
BUILD_DIR = build
TEST_DIR = tests

# 源文件
WEBSERVER_SRC = $(WEB_DIR)/server.c

# 依赖库源文件
SATELLITE_SRC = $(SRC_DIR)/satellite/satellite.c \
                $(SRC_DIR)/satellite/rinex_parser.c \
                $(SRC_DIR)/satellite/coordinate.c

AIRCRAFT_SRC = $(SRC_DIR)/aircraft/aircraft.c \
                $(SRC_DIR)/aircraft/trajectory.c \
                $(SRC_DIR)/aircraft/attitude.c \
                $(SRC_DIR)/aircraft/csv_parser.c

OBSTRUCTION_SRC = $(SRC_DIR)/obstruction/obstruction.c \
                  $(SRC_DIR)/obstruction/geometry.c \
                  $(SRC_DIR)/obstruction/vector3d.c \
                  $(SRC_DIR)/obstruction/ray.c \
                  $(SRC_DIR)/obstruction/box.c \
                  $(SRC_DIR)/obstruction/aircraft_geometry.c

WEBSOCKET_SRC = $(SRC_DIR)/web/websocket.c

# Web模块源文件
WEB_SRC = $(SRC_DIR)/web/http_server.c \
          $(SRC_DIR)/web/api.c \
          $(SRC_DIR)/web/json_utils.c

# 工具库源文件
UTILS_SRC = $(SRC_DIR)/utils/utils.c \
            $(SRC_DIR)/utils/logger.c \
            $(SRC_DIR)/utils/memory.c \
            $(SRC_DIR)/utils/time_utils.c

# 目标文件
WEBSERVER_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(WEBSERVER_SRC))
SATELLITE_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SATELLITE_SRC))
AIRCRAFT_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(AIRCRAFT_SRC))
OBSTRUCTION_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(OBSTRUCTION_SRC))
WEBSOCKET_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(WEBSOCKET_SRC))
WEB_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(WEB_SRC))
UTILS_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(UTILS_SRC))

# 可执行文件
WEBSERVER_TARGET = $(BUILD_DIR)/web_server

# 默认目标
all: directories $(WEBSERVER_TARGET)

# 创建目录
directories:
	@mkdir -p $(BUILD_DIR)/$(WEB_DIR)
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR)/satellite
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR)/aircraft
	@mkdir -p $(BUILD_DIR)/$(SRC_DIR)/obstruction
	@mkdir -p $(BUILD_DIR)/tests

# 编译Web服务器
$(WEBSERVER_TARGET): $(WEBSERVER_OBJ) $(SATELLITE_OBJ) $(AIRCRAFT_OBJ) $(OBSTRUCTION_OBJ) $(WEBSOCKET_OBJ) $(WEB_OBJ) $(UTILS_OBJ)
	@echo "链接Web服务器..."
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 编译规则
$(BUILD_DIR)/%.o: %.c
	@echo "编译 $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 特殊规则处理当前目录下的文件
$(BUILD_DIR)/$(WEB_DIR)/%.o: %.c
	@echo "编译 $<..."
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 运行Web服务器
run: $(WEBSERVER_TARGET)
	@echo "启动Web服务器..."
	@cd $(WEB_DIR) && ../$(WEBSERVER_TARGET)

# 运行测试
test: $(WEBSERVER_TARGET)
	@echo "运行Web服务器测试..."
	@cd $(WEB_DIR) && ../$(WEBSERVER_TARGET) -h

# 清理
clean:
	@echo "清理编译文件..."
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/$(WEB_DIR)/*.o $(BUILD_DIR)/$(SRC_DIR)/*.o
	rm -f $(WEBSERVER_TARGET)

# 完全清理
distclean: clean
	@echo "完全清理..."
	rm -rf $(BUILD_DIR)

# 安装
install: $(WEBSERVER_TARGET)
	@echo "安装Web服务器..."
	@mkdir -p /usr/local/bin
	@cp $(WEBSERVER_TARGET) /usr/local/bin/beidou-web-server
	@chmod +x /usr/local/bin/beidou-web-server

# 开发模式
dev: $(WEBSERVER_TARGET)
	@echo "开发模式启动Web服务器..."
	@cd $(WEB_DIR) && ../$(WEBSERVER_TARGET) -p 8080 -d . -v

# 调试模式
debug: CFLAGS += -g -DDEBUG
debug: $(WEBSERVER_TARGET)
	@echo "调试模式启动Web服务器..."
	@cd $(WEB_DIR) && ../$(WEBSERVER_TARGET) -p 8080 -d . -v

# 帮助
help:
	@echo "可用的make目标:"
	@echo "  all        - 编译所有目标"
	@echo "  run        - 运行Web服务器"
	@echo "  test       - 运行测试"
	@echo "  clean      - 清理编译文件"
	@echo "  distclean  - 完全清理"
	@echo "  install    - 安装到系统"
	@echo "  dev        - 开发模式启动"
	@echo "  debug      - 调试模式启动"
	@echo "  help       - 显示帮助"

.PHONY: all directories run test clean distclean install dev debug help