#!/usr/bin/env python3
"""
简单的HTTP服务器用于测试Web前端界面
支持静态文件服务和模拟API响应
"""

import http.server
import socketserver
import json
import time
import threading
import urllib.parse
from pathlib import Path

class BeidouAPIHandler(http.server.SimpleHTTPRequestHandler):
    """自定义HTTP请求处理器"""
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=".", **kwargs)
    
    def do_GET(self):
        """处理GET请求"""
        parsed_path = urllib.parse.urlparse(self.path)
        path = parsed_path.path
        
        # 检查是否为API请求
        if path.startswith('/api/'):
            self.handle_api_request(path)
        else:
            # 静态文件服务
            super().do_GET()
    
    def handle_api_request(self, path):
        """处理API请求"""
        self.send_response(200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        
        response_data = {}
        
        if path == '/api/status':
            response_data = {
                'timestamp': time.strftime('%Y-%m-%dT%H:%M:%S'),
                'system': {
                    'status': '运行中',
                    'uptime': 3600,
                    'version': '1.0.0',
                    'memory_usage': 45,
                    'cpu_usage': 12.5
                },
                'server': {
                    'port': 8080,
                    'active_connections': 1,
                    'total_requests': 42,
                    'error_count': 0
                },
                'modules': {
                    'satellite': '已加载',
                    'aircraft': '已加载',
                    'obstruction': '已加载',
                    'websocket': '已启用'
                }
            }
            
        elif path == '/api/satellite':
            response_data = {
                'timestamp': time.strftime('%Y-%m-%dT%H:%M:%S'),
                'satellites': []
            }
            
            # 生成12颗卫星的模拟数据
            for i in range(12):
                prn = i + 1
                azimuth = (i * 30) + (hash(time.time()) % 10 - 5)
                elevation = 15 + (hash(time.time()) % 60)
                signal_strength = 35 + (hash(time.time()) % 20)
                visible = (elevation > 10 and signal_strength > 30)
                
                response_data['satellites'].append({
                    'prn': prn,
                    'azimuth': round(azimuth, 1),
                    'elevation': round(elevation, 1),
                    'signal_strength': round(signal_strength, 1),
                    'visible': visible,
                    'obstruction': '无' if visible else '机身遮挡'
                })
                
        elif path == '/api/trajectory':
            import random
            response_data = {
                'timestamp': time.strftime('%Y-%m-%dT%H:%M:%S'),
                'trajectory': {
                    'flight_id': 'TEST001',
                    'aircraft_type': 'B737-800',
                    'current_phase': '巡航',
                    'position': {
                        'latitude': round(39.9042 + random.uniform(-0.01, 0.01), 6),
                        'longitude': round(116.4074 + random.uniform(-0.01, 0.01), 6),
                        'altitude': round(10000 + random.uniform(-1000, 1000), 1)
                    },
                    'attitude': {
                        'roll': round(random.uniform(-5, 5), 1),
                        'pitch': round(random.uniform(-3, 3), 1),
                        'yaw': round(random.uniform(0, 360), 1)
                    },
                    'velocity': {
                        'speed': round(250 + random.uniform(-50, 50), 1),
                        'vertical_speed': round(random.uniform(-10, 10), 1)
                    }
                }
            }
            
        elif path == '/api/analysis':
            import random
            visible_count = 8 + random.randint(0, 4)
            obstructed_count = 12 - visible_count
            
            response_data = {
                'timestamp': time.strftime('%Y-%m-%dT%H:%M:%S'),
                'analysis': {
                    'total_satellites': 12,
                    'visible_satellites': visible_count,
                    'obstructed_satellites': obstructed_count,
                    'avg_signal_strength': round(40 + random.uniform(0, 15), 1),
                    'visibility_stats': {
                        'excellent': max(0, visible_count - 2),
                        'good': 2,
                        'poor': 1,
                        'none': max(0, 12 - visible_count)
                    },
                    'obstruction_stats': {
                        'no_obstruction': max(0, visible_count - 1),
                        'partial_obstruction': 1,
                        'full_obstruction': obstructed_count
                    },
                    'coverage_quality': f"{round((visible_count / 12) * 100, 1)}%"
                }
            }
            
        else:
            response_data = {
                'error': 'API endpoint not found',
                'path': path
            }
            self.send_response(404)
        
        # 发送JSON响应
        response_json = json.dumps(response_data, ensure_ascii=False, indent=2)
        self.wfile.write(response_json.encode('utf-8'))
    
    def log_message(self, format, *args):
        """自定义日志格式"""
        print(f"[{time.strftime('%Y-%m-%d %H:%M:%S')}] {format % args}")

def start_server(port=8080):
    """启动HTTP服务器"""
    handler = BeidouAPIHandler
    
    # 尝试绑定端口，如果失败则尝试其他端口
    max_retries = 10
    for attempt in range(max_retries):
        try:
            with socketserver.TCPServer(("", port), handler) as httpd:
                print(f"北斗导航卫星可见性分析系统 - Web服务器")
                print(f"=========================================")
                print(f"服务器已启动，监听端口: {port}")
                print(f"HTTP服务地址: http://localhost:{port}")
                print(f"按Ctrl+C停止服务器")
                print(f"=========================================")
                
                try:
                    httpd.serve_forever()
                except KeyboardInterrupt:
                    print("\n正在停止服务器...")
                    httpd.shutdown()
                    print("服务器已停止")
                return
        except OSError as e:
            if e.winerror == 10048:  # 端口已被占用
                print(f"端口 {port} 已被占用，尝试端口 {port + 1}")
                port += 1
            else:
                raise
    
    print(f"无法找到可用端口（尝试了 {max_retries} 次）")
    raise RuntimeError("无法启动服务器：无可用端口")

if __name__ == "__main__":
    import sys
    
    port = 8080
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    
    start_server(port)