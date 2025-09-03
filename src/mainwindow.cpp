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
    niftiReader = vtkSmartPointer<vtkNIFTIImageReader>::New();
    
    // 创建图像查看器
    axialViewer = vtkSmartPointer<vtkImageViewer2>::New();
    coronalViewer = vtkSmartPointer<vtkImageViewer2>::New();
    sagittalViewer = vtkSmartPointer<vtkImageViewer2>::New();
    
    // 创建3D表面渲染组件
    surfaceRenderer = vtkSmartPointer<vtkRenderer>::New();
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
    
    // 加载NIFTI动作
    loadNIFTIAct = new QAction("打开NIFTI文件(&N)", this);
    loadNIFTIAct->setShortcut(QKeySequence("Ctrl+N"));
    loadNIFTIAct->setStatusTip("选择NIFTI文件(.nii或.nii.gz)");
    connect(loadNIFTIAct, &QAction::triggered, this, &MainWindow::loadNIFTIFile);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu("文件(&F)");
    fileMenu->addAction(loadDICOMAct);
    fileMenu->addAction(loadNIFTIAct);
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
    
    // 创建按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    // 创建加载DICOM按钮
    loadButton = new QPushButton("选择DICOM文件夹", this);
    loadButton->setMaximumHeight(40);
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::loadDICOMFiles);
    buttonLayout->addWidget(loadButton);
    
    // 创建加载NIFTI按钮  
    loadNiftiButton = new QPushButton("选择NIFTI文件", this);
    loadNiftiButton->setMaximumHeight(40);
    connect(loadNiftiButton, &QPushButton::clicked, this, &MainWindow::loadNIFTIFile);
    buttonLayout->addWidget(loadNiftiButton);
    
    centralLayout->addLayout(buttonLayout);
    
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
    
    // 3D表面视图 (右下)
    QWidget *surfaceViewWidget = new QWidget();
    surfaceViewWidget->setMinimumSize(300, 300);
    QVBoxLayout *surfaceLayout = new QVBoxLayout(surfaceViewWidget);
    
    QLabel *surfaceLabel = new QLabel("3D表面重建 (Surface Reconstruction)");
    surfaceLabel->setAlignment(Qt::AlignCenter);
    surfaceLabel->setStyleSheet("QLabel { background-color: #8e44ad; color: white; padding: 5px; }");
    
    surfaceWidget = new QVTKOpenGLWidget();
    surfaceWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // 添加等值面阈值控制
    isoValueLabel = new QLabel("等值面阈值: 0");
    isoValueLabel->setAlignment(Qt::AlignCenter);
    
    isoValueSlider = new QSlider(Qt::Horizontal);
    isoValueSlider->setMinimum(0);
    isoValueSlider->setMaximum(100);
    isoValueSlider->setValue(30);
    connect(isoValueSlider, &QSlider::valueChanged, this, &MainWindow::onIsoValueSliderChanged);
    
    // 添加平滑度控制
    smoothnessLabel = new QLabel("平滑度: 50");
    smoothnessLabel->setAlignment(Qt::AlignCenter);
    
    smoothnessSlider = new QSlider(Qt::Horizontal);
    smoothnessSlider->setMinimum(0);
    smoothnessSlider->setMaximum(100);
    smoothnessSlider->setValue(50);
    connect(smoothnessSlider, &QSlider::valueChanged, this, &MainWindow::onIsoValueSliderChanged);
    
    surfaceLayout->addWidget(surfaceLabel);
    surfaceLayout->addWidget(surfaceWidget);
    surfaceLayout->addWidget(isoValueLabel);
    surfaceLayout->addWidget(isoValueSlider);
    surfaceLayout->addWidget(smoothnessLabel);
    surfaceLayout->addWidget(smoothnessSlider);
    
    // 将四个视图添加到网格布局
    mainLayout->addWidget(axialViewWidget, 0, 0);
    mainLayout->addWidget(coronalViewWidget, 0, 1);
    mainLayout->addWidget(sagittalViewWidget, 1, 0);
    mainLayout->addWidget(surfaceViewWidget, 1, 1);
}

