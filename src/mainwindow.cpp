#include "mainwindow.h"
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTimer>
#include <QLabel>
#include <QVTKOpenGLWidget.h>
#include <QKeyEvent>
#include <QDebug>

// 包含静态库头文件
#include "BronchoscopyAPI.h"

// VTK头文件
#include <vtkRenderWindow.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkOBJReader.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

#include <fstream>
#include <sstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , overviewWidget(nullptr)
    , endoscopeWidget(nullptr)
    , autoPlayTimer(nullptr)
    , animationTimer(nullptr)
    , isPlaying(false)
    , isAnimating(false)
    , bronchoscopyAPI(std::make_unique<BronchoscopyLib::BronchoscopyAPI>())
{
    setWindowTitle("支气管腔镜可视化系统");
    resize(1200, 600);

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    
    // 初始化API
    bronchoscopyAPI->Initialize();
    
    // 设置双窗口界面
    setupDualViewWidget();
    
    // 设置自动播放定时器
    autoPlayTimer = new QTimer(this);
    connect(autoPlayTimer, &QTimer::timeout, this, &MainWindow::navigateNext);
    
    // 设置动画更新定时器（60FPS）
    animationTimer = new QTimer(this);
    animationTimer->setInterval(16);  // 约60FPS
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::updateAnimation);
}

MainWindow::~MainWindow()
{
}

void MainWindow::createActions()
{
    // 文件菜单动作
    loadModelAct = new QAction("加载气管模型(&M)...", this);
    loadModelAct->setShortcut(QKeySequence("Ctrl+M"));
    loadModelAct->setStatusTip("加载气管模型文件 (.vtk, .vtp, .stl)");
    connect(loadModelAct, &QAction::triggered, this, &MainWindow::loadAirwayModel);
    
    loadPathAct = new QAction("加载相机路径(&P)...", this);
    loadPathAct->setShortcut(QKeySequence("Ctrl+P"));
    loadPathAct->setStatusTip("加载相机路径文件 (.txt, .csv)");
    connect(loadPathAct, &QAction::triggered, this, &MainWindow::loadCameraPath);
    
    exitAct = new QAction("退出(&Q)", this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip("退出应用程序");
    connect(exitAct, &QAction::triggered, this, &QWidget::close);
    
    // 导航菜单动作
    nextAct = new QAction("前进(&N)", this);
    nextAct->setShortcut(QKeySequence("Right"));
    nextAct->setStatusTip("沿路径前进");
    nextAct->setEnabled(false);
    connect(nextAct, &QAction::triggered, this, &MainWindow::navigateNext);
    
    previousAct = new QAction("后退(&B)", this);
    previousAct->setShortcut(QKeySequence("Left"));
    previousAct->setStatusTip("沿路径后退");
    previousAct->setEnabled(false);
    connect(previousAct, &QAction::triggered, this, &MainWindow::navigatePrevious);
    
    resetAct = new QAction("重置(&R)", this);
    resetAct->setShortcut(QKeySequence("Home"));
    resetAct->setStatusTip("回到路径起点");
    resetAct->setEnabled(false);
    connect(resetAct, &QAction::triggered, this, &MainWindow::resetNavigation);
    
    playAct = new QAction("播放/暂停(&P)", this);
    playAct->setShortcut(QKeySequence("Space"));
    playAct->setStatusTip("自动播放路径");
    playAct->setEnabled(false);
    playAct->setCheckable(true);
    connect(playAct, &QAction::triggered, this, &MainWindow::toggleAutoPlay);
    
    // 帮助菜单动作
    aboutAct = new QAction("关于(&A)", this);
    aboutAct->setStatusTip("显示关于对话框");
    connect(aboutAct, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于支气管腔镜可视化",
                          "支气管腔镜可视化系统\n\n"
                          "功能特点：\n"
                          "- 双窗口显示（全局视图 + 内窥镜视图）\n"
                          "- 相机路径导航\n"
                          "- 实时位置跟踪\n\n"
                          "基于VTK和Qt开发");
    });
}

