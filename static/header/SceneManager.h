#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include <memory>

// 前向声明
class vtkPolyData;

namespace BronchoscopyLib {
    
    // 前向声明
    class CameraController;
    class ModelManager;
    class PathVisualization;
    class RenderingEngine;
    class NavigationController;
    struct PathNode;
    
    /**
     * SceneManager - 场景协调管理器
     * 负责协调所有模块之间的交互，管理场景状态，提供统一的场景操作接口
     */
    class SceneManager {
    public:
        SceneManager();
        ~SceneManager();
        
        // 设置各个模块（依赖注入）
        void SetCameraController(CameraController* controller);
        void SetModelManager(ModelManager* manager);
        void SetPathVisualization(PathVisualization* pathViz);
        void SetRenderingEngine(RenderingEngine* engine);
        void SetNavigationController(NavigationController* navController);
        
        // 初始化场景
        void InitializeScene();
        
        // 场景更新
        void UpdateScene();
        void UpdateFromNavigation(PathNode* node, int index);
        
        // 场景清理
        void ClearScene();
        void ClearModel();
        void ClearPath();
        
        // 场景状态管理
        void SetShowPath(bool show);
        void SetShowMarker(bool show);
        bool IsPathVisible() const;
        bool IsMarkerVisible() const;
        
        // 场景重置
        void ResetCameras();
        void ResetToDefaultView();
        
        // 协调操作
        bool OnModelLoaded(vtkPolyData* polyData);
        void OnPathLoaded();
        void OnNavigationChanged(PathNode* node, int index);
        
        // 渲染触发
        void RequestRender();
        
        // 获取场景信息
        bool HasModel() const;
        bool HasPath() const;
        bool IsSceneReady() const;
        
        // 场景配置
        void SetAutoRender(bool autoRender);
        bool GetAutoRender() const;
        
        // 调试信息
        void PrintSceneInfo() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // SCENE_MANAGER_H