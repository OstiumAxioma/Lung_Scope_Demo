#include "mainwindow.h"
#include <QApplication>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTimer>
#include <QDebug>
#include <vtkRendererCollection.h>

// 实现自定义交互器样式
vtkStandardNewMacro(CenterZoomInteractorStyle);

void CenterZoomInteractorStyle::OnMouseWheelForward()
{
    ZoomAtCenter(1.1);  // 放大10%
}

void CenterZoomInteractorStyle::OnMouseWheelBackward()
{
    ZoomAtCenter(0.9);  // 缩小10%
}

void CenterZoomInteractorStyle::OnRightButtonDown()
{
    rightButtonPressed = true;
    // 记录鼠标按下时的位置
    this->Interactor->GetEventPosition(lastMousePos);
}

void CenterZoomInteractorStyle::OnRightButtonUp()
{
    rightButtonPressed = false;
}

void CenterZoomInteractorStyle::OnMouseMove()
{
    if (rightButtonPressed)
    {
        // 获取当前鼠标位置
        int currentPos[2];
        this->Interactor->GetEventPosition(currentPos);
        
        // 计算鼠标移动的距离
        int deltaX = currentPos[0] - lastMousePos[0];
        int deltaY = currentPos[1] - lastMousePos[1];
        
        // 获取渲染器和相机
        vtkRenderWindowInteractor* rwi = this->Interactor;
        vtkRenderer* renderer = rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
        vtkCamera* camera = renderer->GetActiveCamera();
        
        if (camera)
        {
            // 获取当前相机的并行投影比例
            double parallelScale = camera->GetParallelScale();
            
            // 计算平移量（根据视窗大小和缩放比例调整）
            int* size = renderer->GetSize();
            double panX = -deltaX * parallelScale * 2.0 / size[0];
            double panY = -deltaY * parallelScale * 2.0 / size[1];
            
            // 获取当前相机焦点
            double focalPoint[3];
            camera->GetFocalPoint(focalPoint);
            
            // 应用平移
            camera->SetFocalPoint(focalPoint[0] + panX, focalPoint[1] + panY, focalPoint[2]);
            
            // 获取相机位置并同步平移
            double position[3];
            camera->GetPosition(position);
            camera->SetPosition(position[0] + panX, position[1] + panY, position[2]);
            
            // 刷新渲染
            rwi->GetRenderWindow()->Render();
        }
        
        // 更新最后鼠标位置
        lastMousePos[0] = currentPos[0];
        lastMousePos[1] = currentPos[1];
    }
    else
    {
        // 如果不是右键拖拽，调用父类的鼠标移动处理
        this->Superclass::OnMouseMove();
    }
}

