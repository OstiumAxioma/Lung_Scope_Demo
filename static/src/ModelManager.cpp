#include "ModelManager.h"
#include "ShaderSystem.h"

// VTK头文件
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkPolyDataNormals.h>

#include <iostream>

namespace BronchoscopyLib {
    
    class ModelManager::Impl {
    public:
        // 模型数据
        vtkSmartPointer<vtkPolyData> airwayModel;
        vtkSmartPointer<vtkPolyData> smoothedModel;  // 平滑法线后的模型
        
        // 独立的mapper（每个Actor独立的mapper避免渲染冲突）
        vtkSmartPointer<vtkPolyDataMapper> overviewMapper;
        vtkSmartPointer<vtkPolyDataMapper> endoscopeMapper;
        
        // Actor
        vtkSmartPointer<vtkActor> overviewActor;
        vtkSmartPointer<vtkActor> endoscopeActor;
        
        // 默认颜色和透明度设置
        double overviewColor[3];
        double endoscopeColor[3];
        double overviewOpacity;
        double smoothingAngle;  // 平滑角度
        
        Impl() : overviewOpacity(0.7), smoothingAngle(80.0) {
            // 默认颜色
            overviewColor[0] = 0.8;
            overviewColor[1] = 0.8;
            overviewColor[2] = 0.9;
            
            endoscopeColor[0] = 0.9;
            endoscopeColor[1] = 0.7;
            endoscopeColor[2] = 0.7;
        }
        
        void CreateMappers() {
            if (!airwayModel) return;
            
            // 生成平滑法线
            vtkSmartPointer<vtkPolyDataNormals> normalGenerator = 
                vtkSmartPointer<vtkPolyDataNormals>::New();
            normalGenerator->SetInputData(airwayModel);
            normalGenerator->ComputePointNormalsOn();
            normalGenerator->ComputeCellNormalsOff();
            
            // 设置特征角度 - 控制哪些边缘保持锐利
            // 180度 = 完全平滑，30-60度 = 保留明显的边缘
            normalGenerator->SetFeatureAngle(smoothingAngle);
            
            // 分割锐利边缘
            normalGenerator->SplittingOn();
            
            // 确保法线一致性
            normalGenerator->ConsistencyOn();
            
            // 自动确定法线方向
            normalGenerator->AutoOrientNormalsOn();
            
            normalGenerator->Update();
            smoothedModel = normalGenerator->GetOutput();
            
            // 使用OpenGLPolyDataMapper以支持自定义shader
            overviewMapper = vtkSmartPointer<vtkOpenGLPolyDataMapper>::New();
            overviewMapper->SetInputData(smoothedModel);
            overviewMapper->ScalarVisibilityOff();
            
            endoscopeMapper = vtkSmartPointer<vtkOpenGLPolyDataMapper>::New();
            endoscopeMapper->SetInputData(smoothedModel);
            endoscopeMapper->ScalarVisibilityOff();
            
            std::cout << "ModelManager: Applied smooth shading with feature angle " 
                     << smoothingAngle << " degrees" << std::endl;
        }
    };
    
    ModelManager::ModelManager() : pImpl(std::make_unique<Impl>()) {
    }
    
    ModelManager::~ModelManager() = default;
    
    bool ModelManager::LoadModel(vtkPolyData* polyData) {
        if (!polyData) {
            std::cerr << "ModelManager: Invalid polyData (null)" << std::endl;
            return false;
        }
        
        std::cout << "\n=== ModelManager: LoadModel ===" << std::endl;
        std::cout << "Input PolyData points: " << polyData->GetNumberOfPoints() << std::endl;
        std::cout << "Input PolyData cells: " << polyData->GetNumberOfCells() << std::endl;
        
        // 深拷贝polyData，确保数据的生命周期独立于外部
        pImpl->airwayModel = vtkSmartPointer<vtkPolyData>::New();
        pImpl->airwayModel->DeepCopy(polyData);
        
        // 创建mappers
        pImpl->CreateMappers();
        
        // 如果Actor已存在，更新它们的mapper
        if (pImpl->overviewActor && pImpl->overviewMapper) {
            pImpl->overviewActor->SetMapper(pImpl->overviewMapper);
        }
        if (pImpl->endoscopeActor && pImpl->endoscopeMapper) {
            pImpl->endoscopeActor->SetMapper(pImpl->endoscopeMapper);
        }
        
        std::cout << "Model loaded successfully" << std::endl;
        std::cout << "================================" << std::endl;
        
        return true;
    }
    
