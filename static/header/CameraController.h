#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <memory>

// 前向声明VTK类
class vtkCamera;
class vtkRenderer;

namespace BronchoscopyLib {
    
    // 前向声明
    struct PathNode;
    
    /**
     * CameraController - 管理双相机系统
     * 负责overview和endoscope两个相机的创建、配置和更新
     */
    class CameraController {
    public:
        CameraController();
        ~CameraController();
        
        // 初始化相机
        void InitializeCameras();
        
        // 获取相机对象
        vtkCamera* GetOverviewCamera() const;
        vtkCamera* GetEndoscopeCamera() const;
        
        // 关联相机到渲染器
        void AttachToRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer);
        
        // 重置相机位置
        void ResetCameras(double bounds[6]);
        void ResetOverviewCamera();
        void ResetEndoscopeCamera();
        
        // 更新内窥镜相机位置（根据路径点）
        void UpdateEndoscopeCamera(const PathNode* pathNode);
        void UpdateEndoscopeCamera(const double position[3], const double direction[3]);
        
        // 相机参数设置
        void SetEndoscopeFOV(double angle);
        void SetEndoscopeViewUp(double x, double y, double z);
        
        // 获取相机状态（用于调试）
        void PrintCameraStatus(bool overview = true, bool endoscope = true) const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // CAMERA_CONTROLLER_H