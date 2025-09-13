#include "CameraController.h"
#include "CameraPath.h"

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkMath.h>

#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace BronchoscopyLib {
    
    class CameraController::Impl {
    public:
        // 相机对象
        vtkSmartPointer<vtkCamera> overviewCamera;
        vtkSmartPointer<vtkCamera> endoscopeCamera;
        
        // 关联的渲染器（弱引用，不负责生命周期）
        vtkRenderer* overviewRenderer;
        vtkRenderer* endoscopeRenderer;
        
        // 内窥镜相机参数
        double endoscopeFOV;
        double endoscopeViewUp[3];
        
        // 动画相关
        bool isTransitioning;
        double transitionProgress;      // 0.0 到 1.0
        double transitionDuration;      // 动画持续时间（秒）
        PathNode transitionStartNode;   // 动画起始状态
        PathNode transitionTargetNode;  // 动画目标状态
        std::chrono::steady_clock::time_point transitionStartTime;
        
        Impl() : overviewRenderer(nullptr), endoscopeRenderer(nullptr), 
                 endoscopeFOV(60.0),
                 isTransitioning(false), transitionProgress(0.0), transitionDuration(0.5) {
            // 默认向上方向
            endoscopeViewUp[0] = 0.0;
            endoscopeViewUp[1] = 1.0;
            endoscopeViewUp[2] = 0.0;
        }
        
        // 缓动函数：平滑的加速和减速
        double EaseInOutCubic(double t) {
            if (t < 0.5) {
                return 4 * t * t * t;
            } else {
                double p = 2 * t - 2;
                return 1 + p * p * p / 2;
            }
        }
        
        // 球面线性插值（SLERP）用于平滑旋转
        void SlerpVectors(const double v1[3], const double v2[3], double t, double result[3]) {
            // 计算两个向量的点积（夹角余弦）
            double dot = vtkMath::Dot(v1, v2);
            
            // 限制dot在[-1, 1]范围内，避免acos的数值误差
            dot = std::max(-1.0, std::min(1.0, dot));
            
            // 如果向量几乎平行，使用线性插值
            if (dot > 0.9995) {
                for (int i = 0; i < 3; i++) {
                    result[i] = v1[i] + (v2[i] - v1[i]) * t;
                }
                vtkMath::Normalize(result);
                return;
            }
            
            // 如果向量几乎相反，需要特殊处理
            if (dot < -0.9995) {
                // 找一个垂直向量
                double perpendicular[3];
                if (std::abs(v1[0]) < 0.9) {
                    perpendicular[0] = 1.0;
                    perpendicular[1] = 0.0;
                    perpendicular[2] = 0.0;
                } else {
                    perpendicular[0] = 0.0;
                    perpendicular[1] = 1.0;
                    perpendicular[2] = 0.0;
                }
                
                // 计算垂直向量
                double temp[3];
                vtkMath::Cross(v1, perpendicular, temp);
                vtkMath::Normalize(temp);
                
                // 旋转180度
                double angle = M_PI * t;
                double cosAngle = cos(angle);
                double sinAngle = sin(angle);
                
                for (int i = 0; i < 3; i++) {
                    result[i] = v1[i] * cosAngle + temp[i] * sinAngle;
                }
                return;
            }
            
            // 计算夹角
            double theta = acos(dot);
            double sinTheta = sin(theta);
            
            // 计算插值权重
            double w1 = sin((1.0 - t) * theta) / sinTheta;
            double w2 = sin(t * theta) / sinTheta;
            
            // 球面插值
            for (int i = 0; i < 3; i++) {
                result[i] = v1[i] * w1 + v2[i] * w2;
            }
            
            vtkMath::Normalize(result);
        }
    };
    
    CameraController::CameraController() : pImpl(std::make_unique<Impl>()) {
        InitializeCameras();
    }
    
    CameraController::~CameraController() = default;
    
    void CameraController::InitializeCameras() {
        // 创建相机
        pImpl->overviewCamera = vtkSmartPointer<vtkCamera>::New();
        pImpl->endoscopeCamera = vtkSmartPointer<vtkCamera>::New();
        
        // 设置默认参数
        pImpl->endoscopeCamera->SetViewAngle(pImpl->endoscopeFOV);
        pImpl->endoscopeCamera->SetViewUp(pImpl->endoscopeViewUp);
    }
    
    vtkCamera* CameraController::GetOverviewCamera() const {
        return pImpl->overviewCamera;
    }
    
    vtkCamera* CameraController::GetEndoscopeCamera() const {
        return pImpl->endoscopeCamera;
    }
    
    void CameraController::AttachToRenderers(vtkRenderer* overviewRenderer, 
                                            vtkRenderer* endoscopeRenderer) {
        pImpl->overviewRenderer = overviewRenderer;
        pImpl->endoscopeRenderer = endoscopeRenderer;
        
        if (overviewRenderer && pImpl->overviewCamera) {
            overviewRenderer->SetActiveCamera(pImpl->overviewCamera);
        }
        
        if (endoscopeRenderer && pImpl->endoscopeCamera) {
            endoscopeRenderer->SetActiveCamera(pImpl->endoscopeCamera);
        }
    }
    
    void CameraController::ResetCameras(double bounds[6]) {
        // 重置overview相机
        if (pImpl->overviewRenderer) {
            pImpl->overviewRenderer->ResetCamera();
        }
        
        // 为endoscope相机设置一个合理的初始位置（在模型内部）
        // 使用模型中心作为初始位置
        double center[3];
        center[0] = (bounds[0] + bounds[1]) / 2.0;
        center[1] = (bounds[2] + bounds[3]) / 2.0;
        center[2] = (bounds[4] + bounds[5]) / 2.0;
        
        pImpl->endoscopeCamera->SetPosition(center[0], center[1] + 50, center[2]);
        pImpl->endoscopeCamera->SetFocalPoint(center[0], center[1], center[2]);
        pImpl->endoscopeCamera->SetViewUp(pImpl->endoscopeViewUp);
        pImpl->endoscopeCamera->SetViewAngle(pImpl->endoscopeFOV);
        
        if (pImpl->endoscopeRenderer) {
            pImpl->endoscopeRenderer->ResetCameraClippingRange();
        }
        
        // 调试输出
        PrintCameraStatus();
    }
    
    void CameraController::ResetOverviewCamera() {
        if (pImpl->overviewRenderer) {
            pImpl->overviewRenderer->ResetCamera();
        }
    }
    
    void CameraController::ResetEndoscopeCamera() {
        if (pImpl->endoscopeRenderer) {
            pImpl->endoscopeRenderer->ResetCamera();
        }
    }
    
    void CameraController::UpdateEndoscopeCamera(const PathNode* pathNode) {
        if (!pathNode || !pImpl->endoscopeCamera) return;
        
        UpdateEndoscopeCamera(pathNode->position, pathNode->direction);
    }
    
    void CameraController::UpdateEndoscopeCamera(const double position[3], 
                                                const double direction[3]) {
        if (!pImpl->endoscopeCamera) return;
        
        // 调试输出：路径点坐标
        std::cout << "=== UpdateEndoscopeCamera ===" << std::endl;
        std::cout << "Camera position: (" 
                  << position[0] << ", " 
                  << position[1] << ", " 
                  << position[2] << ")" << std::endl;
        std::cout << "Camera direction: (" 
                  << direction[0] << ", " 
                  << direction[1] << ", " 
                  << direction[2] << ")" << std::endl;
        
        // 设置相机位置
        pImpl->endoscopeCamera->SetPosition(position);
        
        // 计算焦点位置（沿着方向向前看）
        double focalPoint[3];
        for (int i = 0; i < 3; i++) {
            focalPoint[i] = position[i] + 10.0 * direction[i];
        }
        pImpl->endoscopeCamera->SetFocalPoint(focalPoint);
        
        // 设置上方向
        pImpl->endoscopeCamera->SetViewUp(pImpl->endoscopeViewUp);
        
        // 设置视场角
        pImpl->endoscopeCamera->SetViewAngle(pImpl->endoscopeFOV);
        
        // 重置相机裁剪范围
        if (pImpl->endoscopeRenderer) {
            pImpl->endoscopeRenderer->ResetCameraClippingRange();
        }
        
        // 调试输出：相机状态
        double* camPos = pImpl->endoscopeCamera->GetPosition();
        double* camFocal = pImpl->endoscopeCamera->GetFocalPoint();
        double* clippingRange = pImpl->endoscopeCamera->GetClippingRange();
        
        std::cout << "Camera focal point: (" 
                  << camFocal[0] << ", " << camFocal[1] << ", " << camFocal[2] << ")" << std::endl;
        std::cout << "Camera FOV: " << pImpl->endoscopeCamera->GetViewAngle() << std::endl;
        std::cout << "Camera clipping range: [" 
                  << clippingRange[0] << ", " << clippingRange[1] << "]" << std::endl;
        std::cout << "==============================" << std::endl;
    }
    
    void CameraController::SetEndoscopeFOV(double angle) {
        pImpl->endoscopeFOV = angle;
        if (pImpl->endoscopeCamera) {
            pImpl->endoscopeCamera->SetViewAngle(angle);
        }
    }
    
    void CameraController::SetEndoscopeViewUp(double x, double y, double z) {
        pImpl->endoscopeViewUp[0] = x;
        pImpl->endoscopeViewUp[1] = y;
        pImpl->endoscopeViewUp[2] = z;
        
        if (pImpl->endoscopeCamera) {
            pImpl->endoscopeCamera->SetViewUp(x, y, z);
        }
    }
    
    void CameraController::PrintCameraStatus(bool overview, bool endoscope) const {
        std::cout << "=== Camera Status ===" << std::endl;
        
        if (overview && pImpl->overviewCamera) {
            double* pos = pImpl->overviewCamera->GetPosition();
            std::cout << "Overview camera position: (" 
                      << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << std::endl;
        }
        
        if (endoscope && pImpl->endoscopeCamera) {
            double* pos = pImpl->endoscopeCamera->GetPosition();
            std::cout << "Endoscope camera position: (" 
                      << pos[0] << ", " << pos[1] << ", " << pos[2] << ")" << std::endl;
        }
        
        std::cout << "===================" << std::endl;
    }
    
    void CameraController::StartTransition(const PathNode* targetNode) {
        if (!targetNode || !pImpl->endoscopeCamera) return;
        
        // 保存当前相机状态作为起点
        GetCurrentEndoscopeState(&pImpl->transitionStartNode);
        
        // 设置目标
        pImpl->transitionTargetNode = *targetNode;
        
        // 计算两点之间的距离
        double distance = 0.0;
        for (int i = 0; i < 3; i++) {
            double diff = pImpl->transitionTargetNode.position[i] - pImpl->transitionStartNode.position[i];
            distance += diff * diff;
        }
        distance = sqrt(distance);
        
        // 根据距离动态计算过渡时间
        // 基础时间0.3秒，每10个单位距离增加0.1秒，最大不超过1.5秒
        double baseTime = 0.3;
        double speedFactor = 0.01;  // 每单位距离的时间
        pImpl->transitionDuration = baseTime + distance * speedFactor;
        
        // 限制最大和最小时间
        if (pImpl->transitionDuration > 1.5) {
            pImpl->transitionDuration = 1.5;
        } else if (pImpl->transitionDuration < 0.2) {
            pImpl->transitionDuration = 0.2;
        }
        
        std::cout << "Transition distance: " << distance 
                  << ", duration: " << pImpl->transitionDuration << "s" << std::endl;
        
        // 初始化动画参数
        pImpl->isTransitioning = true;
        pImpl->transitionProgress = 0.0;
        pImpl->transitionStartTime = std::chrono::steady_clock::now();
    }
    
    bool CameraController::UpdateTransition() {
        if (!pImpl->isTransitioning) return false;
        
        // 计算经过的时间
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = currentTime - pImpl->transitionStartTime;
        double elapsedSeconds = elapsed.count();
        
        // 计算进度
        pImpl->transitionProgress = elapsedSeconds / pImpl->transitionDuration;
        
        if (pImpl->transitionProgress >= 1.0) {
            // 动画完成
            pImpl->transitionProgress = 1.0;
            pImpl->isTransitioning = false;
        }
        
        // 应用缓动函数
        double easedProgress = pImpl->EaseInOutCubic(pImpl->transitionProgress);
        
        // 插值位置
        double interpolatedPos[3];
        for (int i = 0; i < 3; i++) {
            interpolatedPos[i] = pImpl->transitionStartNode.position[i] + 
                               (pImpl->transitionTargetNode.position[i] - pImpl->transitionStartNode.position[i]) * easedProgress;
        }
        
        // 使用球面插值(SLERP)插值方向，实现平滑旋转
        double interpolatedDir[3];
        pImpl->SlerpVectors(pImpl->transitionStartNode.direction, 
                           pImpl->transitionTargetNode.direction, 
                           easedProgress, 
                           interpolatedDir);
        
        // 更新相机
        UpdateEndoscopeCamera(interpolatedPos, interpolatedDir);
        
        return pImpl->isTransitioning;  // 返回动画是否还在进行
    }
    
    void CameraController::SetTransitionDuration(double seconds) {
        if (seconds > 0.0) {
            pImpl->transitionDuration = seconds;
        }
    }
    
    bool CameraController::IsTransitioning() const {
        return pImpl->isTransitioning;
    }
    
    void CameraController::GetCurrentEndoscopeState(PathNode* state) const {
        if (!state || !pImpl->endoscopeCamera) return;
        
        pImpl->endoscopeCamera->GetPosition(state->position);
        
        // 计算方向向量（从位置指向焦点）
        double focalPoint[3];
        pImpl->endoscopeCamera->GetFocalPoint(focalPoint);
        for (int i = 0; i < 3; i++) {
            state->direction[i] = focalPoint[i] - state->position[i];
        }
        
        // 归一化方向
        double length = vtkMath::Norm(state->direction);
        if (length > 0) {
            for (int i = 0; i < 3; i++) {
                state->direction[i] /= length;
            }
        }
    }
    
} // namespace BronchoscopyLib