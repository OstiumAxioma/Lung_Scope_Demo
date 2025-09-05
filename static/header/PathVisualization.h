#ifndef PATH_VISUALIZATION_H
#define PATH_VISUALIZATION_H

#include <memory>
#include <vector>

// 前向声明VTK类
class vtkActor;
class vtkRenderer;
class vtkRenderWindow;
class vtkPolyData;

namespace BronchoscopyLib {
    
    // 前向声明
    class CameraPath;
    struct PathNode;
    
    /**
     * PathVisualization - 管理路径和标记的可视化
     * 负责路径管道（绿色）和位置标记（红球）的显示
     */
    class PathVisualization {
    public:
        PathVisualization();
        ~PathVisualization();
        
        // 初始化可视化组件
        void Initialize();
        
        // 设置相机路径对象
        void SetCameraPath(CameraPath* path);
        
        // 加载路径数据（从点序列创建路径）
        bool LoadPathFromPositions(const std::vector<double>& positions);
        
        // 更新位置标记（红球）
        void UpdatePositionMarker(const PathNode* pathNode);
        void UpdatePositionMarker(const double position[3]);
        
        // 添加到渲染器
        void AddPathToRenderer(vtkRenderer* renderer);
        void AddMarkerToRenderer(vtkRenderer* renderer);
        void AddToRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer = nullptr);
        
        // 从渲染器移除
        void RemoveFromRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer = nullptr);
        
        // 显示控制
        void ShowPath(bool show);
        void ShowMarker(bool show);
        bool IsPathVisible() const;
        bool IsMarkerVisible() const;
        
        // 外观设置
        void SetPathColor(double r, double g, double b);
        void SetMarkerColor(double r, double g, double b);
        void SetPathOpacity(double opacity);
        void SetMarkerRadius(double radius);
        void SetPathTubeRadius(double radius);
        
        // 获取路径对象
        CameraPath* GetCameraPath() const;
        int GetTotalPathNodes() const;
        
        // 清理
        void ClearPath();
        void ClearAll();
        
        // 需要渲染窗口引用以便在更新标记后触发渲染
        void SetOverviewWindow(vtkRenderWindow* window);
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // PATH_VISUALIZATION_H