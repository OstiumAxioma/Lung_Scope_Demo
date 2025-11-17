#ifndef BRONCHOSCOPY_API_H
#define BRONCHOSCOPY_API_H

#include <memory>
#include <string>
#include <vector>
#include "BronchoscopyDataTypes.h"

// Forward declarations
class vtkPolyData;
class vtkRenderWindow;
class vtkRenderer;

namespace BronchoscopyLib {
    
    /**
     * BronchoscopyAPI - Unified external interface for the bronchoscopy visualization system
     * 
     * This class provides a simplified interface to the bronchoscopy visualization functionality,
     * encapsulating all internal modules and maintaining compatibility with existing code.
     */
    class BronchoscopyAPI {
    public:
        BronchoscopyAPI();
        ~BronchoscopyAPI();
        
        // Initialization
        void Initialize();
        
        // Model management
        bool LoadAirwayModel(vtkPolyData* polyData);
        
        // Path management
        bool LoadCameraPath(const std::vector<double>& positions);
        
        // Navigation control
        void MoveToNext();
        void MoveToPrevious();
        void MoveToFirst();
        void MoveToLast();
        int GetCurrentNodeIndex() const;
        int GetTotalPathNodes() const;
        
        // Render window setup
        void SetOverviewRenderWindow(vtkRenderWindow* window);
        void SetEndoscopeRenderWindow(vtkRenderWindow* window);
        
        // Get renderers (for embedding in Qt widgets)
        vtkRenderer* GetOverviewRenderer();
        vtkRenderer* GetEndoscopeRenderer();
        
        // Display control
        void ShowPath(bool show);
        void ShowMarker(bool show);
        bool IsPathVisible() const;
        bool IsMarkerVisible() const;
        
        // Appearance settings
        void SetPathColor(double r, double g, double b);
        void SetMarkerColor(double r, double g, double b);
        void SetPathOpacity(double opacity);
        void SetMarkerRadius(double radius);
        void SetModelOpacity(double opacity);
        // 新增：内窥镜视图路径线宽（像素）
        void SetPathLineWidth(double width);
        
        // Camera control
        void ResetCameras();
        void Render();
        void SetEndoscopeFOV(double angle);
        bool CaptureEndoscopeImage(const std::string& filePath, int width = 256, int height = 256);
        bool GetCurrentEndoscopePose(CameraPose& pose) const;
        bool SetCameraByDistance(double distance);
        double GetPathTotalLength() const;
        void ApplyRollOffset(double degrees);
        void ApplyMaterialParameters(const MaterialParameters& params);
        
        // Animation control
        bool UpdateAnimation();
        void SetAnimationDuration(double seconds);
        // Spline control
        void SetSplineT(double t);
        
        // Query state
        bool HasModel() const;
        bool HasPath() const;
        
        // 新增：自动播放控制（来自NavigationController）
        void StartAutoPlay(int intervalMs = 100);
        void StopAutoPlay();
        void PauseAutoPlay();
        void ResumeAutoPlay();
        bool IsPlaying() const;
        void SetPlaySpeed(double speed);
        double GetPlaySpeed() const;
        void SetLoopMode(bool loop);
        bool GetLoopMode() const;
        
        // 新增：进度控制（来自NavigationController）
        bool MoveToPosition(int index);
        double GetProgressPercentage() const;
        bool IsAtStart() const;
        bool IsAtEnd() const;
        
        // 新增：场景管理（来自SceneManager）
        void ClearScene();
        void ClearModel();
        void ClearPath();
        void PrintSceneInfo() const;
        
        // 新增：渲染控制（来自RenderingEngine）
        void SetOverviewBackground(double r, double g, double b);
        void SetEndoscopeBackground(double r, double g, double b);
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // BRONCHOSCOPY_API_H
