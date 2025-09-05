#ifndef NAVIGATION_CONTROLLER_H
#define NAVIGATION_CONTROLLER_H

#include <memory>
#include <functional>

namespace BronchoscopyLib {
    
    // 前向声明
    class CameraPath;
    struct PathNode;
    
    /**
     * NavigationController - 管理路径导航和自动播放
     * 负责路径导航控制、自动播放、导航状态管理
     */
    class NavigationController {
    public:
        NavigationController();
        ~NavigationController();
        
        // 设置相机路径
        void SetCameraPath(CameraPath* path);
        
        // 导航控制
        bool MoveToNext();
        bool MoveToPrevious();
        void MoveToFirst();
        void MoveToLast();
        bool MoveToPosition(int index);
        
        // 获取当前状态
        PathNode* GetCurrentNode() const;
        int GetCurrentIndex() const;
        int GetTotalNodes() const;
        bool IsAtStart() const;
        bool IsAtEnd() const;
        
        // 自动播放控制
        void StartAutoPlay(int intervalMs = 100);
        void StopAutoPlay();
        void PauseAutoPlay();
        void ResumeAutoPlay();
        bool IsPlaying() const;
        void SetPlaySpeed(double speed);  // 1.0 = normal, 2.0 = 2x speed
        double GetPlaySpeed() const;
        void SetPlayInterval(int intervalMs);
        int GetPlayInterval() const;
        
        // 设置导航回调（当位置改变时调用）
        using NavigationCallback = std::function<void(PathNode*, int)>;
        void SetNavigationCallback(NavigationCallback callback);
        
        // 设置播放结束回调
        using PlaybackEndCallback = std::function<void()>;
        void SetPlaybackEndCallback(PlaybackEndCallback callback);
        
        // 导航循环模式
        void SetLoopMode(bool loop);
        bool GetLoopMode() const;
        
        // 重置导航状态
        void Reset();
        
        // 获取进度百分比
        double GetProgressPercentage() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };
    
} // namespace BronchoscopyLib

#endif // NAVIGATION_CONTROLLER_H