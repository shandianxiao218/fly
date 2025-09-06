/**
 * 北斗导航卫星可见性分析系统 - 主应用模块
 * 
 * 本模块是系统的核心控制器，负责：
 * - 应用初始化
 * - 视图管理
 * - 数据更新
 * - 用户交互处理
 * - 系统状态监控
 * 
 * @author Claude Code
 * @version 1.0
 * @date 2025-09-06
 */

// 全局应用状态
const AppState = {
    currentView: 'dashboard',
    autoRefresh: true,
    refreshInterval: 5000,
    refreshTimer: null,
    lastUpdate: null,
    systemStatus: 'unknown',
    isInitialized: false
};

// 缓存的数据
const DataCache = {
    satellite: null,
    trajectory: null,
    analysis: null,
    status: null
};

/**
 * 应用初始化
 */
async function initializeApp() {
    console.log('正在初始化北斗导航卫星可见性分析系统...');
    
    try {
        // 显示加载状态
        showLoading(true);
        
        // 检查服务器连接
        const isConnected = await ApiClient.checkServerConnection();
        if (!isConnected) {
            showMessage('无法连接到服务器，请确保后端服务正在运行', 'error');
            updateSystemStatus('error');
            return;
        }
        
        // 初始化UI组件
        initializeUIComponents();
        
        // 绑定事件监听器
        bindEventListeners();
        
        // 加载初始数据
        await loadInitialData();
        
        // 启动自动刷新
        startAutoRefresh();
        
        // 标记为已初始化
        AppState.isInitialized = true;
        
        showMessage('系统初始化完成', 'success');
        updateSystemStatus('online');
        
    } catch (error) {
        console.error('应用初始化失败:', error);
        showMessage('系统初始化失败: ' + error.message, 'error');
        updateSystemStatus('error');
    } finally {
        showLoading(false);
    }
}

/**
 * 初始化UI组件
 */
function initializeUIComponents() {
    // 初始化图表容器
    const charts = [
        'satellitePieChart',
        'visibilityTrendChart', 
        'trajectoryChart',
        'analysisChart',
        'heatmapChart'
    ];
    
    charts.forEach(chartId => {
        const canvas = document.getElementById(chartId);
        if (canvas) {
            canvas.style.height = '300px';
        }
    });
    
    // 设置默认值
    document.getElementById('autoRefresh').checked = AppState.autoRefresh;
    document.getElementById('refreshInterval').value = AppState.refreshInterval;
}

/**
 * 绑定事件监听器
 */
function bindEventListeners() {
    // 自动刷新开关
    document.getElementById('autoRefresh').addEventListener('change', (e) => {
        AppState.autoRefresh = e.target.checked;
        if (AppState.autoRefresh) {
            startAutoRefresh();
        } else {
            stopAutoRefresh();
        }
    });
    
    // 刷新间隔变更
    document.getElementById('refreshInterval').addEventListener('change', (e) => {
        AppState.refreshInterval = parseInt(e.target.value);
        if (AppState.autoRefresh) {
            stopAutoRefresh();
            startAutoRefresh();
        }
    });
    
    // 数据生成按钮
    document.getElementById('generateDataBtn')?.addEventListener('click', generateData);
    
    // 显示选项变更
    ['show3D', 'showGrid', 'showLabels'].forEach(optionId => {
        document.getElementById(optionId)?.addEventListener('change', updateDisplayOptions);
    });
    
    // 窗口大小变化
    window.addEventListener('resize', debounce(handleResize, 250));
}

/**
 * 加载初始数据
 */
async function loadInitialData() {
    try {
        // 批量加载所有数据
        const requests = [
            { type: 'status' },
            { type: 'satellite' },
            { type: 'trajectory' },
            { type: 'analysis' }
        ];
        
        const results = await ApiClient.batchRequests(requests);
        
        if (results.success) {
            // 更新缓存
            DataCache.status = results.data[0];
            DataCache.satellite = results.data[1];
            DataCache.trajectory = results.data[2];
            DataCache.analysis = results.data[3];
            
            // 更新UI
            updateUI();
            showMessage('数据加载完成', 'success');
        } else {
            throw new Error(results.error);
        }
        
    } catch (error) {
        console.error('加载初始数据失败:', error);
        showMessage('数据加载失败: ' + error.message, 'error');
    }
}

