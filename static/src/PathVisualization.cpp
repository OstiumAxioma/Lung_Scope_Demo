#include "PathVisualization.h"
#include "CameraPath.h"

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <iostream>

namespace BronchoscopyLib {
    
    class PathVisualization::Impl {
    public:
        // 路径管理
        CameraPath* cameraPath;  // 不拥有，只是引用
        std::unique_ptr<CameraPath> ownedCameraPath;  // 如果内部创建则拥有
        
        // 路径可视化
        vtkSmartPointer<vtkActor> pathActor;
        vtkSmartPointer<vtkPolyDataMapper> pathMapper;
        double pathColor[3];
        double pathOpacity;
        double pathTubeRadius;
        bool showPath;
        
        // 位置标记（红球）
        vtkSmartPointer<vtkSphereSource> positionMarker;
        vtkSmartPointer<vtkActor> markerActor;
        vtkSmartPointer<vtkPolyDataMapper> markerMapper;
        double markerColor[3];
        double markerRadius;
        bool showMarker;
        
        // 渲染窗口引用（用于更新后触发渲染）
        vtkRenderWindow* overviewWindow;
        vtkRenderer* overviewRenderer;
        
        Impl() : cameraPath(nullptr), overviewWindow(nullptr), overviewRenderer(nullptr),
                 pathOpacity(0.5), pathTubeRadius(1.0), markerRadius(2.0),
                 showPath(true), showMarker(true) {
            // 默认颜色
            pathColor[0] = 0.0; pathColor[1] = 1.0; pathColor[2] = 0.0;  // 绿色
            markerColor[0] = 1.0; markerColor[1] = 0.0; markerColor[2] = 0.0;  // 红色
        }
        
        void CreatePathVisualization() {
            if (!cameraPath) return;
            
            // 生成路径管道
            vtkPolyData* pathPolyData = cameraPath->GeneratePathTube(pathTubeRadius);
            if (!pathPolyData) return;
            
            // 创建mapper和actor
            pathMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            pathMapper->SetInputData(pathPolyData);
            
            pathActor = vtkSmartPointer<vtkActor>::New();
            pathActor->SetMapper(pathMapper);
            pathActor->GetProperty()->SetColor(pathColor);
            pathActor->GetProperty()->SetOpacity(pathOpacity);
            pathActor->SetVisibility(showPath);
            
            // 释放pathPolyData（GeneratePathTube已经增加了引用计数）
            pathPolyData->UnRegister(nullptr);
        }
    };
    
    PathVisualization::PathVisualization() : pImpl(std::make_unique<Impl>()) {
    }
    
    PathVisualization::~PathVisualization() = default;
    
    void PathVisualization::Initialize() {
        // 初始化位置标记（红球）
        pImpl->positionMarker = vtkSmartPointer<vtkSphereSource>::New();
        pImpl->positionMarker->SetCenter(0, 0, 0);  // 保持在原点
        pImpl->positionMarker->SetRadius(pImpl->markerRadius);
        pImpl->positionMarker->SetPhiResolution(20);
        pImpl->positionMarker->SetThetaResolution(20);
        
        pImpl->markerMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        pImpl->markerMapper->SetInputConnection(pImpl->positionMarker->GetOutputPort());
        
        pImpl->markerActor = vtkSmartPointer<vtkActor>::New();
        pImpl->markerActor->SetMapper(pImpl->markerMapper);
        pImpl->markerActor->GetProperty()->SetColor(pImpl->markerColor);
        pImpl->markerActor->SetVisibility(pImpl->showMarker);
    }
    
    void PathVisualization::SetCameraPath(CameraPath* path) {
        pImpl->cameraPath = path;
        pImpl->ownedCameraPath.reset();  // 清除拥有的路径
        
        // 创建路径可视化
        pImpl->CreatePathVisualization();
    }
    
    bool PathVisualization::LoadPathFromPositions(const std::vector<double>& positions) {
        if (positions.size() < 6 || positions.size() % 3 != 0) {
            std::cerr << "PathVisualization: Invalid path data - need at least 2 points" << std::endl;
            return false;
        }
        
        // 创建新的CameraPath对象
        auto path = std::make_unique<CameraPath>();
        int numPoints = positions.size() / 3;
        
        // 处理每个点，自动计算方向
        for (int i = 0; i < numPoints; i++) {
            double pos[3] = {positions[i*3], positions[i*3+1], positions[i*3+2]};
            double dir[3];
            
            if (i < numPoints - 1) {
                // 非末尾点：方向指向下一个点
                double nextPos[3] = {positions[(i+1)*3], positions[(i+1)*3+1], positions[(i+1)*3+2]};
                for (int j = 0; j < 3; j++) {
                    dir[j] = nextPos[j] - pos[j];
                }
            } else {
                // 末尾点：使用前一个点的方向
                if (i > 0) {
                    double prevPos[3] = {positions[(i-1)*3], positions[(i-1)*3+1], positions[(i-1)*3+2]};
                    for (int j = 0; j < 3; j++) {
                        dir[j] = pos[j] - prevPos[j];
                    }
                } else {
                    // 只有一个点的特殊情况
                    dir[0] = 0; dir[1] = 0; dir[2] = 1;
                }
            }
            
            // 归一化方向向量
            double length = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2]);
            if (length > 0.0) {
                dir[0] /= length;
                dir[1] /= length;
                dir[2] /= length;
            }
            
