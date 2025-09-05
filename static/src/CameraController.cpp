#include "CameraController.h"
#include "CameraPath.h"

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkMath.h>

#include <iostream>

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
        
        Impl() : overviewRenderer(nullptr), endoscopeRenderer(nullptr), 
                 endoscopeFOV(60.0) {
            // 默认向上方向
            endoscopeViewUp[0] = 0.0;
            endoscopeViewUp[1] = 1.0;
            endoscopeViewUp[2] = 0.0;
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
    
} // namespace BronchoscopyLib