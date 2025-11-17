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
#include <QSlider>
#include <QVTKOpenGLWidget.h>
#include <QKeyEvent>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QSignalBlocker>

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
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , overviewWidget(nullptr)
    , endoscopeWidget(nullptr)
    , animationTimer(nullptr)
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
    
    // 设置动画更新定时器（60FPS）
    animationTimer = new QTimer(this);
    animationTimer->setInterval(16);  // 约60FPS
    connect(animationTimer, &QTimer::timeout, this, &MainWindow::updateAnimation);

    updateActionStates();
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

    generateDataAct = new QAction("生成训练数据(&G)", this);
    generateDataAct->setStatusTip("沿当前路径批量生成256x256 PNG与位姿JSON");
    generateDataAct->setEnabled(false);
    connect(generateDataAct, &QAction::triggered, this, &MainWindow::generateDataset);
    
    exitAct = new QAction("退出(&Q)", this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip("退出应用程序");
    connect(exitAct, &QAction::triggered, this, &QWidget::close);
    
    // 导航菜单动作（仅保留重置）
    
    resetAct = new QAction("重置(&R)", this);
    resetAct->setShortcut(QKeySequence("Home"));
    resetAct->setStatusTip("回到路径起点");
    resetAct->setEnabled(false);
    connect(resetAct, &QAction::triggered, this, &MainWindow::resetNavigation);
    
    
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
    fileMenu->addAction(generateDataAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
    
    // 导航菜单（仅保留重置）
    navigationMenu = menuBar()->addMenu("导航(&N)");
    navigationMenu->addAction(resetAct);
    
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
    fileToolBar->addAction(generateDataAct);
    
    // 导航工具栏（改为滑条控制全局T）
    navigationToolBar = addToolBar("导航");
    
    // 添加滑条
    splineSlider = new QSlider(Qt::Horizontal, this);
    splineSlider->setRange(0, 1000); // 0.000 .. 1.000
    splineSlider->setTickInterval(50);
    splineSlider->setEnabled(false);
    navigationToolBar->addWidget(new QLabel("T:"));
    navigationToolBar->addWidget(splineSlider);
    connect(splineSlider, &QSlider::valueChanged, this, &MainWindow::onSplineSliderChanged);
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
    
    bool loaded = polyData && bronchoscopyAPI->LoadAirwayModel(polyData);
    if (loaded) {
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

    updateActionStates();
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
    
    bool loaded = !positions.empty() && bronchoscopyAPI->LoadCameraPath(positions);
    if (loaded) {
        int total = bronchoscopyAPI->GetTotalPathNodes();
        statusBar()->showMessage(QString("成功加载路径: %1 (%2个节点)").arg(fileName).arg(total), 3000);
        statusLabel->setText("T = 0.000");
        
        if (splineSlider) {
            QSignalBlocker blocker(splineSlider);
            splineSlider->setValue(0);
        }
        
        // 强制刷新endoscope视图（相机位置已更新）
        endoscopeWidget->GetRenderWindow()->Render();
        qDebug() << "Forced render after loading path";
    } else {
        QMessageBox::warning(this, "加载失败", "无法加载路径文件，请检查文件格式\n需要至少2个点");
        statusBar()->showMessage("路径加载失败", 3000);
    }

    updateActionStates();
}

// 已移除 navigateNext/navigatePrevious（改用样条T控制）

void MainWindow::resetNavigation()
{
    bronchoscopyAPI->MoveToFirst();
    int total = bronchoscopyAPI->GetTotalPathNodes();
    statusLabel->setText(QString("路径: 1/%1").arg(total));
    
    // 清理：不再支持自动播放
}

// 已移除 toggleAutoPlay（不再支持自动播放）

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Home:
            if (resetAct->isEnabled()) resetNavigation();
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

void MainWindow::onSplineSliderChanged(int value)
{
    // 停止任何动画/自动播放
    if (animationTimer) animationTimer->stop();
    isAnimating = false;
    // 不再支持自动播放，清理相关状态

    currentT = std::max(0, value) / 1000.0;
    bronchoscopyAPI->SetSplineT(currentT);

    // 更新状态栏
    statusLabel->setText(QString("T = %1").arg(currentT, 0, 'f', 3));

    // 刷新渲染窗口
    if (overviewWidget) overviewWidget->GetRenderWindow()->Render();
    if (endoscopeWidget) endoscopeWidget->GetRenderWindow()->Render();
}

QString MainWindow::ensureResultDirectory() const
{
    auto createResult = [](QDir dir) -> QString {
        if (!dir.exists("result")) {
            if (!dir.mkpath("result")) {
                return QString();
            }
        }
        return dir.filePath("result");
    };

    QDir probe(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        if (QFile::exists(probe.filePath("config.cmake")) ||
            QFile::exists(probe.filePath("README.md"))) {
            QString candidate = createResult(probe);
            if (!candidate.isEmpty()) {
                return candidate;
            }
        }
        if (!probe.cdUp()) {
            break;
        }
    }

    QDir fallback(QDir::currentPath());
    return createResult(fallback);
}

void MainWindow::updateActionStates()
{
    const bool hasPath = bronchoscopyAPI->HasPath();
    const bool hasModel = bronchoscopyAPI->HasModel();

    if (resetAct) {
        resetAct->setEnabled(hasPath);
    }
    if (splineSlider) {
        if (!hasPath) {
            QSignalBlocker blocker(splineSlider);
            splineSlider->setValue(0);
        }
        splineSlider->setEnabled(hasPath);
    }
    if (generateDataAct) {
        generateDataAct->setEnabled(hasPath && hasModel);
    }
}

void MainWindow::generateDataset()
{
    if (!bronchoscopyAPI->HasModel() || !bronchoscopyAPI->HasPath()) {
        QMessageBox::warning(this, "生成失败", "请先加载气管模型和相机路径");
        return;
    }

    bool ok = false;
    QString inputId = QInputDialog::getText(
        this,
        "路径编号",
        "请输入路径编号（两位，例如 01）：",
        QLineEdit::Normal,
        datasetPathId,
        &ok);
    if (!ok) {
        return;
    }

    inputId = inputId.trimmed();
    if (!inputId.isEmpty()) {
        datasetPathId = inputId;
    }
    if (datasetPathId.isEmpty()) {
        datasetPathId = "01";
    }
    datasetPathId = datasetPathId.rightJustified(2, QLatin1Char('0'));

    QString resultDirPath = ensureResultDirectory();
    if (resultDirPath.isEmpty()) {
        QMessageBox::warning(this, "生成失败", "无法创建 result 目录，请检查写入权限");
        return;
    }
    QDir resultDir(resultDirPath);

    struct PathVisibilityRestorer {
        BronchoscopyLib::BronchoscopyAPI* api;
        bool previous;
        ~PathVisibilityRestorer() {
            if (api) {
                api->SetEndoscopePathVisible(previous);
            }
        }
    };
    PathVisibilityRestorer visibilityGuard{bronchoscopyAPI.get(),
                                          bronchoscopyAPI->IsEndoscopePathVisible()};
    bronchoscopyAPI->SetEndoscopePathVisible(false);

    constexpr double targetFov = 60.0;
    bronchoscopyAPI->SetEndoscopeFOV(targetFov);

    double totalLength = bronchoscopyAPI->GetPathTotalLength();
    if (totalLength <= 0.0) {
        QMessageBox::warning(this, "生成失败", "路径长度无效，无法生成数据");
        return;
    }

    auto randomRange = [](double minVal, double maxVal) {
        double t = QRandomGenerator::global()->generateDouble();
        return minVal + (maxVal - minVal) * t;
    };

    QJsonArray frames;
    int frameIndex = 1;
    double lastSampleDistance = -1.0;

    auto captureFrame = [&](double distance) -> bool {
        if (!bronchoscopyAPI->SetCameraByDistance(distance)) {
            QMessageBox::warning(this, "生成失败", "无法根据路径距离定位相机");
            return false;
        }

        double rollOffset = randomRange(-datasetRollRangeDeg, datasetRollRangeDeg);
        bronchoscopyAPI->ApplyRollOffset(rollOffset);

        BronchoscopyLib::MaterialParameters matParams;
        matParams.brightness = randomRange(0.8, 1.3);
        matParams.reflectivity = randomRange(0.05, 0.5);
        matParams.ambient = randomRange(0.1, 0.4);
        matParams.attenuation = randomRange(0.05, 0.35);
        bronchoscopyAPI->ApplyMaterialParameters(matParams);

        QString imageName = QString("path_%1_%2.png")
                                .arg(datasetPathId)
                                .arg(frameIndex, 2, 10, QLatin1Char('0'));
        QString imagePath = resultDir.filePath(imageName);

        if (!bronchoscopyAPI->CaptureEndoscopeImage(imagePath.toStdString(), 256, 256)) {
            QMessageBox::warning(this, "生成失败", "保存PNG失败，请检查result目录写入权限");
            return false;
        }

        BronchoscopyLib::CameraPose pose;
        if (!bronchoscopyAPI->GetCurrentEndoscopePose(pose)) {
            QMessageBox::warning(this, "生成失败", "无法获取当前相机位姿");
            return false;
        }

        QJsonObject entry;
        entry["image"] = imageName;

        QJsonObject positionObject;
        positionObject["x"] = pose.position[0];
        positionObject["y"] = pose.position[1];
        positionObject["z"] = pose.position[2];
        entry["position"] = positionObject;

        QJsonObject eulerObject;
        eulerObject["roll"] = pose.euler[0];
        eulerObject["pitch"] = pose.euler[1];
        eulerObject["yaw"] = pose.euler[2];
        entry["euler_deg"] = eulerObject;

        QJsonArray rotationRows;
        for (int r = 0; r < 3; ++r) {
            QJsonArray row;
            for (int c = 0; c < 3; ++c) {
                row.append(pose.rotationMatrix[r * 3 + c]);
            }
            rotationRows.append(row);
        }
        entry["rotation_matrix"] = rotationRows;
        entry["distance_mm"] = distance;
        entry["roll_offset_deg"] = rollOffset;

        frames.append(entry);
        ++frameIndex;
        lastSampleDistance = distance;
        return true;
    };

    for (double distance = 0.0; distance <= totalLength + 1e-6; distance += datasetStepMm) {
        if (!captureFrame(distance)) {
            return;
        }
    }
    if (totalLength - lastSampleDistance > 1e-3) {
        if (!captureFrame(totalLength)) {
            return;
        }
    }

    if (frames.isEmpty()) {
        QMessageBox::warning(this, "生成失败", "未能生成任何图像帧");
        return;
    }

    QJsonObject root;
    root["path_id"] = datasetPathId;
    root["fov_deg"] = targetFov;
    root["step_mm"] = datasetStepMm;
    root["frame_count"] = frames.size();
    root["frames"] = frames;

    QString jsonName = QString("path_%1.json").arg(datasetPathId);
    QFile jsonFile(resultDir.filePath(jsonName));
    if (!jsonFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, "生成失败", "无法写入JSON文件，请检查result目录权限");
        return;
    }
    jsonFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    jsonFile.close();

    statusBar()->showMessage(
        QString("已生成 %1 张图像和JSON: %2")
            .arg(frames.size())
            .arg(jsonName),
        5000);
    QMessageBox::information(this, "生成完成",
                             QString("生成完成，共导出 %1 张PNG。\n保存于：%2")
                                 .arg(frames.size())
                                 .arg(resultDir.filePath("")));
}
