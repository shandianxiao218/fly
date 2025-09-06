#include "obstruction.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief 创建3D向量
 * @param x X坐标
 * @param y Y坐标
 * @param z Z坐标
 * @return 创建的向量
 */
Vector3D vector3d_create(double x, double y, double z) {
    Vector3D v = {x, y, z};
    return v;
}

/**
 * @brief 向量加法
 * @param v1 向量1
 * @param v2 向量2
 * @return 和向量
 */
Vector3D vector3d_add(const Vector3D* v1, const Vector3D* v2) {
    return vector3d_create(v1->x + v2->x, v1->y + v2->y, v1->z + v2->z);
}

/**
 * @brief 向量减法
 * @param v1 向量1
 * @param v2 向量2
 * @return 差向量
 */
Vector3D vector3d_subtract(const Vector3D* v1, const Vector3D* v2) {
    return vector3d_create(v1->x - v2->x, v1->y - v2->y, v1->z - v2->z);
}

/**
 * @brief 向量标量乘法
 * @param v 向量
 * @param scalar 标量
 * @return 乘积向量
 */
Vector3D vector3d_multiply(const Vector3D* v, double scalar) {
    return vector3d_create(v->x * scalar, v->y * scalar, v->z * scalar);
}

/**
 * @brief 向量点积
 * @param v1 向量1
 * @param v2 向量2
 * @return 点积结果
 */
double vector3d_dot(const Vector3D* v1, const Vector3D* v2) {
    return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

/**
 * @brief 向量叉积
 * @param v1 向量1
 * @param v2 向量2
 * @return 叉积向量
 */
Vector3D vector3d_cross(const Vector3D* v1, const Vector3D* v2) {
    return vector3d_create(
        v1->y * v2->z - v1->z * v2->y,
        v1->z * v2->x - v1->x * v2->z,
        v1->x * v2->y - v1->y * v2->x
    );
}

/**
 * @brief 计算向量长度
 * @param v 向量
 * @return 向量长度
 */
double vector3d_length(const Vector3D* v) {
    return sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

/**
 * @brief 向量归一化
 * @param v 向量
 * @return 归一化向量
 */
Vector3D vector3d_normalize(const Vector3D* v) {
    double length = vector3d_length(v);
    if (length == 0.0) {
        return vector3d_create(0.0, 0.0, 0.0);
    }
    return vector3d_multiply(v, 1.0 / length);
}

/**
 * @brief 向量旋转
 * @param v 向量
 * @param matrix 旋转矩阵
 * @return 旋转后的向量
 */
Vector3D vector3d_rotate(const Vector3D* v, const RotationMatrix* matrix) {
    return vector3d_create(
        matrix->m[0][0] * v->x + matrix->m[0][1] * v->y + matrix->m[0][2] * v->z,
        matrix->m[1][0] * v->x + matrix->m[1][1] * v->y + matrix->m[1][2] * v->z,
        matrix->m[2][0] * v->x + matrix->m[2][1] * v->y + matrix->m[2][2] * v->z
    );
}

/**
 * @brief 从欧拉角创建旋转矩阵
 * @param pitch 俯仰角 (度)
 * @param roll 横滚角 (度)
 * @param yaw 偏航角 (度)
 * @return 旋转矩阵
 */
RotationMatrix rotation_matrix_create_from_euler(double pitch, double roll, double yaw) {
    RotationMatrix matrix;
    
    // 转换为弧度
    double p = pitch * M_PI / 180.0;
    double r = roll * M_PI / 180.0;
    double y = yaw * M_PI / 180.0;
    
    // 计算三角函数值
    double cp = cos(p), sp = sin(p);
    double cr = cos(r), sr = sin(r);
    double cy = cos(y), sy = sin(y);
    
    // 计算旋转矩阵元素
    matrix.m[0][0] = cy * cr + sp * sy * sr;
    matrix.m[0][1] = sy * cp;
    matrix.m[0][2] = cy * sr - sp * sy * cr;
    
    matrix.m[1][0] = -sy * cr + sp * cy * sr;
    matrix.m[1][1] = cy * cp;
    matrix.m[1][2] = -sy * sr - sp * cy * cr;
    
    matrix.m[2][0] = -cp * sr;
    matrix.m[2][1] = sp;
    matrix.m[2][2] = cp * cr;
    
    return matrix;
}

/**
 * @brief 旋转矩阵乘法
 * @param m1 矩阵1
 * @param m2 矩阵2
 * @return 乘积矩阵
 */
RotationMatrix rotation_matrix_multiply(const RotationMatrix* m1, const RotationMatrix* m2) {
    RotationMatrix result;
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result.m[i][j] = 0.0;
            for (int k = 0; k < 3; k++) {
                result.m[i][j] += m1->m[i][k] * m2->m[k][j];
            }
        }
    }
    
    return result;
}

/**
 * @brief 射线与长方体相交检测
 * @param ray 射线
 * @param box 长方体
 * @param intersection 交点 (输出)
 * @param distance 相交距离 (输出)
 * @return 1表示相交，0表示不相交
 */
