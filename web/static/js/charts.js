/**
 * 北斗导航卫星可见性分析系统 - 图表模块
 * 
 * 本模块负责所有图表的创建和更新，包括：
 * - 卫星可见性饼图
 * - 可见性趋势线图
 * - 轨迹分析图表
 * - 遮挡分析热力图
 * - 3D卫星视图
 * 
 * @author Claude Code
 * @version 1.0
 * @date 2025-09-06
 */

// 图表实例存储
const chartInstances = {};

// 图表配置
const CHART_CONFIG = {
    responsive: true,
    maintainAspectRatio: false,
    animation: {
        duration: 1000,
        easing: 'easeInOutQuart'
    },
    plugins: {
        legend: {
            position: 'top',
            labels: {
                usePointStyle: true,
                padding: 20
            }
        },
        tooltip: {
            backgroundColor: 'rgba(0,0,0,0.8)',
            titleColor: '#fff',
            bodyColor: '#fff',
            borderColor: '#ddd',
            borderWidth: 1,
            cornerRadius: 6,
            displayColors: true
        }
    }
};

// 颜色配置
const COLORS = {
    primary: '#667eea',
    secondary: '#764ba2',
    success: '#28a745',
    warning: '#ffc107',
    danger: '#dc3545',
    info: '#17a2b8',
    light: '#f8f9fa',
    dark: '#343a40',
    gradient: ['#667eea', '#764ba2'],
    satellite: ['#ff6b6b', '#4ecdc4', '#45b7d1', '#96ceb4', '#ffeaa7', '#dda0dd'],
    trajectory: ['#e74c3c', '#3498db', '#2ecc71', '#f39c12', '#9b59b6']
};

/**
 * 创建卫星可见性饼图
 * @param {string} canvasId - Canvas元素ID
 * @param {object} data - 卫星数据
 */
function createSatellitePieChart(canvasId, data) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    
    // 销毁现有图表
    if (chartInstances[canvasId]) {
        chartInstances[canvasId].destroy();
    }

    // 处理数据
    const visibleCount = data.satellites?.filter(s => s.visible).length || 0;
    const invisibleCount = data.satellites?.filter(s => !s.visible).length || 0;

    chartInstances[canvasId] = new Chart(ctx, {
        type: 'doughnut',
        data: {
            labels: ['可见卫星', '不可见卫星'],
            datasets: [{
                data: [visibleCount, invisibleCount],
                backgroundColor: [COLORS.success, COLORS.danger],
                borderWidth: 2,
                borderColor: '#fff'
            }]
        },
        options: {
            ...CHART_CONFIG,
            cutout: '60%',
            plugins: {
                ...CHART_CONFIG.plugins,
                legend: {
                    position: 'bottom',
                    labels: {
                        padding: 15,
                        usePointStyle: true
                    }
                }
            }
        }
    });
}

/**
 * 创建可见性趋势线图
 * @param {string} canvasId - Canvas元素ID
 * @param {object} data - 趋势数据
 */
function createVisibilityTrendChart(canvasId, data) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    
    // 销毁现有图表
    if (chartInstances[canvasId]) {
        chartInstances[canvasId].destroy();
    }

    // 生成时间序列数据
    const timeLabels = [];
    const visibilityData = [];
    const currentTime = new Date();
    
    for (let i = 23; i >= 0; i--) {
        const time = new Date(currentTime.getTime() - i * 60 * 60 * 1000);
        timeLabels.push(time.getHours() + ':00');
        visibilityData.push(Math.floor(Math.random() * 20) + 10); // 模拟数据
    }

    chartInstances[canvasId] = new Chart(ctx, {
        type: 'line',
        data: {
            labels: timeLabels,
            datasets: [{
                label: '可见卫星数量',
                data: visibilityData,
                borderColor: COLORS.primary,
                backgroundColor: COLORS.primary + '20',
                borderWidth: 3,
                fill: true,
                tension: 0.4,
                pointBackgroundColor: COLORS.primary,
                pointBorderColor: '#fff',
                pointBorderWidth: 2,
                pointRadius: 5,
                pointHoverRadius: 7
            }]
        },
        options: {
            ...CHART_CONFIG,
            scales: {
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: '卫星数量'
                    }
                },
                x: {
                    title: {
                        display: true,
                        text: '时间'
                    }
                }
            }
        }
    });
}

/**
 * 创建轨迹分析图表
 * @param {string} canvasId - Canvas元素ID
 * @param {object} data - 轨迹数据
 */