void CenterZoomInteractorStyle::ZoomAtCenter(double factor)
{
    vtkRenderWindowInteractor* rwi = this->Interactor;
    vtkRenderer* renderer = rwi->GetRenderWindow()->GetRenderers()->GetFirstRenderer();
    vtkCamera* camera = renderer->GetActiveCamera();
    
    if (camera)
    {
        // 获取当前相机的并行投影比例
        double parallelScale = camera->GetParallelScale();
        // 设置新的比例，实现以视图中心为缩放中心的缩放
        camera->SetParallelScale(parallelScale / factor);
        
        // 刷新渲染
        rwi->GetRenderWindow()->Render();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("DICOM Reader - 医学图像查看器");
    resize(1200, 800);

    // 初始化VTK智能指针
    dicomReader = vtkSmartPointer<vtkDICOMImageReader>::New();
    
    // 创建图像查看器
    axialViewer = vtkSmartPointer<vtkImageViewer2>::New();
    coronalViewer = vtkSmartPointer<vtkImageViewer2>::New();
    sagittalViewer = vtkSmartPointer<vtkImageViewer2>::New();
    
    // 创建3D表面渲染组件
    volumeRenderer = vtkSmartPointer<vtkRenderer>::New();
    marchingCubes = vtkSmartPointer<vtkMarchingCubes>::New();
    surfaceMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    surfaceActor = vtkSmartPointer<vtkActor>::New();
    
    // 创建图像切片器
    axialReslice = vtkSmartPointer<vtkImageReslice>::New();
    coronalReslice = vtkSmartPointer<vtkImageReslice>::New();
    sagittalReslice = vtkSmartPointer<vtkImageReslice>::New();

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    setupUI();
}

MainWindow::~MainWindow()
{
}

void MainWindow::createActions()
{
    // 退出动作
    exitAct = new QAction("退出(&Q)", this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip("退出应用程序");
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // 关于动作
    aboutAct = new QAction("关于(&A)", this);
    aboutAct->setStatusTip("显示应用程序的关于对话框");
    connect(aboutAct, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于 DICOM Reader",
                          "这是一个基于VTK和Qt的DICOM医学图像查看器。\\n"
                          "支持轴位、冠状、矢状三视图显示和3D表面重建。\\n\\n"
                          "交互操作：\\n"
                          "• 鼠标滚轮：以视图中心为基点缩放\\n"
                          "• 右键拖拽：平移视图\\n"
                          "• 3D视图：左键旋转，滚轮缩放，中键平移");
    });

    // 加载DICOM动作
    loadDICOMAct = new QAction("打开DICOM序列(&O)", this);
    loadDICOMAct->setShortcut(QKeySequence::Open);
    loadDICOMAct->setStatusTip("选择文件夹加载DICOM序列");
    connect(loadDICOMAct, &QAction::triggered, this, &MainWindow::loadDICOMFiles);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction(loadDICOMAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    helpMenu = menuBar()->addMenu("帮助(&H)");
    helpMenu->addAction(aboutAct);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar("文件");
    fileToolBar->addAction(loadDICOMAct);
    fileToolBar->addAction(exitAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage("就绪");
}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    
    // 创建加载按钮
    loadButton = new QPushButton("选择DICOM文件夹", this);
    loadButton->setMaximumHeight(40);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadDICOMFiles);
    centralLayout->addWidget(loadButton);
    
    // 创建四视图容器
    QWidget *viewsWidget = new QWidget();
    mainLayout = new QGridLayout(viewsWidget);
    centralLayout->addWidget(viewsWidget);
    
    setupFourViews();
    
    statusBar()->showMessage("界面初始化完成", 2000);
}

void MainWindow::setupFourViews()
{
    mainLayout->setSpacing(5);
    
    // 轴位视图 (左上)
    QWidget *axialViewWidget = new QWidget();
    axialViewWidget->setMinimumSize(300, 300);
    QVBoxLayout *axialLayout = new QVBoxLayout(axialViewWidget);
    
    axialLabel = new QLabel("轴位视图 (Axial) - 切片: 0/0");
    axialLabel->setAlignment(Qt::AlignCenter);
    axialLabel->setStyleSheet("QLabel { background-color: #2c3e50; color: white; padding: 5px; }");
    
    axialWidget = new QVTKOpenGLWidget();
    axialWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    axialSlider = new QSlider(Qt::Horizontal);
    axialSlider->setMinimum(0);
    axialSlider->setMaximum(100);
    axialSlider->setValue(0);
    connect(axialSlider, &QSlider::valueChanged, this, &MainWindow::onAxialSliderChanged);
    
    axialLayout->addWidget(axialLabel);
    axialLayout->addWidget(axialWidget);
    axialLayout->addWidget(axialSlider);
    
    // 冠状视图 (右上)
    QWidget *coronalViewWidget = new QWidget();
    coronalViewWidget->setMinimumSize(300, 300);
    QVBoxLayout *coronalLayout = new QVBoxLayout(coronalViewWidget);
    
    coronalLabel = new QLabel("冠状视图 (Coronal) - 切片: 0/0");
    coronalLabel->setAlignment(Qt::AlignCenter);
    coronalLabel->setStyleSheet("QLabel { background-color: #27ae60; color: white; padding: 5px; }");
    
    coronalWidget = new QVTKOpenGLWidget();
    coronalWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    coronalSlider = new QSlider(Qt::Horizontal);
    coronalSlider->setMinimum(0);
    coronalSlider->setMaximum(100);
    coronalSlider->setValue(0);
    connect(coronalSlider, &QSlider::valueChanged, this, &MainWindow::onCoronalSliderChanged);
    
    coronalLayout->addWidget(coronalLabel);
    coronalLayout->addWidget(coronalWidget);
    coronalLayout->addWidget(coronalSlider);
    
    // 矢状视图 (左下)
    QWidget *sagittalViewWidget = new QWidget();
    sagittalViewWidget->setMinimumSize(300, 300);
    QVBoxLayout *sagittalLayout = new QVBoxLayout(sagittalViewWidget);
    
    sagittalLabel = new QLabel("矢状视图 (Sagittal) - 切片: 0/0");
    sagittalLabel->setAlignment(Qt::AlignCenter);
    sagittalLabel->setStyleSheet("QLabel { background-color: #e74c3c; color: white; padding: 5px; }");
    
    sagittalWidget = new QVTKOpenGLWidget();
    sagittalWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    sagittalSlider = new QSlider(Qt::Horizontal);
    sagittalSlider->setMinimum(0);
    sagittalSlider->setMaximum(100);
    sagittalSlider->setValue(0);
    connect(sagittalSlider, &QSlider::valueChanged, this, &MainWindow::onSagittalSliderChanged);
    
    sagittalLayout->addWidget(sagittalLabel);
    sagittalLayout->addWidget(sagittalWidget);
    sagittalLayout->addWidget(sagittalSlider);
    
    // 3D体积视图 (右下)
    QWidget *volumeViewWidget = new QWidget();
    volumeViewWidget->setMinimumSize(300, 300);
    QVBoxLayout *volumeLayout = new QVBoxLayout(volumeViewWidget);
    
    QLabel *volumeLabel = new QLabel("3D表面重建 (Surface Reconstruction)");
    volumeLabel->setAlignment(Qt::AlignCenter);
    volumeLabel->setStyleSheet("QLabel { background-color: #8e44ad; color: white; padding: 5px; }");
    
    volumeWidget = new QVTKOpenGLWidget();
    volumeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(volumeWidget);
    
    // 将四个视图添加到网格布局
    mainLayout->addWidget(axialViewWidget, 0, 0);
    mainLayout->addWidget(coronalViewWidget, 0, 1);
    mainLayout->addWidget(sagittalViewWidget, 1, 0);
    mainLayout->addWidget(volumeViewWidget, 1, 1);
}