            path->AddPoint(pos, dir);
        }
        
        // 设置并拥有这个路径
        pImpl->cameraPath = path.get();
        pImpl->ownedCameraPath = std::move(path);
        
        // 创建路径可视化
        pImpl->CreatePathVisualization();
        
        std::cout << "PathVisualization: Loaded " << numPoints << " path points" << std::endl;
        return true;
    }
    
    void PathVisualization::UpdatePositionMarker(const PathNode* pathNode) {
        if (!pathNode) return;
        UpdatePositionMarker(pathNode->position);
    }
    
    void PathVisualization::UpdatePositionMarker(const double position[3]) {
        if (!pImpl->markerActor) return;
        
        // 通过设置actor的位置而不是修改数据源（避免渲染问题）
        // VTK的SetPosition需要非const参数，所以使用三个独立的值
        pImpl->markerActor->SetPosition(position[0], position[1], position[2]);
        
        // 标记渲染器需要更新
        if (pImpl->overviewRenderer) {
            pImpl->overviewRenderer->Modified();
        }
        
        // 如果窗口存在，立即渲染
        if (pImpl->overviewWindow) {
            pImpl->overviewWindow->Render();
        }
    }
    
    void PathVisualization::AddPathToRenderer(vtkRenderer* renderer) {
        if (renderer && pImpl->pathActor) {
            renderer->AddActor(pImpl->pathActor);
        }
    }
    
    void PathVisualization::AddMarkerToRenderer(vtkRenderer* renderer) {
        if (renderer && pImpl->markerActor) {
            renderer->AddActor(pImpl->markerActor);
            pImpl->overviewRenderer = renderer;  // 保存引用用于更新
        }
    }
    
    void PathVisualization::AddToRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer) {
        // 路径和标记只在overview中显示
        if (overviewRenderer) {
            AddPathToRenderer(overviewRenderer);
            AddMarkerToRenderer(overviewRenderer);
        }
    }
    
    void PathVisualization::RemoveFromRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer) {
        if (overviewRenderer) {
            if (pImpl->pathActor) {
                overviewRenderer->RemoveActor(pImpl->pathActor);
            }
            if (pImpl->markerActor) {
                overviewRenderer->RemoveActor(pImpl->markerActor);
            }
        }
    }
    
    void PathVisualization::ShowPath(bool show) {
        pImpl->showPath = show;
        if (pImpl->pathActor) {
            pImpl->pathActor->SetVisibility(show);
        }
    }
    
    void PathVisualization::ShowMarker(bool show) {
        pImpl->showMarker = show;
        if (pImpl->markerActor) {
            pImpl->markerActor->SetVisibility(show);
        }
    }
    
    bool PathVisualization::IsPathVisible() const {
        return pImpl->showPath;
    }
    
    bool PathVisualization::IsMarkerVisible() const {
        return pImpl->showMarker;
    }
    
    void PathVisualization::SetPathColor(double r, double g, double b) {
        pImpl->pathColor[0] = r;
        pImpl->pathColor[1] = g;
        pImpl->pathColor[2] = b;
        
        if (pImpl->pathActor) {
            pImpl->pathActor->GetProperty()->SetColor(r, g, b);
        }
    }
    
    void PathVisualization::SetMarkerColor(double r, double g, double b) {
        pImpl->markerColor[0] = r;
        pImpl->markerColor[1] = g;
        pImpl->markerColor[2] = b;
        
        if (pImpl->markerActor) {
            pImpl->markerActor->GetProperty()->SetColor(r, g, b);
        }
    }
    
    void PathVisualization::SetPathOpacity(double opacity) {
        pImpl->pathOpacity = opacity;
        if (pImpl->pathActor) {
            pImpl->pathActor->GetProperty()->SetOpacity(opacity);
        }
    }
    
    void PathVisualization::SetMarkerRadius(double radius) {
        pImpl->markerRadius = radius;
        if (pImpl->positionMarker) {
            pImpl->positionMarker->SetRadius(radius);
        }
    }
    
    void PathVisualization::SetPathTubeRadius(double radius) {
        pImpl->pathTubeRadius = radius;
        // 如果需要重新生成路径，可以在这里实现
    }
    
    CameraPath* PathVisualization::GetCameraPath() const {
        return pImpl->cameraPath;
    }
    
    int PathVisualization::GetTotalPathNodes() const {
        return pImpl->cameraPath ? pImpl->cameraPath->GetTotalNodes() : 0;
    }
    
    void PathVisualization::ClearPath() {
        pImpl->pathActor = nullptr;
        pImpl->pathMapper = nullptr;
        pImpl->ownedCameraPath.reset();
        if (pImpl->cameraPath == pImpl->ownedCameraPath.get()) {
            pImpl->cameraPath = nullptr;
        }
    }
    
    void PathVisualization::ClearAll() {
        ClearPath();
        pImpl->markerActor = nullptr;
        pImpl->markerMapper = nullptr;
        pImpl->positionMarker = nullptr;
    }
    
    void PathVisualization::SetOverviewWindow(vtkRenderWindow* window) {
        pImpl->overviewWindow = window;
    }
    
} // namespace BronchoscopyLib