function createTrajectoryChart(canvasId, data) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    
    // 销毁现有图表
    if (chartInstances[canvasId]) {
        chartInstances[canvasId].destroy();
    }

    // 处理轨迹数据
    const trajectories = data.trajectories || [];
    const datasets = trajectories.map((trajectory, index) => ({
        label: `飞行器 ${trajectory.id || index + 1}`,
        data: trajectory.points?.map(point => ({
            x: point.longitude,
            y: point.latitude
        })) || [],
        borderColor: COLORS.trajectory[index % COLORS.trajectory.length],
        backgroundColor: COLORS.trajectory[index % COLORS.trajectory.length] + '40',
        borderWidth: 2,
        pointRadius: 4,
        pointHoverRadius: 6,
        showLine: true,
        tension: 0.1
    }));

    chartInstances[canvasId] = new Chart(ctx, {
        type: 'scatter',
        data: {
            datasets: datasets
        },
        options: {
            ...CHART_CONFIG,
            scales: {
                x: {
                    type: 'linear',
                    position: 'bottom',
                    title: {
                        display: true,
                        text: '经度 (°)'
                    },
                    min: -180,
                    max: 180
                },
                y: {
                    title: {
                        display: true,
                        text: '纬度 (°)'
                    },
                    min: -90,
                    max: 90
                }
            },
            plugins: {
                ...CHART_CONFIG.plugins,
                tooltip: {
                    callbacks: {
                        label: function(context) {
                            return `${context.dataset.label}: (${context.parsed.x.toFixed(2)}°, ${context.parsed.y.toFixed(2)}°)`;
                        }
                    }
                }
            }
        }
    });
}

/**
 * 创建分析结果图表
 * @param {string} canvasId - Canvas元素ID
 * @param {object} data - 分析数据
 */
function createAnalysisChart(canvasId, data) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    
    // 销毁现有图表
    if (chartInstances[canvasId]) {
        chartInstances[canvasId].destroy();
    }

    // 处理分析数据
    const analysisData = data.analysis || {};
    const labels = ['高度角 < 5°', '高度角 5-15°', '高度角 15-30°', '高度角 30-60°', '高度角 > 60°'];
    const values = [
        analysisData.low_elevation || 0,
        analysisData.medium_low_elevation || 0,
        analysisData.medium_elevation || 0,
        analysisData.medium_high_elevation || 0,
        analysisData.high_elevation || 0
    ];

    chartInstances[canvasId] = new Chart(ctx, {
        type: 'bar',
        data: {
            labels: labels,
            datasets: [{
                label: '卫星数量',
                data: values,
                backgroundColor: [
                    COLORS.danger,
                    COLORS.warning,
                    COLORS.info,
                    COLORS.primary,
                    COLORS.success
                ],
                borderColor: [
                    COLORS.danger,
                    COLORS.warning,
                    COLORS.info,
                    COLORS.primary,
                    COLORS.success
                ],
                borderWidth: 2
            }]
        },
        options: {
            ...CHART_CONFIG,
            scales: {
                y: {
                    beginAtZero: true,
                    title: {
                        display: true,
                        text: '卫星数量'
                    }
                },
                x: {
                    title: {
                        display: true,
                        text: '高度角范围'
                    }
                }
            }
        }
    });
}

/**
 * 创建热力图
 * @param {string} canvasId - Canvas元素ID
 * @param {object} data - 热力图数据
 */
function createHeatmapChart(canvasId, data) {
    const ctx = document.getElementById(canvasId).getContext('2d');
    
    // 销毁现有图表
    if (chartInstances[canvasId]) {
        chartInstances[canvasId].destroy();
    }

    // 生成热力图数据
    const heatmapData = data.heatmap || [];
    const labels = ['N', 'NE', 'E', 'SE', 'S', 'SW', 'W', 'NW'];
    
    // 模拟热力图数据
    const values = labels.map(() => Math.floor(Math.random() * 100));

    chartInstances[canvasId] = new Chart(ctx, {
        type: 'polarArea',
        data: {
            labels: labels,
            datasets: [{
                label: '遮挡强度',
                data: values,
                backgroundColor: [
                    COLORS.danger + '80',
                    COLORS.warning + '80',
                    COLORS.info + '80',
                    COLORS.primary + '80',
                    COLORS.success + '80',
                    COLORS.info + '80',
                    COLORS.warning + '80',
                    COLORS.danger + '80'
                ],
                borderColor: [
                    COLORS.danger,
                    COLORS.warning,
                    COLORS.info,
                    COLORS.primary,
                    COLORS.success,
                    COLORS.info,
                    COLORS.warning,
                    COLORS.danger
                ],
                borderWidth: 2
            }]
        },
        options: {
            ...CHART_CONFIG,
            scales: {
                r: {
                    beginAtZero: true,
                    max: 100,
                    title: {
                        display: true,
                        text: '遮挡强度 (%)'
                    }
                }
            }
        }
    });
}