void MainWindow::createMenus()
{
    // 文件菜单
    fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction(loadModelAct);
    fileMenu->addAction(loadPathAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
    
    // 导航菜单
    navigationMenu = menuBar()->addMenu("导航(&N)");
    navigationMenu->addAction(previousAct);
    navigationMenu->addAction(nextAct);
    navigationMenu->addAction(resetAct);
    navigationMenu->addSeparator();
    navigationMenu->addAction(playAct);
    
    // 视图菜单
    viewMenu = menuBar()->addMenu("视图(&V)");
    QAction* showPathAct = viewMenu->addAction("显示路径");
    showPathAct->setCheckable(true);
    showPathAct->setChecked(true);
    connect(showPathAct, &QAction::toggled, [this](bool checked) {
        bronchoscopyAPI->ShowPath(checked);
    });
    
    QAction* showMarkerAct = viewMenu->addAction("显示位置标记");
    showMarkerAct->setCheckable(true);
    showMarkerAct->setChecked(true);
    connect(showMarkerAct, &QAction::toggled, [this](bool checked) {
        bronchoscopyAPI->ShowMarker(checked);
    });
    
    // 帮助菜单
    helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars()
{
    // 文件工具栏
    fileToolBar = addToolBar("文件");
    fileToolBar->addAction(loadModelAct);
    fileToolBar->addAction(loadPathAct);
    
    // 导航工具栏
    navigationToolBar = addToolBar("导航");
    navigationToolBar->addAction(previousAct);
    navigationToolBar->addAction(nextAct);
    navigationToolBar->addAction(resetAct);
    navigationToolBar->addSeparator();
    navigationToolBar->addAction(playAct);
}

void MainWindow::createStatusBar()
{
    statusLabel = new QLabel("就绪");
    statusBar()->addPermanentWidget(statusLabel);
    statusBar()->showMessage("请加载气管模型和相机路径", 3000);
}

void MainWindow::setupDualViewWidget()
{
    // 创建两个VTK窗口
    overviewWidget = new QVTKOpenGLWidget(this);
    endoscopeWidget = new QVTKOpenGLWidget(this);
    
    // 强制初始化OpenGL上下文
    overviewWidget->SetRenderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New());
    endoscopeWidget->SetRenderWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New());
    
    // 创建分割器
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    
    // 左侧：全局视图
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    QLabel *leftLabel = new QLabel("全局视图");
    leftLabel->setAlignment(Qt::AlignCenter);
    leftLabel->setFixedHeight(25);  // 固定标签高度
    leftLabel->setStyleSheet("font-weight: bold; font-size: 12px; padding: 3px; background-color: #e0e0e0; border: 1px solid #ccc;");
    leftLayout->addWidget(leftLabel);
    leftLayout->addWidget(overviewWidget, 1);  // 添加stretch因子，让VTK窗口占据剩余空间
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    
    // 右侧：内窥镜视图
    QWidget *rightPanel = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    QLabel *rightLabel = new QLabel("内窥镜视图");
    rightLabel->setAlignment(Qt::AlignCenter);
    rightLabel->setFixedHeight(25);  // 固定标签高度
    rightLabel->setStyleSheet("font-weight: bold; font-size: 12px; padding: 3px; background-color: #e0e0e0; border: 1px solid #ccc;");
    rightLayout->addWidget(rightLabel);
    rightLayout->addWidget(endoscopeWidget, 1);  // 添加stretch因子，让VTK窗口占据剩余空间
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    
    // 添加到分割器
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setSizes(QList<int>() << 600 << 600);
    
    // 设置为中央部件
    setCentralWidget(splitter);
    
    // 连接渲染窗口到API
    bronchoscopyAPI->SetOverviewRenderWindow(overviewWidget->GetRenderWindow());
    bronchoscopyAPI->SetEndoscopeRenderWindow(endoscopeWidget->GetRenderWindow());
    
    // 调试：确保渲染窗口和交互器正确连接
    qDebug() << "Overview RenderWindow:" << overviewWidget->GetRenderWindow();
    qDebug() << "Overview Interactor:" << overviewWidget->GetInteractor();
    qDebug() << "Endoscope RenderWindow:" << endoscopeWidget->GetRenderWindow();
    qDebug() << "Endoscope Interactor:" << endoscopeWidget->GetInteractor();
    
    // 确保交互器已初始化
    if (overviewWidget->GetInteractor()) {
        overviewWidget->GetInteractor()->Initialize();
    }
    if (endoscopeWidget->GetInteractor()) {
        endoscopeWidget->GetInteractor()->Initialize();
    }
    
    // 初始渲染
    bronchoscopyAPI->Render();
}

