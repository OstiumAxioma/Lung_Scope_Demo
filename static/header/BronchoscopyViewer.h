#ifndef BRONCHOSCOPY_VIEWER_H
#define BRONCHOSCOPY_VIEWER_H

#include <memory>
#include <vector>

// 前向声明VTK类
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkActor;
class vtkPolyData;
class vtkCamera;

namespace BronchoscopyLib {
    
    // 前向声明
    class CameraPath;

    class BronchoscopyViewer {
    public:
        BronchoscopyViewer();
        ~BronchoscopyViewer();

        // 初始化
        void Initialize();
        
        // 数据加载（静态库只接收数据，不处理文件I/O）
        bool LoadAirwayModel(vtkPolyData* polyData);
        
        // 加载相机路径：从点序列加载（自动计算方向）
        bool LoadCameraPath(const std::vector<double>& positions);   // x1,y1,z1,x2,y2,z2...
        
        // 获取渲染器（供主程序两个窗口使用）
        vtkRenderer* GetOverviewRenderer();   // 左窗口：全局视图
        vtkRenderer* GetEndoscopeRenderer();  // 右窗口：内窥镜视图
        
        // 设置渲染窗口
        void SetOverviewRenderWindow(vtkRenderWindow* window);
        void SetEndoscopeRenderWindow(vtkRenderWindow* window);
        
        // 相机路径导航
        void MoveToNext();
        void MoveToPrevious();
        void MoveToPosition(int index);
        void ResetToStart();
        void SetAutoPlay(bool play);
        void SetPlaySpeed(double speed);  // 1.0 = 正常速度
        
        // 获取状态
        int GetCurrentPathIndex() const;
        int GetTotalPathNodes() const;
        bool IsPlaying() const;
        
        // 动画更新（需要在渲染循环中调用）
        bool UpdateAnimation();  // 返回true表示动画正在进行
        void SetAnimationDuration(double seconds);  // 设置动画持续时间
        
        // 可视化控制
        void ShowPath(bool show);
        void ShowPositionMarker(bool show);
        void SetPathColor(double r, double g, double b);
        void SetMarkerColor(double r, double g, double b);
        void SetPathOpacity(double opacity);
        
        // 更新渲染
        void UpdateVisualization();
        void Render();
        
        // 清理
        void ClearAll();
        void ClearPath();
        void ClearModel();

    private:
        // 内部辅助方法 - 应用已构建的路径对象
        bool ApplyCameraPath(CameraPath* path);
        
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };

} // namespace BronchoscopyLib

#endif // BRONCHOSCOPY_VIEWER_H