/**
 * 更新UI
 */
function updateUI() {
    // 更新状态栏
    updateStatusBar();
    
    // 更新图表
    updateCharts();
    
    // 更新卫星表格
    updateSatelliteTable();
    
    // 更新时间戳
    AppState.lastUpdate = new Date();
    document.getElementById('lastUpdate').textContent = AppState.lastUpdate.toLocaleTimeString();
}

/**
 * 更新状态栏
 */
function updateStatusBar() {
    // 系统状态
    const statusBadge = document.getElementById('systemStatus');
    if (DataCache.status?.success) {
        statusBadge.textContent = '在线';
        statusBadge.className = 'badge bg-success';
    } else {
        statusBadge.textContent = '离线';
        statusBadge.className = 'badge bg-danger';
    }
    
    // 可见卫星数量
    const visibleCount = DataCache.satellite?.data?.satellites?.filter(s => s.visible).length || 0;
    document.getElementById('visibleSatellites').textContent = visibleCount;
    
    // 飞行器数量
    const aircraftCount = DataCache.trajectory?.data?.trajectories?.length || 0;
    document.getElementById('aircraftCount').textContent = aircraftCount;
}

/**
 * 更新图表
 */
function updateCharts() {
    const chartData = {
        satellite: DataCache.satellite?.data,
        trajectory: DataCache.trajectory?.data,
        analysis: DataCache.analysis?.data
    };
    
    ChartManager.updateAllCharts(chartData);
}

/**
 * 更新卫星表格
 */
function updateSatelliteTable() {
    const tableBody = document.getElementById('satelliteTable');
    if (!tableBody) return;
    
    const satellites = DataCache.satellite?.data?.satellites || [];
    
    if (satellites.length === 0) {
        tableBody.innerHTML = '<tr><td colspan="6" class="text-center">暂无数据</td></tr>';
        return;
    }
    
    tableBody.innerHTML = satellites.map(satellite => `
        <tr>
            <td>${satellite.id || 'N/A'}</td>
            <td>${satellite.longitude?.toFixed(2) || 'N/A'}°</td>
            <td>${satellite.latitude?.toFixed(2) || 'N/A'}°</td>
            <td>${satellite.altitude?.toFixed(2) || 'N/A'} km</td>
            <td>
                <span class="badge ${satellite.visible ? 'bg-success' : 'bg-danger'}">
                    ${satellite.visible ? '可见' : '不可见'}
                </span>
            </td>
            <td>
                <div class="progress" style="height: 20px;">
                    <div class="progress-bar ${getSignalStrengthClass(satellite.signal_strength)}" 
                         style="width: ${satellite.signal_strength || 0}%">
                        ${satellite.signal_strength || 0}%
                    </div>
                </div>
            </td>
        </tr>
    `).join('');
}

/**
 * 获取信号强度样式类
 * @param {number} strength - 信号强度 (0-100)
 * @returns {string} - 样式类名
 */
function getSignalStrengthClass(strength) {
    if (strength >= 80) return 'bg-success';
    if (strength >= 60) return 'bg-info';
    if (strength >= 40) return 'bg-warning';
    return 'bg-danger';
}

/**
 * 启动自动刷新
 */
function startAutoRefresh() {
    if (AppState.refreshTimer) {
        clearInterval(AppState.refreshTimer);
    }
    
    AppState.refreshTimer = setInterval(async () => {
        await refreshData();
    }, AppState.refreshInterval);
    
    console.log(`自动刷新已启动，间隔: ${AppState.refreshInterval}ms`);
}

