# 北斗导航卫星可见性分析系统 - Web前端

## 概述

本Web前端提供了北斗导航卫星可见性分析系统的可视化界面，使用Chart.js进行数据可视化，支持实时数据更新和交互式操作。

## 功能特性

- 📊 **实时数据可视化** - 卫星位置、信号强度、可见性分析等
- 🌐 **WebSocket支持** - 实时数据推送和状态更新
- 📱 **响应式设计** - 支持桌面和移动设备
- 🎨 **交互式图表** - 支持缩放、筛选和数据导出
- 🔧 **模拟数据** - 提供测试用的模拟API数据

## 快速开始

### 方法1: 使用Python测试服务器（推荐）

```bash
# 启动Web服务器
cd web
./start_server.sh

# 或者指定端口
./start_server.sh 8080
```

### 方法2: 直接使用Python

```bash
cd web
python3 test_server.py
```

### 方法3: 使用系统自带的服务器

```bash
cd web
python3 -m http.server 8080
```

## 访问方式

启动服务器后，在浏览器中访问：
- **主页**: http://localhost:8080
- **API测试**: http://localhost:8080/api/status

## API接口

### 系统状态
- **URL**: `/api/status`
- **方法**: GET
- **描述**: 获取系统运行状态和统计信息

### 卫星数据
- **URL**: `/api/satellite`
- **方法**: GET
- **描述**: 获取卫星位置和信号数据

### 轨迹数据
- **URL**: `/api/trajectory`
- **方法**: GET
- **描述**: 获取飞行轨迹数据

### 分析结果
- **URL**: `/api/analysis`
- **方法**: GET
- **描述**: 获取可见性和遮挡分析结果

## 界面功能

### 1. 状态栏
- 服务器连接状态
- 系统运行状态
- 在线时间显示

### 2. 控制面板
- WebSocket连接控制
- 数据加载按钮
- 分析操作按钮
- 数据导出功能

### 3. 数据可视化
- **卫星位置分布图** - 散点图显示卫星方位角和仰角
- **信号强度变化图** - 折线图显示信号强度随时间变化
- **可见性分析图** - 柱状图显示可见卫星数量
- **遮挡分析图** - 饼图显示遮挡状态分布

### 4. 实时数据表
- 显示详细的卫星数据
- 包括时间、PRN、仰角、方位角、信号强度等信息
- 支持实时更新

### 5. 系统日志
- 显示系统运行日志
- 支持滚动查看历史日志

## 技术栈

- **前端**: HTML5, CSS3, JavaScript (ES6+)
- **图表库**: Chart.js
- **通信**: WebSocket, Fetch API
- **后端**: Python HTTP服务器 (测试用)
- **样式**: 响应式设计，支持移动设备

## 浏览器兼容性

- Chrome 60+
- Firefox 55+
- Safari 12+
- Edge 79+

## 开发和测试

### 本地开发

```bash
# 启动开发服务器
cd web
python3 test_server.py 8080

# 在浏览器中打开
open http://localhost:8080
```

### 自定义API

如果要连接真实的后端API，需要修改`index.html`中的API地址：

```javascript
// 修改API基础URL
const API_BASE_URL = 'http://your-api-server:8080/api';
```

### 数据格式

#### 卫星数据格式
```json
{
  "timestamp": "2025-09-06T12:00:00",
  "satellites": [
    {
      "prn": 1,
      "azimuth": 45.0,
      "elevation": 30.0,
      "signal_strength": 45.5,
      "visible": true,
      "obstruction": "无"
    }
  ]
}
```

#### 分析数据格式
```json
{
  "timestamp": "2025-09-06T12:00:00",
  "analysis": {
    "total_satellites": 12,
    "visible_satellites": 8,
    "obstructed_satellites": 4,
    "avg_signal_strength": 42.3,
    "coverage_quality": "66.7%"
  }
}
```

## 故障排除

### 常见问题

1. **无法访问页面**
   - 检查服务器是否启动
   - 检查端口是否被占用
   - 检查防火墙设置

2. **图表不显示**
   - 检查网络连接，确保Chart.js库加载正常
   - 检查浏览器控制台错误信息

3. **WebSocket连接失败**
   - 检查WebSocket服务是否启动
   - 检查WebSocket端口配置

4. **API数据无法加载**
   - 检查API服务是否正常运行
   - 检查CORS设置

### 调试模式

在浏览器中打开开发者工具（F12），查看：
- Console标签页：JavaScript错误信息
- Network标签页：网络请求状态
- Application标签页：本地存储和缓存

## 性能优化

- 图表数据更新频率建议不超过1秒
- 大量数据时建议使用数据分页
- 启用浏览器缓存以减少重复加载

## 安全注意事项

- 仅在可信网络环境中使用
- 不要在生产环境中使用测试服务器
- 定期更新依赖库版本

## 下一步开发计划

1. 集成真实的C语言后端API
2. 添加用户认证和权限管理
3. 实现数据持久化存储
4. 添加更多的可视化图表类型
5. 优化移动端体验

## 贡献

欢迎提交Issue和Pull Request来改进本项目。

## 许可证

本项目采用MIT许可证。