    vtkPolyData* ModelManager::GetModelData() const {
        return pImpl->airwayModel;
    }
    
    vtkActor* ModelManager::CreateOverviewActor() {
        if (!pImpl->airwayModel || !pImpl->overviewMapper) {
            std::cerr << "ModelManager: Cannot create overview actor without model data" << std::endl;
            return nullptr;
        }
        
        pImpl->overviewActor = vtkSmartPointer<vtkActor>::New();
        pImpl->overviewActor->SetMapper(pImpl->overviewMapper);
        pImpl->overviewActor->GetProperty()->SetColor(pImpl->overviewColor);
        pImpl->overviewActor->GetProperty()->SetOpacity(pImpl->overviewOpacity);
        
        return pImpl->overviewActor;
    }
    
    vtkActor* ModelManager::CreateEndoscopeActor() {
        if (!pImpl->airwayModel || !pImpl->endoscopeMapper) {
            std::cerr << "ModelManager: Cannot create endoscope actor without model data" << std::endl;
            return nullptr;
        }
        
        pImpl->endoscopeActor = vtkSmartPointer<vtkActor>::New();
        pImpl->endoscopeActor->SetMapper(pImpl->endoscopeMapper);
        pImpl->endoscopeActor->GetProperty()->SetColor(pImpl->endoscopeColor);
        
        return pImpl->endoscopeActor;
    }
    
    vtkActor* ModelManager::GetOverviewActor() const {
        return pImpl->overviewActor;
    }
    
    vtkActor* ModelManager::GetEndoscopeActor() const {
        return pImpl->endoscopeActor;
    }
    