void MainWindow::loadDICOMFiles()
{
    QString dirName = QFileDialog::getExistingDirectory(this,
        "选择包含DICOM文件的文件夹", "", 
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (dirName.isEmpty()) {
        return;
    }
    
    try {
        statusBar()->showMessage("正在加载DICOM序列...");
        
        // 读取DICOM文件夹
        dicomReader->SetDirectoryName(dirName.toStdString().c_str());
        dicomReader->Update();
        
        imageData = dicomReader->GetOutput();
        if (!imageData) {
            QMessageBox::warning(this, "错误", "无法读取DICOM文件");
            return;
        }
        
        // 获取图像信息
        imageData->GetDimensions(dimensions);
        imageData->GetSpacing(spacing);
        imageData->GetOrigin(origin);
        
        qDebug() << "图像尺寸:" << dimensions[0] << "x" << dimensions[1] << "x" << dimensions[2];
        qDebug() << "体素间距:" << spacing[0] << spacing[1] << spacing[2];
        
        // 设置滑条范围
        axialSlider->setMaximum(dimensions[2] - 1);
        axialSlider->setValue(dimensions[2] / 2);
        
        coronalSlider->setMaximum(dimensions[1] - 1);
        coronalSlider->setValue(dimensions[1] / 2);
        
        sagittalSlider->setMaximum(dimensions[0] - 1);
        sagittalSlider->setValue(dimensions[0] / 2);
        
        // 设置图像查看器
        setupImageViewer(axialViewer, axialWidget, 2);  // Z轴方向
        setupImageViewer(coronalViewer, coronalWidget, 1);  // Y轴方向
        setupImageViewer(sagittalViewer, sagittalWidget, 0);  // X轴方向
        
        // 初始化显示
        updateAxialView(axialSlider->value());
        updateCoronalView(coronalSlider->value());
        updateSagittalView(sagittalSlider->value());
        update3DView();
        
        statusBar()->showMessage("DICOM序列加载成功", 3000);
        
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "错误", QString("加载DICOM文件失败: %1").arg(e.what()));
        statusBar()->showMessage("加载失败", 3000);
    }
}