void MainWindow::loadAirwayModel()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "选择气管模型文件", 
        "",
        "3D模型文件 (*.vtk *.vtp *.obj);;VTK文件 (*.vtk);;VTP文件 (*.vtp);;OBJ文件 (*.obj);;所有文件 (*)");
    
    if (fileName.isEmpty()) return;
    
    // 主程序负责文件读取
    vtkSmartPointer<vtkPolyData> polyData;
    std::string fileStr = fileName.toStdString();
    
    if (fileName.endsWith(".vtk", Qt::CaseInsensitive)) {
        vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
        reader->SetFileName(fileStr.c_str());
        reader->Update();
        polyData = reader->GetOutput();
    } else if (fileName.endsWith(".vtp", Qt::CaseInsensitive)) {
        vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        reader->SetFileName(fileStr.c_str());
        reader->Update();
        polyData = reader->GetOutput();
    } else if (fileName.endsWith(".obj", Qt::CaseInsensitive)) {
        vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
        reader->SetFileName(fileStr.c_str());
        reader->Update();
        polyData = reader->GetOutput();
    } else {
        QMessageBox::warning(this, "加载失败", "不支持的文件格式");
        return;
    }
    
    // 传递数据给静态库
    if (polyData && bronchoscopyAPI->LoadAirwayModel(polyData)) {
        statusBar()->showMessage(QString("成功加载模型: %1").arg(fileName), 3000);
        statusLabel->setText("模型已加载");
        
        // 强制刷新两个视图
        overviewWidget->GetRenderWindow()->Render();
        endoscopeWidget->GetRenderWindow()->Render();
        
        qDebug() << "Forced render after loading model";
    } else {
        QMessageBox::warning(this, "加载失败", "无法加载模型文件");
        statusBar()->showMessage("模型加载失败", 3000);
    }
}

void MainWindow::loadCameraPath()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "选择相机路径文件",
        "",
        "路径文件 (*.txt *.csv);;文本文件 (*.txt);;CSV文件 (*.csv);;所有文件 (*)");
    
    if (fileName.isEmpty()) return;
    
    // 主程序负责文件读取
    std::ifstream file(fileName.toStdString());
    if (!file.is_open()) {
        QMessageBox::warning(this, "加载失败", "无法打开文件");
        return;
    }
    
    std::vector<double> positions;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;  // 跳过空行和注释
        
        std::istringstream iss(line);
        double x, y, z;
        char comma;
        
        // 支持两种格式：
        // 格式1: x, y, z （只有位置）
        // 格式2: x, y, z, dx, dy, dz （位置和方向，但忽略方向）
        
        if (line.find(',') != std::string::npos) {
            // 逗号分隔
            iss >> x >> comma >> y >> comma >> z;
        } else {
            // 空格分隔
            iss >> x >> y >> z;
        }
        
        if (!iss.fail()) {
            positions.push_back(x);
            positions.push_back(y);
            positions.push_back(z);
        }
    }
    
    file.close();
    
    // 传递数据给静态库（只传递位置，方向自动计算）
    if (!positions.empty() && bronchoscopyAPI->LoadCameraPath(positions)) {
        int total = bronchoscopyAPI->GetTotalPathNodes();
        statusBar()->showMessage(QString("成功加载路径: %1 (%2个节点)").arg(fileName).arg(total), 3000);
        statusLabel->setText(QString("路径: 1/%1").arg(total));
        
        // 启用导航控制
        nextAct->setEnabled(true);
        previousAct->setEnabled(true);
        resetAct->setEnabled(true);
        playAct->setEnabled(true);
        
        // 强制刷新endoscope视图（相机位置已更新）
        endoscopeWidget->GetRenderWindow()->Render();
        qDebug() << "Forced render after loading path";
    } else {
        QMessageBox::warning(this, "加载失败", "无法加载路径文件，请检查文件格式\n需要至少2个点");
        statusBar()->showMessage("路径加载失败", 3000);
    }
}