int ray_box_intersection(const Ray* ray, const ObstructionBody* box, 
                         Vector3D* intersection, double* distance) {
    // 将射线转换到长方体的局部坐标系
    Vector3D local_origin = vector3d_subtract(&ray->origin, &box->center);
    
    // 长方体的边界
    double half_x = box->size.x / 2.0;
    double half_y = box->size.y / 2.0;
    double half_z = box->size.z / 2.0;
    
    // 计算射线与长方体各面的交点
    double t_min = 0.0;
    double t_max = ray->length;
    
    // X方向
    if (fabs(ray->direction.x) > 1e-10) {
        double t1 = (-half_x - local_origin.x) / ray->direction.x;
        double t2 = (half_x - local_origin.x) / ray->direction.x;
        
        if (t1 > t2) {
            double temp = t1;
            t1 = t2;
            t2 = temp;
        }
        
        t_min = (t1 > t_min) ? t1 : t_min;
        t_max = (t2 < t_max) ? t2 : t_max;
        
        if (t_min > t_max) {
            return 0; // 不相交
        }
    } else if (local_origin.x < -half_x || local_origin.x > half_x) {
        return 0; // 射线平行于X平面且在长方体外
    }
    
    // Y方向
    if (fabs(ray->direction.y) > 1e-10) {
        double t1 = (-half_y - local_origin.y) / ray->direction.y;
        double t2 = (half_y - local_origin.y) / ray->direction.y;
        
        if (t1 > t2) {
            double temp = t1;
            t1 = t2;
            t2 = temp;
        }
        
        t_min = (t1 > t_min) ? t1 : t_min;
        t_max = (t2 < t_max) ? t2 : t_max;
        
        if (t_min > t_max) {
            return 0; // 不相交
        }
    } else if (local_origin.y < -half_y || local_origin.y > half_y) {
        return 0; // 射线平行于Y平面且在长方体外
    }
    
    // Z方向
    if (fabs(ray->direction.z) > 1e-10) {
        double t1 = (-half_z - local_origin.z) / ray->direction.z;
        double t2 = (half_z - local_origin.z) / ray->direction.z;
        
        if (t1 > t2) {
            double temp = t1;
            t1 = t2;
            t2 = temp;
        }
        
        t_min = (t1 > t_min) ? t1 : t_min;
        t_max = (t2 < t_max) ? t2 : t_max;
        
        if (t_min > t_max) {
            return 0; // 不相交
        }
    } else if (local_origin.z < -half_z || local_origin.z > half_z) {
        return 0; // 射线平行于Z平面且在长方体外
    }
    
    // 检查交点是否在射线范围内
    if (t_min < 0.0 || t_min > ray->length) {
        return 0;
    }
    
    // 计算交点
    if (intersection) {
        Vector3D direction_multiplied = vector3d_multiply(&ray->direction, t_min);
        Vector3D local_intersection = vector3d_add(&local_origin, &direction_multiplied);
        *intersection = vector3d_add(&local_intersection, &box->center);
    }
    
    if (distance) {
        *distance = t_min;
    }
    
    return 1;
}

/**
 * @brief 射线与飞机部件相交检测
 * @param ray 射线
 * @param component 飞机部件
 * @param intersection 交点 (输出)
 * @param distance 相交距离 (输出)
 * @return 1表示相交，0表示不相交
 */
int ray_component_intersection(const Ray* ray, const AircraftComponent* component,
                               Vector3D* intersection, double* distance) {
    // 创建遮挡体
    ObstructionBody body;
    body.center = component->position;
    body.size = component->size;
    body.rotation = rotation_matrix_create_from_euler(
        component->rotation.x, component->rotation.y, component->rotation.z
    );
    body.part_type = component->part_type;
    
    // 将射线转换到部件的局部坐标系
    Vector3D local_origin = vector3d_subtract(&ray->origin, &component->position);
    
    // 逆旋转射线
    RotationMatrix inv_rotation = body.rotation;
    // 简化处理：假设旋转矩阵的逆等于转置
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < i; j++) {
            double temp = inv_rotation.m[i][j];
            inv_rotation.m[i][j] = inv_rotation.m[j][i];
            inv_rotation.m[j][i] = temp;
        }
    }
    
    Vector3D local_direction = vector3d_rotate(&ray->direction, &inv_rotation);
    Vector3D rotated_origin = vector3d_rotate(&local_origin, &inv_rotation);
    
    // 创建局部射线
    Ray local_ray;
    local_ray.origin = rotated_origin;
    local_ray.direction = local_direction;
    local_ray.length = ray->length;
    
    // 检测相交
    Vector3D local_intersection;
    double local_distance;
    
    if (!ray_box_intersection(&local_ray, &body, &local_intersection, &local_distance)) {
        return 0;
    }
    
    // 将交点转换回世界坐标系
    if (intersection) {
        Vector3D world_intersection = vector3d_rotate(&local_intersection, &body.rotation);
        *intersection = vector3d_add(&world_intersection, &component->position);
    }
    
    if (distance) {
        *distance = local_distance;
    }
    
    return 1;
}