    void ModelManager::AddToRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer) {
        // 初始化ShaderSystem
        static ShaderSystem shaderSystem;
        static bool shaderInitialized = false;
        if (!shaderInitialized) {
            shaderSystem.Initialize();
            shaderInitialized = true;
        }
        
        // 创建Actor如果还不存在
        if (!pImpl->overviewActor && pImpl->airwayModel) {
            CreateOverviewActor();
        }
        if (!pImpl->endoscopeActor && pImpl->airwayModel) {
            CreateEndoscopeActor();
        }
        
        // 添加到渲染器并应用shader
        if (overviewRenderer && pImpl->overviewActor) {
            overviewRenderer->AddActor(pImpl->overviewActor);
            
            // 先应用视图shader
            ShaderSystem::ShaderConfig config(ShaderSystem::SURFACE, 
                                             ShaderSystem::EFFECT_NONE, 
                                             ShaderSystem::VIEW_OVERVIEW);
            shaderSystem.ApplyShader(pImpl->overviewActor, config);
            
            // 再应用材质shader（不会清除之前的替换）
            shaderSystem.ApplyMaterialShader(pImpl->overviewActor, ShaderSystem::MATERIAL_TISSUE);
            
            std::cout << "Overview actor added with tissue material and view shader" << std::endl;
        }
        
        if (endoscopeRenderer && pImpl->endoscopeActor) {
            endoscopeRenderer->AddActor(pImpl->endoscopeActor);
            
            // 先应用视图shader
            ShaderSystem::ShaderConfig config(ShaderSystem::SURFACE, 
                                             ShaderSystem::EFFECT_NONE, 
                                             ShaderSystem::VIEW_ENDOSCOPE);
            shaderSystem.ApplyShader(pImpl->endoscopeActor, config);
            
            // 再应用材质shader（不会清除之前的替换）
            shaderSystem.ApplyMaterialShader(pImpl->endoscopeActor, ShaderSystem::MATERIAL_TISSUE);
            
            std::cout << "Endoscope actor added with tissue material and view shader" << std::endl;
        }
    }
    
    void ModelManager::RemoveFromRenderers(vtkRenderer* overviewRenderer, vtkRenderer* endoscopeRenderer) {
        if (overviewRenderer && pImpl->overviewActor) {
            overviewRenderer->RemoveActor(pImpl->overviewActor);
        }
        
        if (endoscopeRenderer && pImpl->endoscopeActor) {
            endoscopeRenderer->RemoveActor(pImpl->endoscopeActor);
        }
    }
    
    void ModelManager::SetOverviewOpacity(double opacity) {
        pImpl->overviewOpacity = opacity;
        if (pImpl->overviewActor) {
            pImpl->overviewActor->GetProperty()->SetOpacity(opacity);
        }
    }
    
    void ModelManager::SetOverviewColor(double r, double g, double b) {
        pImpl->overviewColor[0] = r;
        pImpl->overviewColor[1] = g;
        pImpl->overviewColor[2] = b;
        
        if (pImpl->overviewActor) {
            pImpl->overviewActor->GetProperty()->SetColor(r, g, b);
        }
    }
    
    void ModelManager::SetEndoscopeColor(double r, double g, double b) {
        pImpl->endoscopeColor[0] = r;
        pImpl->endoscopeColor[1] = g;
        pImpl->endoscopeColor[2] = b;
        
        if (pImpl->endoscopeActor) {
            pImpl->endoscopeActor->GetProperty()->SetColor(r, g, b);
        }
    }
    
    void ModelManager::SetSmoothingAngle(double angle) {
        // 限制角度范围在0-180度
        if (angle < 0.0) angle = 0.0;
        if (angle > 180.0) angle = 180.0;
        
        pImpl->smoothingAngle = angle;
        
        // 如果模型已加载，重新创建mapper以应用新的平滑度
        if (pImpl->airwayModel) {
            pImpl->CreateMappers();
            
            // 更新Actor的mapper
            if (pImpl->overviewActor && pImpl->overviewMapper) {
                pImpl->overviewActor->SetMapper(pImpl->overviewMapper);
            }
            if (pImpl->endoscopeActor && pImpl->endoscopeMapper) {
                pImpl->endoscopeActor->SetMapper(pImpl->endoscopeMapper);
            }
            
            std::cout << "ModelManager: Updated smoothing angle to " << angle << " degrees" << std::endl;
        }
    }
    
    void ModelManager::GetModelBounds(double bounds[6]) const {
        if (pImpl->airwayModel) {
            pImpl->airwayModel->GetBounds(bounds);
        } else {
            // 返回无效边界
            for (int i = 0; i < 6; i++) {
                bounds[i] = 0.0;
            }
        }
    }
    
    void ModelManager::ClearModel() {
        pImpl->airwayModel = nullptr;
        pImpl->overviewMapper = nullptr;
        pImpl->endoscopeMapper = nullptr;
        pImpl->overviewActor = nullptr;
        pImpl->endoscopeActor = nullptr;
    }
    
    bool ModelManager::HasModel() const {
        return pImpl->airwayModel != nullptr;
    }
    
    void ModelManager::PrintModelInfo() const {
        if (!pImpl->airwayModel) {
            std::cout << "No model loaded" << std::endl;
            return;
        }
        
        std::cout << "=== Model Info ===" << std::endl;
        std::cout << "Points: " << pImpl->airwayModel->GetNumberOfPoints() << std::endl;
        std::cout << "Cells: " << pImpl->airwayModel->GetNumberOfCells() << std::endl;
        
        double bounds[6];
        GetModelBounds(bounds);
        std::cout << "Bounds: [" 
                  << bounds[0] << ", " << bounds[1] << "] ["
                  << bounds[2] << ", " << bounds[3] << "] ["
                  << bounds[4] << ", " << bounds[5] << "]" << std::endl;
        std::cout << "==================" << std::endl;
    }
    
} // namespace BronchoscopyLib