void MainWindow::navigateNext()
{
    // 如果正在动画中，跳过
    if (isAnimating) {
        return;
    }
    
    bronchoscopyAPI->MoveToNext();
    int current = bronchoscopyAPI->GetCurrentNodeIndex() + 1;
    int total = bronchoscopyAPI->GetTotalPathNodes();
    statusLabel->setText(QString("路径: %1/%2").arg(current).arg(total));
    
    // 调试输出
    qDebug() << "Current index:" << (current-1) << "Total nodes:" << total;
    
    // 启动动画定时器
    isAnimating = true;
    animationTimer->start();
    
    // 如果到达末尾，停止自动播放
    if (current >= total && isPlaying) {
        toggleAutoPlay();
    }
}

void MainWindow::navigatePrevious()
{
    // 如果正在动画中，跳过
    if (isAnimating) {
        return;
    }
    
    bronchoscopyAPI->MoveToPrevious();
    int current = bronchoscopyAPI->GetCurrentNodeIndex() + 1;
    int total = bronchoscopyAPI->GetTotalPathNodes();
    statusLabel->setText(QString("路径: %1/%2").arg(current).arg(total));
    
    // 启动动画定时器
    isAnimating = true;
    animationTimer->start();
}

void MainWindow::resetNavigation()
{
    bronchoscopyAPI->MoveToFirst();
    int total = bronchoscopyAPI->GetTotalPathNodes();
    statusLabel->setText(QString("路径: 1/%1").arg(total));
    
    // 停止自动播放
    if (isPlaying) {
        toggleAutoPlay();
    }
}

void MainWindow::toggleAutoPlay()
{
    isPlaying = !isPlaying;
    
    if (isPlaying) {
        autoPlayTimer->start(100); // 每100ms前进一步
        statusBar()->showMessage("开始自动播放", 2000);
        playAct->setText("暂停(&P)");
    } else {
        autoPlayTimer->stop();
        statusBar()->showMessage("停止自动播放", 2000);
        playAct->setText("播放(&P)");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Right:
        case Qt::Key_Up:
            if (nextAct->isEnabled()) navigateNext();
            break;
        case Qt::Key_Left:
        case Qt::Key_Down:
            if (previousAct->isEnabled()) navigatePrevious();
            break;
        case Qt::Key_Home:
            if (resetAct->isEnabled()) resetNavigation();
            break;
        case Qt::Key_Space:
            if (playAct->isEnabled()) toggleAutoPlay();
            break;
        case Qt::Key_Plus:
            if (autoPlayTimer && isPlaying) {
                int interval = autoPlayTimer->interval();
                if (interval > 20) {
                    autoPlayTimer->setInterval(interval - 20);
                    statusBar()->showMessage(QString("播放速度: %1ms").arg(interval - 20), 1000);
                }
            }
            break;
        case Qt::Key_Minus:
            if (autoPlayTimer && isPlaying) {
                int interval = autoPlayTimer->interval();
                if (interval < 1000) {
                    autoPlayTimer->setInterval(interval + 20);
                    statusBar()->showMessage(QString("播放速度: %1ms").arg(interval + 20), 1000);
                }
            }
            break;
        default:
            QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::updateAnimation()
{
    // 调用API的动画更新函数
    bool stillAnimating = bronchoscopyAPI->UpdateAnimation();
    
    // 刷新渲染窗口
    if (overviewWidget) {
        overviewWidget->GetRenderWindow()->Render();
    }
    if (endoscopeWidget) {
        endoscopeWidget->GetRenderWindow()->Render();
    }
    
    // 如果动画结束，停止定时器
    if (!stillAnimating) {
        animationTimer->stop();
        isAnimating = false;
    }
}