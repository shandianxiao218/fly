/**
 * 北斗导航卫星可见性分析系统 - API调用模块
 * 
 * 本模块负责与后端API服务器进行通信，包括：
 * - 系统状态查询
 * - 卫星数据获取
 * - 轨迹数据获取
 * - 分析结果获取
 * - 数据生成和上传
 * 
 * @author Claude Code
 * @version 1.0
 * @date 2025-09-06
 */

// API配置
const API_CONFIG = {
    baseURL: 'http://localhost:8080',
    timeout: 10000,
    headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json'
    }
};

// API端点
const API_ENDPOINTS = {
    status: '/api/status',
    satellite: '/api/satellite',
    trajectory: '/api/trajectory',
    analysis: '/api/analysis'
};

/**
 * 通用API请求函数
 * @param {string} endpoint - API端点
 * @param {string} method - HTTP方法
 * @param {object} data - 请求数据
 * @returns {Promise} - 返回Promise对象
 */
async function apiRequest(endpoint, method = 'GET', data = null) {
    const url = API_CONFIG.baseURL + endpoint;
    
    const config = {
        method: method,
        headers: API_CONFIG.headers,
        mode: 'cors',
        credentials: 'same-origin'
    };

    if (data && (method === 'POST' || method === 'PUT')) {
        config.body = JSON.stringify(data);
    }

    try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), API_CONFIG.timeout);
        
        const response = await fetch(url, {
            ...config,
            signal: controller.signal
        });
        
        clearTimeout(timeoutId);

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }

        const result = await response.json();
        return result;
        
    } catch (error) {
        console.error('API请求失败:', error);
        throw error;
    }
}

/**
 * 获取系统状态
 * @returns {Promise<object>} - 系统状态信息
 */
async function getSystemStatus() {
    try {
        const response = await apiRequest(API_ENDPOINTS.status);
        return {
            success: true,
            data: response,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 获取卫星数据
 * @param {object} params - 查询参数
 * @returns {Promise<object>} - 卫星数据
 */
async function getSatelliteData(params = {}) {
    try {
        const queryParams = new URLSearchParams(params).toString();
        const endpoint = queryParams ? `${API_ENDPOINTS.satellite}?${queryParams}` : API_ENDPOINTS.satellite;
        
        const response = await apiRequest(endpoint);
        return {
            success: true,
            data: response,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 获取轨迹数据
 * @param {object} params - 查询参数
 * @returns {Promise<object>} - 轨迹数据
 */
async function getTrajectoryData(params = {}) {
    try {
        const queryParams = new URLSearchParams(params).toString();
        const endpoint = queryParams ? `${API_ENDPOINTS.trajectory}?${queryParams}` : API_ENDPOINTS.trajectory;
        
        const response = await apiRequest(endpoint);
        return {
            success: true,
            data: response,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 获取分析结果
 * @param {object} params - 查询参数
 * @returns {Promise<object>} - 分析结果
 */
async function getAnalysisData(params = {}) {
    try {
        const queryParams = new URLSearchParams(params).toString();
        const endpoint = queryParams ? `${API_ENDPOINTS.analysis}?${queryParams}` : API_ENDPOINTS.analysis;
        
        const response = await apiRequest(endpoint);
        return {
            success: true,
            data: response,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 生成测试数据
 * @param {object} params - 生成参数
 * @returns {Promise<object>} - 生成结果
 */
async function generateTestData(params = {}) {
    try {
        const response = await apiRequest(API_ENDPOINTS.satellite, 'POST', params);
        return {
            success: true,
            data: response,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 上传轨迹数据
 * @param {object} data - 轨迹数据
 * @returns {Promise<object>} - 上传结果
 */
async function uploadTrajectoryData(data) {
    try {
        const response = await apiRequest(API_ENDPOINTS.trajectory, 'POST', data);
        return {
            success: true,
            data: response,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 执行分析
 * @param {object} params - 分析参数
 * @returns {Promise<object>} - 分析结果
 */
async function runAnalysis(params = {}) {
    try {
        const response = await apiRequest(API_ENDPOINTS.analysis, 'POST', params);
        return {
            success: true,
            data: response,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 检查服务器连接状态
 * @returns {Promise<boolean>} - 连接状态
 */
async function checkServerConnection() {
    try {
        const response = await apiRequest(API_ENDPOINTS.status);
        return response.success;
    } catch (error) {
        return false;
    }
}

/**
 * 批量获取数据
 * @param {array} requests - 请求列表
 * @returns {Promise<object>} - 批量结果
 */
async function batchRequests(requests) {
    const promises = requests.map(request => {
        switch (request.type) {
            case 'status':
                return getSystemStatus();
            case 'satellite':
                return getSatelliteData(request.params);
            case 'trajectory':
                return getTrajectoryData(request.params);
            case 'analysis':
                return getAnalysisData(request.params);
            default:
                return Promise.reject(new Error(`未知的请求类型: ${request.type}`));
        }
    });

    try {
        const results = await Promise.all(promises);
        return {
            success: true,
            data: results,
            timestamp: new Date().toISOString()
        };
    } catch (error) {
        return {
            success: false,
            error: error.message,
            timestamp: new Date().toISOString()
        };
    }
}

/**
 * 获取API健康状态
 * @returns {Promise<object>} - 健康状态信息
 */
async function getApiHealth() {
    const endpoints = [
        { name: 'status', endpoint: API_ENDPOINTS.status },
        { name: 'satellite', endpoint: API_ENDPOINTS.satellite },
        { name: 'trajectory', endpoint: API_ENDPOINTS.trajectory },
        { name: 'analysis', endpoint: API_ENDPOINTS.analysis }
    ];

    const healthChecks = await Promise.allSettled(
        endpoints.map(ep => apiRequest(ep.endpoint))
    );

    const healthStatus = {};
    endpoints.forEach((ep, index) => {
        const result = healthChecks[index];
        healthStatus[ep.name] = {
            status: result.status === 'fulfilled' ? 'healthy' : 'unhealthy',
            responseTime: result.status === 'fulfilled' ? 'OK' : 'Timeout',
            error: result.status === 'rejected' ? result.reason.message : null
        };
    });

    return healthStatus;
}

// 导出API函数
window.ApiClient = {
    getSystemStatus,
    getSatelliteData,
    getTrajectoryData,
    getAnalysisData,
    generateTestData,
    uploadTrajectoryData,
    runAnalysis,
    checkServerConnection,
    batchRequests,
    getApiHealth,
    API_CONFIG,
    API_ENDPOINTS
};