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

#include <iostream>

namespace BronchoscopyLib {
    
    class ModelManager::Impl {
    public:
        // 模型数据
        vtkSmartPointer<vtkPolyData> airwayModel;
        
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
        
        Impl() : overviewOpacity(0.7) {
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
            
            // 使用OpenGLPolyDataMapper以支持自定义shader
            overviewMapper = vtkSmartPointer<vtkOpenGLPolyDataMapper>::New();
            overviewMapper->SetInputData(airwayModel);
            overviewMapper->ScalarVisibilityOff();
            
            endoscopeMapper = vtkSmartPointer<vtkOpenGLPolyDataMapper>::New();
            endoscopeMapper->SetInputData(airwayModel);
            endoscopeMapper->ScalarVisibilityOff();
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
            // 应用overview shader
            ShaderSystem::ShaderConfig config(ShaderSystem::SURFACE, 
                                             ShaderSystem::EFFECT_NONE, 
                                             ShaderSystem::VIEW_OVERVIEW);
            shaderSystem.ApplyShader(pImpl->overviewActor, config);
            
            overviewRenderer->AddActor(pImpl->overviewActor);
            std::cout << "Overview actor added to renderer with shader" << std::endl;
        }
        
        if (endoscopeRenderer && pImpl->endoscopeActor) {
            // 应用endoscope shader
            ShaderSystem::ShaderConfig config(ShaderSystem::SURFACE, 
                                             ShaderSystem::EFFECT_NONE, 
                                             ShaderSystem::VIEW_ENDOSCOPE);
            shaderSystem.ApplyShader(pImpl->endoscopeActor, config);
            
            endoscopeRenderer->AddActor(pImpl->endoscopeActor);
            std::cout << "Endoscope actor added to renderer with shader" << std::endl;
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