/**
 * 停止自动刷新
 */
function stopAutoRefresh() {
    if (AppState.refreshTimer) {
        clearInterval(AppState.refreshTimer);
        AppState.refreshTimer = null;
        console.log('自动刷新已停止');
    }
}

/**
 * 刷新数据
 */
async function refreshData() {
    try {
        showLoading(true);
        
        // 批量获取最新数据
        const requests = [
            { type: 'status' },
            { type: 'satellite' },
            { type: 'trajectory' },
            { type: 'analysis' }
        ];
        
        const results = await ApiClient.batchRequests(requests);
        
        if (results.success) {
            // 更新缓存
            DataCache.status = results.data[0];
            DataCache.satellite = results.data[1];
            DataCache.trajectory = results.data[2];
            DataCache.analysis = results.data[3];
            
            // 更新UI
            updateUI();
            
        } else {
            console.error('数据刷新失败:', results.error);
        }
        
    } catch (error) {
        console.error('刷新数据失败:', error);
    } finally {
        showLoading(false);
    }
}

/**
 * 生成测试数据
 */
async function generateData() {
    try {
        showLoading(true);
        
        const params = {
            satellite_count: 12,
            trajectory_count: 3,
            analysis_type: 'visibility'
        };
        
        const result = await ApiClient.generateTestData(params);
        
        if (result.success) {
            showMessage('测试数据生成成功', 'success');
            // 刷新数据
            await refreshData();
        } else {
            throw new Error(result.error);
        }
        
    } catch (error) {
        console.error('生成测试数据失败:', error);
        showMessage('生成测试数据失败: ' + error.message, 'error');
    } finally {
        showLoading(false);
    }
}

/**
 * 更新显示选项
 */
function updateDisplayOptions() {
    const show3D = document.getElementById('show3D')?.checked;
    const showGrid = document.getElementById('showGrid')?.checked;
    const showLabels = document.getElementById('showLabels')?.checked;
    
    // 更新3D视图显示
    const satellite3D = document.getElementById('satellite3D');
    if (satellite3D) {
        satellite3D.style.display = show3D ? 'block' : 'none';
    }
    
    // 这里可以添加更多的显示选项处理
    console.log('显示选项更新:', { show3D, showGrid, showLabels });
}

/**
 * 更新系统状态
 * @param {string} status - 系统状态
 */
function updateSystemStatus(status) {
    AppState.systemStatus = status;
    const statusBadge = document.getElementById('systemStatus');
    
    if (statusBadge) {
        switch (status) {
            case 'online':
                statusBadge.textContent = '在线';
                statusBadge.className = 'badge bg-success';
                break;
            case 'offline':
                statusBadge.textContent = '离线';
                statusBadge.className = 'badge bg-danger';
                break;
            case 'error':
                statusBadge.textContent = '错误';
                statusBadge.className = 'badge bg-danger';
                break;
            default:
                statusBadge.textContent = '检查中...';
                statusBadge.className = 'badge bg-warning';
        }
    }
}

/**
 * 显示加载状态
 * @param {boolean} show - 是否显示
 */
function showLoading(show) {
    // 这里可以添加加载动画的显示/隐藏逻辑
    console.log('Loading state:', show);
}

/**
 * 显示消息提示
 * @param {string} message - 消息内容
 * @param {string} type - 消息类型
 */
function showMessage(message, type = 'info') {
    const toastEl = document.getElementById('messageToast');
    const toastMessage = document.getElementById('toastMessage');
    
    if (toastEl && toastMessage) {
        toastMessage.textContent = message;
        
        // 设置消息类型样式
        const toastHeader = toastEl.querySelector('.toast-header');
        toastHeader.className = 'toast-header';
        
        switch (type) {
            case 'success':
                toastHeader.classList.add('bg-success', 'text-white');
                break;
            case 'error':
                toastHeader.classList.add('bg-danger', 'text-white');
                break;
            case 'warning':
                toastHeader.classList.add('bg-warning', 'text-white');
                break;
            default:
                toastHeader.classList.add('bg-info', 'text-white');
        }
        
        const toast = new bootstrap.Toast(toastEl);
        toast.show();
    }
}