void MainWindow::setupImageViewer(vtkSmartPointer<vtkImageViewer2> viewer, 
                                 QVTKOpenGLWidget* widget, 
                                 int orientation)
{
    viewer->SetInputData(imageData);
    viewer->SetSliceOrientation(orientation);
    viewer->SetRenderWindow(widget->GetRenderWindow());
    viewer->SetRenderer(viewer->GetRenderer());
    viewer->SetColorWindow(255);
    viewer->SetColorLevel(127);
    viewer->GetRenderer()->SetBackground(0.0, 0.0, 0.0);
    
    // 设置交互器
    vtkSmartPointer<vtkRenderWindowInteractor> interactor = widget->GetRenderWindow()->GetInteractor();
    viewer->SetupInteractor(interactor);
    
    // 设置自定义交互器样式以支持中心缩放
    vtkSmartPointer<CenterZoomInteractorStyle> style = 
        vtkSmartPointer<CenterZoomInteractorStyle>::New();
    interactor->SetInteractorStyle(style);
    
    // 启用交互器
    interactor->Initialize();
}

void MainWindow::onAxialSliderChanged(int value)
{
    updateAxialView(value);
}

void MainWindow::onCoronalSliderChanged(int value)
{
    updateCoronalView(value);
}

void MainWindow::onSagittalSliderChanged(int value)
{
    updateSagittalView(value);
}

void MainWindow::updateAxialView(int slice)
{
    if (!imageData) return;
    
    axialViewer->SetSlice(slice);
    axialViewer->Render();
    axialLabel->setText(QString("轴位视图 (Axial) - 切片: %1/%2").arg(slice + 1).arg(dimensions[2]));
}

void MainWindow::updateCoronalView(int slice)
{
    if (!imageData) return;
    
    coronalViewer->SetSlice(slice);
    coronalViewer->Render();
    coronalLabel->setText(QString("冠状视图 (Coronal) - 切片: %1/%2").arg(slice + 1).arg(dimensions[1]));
}

void MainWindow::updateSagittalView(int slice)
{
    if (!imageData) return;
    
    sagittalViewer->SetSlice(slice);
    sagittalViewer->Render();
    sagittalLabel->setText(QString("矢状视图 (Sagittal) - 切片: %1/%2").arg(slice + 1).arg(dimensions[0]));
}

void MainWindow::update3DView()
{
    if (!imageData) return;
    
    try {
        // 设置渲染器背景
        volumeRenderer->SetBackground(0.1, 0.1, 0.2);
        volumeWidget->GetRenderWindow()->AddRenderer(volumeRenderer);
        
        // 获取图像数据的标量范围
        double range[2];
        imageData->GetScalarRange(range);
        
        // 设置等值面阈值（通常取中间值或根据组织密度设定）
        double isoValue = range[0] + (range[1] - range[0]) * 0.2;  // 取20%位置的值作为阈值
        
        // 配置Marching Cubes算法
        marchingCubes->SetInputData(imageData);
        marchingCubes->SetValue(0, isoValue);
        marchingCubes->Update();
        
        // 配置表面映射器
        surfaceMapper->SetInputConnection(marchingCubes->GetOutputPort());
        surfaceMapper->ScalarVisibilityOff();  // 不使用标量值着色
        
        // 设置表面演员属性
        surfaceActor->SetMapper(surfaceMapper);
        surfaceActor->GetProperty()->SetColor(0.8, 0.7, 0.6);  // 米黄色表面
        surfaceActor->GetProperty()->SetSpecular(0.6);
        surfaceActor->GetProperty()->SetSpecularPower(30);
        surfaceActor->GetProperty()->SetAmbient(0.2);
        surfaceActor->GetProperty()->SetDiffuse(0.8);
        
        // 添加到渲染器
        volumeRenderer->AddActor(surfaceActor);
        volumeRenderer->ResetCamera();
        
        // 设置3D视图的交互器
        setupSurfaceInteractor();
        
        // 渲染
        volumeWidget->GetRenderWindow()->Render();
        
        qDebug() << "3D表面重建完成，等值面阈值:" << isoValue;
        
    } catch (const std::exception& e) {
        qDebug() << "3D渲染错误:" << e.what();
    }
}

void MainWindow::setupSurfaceInteractor()
{
    // 为3D视图设置交互器
    vtkSmartPointer<vtkRenderWindowInteractor> interactor = 
        volumeWidget->GetRenderWindow()->GetInteractor();
    
    // 使用默认的3D交互器样式（支持旋转、缩放、平移）
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = 
        vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);
    
    // 启用交互器
    interactor->Initialize();
}