void MainWindow::loadDICOMFiles()
{
    QString dirName = QFileDialog::getExistingDirectory(this,
        "选择包含DICOM文件的文件夹", "", 
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (dirName.isEmpty()) {
        return;
    }
    
    // 询问用户数据类型
    QStringList items;
    items << "医学影像（CT/MRI）" << "分割标签（Mask/Tag）";
    bool ok;
    QString item = QInputDialog::getItem(this, "选择数据类型",
                                        "请选择DICOM文件的数据类型：", 
                                        items, 0, false, &ok);
    if (!ok || item.isEmpty()) {
        return;
    }
    
    bool isSegmentation = (items.indexOf(item) == 1);
    
    try {
        statusBar()->showMessage("正在加载DICOM序列...");
        
        // 读取DICOM文件夹
        dicomReader->SetDirectoryName(dirName.toStdString().c_str());
        dicomReader->Update();
        
        vtkSmartPointer<vtkImageData> data = dicomReader->GetOutput();
        if (!data) {
            QMessageBox::warning(this, "错误", "无法读取DICOM文件");
            return;
        }
        
        if (isSegmentation) {
            // 处理分割标签数据
            statusBar()->showMessage("正在处理分割标签数据...");
            
            // 获取标签值范围
            double range[2];
            data->GetScalarRange(range);
            qDebug() << "标签值范围:" << range[0] << "-" << range[1];
            
            // 暂时作为普通图像加载，后续可以实现叠加显示
            loadImageData(data, "DICOM分割标签加载成功");
            
        } else {
            // 处理医学影像数据
            loadImageData(data, "DICOM医学影像加载成功");
        }
        
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
    
    // 根据图像范围自动设置窗宽窗位
    double range[2];
    imageData->GetScalarRange(range);
    
    double window = range[1] - range[0];
    double level = range[0] + window / 2.0;
    
    // 对于CT图像（HU值），使用预设窗位
    if (range[1] > 1000 && range[0] < -1000) {
        // 软组织窗位
        window = 400;
        level = 40;
    }
    
    viewer->SetColorWindow(window);
    viewer->SetColorLevel(level);
    viewer->GetRenderer()->SetBackground(0.0, 0.0, 0.0);
    
    qDebug() << "视图" << orientation << "窗宽:" << window << "窗位:" << level;
    
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
        // 清除之前的演员
        surfaceRenderer->RemoveAllViewProps();
        
        // 设置渲染器背景
        surfaceRenderer->SetBackground(0.1, 0.1, 0.2);
        surfaceWidget->GetRenderWindow()->AddRenderer(surfaceRenderer);
        
        // 获取图像数据的标量范围
        imageData->GetScalarRange(imageRange);
        
        // 根据滑动条的百分比计算实际阈值
        double sliderPercent = isoValueSlider->value() / 100.0;
        double isoValue = imageRange[0] + (imageRange[1] - imageRange[0]) * sliderPercent;
        
        // 更新标签显示
        isoValueLabel->setText(QString("等值面阈值: %1 (%2%)").arg(isoValue, 0, 'f', 1).arg(isoValueSlider->value()));
        
        // 先对图像进行高斯平滑以减少噪声
        vtkSmartPointer<vtkImageGaussianSmooth> gaussianSmooth = 
            vtkSmartPointer<vtkImageGaussianSmooth>::New();
        gaussianSmooth->SetInputData(imageData);
        gaussianSmooth->SetDimensionality(3);
        gaussianSmooth->SetRadiusFactor(1.0);  // 平滑半径
        gaussianSmooth->SetStandardDeviation(0.5);  // 标准差
        gaussianSmooth->Update();
        
        // 配置Marching Cubes算法进行面重建
        marchingCubes->SetInputConnection(gaussianSmooth->GetOutputPort());
        marchingCubes->SetValue(0, isoValue);
        marchingCubes->ComputeNormalsOff();  // 暂时关闭，后面用更好的方法计算
        marchingCubes->ComputeGradientsOff();
        marchingCubes->Update();
        
        // 检查是否生成了面片
        if (marchingCubes->GetOutput()->GetNumberOfPoints() == 0) {
            qDebug() << "无法生成面片，尝试调整阈值";
            isoValue = imageRange[0] + (imageRange[1] - imageRange[0]) * 0.1;
            marchingCubes->SetValue(0, isoValue);
            marchingCubes->Update();
        }
        
        // 更新平滑度标签
        smoothnessLabel->setText(QString("平滑度: %1").arg(smoothnessSlider->value()));
        
        // 对生成的表面进行平滑处理
        vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter = 
            vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
        smoothFilter->SetInputConnection(marchingCubes->GetOutputPort());
        smoothFilter->SetNumberOfIterations(smoothnessSlider->value());  // 使用滑条值作为迭代次数
        smoothFilter->SetRelaxationFactor(0.1);   // 松弛因子
        smoothFilter->FeatureEdgeSmoothingOff();  // 保持边缘特征
        smoothFilter->BoundarySmoothingOn();      // 平滑边界
        smoothFilter->Update();
        
        // 减少面片数量以提高性能（可选）
        vtkSmartPointer<vtkDecimatePro> decimate = 
            vtkSmartPointer<vtkDecimatePro>::New();
        decimate->SetInputConnection(smoothFilter->GetOutputPort());
        decimate->SetTargetReduction(0.5);  // 减少50%的面片
        decimate->PreserveTopologyOn();     // 保持拓扑结构
        decimate->Update();
        
        // 重新计算法向量以获得更好的光照效果
        vtkSmartPointer<vtkPolyDataNormals> normals = 
            vtkSmartPointer<vtkPolyDataNormals>::New();
        normals->SetInputConnection(decimate->GetOutputPort());
        normals->SetFeatureAngle(60.0);     // 特征角度
        normals->ConsistencyOn();           // 保持法向量一致性
        normals->SplittingOff();            // 不分割边缘
        normals->Update();
        
        // 配置表面映射器
        surfaceMapper->SetInputConnection(normals->GetOutputPort());
        surfaceMapper->ScalarVisibilityOff();  // 不使用标量值着色
        
        // 设置表面演员属性（改善渲染质量）
        surfaceActor->SetMapper(surfaceMapper);
        surfaceActor->GetProperty()->SetColor(0.95, 0.90, 0.85);  // 更自然的皮肤色
        surfaceActor->GetProperty()->SetSpecular(0.2);      // 降低镜面反射
        surfaceActor->GetProperty()->SetSpecularPower(20);  // 增加镜面反射锐度
        surfaceActor->GetProperty()->SetAmbient(0.2);       // 环境光
        surfaceActor->GetProperty()->SetDiffuse(0.8);       // 漫反射
        surfaceActor->GetProperty()->SetOpacity(1.0);
        
        // 启用Phong着色以获得更平滑的表面
        surfaceActor->GetProperty()->SetInterpolationToPhong();
        
        // 启用抗锯齿
        surfaceActor->GetProperty()->SetLineWidth(1.0);
        surfaceActor->GetProperty()->SetPointSize(1.0);
        
        // 添加到渲染器
        surfaceRenderer->AddActor(surfaceActor);
        surfaceRenderer->ResetCamera();
        
        // 设置相机位置以获得更好的初始视角
        vtkCamera* camera = surfaceRenderer->GetActiveCamera();
        camera->SetViewUp(0, 0, 1);
        camera->SetPosition(1, 0, 0);
        camera->SetFocalPoint(0, 0, 0);
        surfaceRenderer->ResetCamera();
        
        // 设置3D视图的交互器
        setupSurfaceInteractor();
        
        // 启用多重采样抗锯齿（MSAA）
        surfaceWidget->GetRenderWindow()->SetMultiSamples(4);
        
        // 渲染
        surfaceWidget->GetRenderWindow()->Render();
        
        qDebug() << "3D面重建完成，等值面阈值:" << isoValue;
        qDebug() << "原始三角面片数:" << marchingCubes->GetOutput()->GetNumberOfPolys();
        qDebug() << "优化后三角面片数:" << decimate->GetOutput()->GetNumberOfPolys();
        
    } catch (const std::exception& e) {
        qDebug() << "3D渲染错误:" << e.what();
    }
}

void MainWindow::setupSurfaceInteractor()
{
    // 为3D视图设置交互器
    vtkSmartPointer<vtkRenderWindowInteractor> interactor = 
        surfaceWidget->GetRenderWindow()->GetInteractor();
    
    // 使用默认的3D交互器样式（支持旋转、缩放、平移）
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = 
        vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);
    
    // 启用交互器
    interactor->Initialize();
}

