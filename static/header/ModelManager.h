#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include <memory>

// 前向声明VTK类
class vtkPolyData;
class vtkActor;
class vtkRenderer;

namespace BronchoscopyLib {
    
    /**
     * ModelManager - 管理3D模型的加载和显示
     * 负责气管模型的数据管理、Actor创建和渲染设置
     */
    class ModelManager {
    public:
        ModelManager();
        ~ModelManager();
        
        // 加载模型数据（深拷贝，确保独立生命周期）
        bool LoadModel(vtkPolyData* polyData);
        
        // 获取模型数据
        vtkPolyData* GetModelData() const;
        
        // 创建并返回用于不同渲染器的Actor
        vtkActor* CreateOverviewActor();
        vtkActor* CreateEndoscopeActor();
        
        // 获取已创建的Actor
        vtkActor* GetOverviewActor() const;
        vtkActor* GetEndoscopeActor() const;
        
        // 添加Actor到渲染器
        void AddToRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer);
        
        // 从渲染器移除Actor
        void RemoveFromRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer);
        
        // 设置模型显示属性
        void SetOverviewOpacity(double opacity);
        void SetOverviewColor(double r, double g, double b);
        void SetEndoscopeColor(double r, double g, double b);
        
        // 设置平滑度（特征角度，0-180度）
        // 180度 = 完全平滑，0度 = 保留所有边缘
        void SetSmoothingAngle(double angle);
        
        // 获取模型边界
        void GetModelBounds(double bounds[6]) const;
        
        // 清理模型
        void ClearModel();
        
        // 检查是否已加载模型
        bool HasModel() const;
        
        // 调试输出
        void PrintModelInfo() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // MODEL_MANAGER_H