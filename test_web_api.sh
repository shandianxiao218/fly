#!/bin/bash

# 北斗导航卫星可见性分析系统 - Web API测试脚本
echo "北斗导航卫星可见性分析系统 - Web API测试"
echo "=========================================="

# 等待服务器启动
echo "等待服务器启动..."
sleep 2

# 测试API端点
echo ""
echo "测试API端点:"

# 测试状态API
echo "1. 测试 /api/status"
curl -s http://localhost:8080/api/status | jq . || echo "  响应: $(curl -s http://localhost:8080/api/status)"

# 测试卫星数据API
echo ""
echo "2. 测试 /api/satellite"
curl -s http://localhost:8080/api/satellite | jq . || echo "  响应: $(curl -s http://localhost:8080/api/satellite)"

# 测试轨迹数据API
echo ""
echo "3. 测试 /api/trajectory"
curl -s http://localhost:8080/api/trajectory | jq . || echo "  响应: $(curl -s http://localhost:8080/api/trajectory)"

# 测试分析结果API
echo ""
echo "4. 测试 /api/analysis"
curl -s http://localhost:8080/api/analysis | jq . || echo "  响应: $(curl -s http://localhost:8080/api/analysis)"

# 测试POST请求 - 生成轨迹
echo ""
echo "5. 测试 POST /api/trajectory"
curl -s -X POST http://localhost:8080/api/trajectory \
     -H "Content-Type: application/json" \
     -d '{"action":"generate","duration":3600}' | jq . || echo "  响应: $(curl -s -X POST http://localhost:8080/api/trajectory -H "Content-Type: application/json" -d '{"action":"generate","duration":3600}')"

# 测试POST请求 - 执行分析
echo ""
echo "6. 测试 POST /api/analysis"
curl -s -X POST http://localhost:8080/api/analysis \
     -H "Content-Type: application/json" \
     -d '{"action":"analyze","satellite_prn":1}' | jq . || echo "  响应: $(curl -s -X POST http://localhost:8080/api/analysis -H "Content-Type: application/json" -d '{"action":"analyze","satellite_prn":1}')"

# 测试不存在的端点
echo ""
echo "7. 测试不存在的端点 /api/unknown"
curl -s http://localhost:8080/api/unknown | jq . || echo "  响应: $(curl -s http://localhost:8080/api/unknown)"

echo ""
echo "API测试完成！"
echo ""
echo "注意：这是一个模拟的HTTP服务器，实际的HTTP网络功能尚未实现。"
echo "当前版本主要验证API逻辑和数据结构的正确性。"