void MainWindow::loadNIFTIFile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "选择NIFTI文件", "", 
        "NIFTI Files (*.nii *.nii.gz);;All Files (*)");
    
    if (fileName.isEmpty()) {
        return;
    }
    
    // 询问用户数据类型
    QStringList items;
    items << "医学影像（CT/MRI）" << "分割标签（Mask/Tag）";
    bool ok;
    QString item = QInputDialog::getItem(this, "选择数据类型",
                                        "请选择NIFTI文件的数据类型：", 
                                        items, 0, false, &ok);
    if (!ok || item.isEmpty()) {
        return;
    }
    
    bool isSegmentation = (items.indexOf(item) == 1);
    
    try {
        statusBar()->showMessage("正在加载NIFTI文件...");
        
        // 读取NIFTI文件
        niftiReader->SetFileName(fileName.toStdString().c_str());
        niftiReader->Update();
        
        vtkSmartPointer<vtkImageData> data = niftiReader->GetOutput();
        if (!data) {
            QMessageBox::warning(this, "错误", "无法读取NIFTI文件");
            return;
        }
        
        if (isSegmentation) {
            // 处理分割标签数据
            statusBar()->showMessage("正在处理分割标签数据...");
            
            // 获取标签值范围
            double range[2];
            data->GetScalarRange(range);
            qDebug() << "标签值范围:" << range[0] << "-" << range[1];
            
            // 为标签创建颜色映射
            vtkSmartPointer<vtkLookupTable> labelLUT = vtkSmartPointer<vtkLookupTable>::New();
            int numLabels = static_cast<int>(range[1]) + 1;
            labelLUT->SetNumberOfColors(numLabels);
            labelLUT->Build();
            
            // 设置标签颜色（0=背景透明，其他标签用不同颜色）
            labelLUT->SetTableValue(0, 0.0, 0.0, 0.0, 0.0);  // 背景透明
            if (numLabels > 1) labelLUT->SetTableValue(1, 1.0, 0.0, 0.0, 0.5);  // 标签1 红色
            if (numLabels > 2) labelLUT->SetTableValue(2, 0.0, 1.0, 0.0, 0.5);  // 标签2 绿色  
            if (numLabels > 3) labelLUT->SetTableValue(3, 0.0, 0.0, 1.0, 0.5);  // 标签3 蓝色
            if (numLabels > 4) labelLUT->SetTableValue(4, 1.0, 1.0, 0.0, 0.5);  // 标签4 黄色
            
            // 暂时作为普通图像加载，后续可以实现叠加显示
            loadImageData(data, "分割标签加载成功");
            
        } else {
            // 处理医学影像数据
            loadImageData(data, "NIFTI医学影像加载成功");
        }
        
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "错误", QString("加载NIFTI文件失败: %1").arg(e.what()));
        statusBar()->showMessage("加载失败", 3000);
    }
}

