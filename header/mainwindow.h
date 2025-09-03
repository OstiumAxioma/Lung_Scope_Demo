#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// VTK模块初始化
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2)

#include <QMainWindow>
#include <QVTKOpenGLWidget.h>
#include <QSlider>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>

// VTK includes for DICOM and imaging
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkDICOMImageReader.h>
#include <vtkNIFTIImageReader.h>
#include <vtkImageData.h>
#include <vtkImageCast.h>
#include <vtkImageShiftScale.h>
#include <vtkImageViewer2.h>
#include <vtkImageReslice.h>
#include <vtkMatrix4x4.h>
#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <vtkLookupTable.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkDecimatePro.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkInteractorStyleImage.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkRendererCollection.h>
#include <vtkObjectFactory.h>

// 自定义交互器样式，支持以视图中心为缩放中心的滚轮缩放和右键平移
class CenterZoomInteractorStyle : public vtkInteractorStyleImage
{
public:
    static CenterZoomInteractorStyle* New();
    vtkTypeMacro(CenterZoomInteractorStyle, vtkInteractorStyleImage);
    
    CenterZoomInteractorStyle() : rightButtonPressed(false) 
    { 
        lastMousePos[0] = 0; 
        lastMousePos[1] = 0; 
    }
    
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseMove() override;
    
private:
    void ZoomAtCenter(double factor);
    bool rightButtonPressed;
    int lastMousePos[2];
};

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMenuBar;
class QStatusBar;
class QToolBar;
QT_END_NAMESPACE

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
    void setupUI();
    void loadDICOMFiles();
    void loadNIFTIFile();
    void onAxialSliderChanged(int value);
    void onCoronalSliderChanged(int value);
    void onSagittalSliderChanged(int value);
    void onIsoValueSliderChanged(int value);

private:
    void setupFourViews();
    void updateAxialView(int slice);
    void updateCoronalView(int slice);
    void updateSagittalView(int slice);
    void update3DView();
    void setupSurfaceInteractor();
    void setupImageViewer(vtkSmartPointer<vtkImageViewer2> viewer, 
                         QVTKOpenGLWidget* widget, 
                         int orientation);
    void loadImageData(vtkSmartPointer<vtkImageData> data, const QString& sourceInfo);
    
    // UI组件
    QWidget *centralWidget;
    QGridLayout *mainLayout;
    
    // 四个视图窗口
    QVTKOpenGLWidget *axialWidget;
    QVTKOpenGLWidget *coronalWidget;
    QVTKOpenGLWidget *sagittalWidget;
    QVTKOpenGLWidget *surfaceWidget;
    
    // 滑条和标签
    QSlider *axialSlider;
    QSlider *coronalSlider;
    QSlider *sagittalSlider;
    QSlider *isoValueSlider;  // 等值面阈值滑条
    QSlider *smoothnessSlider; // 平滑度滑条
    QLabel *axialLabel;
    QLabel *coronalLabel;
    QLabel *sagittalLabel;
    QLabel *isoValueLabel;    // 等值面阈值标签
    QLabel *smoothnessLabel;  // 平滑度标签
    
    // 菜单和动作
    QMenu *fileMenu;
    QMenu *helpMenu;
    QToolBar *fileToolBar;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *loadDICOMAct;
    QAction *loadNIFTIAct;
    QPushButton *loadButton;
    QPushButton *loadNiftiButton;

    // VTK DICOM相关组件
    vtkSmartPointer<vtkDICOMImageReader> dicomReader;
    vtkSmartPointer<vtkNIFTIImageReader> niftiReader;
    vtkSmartPointer<vtkImageData> imageData;
    
    // 三个方向的图像查看器
    vtkSmartPointer<vtkImageViewer2> axialViewer;
    vtkSmartPointer<vtkImageViewer2> coronalViewer;
    vtkSmartPointer<vtkImageViewer2> sagittalViewer;
    
    // 3D表面渲染组件
    vtkSmartPointer<vtkRenderer> surfaceRenderer;
    vtkSmartPointer<vtkMarchingCubes> marchingCubes;
    vtkSmartPointer<vtkPolyDataMapper> surfaceMapper;
    vtkSmartPointer<vtkActor> surfaceActor;
    
    // 图像切片器
    vtkSmartPointer<vtkImageReslice> axialReslice;
    vtkSmartPointer<vtkImageReslice> coronalReslice;
    vtkSmartPointer<vtkImageReslice> sagittalReslice;
    
    // 图像尺寸信息
    int dimensions[3];
    double spacing[3];
    double origin[3];
    double imageRange[2];  // 图像标量值范围
};

#endif // MAINWINDOW_H 