/**
 * 创建3D卫星视图
 * @param {string} containerId - 容器ID
 * @param {object} data - 卫星数据
 */
function create3DSatelliteView(containerId, data) {
    const container = document.getElementById(containerId);
    if (!container) return;

    // 清除现有内容
    container.innerHTML = '';

    // 创建Three.js场景
    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(75, container.clientWidth / container.clientHeight, 0.1, 1000);
    const renderer = new THREE.WebGLRenderer({ antialias: true });
    
    renderer.setSize(container.clientWidth, container.clientHeight);
    renderer.setClearColor(0x000011);
    container.appendChild(renderer.domElement);

    // 添加地球
    const earthGeometry = new THREE.SphereGeometry(5, 32, 32);
    const earthMaterial = new THREE.MeshBasicMaterial({ 
        color: 0x2233ff,
        wireframe: true
    });
    const earth = new THREE.Mesh(earthGeometry, earthMaterial);
    scene.add(earth);

    // 添加卫星
    const satellites = data.satellites || [];
    const satelliteGroup = new THREE.Group();
    
    satellites.forEach((satellite, index) => {
        const satelliteGeometry = new THREE.SphereGeometry(0.2, 8, 8);
        const satelliteMaterial = new THREE.MeshBasicMaterial({ 
            color: satellite.visible ? 0x00ff00 : 0xff0000 
        });
        const satelliteMesh = new THREE.Mesh(satelliteGeometry, satelliteMaterial);
        
        // 将经纬度转换为3D坐标
        const phi = (90 - satellite.latitude) * Math.PI / 180;
        const theta = (satellite.longitude + 180) * Math.PI / 180;
        const radius = 8;
        
        satelliteMesh.position.x = radius * Math.sin(phi) * Math.cos(theta);
        satelliteMesh.position.y = radius * Math.cos(phi);
        satelliteMesh.position.z = radius * Math.sin(phi) * Math.sin(theta);
        
        satelliteGroup.add(satelliteMesh);
    });
    
    scene.add(satelliteGroup);

    // 设置相机位置
    camera.position.z = 15;
    camera.position.y = 5;
    camera.lookAt(0, 0, 0);

    // 添加动画
    function animate() {
        requestAnimationFrame(animate);
        
        earth.rotation.y += 0.005;
        satelliteGroup.rotation.y += 0.002;
        
        renderer.render(scene, camera);
    }
    
    animate();

    // 处理窗口大小变化
    window.addEventListener('resize', () => {
        camera.aspect = container.clientWidth / container.clientHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(container.clientWidth, container.clientHeight);
    });

    // 保存3D场景引用
    window.satellite3DScene = { scene, camera, renderer, earth, satelliteGroup };
}

/**
 * 更新所有图表
 * @param {object} data - 数据对象
 */
function updateAllCharts(data) {
    if (data.satellite) {
        createSatellitePieChart('satellitePieChart', data.satellite);
        createVisibilityTrendChart('visibilityTrendChart', data.satellite);
        create3DSatelliteView('satellite3D', data.satellite);
    }
    
    if (data.trajectory) {
        createTrajectoryChart('trajectoryChart', data.trajectory);
    }
    
    if (data.analysis) {
        createAnalysisChart('analysisChart', data.analysis);
        createHeatmapChart('heatmapChart', data.analysis);
    }
}

/**
 * 销毁所有图表实例
 */
function destroyAllCharts() {
    Object.keys(chartInstances).forEach(key => {
        if (chartInstances[key]) {
            chartInstances[key].destroy();
            delete chartInstances[key];
        }
    });
}

/**
 * 导出图表为图片
 * @param {string} canvasId - Canvas元素ID
 * @param {string} filename - 文件名
 */
function exportChartAsImage(canvasId, filename) {
    const canvas = document.getElementById(canvasId);
    if (canvas && chartInstances[canvasId]) {
        const url = canvas.toDataURL('image/png');
        const link = document.createElement('a');
        link.download = filename || 'chart.png';
        link.href = url;
        link.click();
    }
}

// 导出图表函数
window.ChartManager = {
    createSatellitePieChart,
    createVisibilityTrendChart,
    createTrajectoryChart,
    createAnalysisChart,
    createHeatmapChart,
    create3DSatelliteView,
    updateAllCharts,
    destroyAllCharts,
    exportChartAsImage,
    COLORS,
    CHART_CONFIG
};