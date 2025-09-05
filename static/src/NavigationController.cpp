#include "NavigationController.h"
#include "CameraPath.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace BronchoscopyLib {
    
    class NavigationController::Impl {
    public:
        // 路径引用（不拥有）
        CameraPath* cameraPath;
        
        // 当前状态
        PathNode* currentNode;
        int currentIndex;
        
        // 自动播放状态
        bool isPlaying;
        bool isPaused;
        double playSpeed;
        int playIntervalMs;
        bool loopMode;
        
        // 回调函数
        NavigationCallback navigationCallback;
        PlaybackEndCallback playbackEndCallback;
        
        // 自动播放线程控制（简化版，实际应用中需要更复杂的线程管理）
        bool stopPlaybackThread;
        
        Impl() : cameraPath(nullptr), currentNode(nullptr), currentIndex(-1),
                 isPlaying(false), isPaused(false), playSpeed(1.0),
                 playIntervalMs(100), loopMode(false), stopPlaybackThread(false) {
        }
        
        void UpdateCurrentState() {
            if (cameraPath) {
                currentNode = cameraPath->GetCurrent();
                currentIndex = cameraPath->GetCurrentIndex();
                
                // 触发导航回调
                if (navigationCallback && currentNode) {
                    navigationCallback(currentNode, currentIndex);
                }
            }
        }
        
        void CheckPlaybackEnd() {
            if (isPlaying && cameraPath && cameraPath->IsAtEnd()) {
                if (loopMode) {
                    // 循环模式：回到开始
                    cameraPath->Reset();
                    UpdateCurrentState();
                } else {
                    // 非循环模式：停止播放
                    isPlaying = false;
                    if (playbackEndCallback) {
                        playbackEndCallback();
                    }
                }
            }
        }
    };
    
    NavigationController::NavigationController() : pImpl(std::make_unique<Impl>()) {
    }
    
    NavigationController::~NavigationController() {
        // 确保停止自动播放
        StopAutoPlay();
    }
    
    void NavigationController::SetCameraPath(CameraPath* path) {
        // 停止当前播放
        StopAutoPlay();
        
        pImpl->cameraPath = path;
        if (path) {
            path->Reset();
            pImpl->UpdateCurrentState();
            std::cout << "NavigationController: Camera path set with " 
                     << path->GetTotalNodes() << " nodes" << std::endl;
        }
    }
    
    bool NavigationController::MoveToNext() {
        if (!pImpl->cameraPath) return false;
        
        bool result = pImpl->cameraPath->MoveNext();
        if (result) {
            pImpl->UpdateCurrentState();
            std::cout << "NavigationController: Moved to node " 
                     << (pImpl->currentIndex + 1) << " / " 
                     << pImpl->cameraPath->GetTotalNodes() << std::endl;
        } else {
            pImpl->CheckPlaybackEnd();
        }
        
        return result;
    }
    
    bool NavigationController::MoveToPrevious() {
        if (!pImpl->cameraPath) return false;
        
        bool result = pImpl->cameraPath->MovePrevious();
        if (result) {
            pImpl->UpdateCurrentState();
            std::cout << "NavigationController: Moved to node " 
                     << (pImpl->currentIndex + 1) << " / " 
                     << pImpl->cameraPath->GetTotalNodes() << std::endl;
        }
        
        return result;
    }
    
    void NavigationController::MoveToFirst() {
        if (!pImpl->cameraPath) return;
        
        pImpl->cameraPath->Reset();
        pImpl->UpdateCurrentState();
        std::cout << "NavigationController: Moved to first node" << std::endl;
    }
    
    void NavigationController::MoveToLast() {
        if (!pImpl->cameraPath) return;
        
        // 移动到最后
        while (pImpl->cameraPath->MoveNext()) {
            // 继续移动
        }
        pImpl->UpdateCurrentState();
        std::cout << "NavigationController: Moved to last node" << std::endl;
    }
    
    bool NavigationController::MoveToPosition(int index) {
        if (!pImpl->cameraPath) return false;
        
        bool result = pImpl->cameraPath->JumpTo(index);
        if (result) {
            pImpl->UpdateCurrentState();
            std::cout << "NavigationController: Jumped to node " 
                     << (index + 1) << std::endl;
        }
        
        return result;
    }
    
    PathNode* NavigationController::GetCurrentNode() const {
        return pImpl->currentNode;
    }
    
    int NavigationController::GetCurrentIndex() const {
        return pImpl->currentIndex;
    }
    
    int NavigationController::GetTotalNodes() const {
        return pImpl->cameraPath ? pImpl->cameraPath->GetTotalNodes() : 0;
    }
    
    bool NavigationController::IsAtStart() const {
        return pImpl->cameraPath ? pImpl->cameraPath->IsAtStart() : true;
    }
    
    bool NavigationController::IsAtEnd() const {
        return pImpl->cameraPath ? pImpl->cameraPath->IsAtEnd() : true;
    }
    
    void NavigationController::StartAutoPlay(int intervalMs) {
        if (!pImpl->cameraPath || pImpl->isPlaying) return;
        
        pImpl->playIntervalMs = intervalMs;
        pImpl->isPlaying = true;
        pImpl->isPaused = false;
        pImpl->stopPlaybackThread = false;
        
        std::cout << "NavigationController: Auto-play started (interval: " 
                 << intervalMs << "ms)" << std::endl;
        
        // 注意：这里需要更复杂的线程管理，简化实现仅供示例
        // 实际应用中应该使用定时器或更合适的机制
    }
    
    void NavigationController::StopAutoPlay() {
        if (pImpl->isPlaying) {
            pImpl->isPlaying = false;
            pImpl->isPaused = false;
            pImpl->stopPlaybackThread = true;
            std::cout << "NavigationController: Auto-play stopped" << std::endl;
        }
    }
    
    void NavigationController::PauseAutoPlay() {
        if (pImpl->isPlaying && !pImpl->isPaused) {
            pImpl->isPaused = true;
            std::cout << "NavigationController: Auto-play paused" << std::endl;
        }
    }
    
    void NavigationController::ResumeAutoPlay() {
        if (pImpl->isPlaying && pImpl->isPaused) {
            pImpl->isPaused = false;
            std::cout << "NavigationController: Auto-play resumed" << std::endl;
        }
    }
    
    bool NavigationController::IsPlaying() const {
        return pImpl->isPlaying && !pImpl->isPaused;
    }
    
    void NavigationController::SetPlaySpeed(double speed) {
        pImpl->playSpeed = speed;
        std::cout << "NavigationController: Play speed set to " << speed << "x" << std::endl;
    }
    
    double NavigationController::GetPlaySpeed() const {
        return pImpl->playSpeed;
    }
    
    void NavigationController::SetPlayInterval(int intervalMs) {
        pImpl->playIntervalMs = intervalMs;
    }
    
    int NavigationController::GetPlayInterval() const {
        return pImpl->playIntervalMs;
    }
    
    void NavigationController::SetNavigationCallback(NavigationCallback callback) {
        pImpl->navigationCallback = callback;
    }
    
    void NavigationController::SetPlaybackEndCallback(PlaybackEndCallback callback) {
        pImpl->playbackEndCallback = callback;
    }
    
    void NavigationController::SetLoopMode(bool loop) {
        pImpl->loopMode = loop;
        std::cout << "NavigationController: Loop mode " 
                 << (loop ? "enabled" : "disabled") << std::endl;
    }
    
    bool NavigationController::GetLoopMode() const {
        return pImpl->loopMode;
    }
    
    void NavigationController::Reset() {
        StopAutoPlay();
        MoveToFirst();
    }
    
    double NavigationController::GetProgressPercentage() const {
        if (!pImpl->cameraPath || pImpl->cameraPath->GetTotalNodes() == 0) {
            return 0.0;
        }
        
        return (static_cast<double>(pImpl->currentIndex + 1) / 
                static_cast<double>(pImpl->cameraPath->GetTotalNodes())) * 100.0;
    }
    
} // namespace BronchoscopyLib