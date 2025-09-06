#!/bin/bash

# 北斗导航卫星可见性分析系统 - Web服务器启动脚本
# 用于启动Web前端界面和测试服务器

echo "北斗导航卫星可见性分析系统 - Web服务器启动脚本"
echo "================================================="

# 检查Python是否可用
if ! command -v python3 &> /dev/null; then
    echo "错误: Python3 未安装"
    exit 1
fi

# 检查web目录是否存在
if [ ! -d "web" ]; then
    echo "错误: web目录不存在"
    exit 1
fi

# 检查index.html是否存在
if [ ! -f "web/index.html" ]; then
    echo "错误: web/index.html 不存在"
    exit 1
fi

# 切换到web目录
cd web

# 设置端口
PORT=${1:-8080}

echo "正在启动Web服务器..."
echo "端口: $PORT"
echo "Web根目录: $(pwd)"
echo "按Ctrl+C停止服务器"
echo "================================================="

# 启动Python HTTP服务器
python3 test_server.py $PORT