void MainWindow::loadImageData(vtkSmartPointer<vtkImageData> data, const QString& sourceInfo)
{
    imageData = data;
    
    // 获取图像信息
    imageData->GetDimensions(dimensions);
    imageData->GetSpacing(spacing);
    imageData->GetOrigin(origin);
    imageData->GetScalarRange(imageRange);
    
    qDebug() << "图像尺寸:" << dimensions[0] << "x" << dimensions[1] << "x" << dimensions[2];
    qDebug() << "体素间距:" << spacing[0] << spacing[1] << spacing[2];
    qDebug() << "标量范围:" << imageRange[0] << "-" << imageRange[1];
    
    // 设置滑条范围
    axialSlider->setMaximum(dimensions[2] - 1);
    axialSlider->setValue(dimensions[2] / 2);
    
    coronalSlider->setMaximum(dimensions[1] - 1);
    coronalSlider->setValue(dimensions[1] / 2);
    
    sagittalSlider->setMaximum(dimensions[0] - 1);
    sagittalSlider->setValue(dimensions[0] / 2);
    
    // 根据图像类型设置合适的初始阈值
    if (imageRange[1] > 1000) {
        // DICOM图像，设置为软组织阈值
        isoValueSlider->setValue(10);  // 约10%位置
    } else {
        // NIFTI或其他归一化图像
        isoValueSlider->setValue(30);  // 30%位置
    }
    
    // 设置图像查看器
    setupImageViewer(axialViewer, axialWidget, 2);  // Z轴方向
    setupImageViewer(coronalViewer, coronalWidget, 1);  // Y轴方向
    setupImageViewer(sagittalViewer, sagittalWidget, 0);  // X轴方向
    
    // 初始化显示
    updateAxialView(axialSlider->value());
    updateCoronalView(coronalSlider->value());
    updateSagittalView(sagittalSlider->value());
    update3DView();
    
    statusBar()->showMessage(sourceInfo, 3000);
}

void MainWindow::onIsoValueSliderChanged(int value)
{
    if (!imageData) return;
    
    // 更新3D视图
    update3DView();
}