/**
 * 切换视图
 * @param {string} viewName - 视图名称
 */
function switchView(viewName) {
    // 隐藏所有视图
    document.querySelectorAll('.view-section').forEach(section => {
        section.style.display = 'none';
    });
    
    // 显示指定视图
    const targetView = document.getElementById(viewName + 'View');
    if (targetView) {
        targetView.style.display = 'block';
        AppState.currentView = viewName;
        
        // 更新导航栏状态
        document.querySelectorAll('.nav-link').forEach(link => {
            link.classList.remove('active');
        });
        
        event.target.classList.add('active');
    }
}

/**
 * 处理窗口大小变化
 */
function handleResize() {
    // 重新调整图表大小
    Object.keys(window.chartInstances || {}).forEach(chartId => {
        if (window.chartInstances[chartId]) {
            window.chartInstances[chartId].resize();
        }
    });
    
    // 重新调整3D视图
    if (window.satellite3DScene) {
        const { renderer, camera, container } = window.satellite3DScene;
        const containerEl = document.getElementById('satellite3D');
        if (containerEl && renderer && camera) {
            camera.aspect = containerEl.clientWidth / containerEl.clientHeight;
            camera.updateProjectionMatrix();
            renderer.setSize(containerEl.clientWidth, containerEl.clientHeight);
        }
    }
}

/**
 * 防抖函数
 * @param {function} func - 要防抖的函数
 * @param {number} wait - 等待时间
 * @returns {function} - 防抖后的函数
 */
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

/**
 * 导出数据
 * @param {string} format - 导出格式
 */
function exportData(format = 'json') {
    const exportData = {
        timestamp: new Date().toISOString(),
        satellite: DataCache.satellite,
        trajectory: DataCache.trajectory,
        analysis: DataCache.analysis,
        status: DataCache.status
    };
    
    let content, filename, mimeType;
    
    switch (format) {
        case 'json':
            content = JSON.stringify(exportData, null, 2);
            filename = `satellite_data_${new Date().toISOString().split('T')[0]}.json`;
            mimeType = 'application/json';
            break;
        case 'csv':
            content = convertToCSV(exportData);
            filename = `satellite_data_${new Date().toISOString().split('T')[0]}.csv`;
            mimeType = 'text/csv';
            break;
        default:
            showMessage('不支持的导出格式', 'error');
            return;
    }
    
    const blob = new Blob([content], { type: mimeType });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = filename;
    link.click();
    URL.revokeObjectURL(url);
    
    showMessage(`数据已导出为 ${format.toUpperCase()} 格式`, 'success');
}

/**
 * 转换为CSV格式
 * @param {object} data - 数据对象
 * @returns {string} - CSV字符串
 */
function convertToCSV(data) {
    // 这里实现数据转换为CSV格式的逻辑
    // 简化实现，实际使用时需要根据数据结构进行转换
    return 'Timestamp,Data\n' + new Date().toISOString() + ',Exported Data';
}

// 视图切换函数
function showDashboard() {
    switchView('dashboard');
}

function showSatelliteView() {
    switchView('satellite');
}

function showTrajectoryView() {
    switchView('trajectory');
}

function showAnalysisView() {
    switchView('analysis');
}

// 页面加载完成后初始化应用
document.addEventListener('DOMContentLoaded', initializeApp);

// 页面卸载前清理资源
window.addEventListener('beforeunload', () => {
    stopAutoRefresh();
    ChartManager.destroyAllCharts();
});

// 导出全局函数和对象
window.App = {
    initializeApp,
    refreshData,
    generateData,
    exportData,
    showMessage,
    switchView,
    showDashboard,
    showSatelliteView,
    showTrajectoryView,
    showAnalysisView,
    AppState,
    DataCache
};