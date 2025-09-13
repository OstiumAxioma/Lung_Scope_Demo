#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// VTK模块初始化 - 解决 "no override found for 'vtkRenderer'" 错误
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle)

#include <QMainWindow>
#include <memory>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMenuBar;
class QStatusBar;
class QToolBar;
class QLabel;
class QTimer;
QT_END_NAMESPACE

class QVTKOpenGLWidget;

// 前向声明
namespace BronchoscopyLib {
    class BronchoscopyAPI;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void setupDualViewWidget();
    void loadAirwayModel();
    void loadCameraPath();
    void navigateNext();
    void navigatePrevious();
    void resetNavigation();
    void toggleAutoPlay();
    void updateAnimation();  // 更新动画帧

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    // UI组件 - 双窗口
    QVTKOpenGLWidget *overviewWidget;   // 左侧：全局视图
    QVTKOpenGLWidget *endoscopeWidget;  // 右侧：内窥镜视图
    
    // 菜单和工具栏
    QMenu *fileMenu;
    QMenu *viewMenu;
    QMenu *navigationMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QToolBar *navigationToolBar;
    
    // 动作
    QAction *loadModelAct;
    QAction *loadPathAct;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *resetAct;
    QAction *playAct;
    
    // 状态标签
    QLabel *statusLabel;
    
    // 使用静态库的支气管镜API
    std::unique_ptr<BronchoscopyLib::BronchoscopyAPI> bronchoscopyAPI;
    
    // 定时器
    QTimer *autoPlayTimer;     // 自动播放定时器
    QTimer *animationTimer;     // 动画更新定时器
    bool isPlaying;
    bool isAnimating;           // 是否正在动画过渡中
};

#endif // MAINWINDOW_H