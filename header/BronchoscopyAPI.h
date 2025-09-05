#ifndef BRONCHOSCOPY_API_H
#define BRONCHOSCOPY_API_H

#include <memory>
#include <vector>

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
        
        // Camera control
        void ResetCameras();
        void Render();
        
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