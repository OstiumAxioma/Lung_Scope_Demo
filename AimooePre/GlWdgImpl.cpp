#include <QPainter>
#include <QPainterPath>
#include <QGuiApplication>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QApplication>
#include <windows.h>
#include <TlHelp32.h>
#include <QOpenGLContext>
#include <QOpenGLPaintDevice>
#include <QWindow>
#include <QOpenGLFunctions>
#include <QWidget>
#include <qopengl.h>
#include <qmath.h>
#include <omp.h>


#include "GlWdgImpl.h"
#include "glut.h"
#include "vm/img/base/ImgVm.h"
#include "component/utils/Utils.h"
#include "component/define/AimLibDefine.h"

template<typename T>
int getCvType() {
    if (std::is_same<T, unsigned char>::value) {
        return CV_8UC1;
    }
    else if (std::is_same<T, int>::value) {
        return CV_32SC1;
    }
    else if (std::is_same<T, float>::value) {
        return CV_32FC1;
    }
    else if (std::is_same<T, double>::value) {
        return CV_64FC1;
    }
    else if (std::is_same<T, short>::value) {
        return CV_16SC1;
    }

    return CV_32FC1;
}

//转成cv::Mat矩阵，利用其批处理计算
template<typename T, typename T2>
void convertCTToMat(T ctData, int width, int height, int depth, std::vector<cv::Mat>& mats) {
    mats.resize(depth);
    for (int z = 0; z < depth; ++z)
    {
        std::size_t offset = static_cast<std::size_t>(z) * height * width;
        mats[z] = cv::Mat(height, width, getCvType<T2>(), ctData + offset);
    }
}

int16 DataMinMax(float temp) {
    unsigned char value = 0;

    if ((temp > 0) && (temp < 255)) {
        value = (int16)temp;
    }
    else if (temp >= 255) {
        value = 255;
    }
    else if (temp < 0) {
        value = 0;
    }
    return value;
}

GlWdgImpl::GlWdgImpl(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    qRegisterMetaType<AimLibDefine::ViewNameEnum>("AimLibDefine::ViewNameEnum");
    this->setAttribute(Qt::WA_DeleteOnClose);
    m_timerId = startTimer(10);
    RegisterTimer::stGetInstance()->setErrCB(std::bind(&GlWdgImpl::handleLicense, this, std::placeholders::_1));
}

GlWdgImpl::~GlWdgImpl() {
    killTimer(m_timerId);
    delete[] m_pImgBuf;
    delete[] m_pLabelImgBuf;
    delete[] m_pFusionImgBuf;
}

std::vector<DrawInfo*> GlWdgImpl::getDrawInfos() {
    return m_drawInfos;
}

QRect GlWdgImpl::getCurRect() {
    return m_imgRct;
}

float GlWdgImpl::getUnitView()
{
    return m_unitView;
}

void GlWdgImpl::setGlWidgetType(AimLibDefine::ViewNameEnum type) {
    m_type = type;
}

AimLibDefine::ViewNameEnum GlWdgImpl::getGlWidgetType() {
    return m_type;
}

bool GlWdgImpl::loadCtInfo(int index) {
    m_needIndex = index;
    return true;
}

bool GlWdgImpl::updateEllipse(double radius) {
    needradius = radius;
    return true;
}

bool GlWdgImpl::updateEllipseCenter(QVector3D center) {
    needcenter = center;
    return true;
}

void GlWdgImpl::setInteractiveMode()
{
    InteractiveMode = true;
}

bool GlWdgImpl::setWindowWC(int windowWidth, int windowCenter)
{
    m_needWindowCenter = windowCenter;
    m_needWindowWidth = windowWidth;
    return true;
}

bool GlWdgImpl::getWindowWC(int& windowWidth, int& windowCenter)
{
    auto pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo();
    if (!pCtInfo || !pCtInfo->pCtInfo)
        return false;
    windowCenter = m_curWindowCenter;// + ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->windowCenter;
    windowWidth = m_curWindowWidth; //  +ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->windowWidth;
    return true;
}

bool GlWdgImpl::getInitWindowWC(int& windowWidth, int& windowCenter)
{
    auto pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo();
    if (!pCtInfo || !pCtInfo->pCtInfo)
        return false;
    windowCenter = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->windowCenter;
    windowWidth = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->windowWidth;
    return true;
}

bool GlWdgImpl::setMinMaxHU(int minHu, int maxHU, bool isUsing)
{
    isUsingHURange = isUsing;
    m_needMinHU = minHu;
    m_needMaxHU = maxHU;
    return true;
}
void GlWdgImpl::setFusionRate(double origRate, double fusionRate)
{
    m_needOrigRate = origRate;
    m_needFusionRate = fusionRate;
}

void GlWdgImpl::showCenter(QVector3D p)
{
    auto pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo();
    if (!pCtInfo || !pCtInfo->pCtInfo)
        return;

    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);

    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[getGlWidgetType()];
    auto xspace = dicom.xSpacing;
    auto yspace = dicom.ySpacing;
    auto zspace = dicom.zSpacing;

    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;//原点做了变动
    float ct_width, ct_height, ctWidthSpacing, ctHeightSpacing;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL: {
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.ySpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.ySpacing;
    } break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.zSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        ct_width = (dicom.width - 1) * dicom.ySpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.ySpacing;
        ctHeightSpacing = dicom.zSpacing;
        break;
    default:
        break;
    }

    float unitView;// unitWidthView, unitHeightView;
    if (ct_width > ct_height)//以最大的为主
    {
        unitView = ct_height / m_scale / m_glRect.height();//单位视图与距离比例
    }
    else
    {
        unitView = ct_width / m_scale / m_glRect.width();//单位视图与距离比例
    }

    float x = 0, y = 0;
    double relativateX, relativateY, relativateZ;
    relativateX = (p.x() - imgPos[0]);
    relativateY = (p.y() - imgPos[1]);
    relativateZ = (p.z() - imgPos[2]);
    switch (m_type) {
    case AimLibDefine::E_AXIAL: {
        x = relativateX / unitView;
        y = relativateY / unitView;
    }break;
    case AimLibDefine::E_CORONAL: {
        x = relativateX / unitView;
        y = (ct_height - relativateZ) / unitView;
    }break;
    case AimLibDefine::E_SAGITTAL: {
        x = relativateY / unitView;
        y = (ct_height - relativateZ) / unitView;
    }break;
    case AimLibDefine::E_SHOW3D:
        break;
    default:
        break;
    }

    x = std::max(0.0f, x - m_glRect.x() - m_glRect.width() / 2);
    y = std::max(0.0f, y - m_glRect.y() - m_glRect.height() / 2);

    //设置位置将其移动到
    ImgVmFactory::stGetInstance()->setViewRect(m_type, QRect(x * unitView / ctWidthSpacing, y * unitView / ctHeightSpacing, 0, 0));
}

QVector3D GlWdgImpl::pointFromGlToSpace(QPoint p) {
    resizeEvent(nullptr);
    auto ret = QVector3D();
    auto pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo();
    if (!pCtInfo || !pCtInfo->pCtInfo)
        return ret;
    QPoint realpoint = QPoint(p.x() - m_glRect.x(), p.y() - m_glRect.y());
    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);
    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[getGlWidgetType()];
    auto xspace = dicom.xSpacing;
    auto yspace = dicom.ySpacing;
    auto zspace = dicom.zSpacing;
    //计算视图比例
    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;//原点做了变动
    float ct_width, ct_height, ctWidthSpacing, ctHeightSpacing;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL: {
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.ySpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.ySpacing;
        imgPos[0] = imgPos[0] + viewRect.x() * ctWidthSpacing;
        imgPos[1] = imgPos[1] + viewRect.y() * ctHeightSpacing;
    } break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.zSpacing;
        imgPos[0] = imgPos[0] + viewRect.x() * ctWidthSpacing;
        imgPos[2] = imgPos[2] - viewRect.y() * ctHeightSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        ct_width = (dicom.width - 1) * dicom.ySpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.ySpacing;
        ctHeightSpacing = dicom.zSpacing;
        imgPos[1] = imgPos[1] + viewRect.x() * ctWidthSpacing;
        imgPos[2] = imgPos[2] - viewRect.y() * ctHeightSpacing;
        break;
    default:
        break;
    }
    float unitView;// , unitHeightView;
    if (ct_width > ct_height)//以最大的为主
    {
        unitView = ct_height / m_scale / m_glRect.height();//单位视图与距离比例
        //unitWidthView = m_glRect.width() * unitHeightView/;
    }
    else
    {
        unitView = ct_width / m_scale / m_glRect.width();//单位视图与距离比例
        //unitHeightView = unitWidthView / ctWidthSpacing * ctHeightSpacing;
    }
    float x = realpoint.x() * unitView;
    float y = realpoint.y() * unitView;

    switch (m_type) {
    case AimLibDefine::E_AXIAL: {
        ret.setZ(imgPos[2] + m_curIndex * zspace);
        ret.setY(imgPos[1] + y);
        ret.setX(imgPos[0] + x);
    }break;
    case AimLibDefine::E_CORONAL: {
        ret.setZ(imgPos[2] + ct_height - y);
        ret.setY(imgPos[1] + m_curIndex * yspace);
        ret.setX(imgPos[0] + x);
    }break;
    case AimLibDefine::E_SAGITTAL: {
        ret.setZ(imgPos[2] + ct_height - y);
        ret.setY(imgPos[1] + x);
        ret.setX(imgPos[0] + m_curIndex * xspace);
    }break;
    case AimLibDefine::E_SHOW3D:
        break;
    default:
        break;
    }

    return ret;
}

QPoint GlWdgImpl::pointFromSpaceToGl(QVector3D p) {
    resizeEvent(nullptr);
    QPoint ret = QPoint();
    auto pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo();
    if (!pCtInfo || !pCtInfo->pCtInfo)
        return ret;

    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);

    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[getGlWidgetType()];
    auto xspace = dicom.xSpacing;
    auto yspace = dicom.ySpacing;
    auto zspace = dicom.zSpacing;

    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;//原点做了变动
    float ct_width, ct_height, ctWidthSpacing, ctHeightSpacing;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL: {
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.ySpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.ySpacing;
        imgPos[0] = imgPos[0] + viewRect.x() * ctWidthSpacing;
        imgPos[1] = imgPos[1] + viewRect.y() * ctHeightSpacing;
    } break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.zSpacing;
        imgPos[0] = imgPos[0] + viewRect.x() * ctWidthSpacing;
        imgPos[2] = imgPos[2] - viewRect.y() * ctHeightSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        ct_width = (dicom.width - 1) * dicom.ySpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.ySpacing;
        ctHeightSpacing = dicom.zSpacing;
        imgPos[1] = imgPos[1] + viewRect.x() * ctWidthSpacing;
        imgPos[2] = imgPos[2] - viewRect.y() * ctHeightSpacing;
        break;
    default:
        break;
    }
    float unitView;// unitWidthView, unitHeightView;
    if (ct_width > ct_height)//以最大的为主
    {
        unitView = ct_height / m_scale / m_glRect.height();//单位视图与距离比例
    }
    else
    {
        unitView = ct_width / m_scale / m_glRect.width();//单位视图与距离比例
    }

    float x = 0, y = 0;
    double relativateX, relativateY, relativateZ;
    relativateX = (p.x() - imgPos[0]);
    relativateY = (p.y() - imgPos[1]);
    relativateZ = (p.z() - imgPos[2]);
    switch (m_type) {
    case AimLibDefine::E_AXIAL: {
        x = relativateX / unitView;
        y = relativateY / unitView;
    }break;
    case AimLibDefine::E_CORONAL: {
        x = relativateX / unitView;
        y = (ct_height - relativateZ) / unitView;
    }break;
    case AimLibDefine::E_SAGITTAL: {
        x = relativateY / unitView;
        y = (ct_height - relativateZ) / unitView;
    }break;
    case AimLibDefine::E_SHOW3D:
        break;
    default:
        break;
    }

    QPoint realpoint = QPoint(std::round(x) + m_glRect.x(), std::round(y) + m_glRect.y());
    //加上平移
    return realpoint;
}

int GlWdgImpl::pointFromSpaceToImgIndex(QVector3D p) {
    resizeEvent(nullptr);
    auto pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo();
    if (!pCtInfo || !pCtInfo->pCtInfo)
        return 0;
    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);
    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[m_type];
    auto xspace = dicom.xSpacing;
    auto yspace = dicom.ySpacing;
    auto zspace = dicom.zSpacing;

    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;//原点做了变动
    double relativateX, relativateY, relativateZ;
    relativateX = (p.x() - imgPos[0]);
    relativateY = (p.y() - imgPos[1]);
    relativateZ = (p.z() - imgPos[2]);

    int index = 0;
    switch (m_type) {
    case AimLibDefine::E_AXIAL: {
        index = std::round(relativateZ / zspace);
    }break;
    case AimLibDefine::E_CORONAL: {
        index = std::round(relativateY / yspace);
    }break;
    case AimLibDefine::E_SAGITTAL: {
        index = std::round(relativateX / xspace);
    }break;
    case AimLibDefine::E_SHOW3D:
        break;
    default:
        break;
    }

    if (index < 0)
        index = 0;
    index = index >= dicom.total ? (dicom.total - 1) : index;

    return index;
}

void GlWdgImpl::drawPoint(QVector3D point, QColor clr, float r) {
    DrawInfo* info = new DrawInfo;
    info->type = DrawType::DT_Point;
    info->info = (DrawInfo::Info*)new PointInfo;
    info->info->point.clr = clr;
    info->info->point.p = point;
    info->info->point.r = r;

    std::unique_lock<std::mutex> lk(m_infoMtx);
    m_drawInfos.push_back(info);
    update();
}

void GlWdgImpl::drawLine(QVector3D point1, QVector3D point2, QColor clr, float w, bool bDash) {
    DrawInfo* info = new DrawInfo;
    info->type = DrawType::DT_Line;
    info->info = (DrawInfo::Info*)new LineInfo;
    info->info->line.clr = clr;
    info->info->line.p1 = point1;
    info->info->line.p2 = point2;
    info->info->line.width = w;
    info->info->line.bDash = bDash;

    std::unique_lock<std::mutex> lk(m_infoMtx);
    m_drawInfos.push_back(info);
    update();
}

void GlWdgImpl::drawPolygon(std::vector<QVector3D> points, QColor penClr, QColor brushClr, float w) {
    DrawInfo* info = new DrawInfo;
    info->type = DrawType::DT_Polygon;
    info->info = (DrawInfo::Info*)new PolygonInfo;
    info->info->polygon.penClr = penClr;
    info->info->polygon.brushClr = brushClr;
    info->info->polygon.ps = points;
    info->info->polygon.width = w;

    std::unique_lock<std::mutex> lk(m_infoMtx);
    m_drawInfos.push_back(info);
    update();
}

void GlWdgImpl::drawCurve(std::vector<QVector3D> points, QColor penClr, QColor brushClr, float w, float arc) {
    DrawInfo* info = new DrawInfo;
    info->type = DrawType::DT_Curve;
    info->info = (DrawInfo::Info*)new PolygonInfo;
    info->info->curveInfo.penClr = penClr;
    info->info->curveInfo.brushClr = brushClr;
    info->info->curveInfo.ps = points;
    info->info->curveInfo.width = w;
    info->info->curveInfo.arc = arc;

    std::unique_lock<std::mutex> lk(m_infoMtx);
    m_drawInfos.push_back(info);
    update();
}

bool GlWdgImpl::isPointInCurve(const QVector3D& point)
{
    if (!m_curvePainterPath.isEmpty())
    {
        return m_curvePainterPath.contains(pointFromSpaceToGl(point));
    }
    return false;
}

void GlWdgImpl::drawText(QString txt, QVector3D pos, int width, int height, QColor clr, uint fontH, uint weight, QTextOption to)
{
    TextInfo textInfo;
    textInfo.text = txt;
    textInfo.pos = pos;
    textInfo.width = width;
    textInfo.height = height;
    textInfo.color = clr;
    textInfo.fontH = fontH;
    textInfo.weight = weight;
    textInfo.to = to;

    std::unique_lock<std::mutex> lk(m_infoMtx);
    m_drawText.emplace_back(textInfo);
    update();
}

void GlWdgImpl::clear() {
    std::unique_lock<std::mutex> lk(m_infoMtx);
    std::for_each(m_drawInfos.begin(), m_drawInfos.end(), [&](DrawInfo* info) {
        //delete[] info->info;
        delete[]info;
        });
    m_drawInfos.clear();
    m_drawText.clear();
    update();
}

void GlWdgImpl::flash() {
    update();
}

void GlWdgImpl::clearWdg() {
}

void GlWdgImpl::scale(double v) {
    m_scale = v;
    update();
}

void GlWdgImpl::translate(QPoint translate)
{
    m_translate = translate;
    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);

    ImgVmFactory::stGetInstance()->setViewRect(m_type, std::max(viewRect.x() - m_translate.x(), 0), std::max(viewRect.y() - m_translate.y(), 0), viewRect.width(), viewRect.height());
    update();
}

void GlWdgImpl::setHighLightValue(int value, QColor color, double opacity)
{
    m_highLightValueList.clear();
    m_highLightValueList.push_back(value);
    if (m_highLightValueList.contains(1))
        m_highLightValueList.push_back(117);
    if (m_highLightValueList.contains(2))
        m_highLightValueList.push_back(118);
    m_highLightValueList.removeOne(0);
    m_highLightColor = color;
    m_highLightOpacity = opacity;
    update();
}

void GlWdgImpl::setHighLightValues(QList<int> valueList, QColor color, double opacity)
{
    m_highLightValueList = valueList;
    if (m_highLightValueList.contains(1))
        m_highLightValueList.push_back(117);
    if (m_highLightValueList.contains(2))
        m_highLightValueList.push_back(118);
    m_highLightValueList.removeOne(0);
    m_highLightColor = color;
    m_highLightOpacity = opacity;
    update();
}

void GlWdgImpl::paintEvent(QPaintEvent* event) {
    QPainter paint(this);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!ImgVmFactory::stGetInstance()->getCtInfo() || !ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo)
        return;

    //获取CT数据
    AimLibDefine::CtInfo* pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo;
    auto dicom = pCtInfo->dicoms[m_type];
    AimLibDefine::DSR_SliceVolumeInfo* pCtSlice = pCtInfo->pSliceInfo;
    signed short* pCTData = pCtSlice->ptrData;

    m_curIndex = std::min(dicom.total - 1, std::max(0, m_curIndex));

    int glRectWidth = this->width() - 144, glRectHeight = this->height() - 67;
    if (m_glRect.width() != 0 && m_glRect.width() < glRectWidth)
    {
        full_ratio = (glRectWidth * 1.0) / m_glRect.width();
    }
    else if (m_glRect.width() > glRectWidth)
    {
        full_ratio = 1.0;
    }

    m_glRect = QRect(90, 30, glRectWidth, glRectHeight);//自定义最大窗口
    //需要考虑图像空间比例
    float ct_width, ct_height, ctWidthSpacing, ctHeightSpacing;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL: {
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.ySpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.ySpacing;
    } break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.xSpacing;
        ctHeightSpacing = dicom.zSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        ct_width = (dicom.width - 1) * dicom.ySpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        ctWidthSpacing = dicom.ySpacing;
        ctHeightSpacing = dicom.zSpacing;
        break;
    default:
        break;
    }

    //绘制标签对应颜色，权限最大
    InterfaceImgLoaderVm* pLabel2ColorIns = ImgVmFactory::stGetLabel2ColorInstance();
    const AimLibDefine::ImgInfo* pLabel2ColorInfo = pLabel2ColorIns->getCtInfo();
    bool label2colorInfo = false;
    signed short* pLabelData = nullptr;
    if (pLabel2ColorInfo)
    {
        label2colorInfo = nullptr != pLabel2ColorInfo->pCtInfo->pSliceInfo->ptrData;
        if (label2colorInfo)
        {
            pLabelData = pLabel2ColorInfo->pCtInfo->pSliceInfo->ptrData;
        }
    }

    //计算窗宽窗位区间
    double Min;
    double Max;
    qint16 uWC;
    qint16 uWW;
    if (pCtSlice->modality == "REGION")
    {
        Min = 0;
        Max = 255;
    }
    else
    {
        uWC = m_curWindowCenter;				//pCtSlice->windowCenter +
        uWW = m_curWindowWidth;				//    pCtSlice->windowWidth +
        Min = uWC - uWW / 2.0;
        Max = uWC + uWW / 2.0;
    }

    // 获取当前要渲染的图像/融合图像
    std::unique_lock<std::mutex> lk(m_loadCtMtx);

    int labelValue;
    QColor color;
    bool isFoundColor;

    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);

    //需要根据定义的spacing，反向推断图像大小
    int img_width, img_height;
    if (ct_width > ct_height)//以最大的为主
    {
        m_unitView = ct_height / m_scale / m_glRect.height();//单位视图与距离比例
        //计算像素大小
        img_height = dicom.height / m_scale;//，以最大为主，按照等比例缩放
        img_width = int(m_glRect.width() * m_unitView / ctWidthSpacing);

        //计算图像视图宽高
        m_imgRct = QRect(m_glRect.x(), m_glRect.y(), m_glRect.width(), m_glRect.height());//图像缩放比例
    }
    else
    {
        m_unitView = ct_width / m_scale / m_glRect.width();//单位视图与距离比例
        //计算像素大小
        img_width = dicom.width / m_scale;//，以最大为主，按照等比例缩放
        img_height = int(m_glRect.height() * m_unitView / ctHeightSpacing);

        //计算图像视图宽高
        m_imgRct = QRect(m_glRect.x(), m_glRect.y(), m_glRect.width(), m_glRect.height());//图像缩放比例
    }

    //不要遍历全部，只需要增量修改图像数据
    int iterHeight = std::min(img_height, dicom.height);
    int iterWidth = std::min(img_width, dicom.width);

    //只需要保证增量循环处理，是从图像位置上开始循环
    int iterx = std::max(viewRect.x(), 0);
    int itery = std::max(viewRect.y(), 0);

    m_curQImg = QImage(img_width, img_height, QImage::Format_RGB32);
    m_curQImg.fill(qRgb(0, 0, 0));//默认填充为0

    iterHeight = std::min(dicom.height, iterHeight + itery);
    iterWidth = std::min(dicom.width, iterWidth + iterx);

    std::vector<int16> ctVecData;

    if (m_type == AimLibDefine::ViewNameEnum::E_AXIAL) {
        long zlocation = m_curIndex * dicom.width * dicom.height;
        //#pragma omp parallel for collapse(2)
        for (int y = itery; y < iterHeight; ++y) {
            for (int x = iterx; x < iterWidth; ++x) {
                int x1 = x - viewRect.x();
                int y1 = y - viewRect.y();

                //获取Ct值
                signed short ctData = pCTData[zlocation + y * dicom.width + x];	//将序列之后的内存值赋给data
                int16_t pixelValue;
                if (isUsingHURange)
                {
                    float range = m_curMaxHU - m_curMinHU;
                    if (range <= 0) range = 1.0f;  // 避免除以零
                    float temp1 = (ctData - m_curMinHU) * 255.0f / range;
                    pixelValue = DataMinMax(temp1);
                }
                else
                {
                    float temp1 = (ctData - Min) * 255.0f / (Max - Min);
                    // 将映射后的值放入缓冲区
                    pixelValue = DataMinMax(temp1);
                }
                ctVecData.emplace_back(pixelValue);
                //获取Label值
                if (label2colorInfo)
                {
                    labelValue = pLabelData[zlocation + y * dicom.width + x];
                    isFoundColor = pLabel2ColorIns->findColorFromLabel(labelValue, color);
                    if (isFoundColor)
                    {
                        color.setHsv(color.hue(), color.saturation(), pixelValue);
                        QRgb rgbPixel = qRgb(color.red(), color.green(), color.blue());
                        m_curQImg.setPixel(x1, y1, rgbPixel);
                    }
                    else
                    {
                        QRgb rgbPixel = qRgb(pixelValue, pixelValue, pixelValue);
                        m_curQImg.setPixel(x1, y1, rgbPixel);
                    }
                }
                else
                {
                    QRgb rgbPixel = qRgb(pixelValue, pixelValue, pixelValue);
                    m_curQImg.setPixel(x1, y1, rgbPixel);
                }
            }
        }

        //std::vector<cv::Mat> mats;
        //convertCTToMat<int16*, int16>(ctVecData.data(), iterWidth, iterHeight, 1, mats);
    }
    else if (m_type == AimLibDefine::ViewNameEnum::E_CORONAL)
    {
        long locationx = dicom.width * dicom.total;
        long locationy = m_curIndex * dicom.width;
        //#pragma omp parallel for collapse(2)
        for (int y = itery; y < iterHeight; ++y) {
            for (int x = iterx; x < iterWidth; ++x) {
                int x1 = x - iterx;
                int y1 = y - itery;
                signed short ctData = pCTData[(dicom.height - 1 - y) * locationx + locationy + x];
                int16_t pixelValue;
                if (isUsingHURange)
                {
                    float range = m_curMaxHU - m_curMinHU;
                    if (range <= 0) range = 1.0f;  // 避免除以零
                    float temp1 = (ctData - m_curMinHU) * 255.0f / range;
                    pixelValue = DataMinMax(temp1);
                }
                else
                {
                    float temp1 = (ctData - Min) * 255.0f / (Max - Min);
                    // 将映射后的值放入缓冲区
                    pixelValue = DataMinMax(temp1);
                }
                //着色且背景不为0
                if (label2colorInfo)
                {
                    labelValue = pLabelData[(dicom.height - 1 - y) * locationx + locationy + x];
                    isFoundColor = pLabel2ColorIns->findColorFromLabel(labelValue, color);
                    if (isFoundColor)
                    {
                        color.setHsv(color.hue(), color.saturation(), pixelValue);
                        QRgb rgbPixel = qRgb(color.red(), color.green(), color.blue());
                        m_curQImg.setPixel(x1, y1, rgbPixel);
                    }
                    else
                    {
                        QRgb rgbPixel = qRgb(pixelValue, pixelValue, pixelValue);
                        m_curQImg.setPixel(x1, y1, rgbPixel);
                    }
                }
                else
                {
                    QRgb rgbPixel = qRgb(pixelValue, pixelValue, pixelValue);
                    m_curQImg.setPixel(x1, y1, rgbPixel);
                }
            }
        }
    }
    else if (m_type == AimLibDefine::ViewNameEnum::E_SAGITTAL)
    {
        long locationx = dicom.width * dicom.total;
        //#pragma omp parallel for collapse(2)
        for (int y = itery; y < iterHeight; ++y) {
            for (int x = iterx; x < iterWidth; ++x) {
                int x1 = x - iterx;
                int y1 = y - itery;
                signed short ctData = pCTData[locationx * (dicom.height - y - 1) + m_curIndex + dicom.total * x];
                int16_t pixelValue;
                if (isUsingHURange)
                {
                    float range = m_curMaxHU - m_curMinHU;
                    if (range <= 0) range = 1.0f;  // 避免除以零
                    float temp1 = (ctData - m_curMinHU) * 255.0f / range;
                    pixelValue = DataMinMax(temp1);
                }
                else
                {
                    float temp1 = (ctData - Min) * 255.0f / (Max - Min);
                    // 将映射后的值放入缓冲区
                    pixelValue = DataMinMax(temp1);
                }

                //着色且背景不为0
                if (label2colorInfo)
                {
                    labelValue = pLabelData[locationx * (dicom.height - y - 1) + m_curIndex + dicom.total * x];
                    isFoundColor = pLabel2ColorIns->findColorFromLabel(labelValue, color);
                    if (isFoundColor)
                    {
                        color.setHsv(color.hue(), color.saturation(), pixelValue);
                        QRgb rgbPixel = qRgb(color.red(), color.green(), color.blue());
                        m_curQImg.setPixel(x1, y1, rgbPixel);
                    }
                    else
                    {
                        QRgb rgbPixel = qRgb(pixelValue, pixelValue, pixelValue);
                        m_curQImg.setPixel(x1, y1, rgbPixel);
                    }
                }
                else
                {
                    QRgb rgbPixel = qRgb(pixelValue, pixelValue, pixelValue);
                    m_curQImg.setPixel(x1, y1, rgbPixel);
                }
            }
        }
    }
    else
    {
        return;
    }
    // 平移
    //paint.translate(m_translate.x(), m_translate.y());

    // 缩放
    //paint.scale(m_scale, m_scale);
    paint.setOpacity(1.0);
    paint.drawImage(m_imgRct, m_curQImg);

    // 生成高亮区域
    /*if (drawHighLightRegion())
    {
        paint.setOpacity(m_highLightOpacity);
        paint.drawImage(m_imgRct, m_curMask);
    }*/

    std::for_each(m_drawInfos.begin(), m_drawInfos.end(), [&](DrawInfo* info) {
        switch (info->type) {
        case DrawType::DT_Line:
            drawLine(&paint, info->info->line);
            break;
        case DrawType::DT_Point:
            drawPoint(&paint, info->info->point);
            break;
        case DrawType::DT_Polygon:
            drawPolygon(&paint, info->info->polygon);
            break;
        case DrawType::DT_Curve:
            drawCurve(&paint, info->info->curveInfo);
            break;

        }
        });
    //绘制文字
    for (auto item : m_drawText)
    {
        drawText(&paint, item);
    }

    if (InteractiveMode)
    {
        //实际长度需要转成二维坐标长度
        float centerradiuradio = curradius * full_ratio;
        QVector3D leftCenterPoint = QVector3D(curcenter.x() + centerradiuradio, curcenter.y(), curcenter.z());
        QVector3D rightCenterPoint = QVector3D(curcenter.x() - centerradiuradio, curcenter.y(), curcenter.z());
        if (m_type == AimLibDefine::ViewNameEnum::E_SAGITTAL)
        {
            leftCenterPoint = QVector3D(curcenter.x(), curcenter.y(), curcenter.z() + centerradiuradio);
            rightCenterPoint = QVector3D(curcenter.x(), curcenter.y(), curcenter.z() - centerradiuradio);
        }
        centerradiuradio = QVector2D(pointFromSpaceToGl(rightCenterPoint) - pointFromSpaceToGl(leftCenterPoint)).length();

        QPainterPath path;
        QPoint centerPoint = pointFromSpaceToGl(curcenter);
        path.addEllipse(centerPoint.x() - centerradiuradio / 2, centerPoint.y() - centerradiuradio / 2, centerradiuradio, centerradiuradio);
        paint.setPen(QPen(QColor("#91A2AE"), 2));
        paint.drawPath(path);

        QPen pen(Qt::red);
        pen.setWidth(5);
        paint.setPen(pen);
        paint.drawPoint(centerPoint);
    }
    else//十字叉
    {
        QPoint centerPoint = pointFromSpaceToGl(curcenter);
        int linelength = 15;
        QPen pen2(QColor("#FFFFFF"));
        paint.setPen(pen2);
        paint.drawLine(centerPoint.x() - linelength, centerPoint.y(), centerPoint.x() + linelength, centerPoint.y());
        paint.drawLine(centerPoint.x(), centerPoint.y() - linelength, centerPoint.x(), centerPoint.y() + linelength);

        QPen pen(Qt::red);
        pen.setWidth(5);
        paint.setPen(pen);
        paint.drawPoint(centerPoint);
    }

}

void GlWdgImpl::resizeEvent(QResizeEvent* event) {

    update();
}

void GlWdgImpl::closeEvent(QCloseEvent* event) {
    event->accept();
}

void GlWdgImpl::timerEvent(QTimerEvent* event) {
    if (m_needIndex != m_curIndex ||
        m_needWindowCenter != m_curWindowCenter ||
        m_needWindowWidth != m_curWindowWidth ||
        m_needFusionRate != m_fusionRate ||
        m_needOrigRate != m_origRate ||
        curradius != needradius ||
        curcenter != needcenter ||
        m_needMaxHU != m_curMaxHU ||
        m_needMinHU != m_curMinHU
        ) {
        m_curIndex = m_needIndex;
        m_curWindowCenter = m_needWindowCenter;
        m_curWindowWidth = m_needWindowWidth;
        m_origRate = m_needOrigRate;
        m_fusionRate = m_needFusionRate;
        curradius = needradius;
        curcenter = needcenter;
        m_curMaxHU = m_needMaxHU;
        m_curMinHU = m_needMinHU;
        update();
    }
}

void GlWdgImpl::mouseReleaseEvent(QMouseEvent* event) {
    auto pos = event->pos();
    pos.setX((pos.x() - m_imgRct.x()) / m_scale);
    pos.setY((pos.y() - m_imgRct.y()) / m_scale);
    event->setLocalPos(pos);
    QWidget::mouseReleaseEvent(event);
}

void GlWdgImpl::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
}

void GlWdgImpl::drawPoint(QPainter* painter, PointInfo& info) {
    QPoint point = pointFromSpaceToGl(info.p);
    QPainterPath path;
    path.addEllipse(point, info.r, info.r);
    painter->fillPath(path, info.clr);
    //painter->setCompositionMode(QPainter::CompositionMode_Clear);
}

void GlWdgImpl::drawLine(QPainter* painter, LineInfo& info) {
    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);
    QPainterPath path;
    path.moveTo(pointFromSpaceToGl(info.p1));
    path.lineTo(pointFromSpaceToGl(info.p2));
    auto prePen = painter->pen();
    QPen pen(info.clr, info.width);
    if (info.bDash) {
        QVector<qreal>dashes;
        qreal space = 4;
        dashes << 3 << space;
        pen.setDashPattern(dashes);
    }
    painter->setPen(pen);
    painter->drawPath(path);
    painter->setPen(prePen);
}

void GlWdgImpl::drawPolygon(QPainter* painter, PolygonInfo& info) {
    QPainterPath path;
    if (info.ps.size() < 2) {
        return;
    }
    path.moveTo(pointFromSpaceToGl(info.ps[0]));
    for (int i = 1; i < info.ps.size(); i++) {
        path.lineTo(pointFromSpaceToGl(info.ps[i]));

    }
    path.closeSubpath();
    auto prePen = painter->pen();
    auto preBrush = painter->brush();
    QPen pen(info.penClr, info.width);
    QBrush brush(info.brushClr);
    painter->setPen(pen);
    painter->setBrush(brush);
    painter->drawPath(path);
    painter->setPen(prePen);
    painter->setBrush(preBrush);
}

void GlWdgImpl::drawCurve(QPainter* painter, CurveInfo& info) {

    QVector<QPointF> points;
    for (int i = 0; i < info.ps.size(); i++)
    {
        points.append(pointFromSpaceToGl(info.ps[i]));
    }

    m_curvePainterPath = createClosedCurveWithTension(points, info.arc);

    QPen pen(info.penClr, info.width);
    painter->setPen(pen);
    painter->drawPath(m_curvePainterPath);

}

void GlWdgImpl::drawText(QPainter* painter, TextInfo& info)
{
    QPoint pos = pointFromSpaceToGl(info.pos);
    Utils::drawText(painter, info.text, QRect(pos.x(), pos.y(), info.width, info.height), info.color, info.fontH, info.to);
}

void GlWdgImpl::drawTag(QPainter* painter)
{
    // 左上角的截面类型
    //auto typeStrRect = QRect(15, 15, 166, 25);
    //auto indexStrRect = QRect(15, 50, 266, 25);
    ////paint.fillRect(typeStrRect, QColor(23, 23, 255));
    //QString typeStr;
    //int totalSize = 0;
    //LineInfo lineX;
    //LineInfo lineY;
    //lineX.width = 2.0;

    //lineX.bDash = true;
    //lineY.width = 2.0;

    //lineY.bDash = true;
    //QString axesX = "";
    //QString axesY = "";

    //switch (m_type) {
    //case AimLibDefine::E_AXIAL:
    //{
    //	typeStr = "Axial";
    //	totalSize = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->zSize;
    //	lineX.p1 = QPointF(m_rect.left(), m_rect.height() * (1.0 - m_curIndex3D.y() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->ySize));
    //	lineX.p2 = QPointF(m_rect.right(), m_rect.height() * (1.0 - m_curIndex3D.y() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->ySize));
    //	lineY.p1 = QPointF(m_rect.width() * (0.5 + m_curIndex3D.x() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->xSize), m_rect.top());
    //	lineY.p2 = QPointF(m_rect.width() * (0.5 + m_curIndex3D.x() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->xSize), m_rect.bottom());
    //	axesX = "X";
    //	axesY = "Y";
    //	lineX.clr = QColor("#0C89EB");
    //	lineY.clr = QColor("#764B97");
    //}
    //break;
    //case AimLibDefine::E_CORONAL:
    //{
    //	typeStr = "Coronal";
    //	totalSize = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->ySize;
    //	lineX.p1 = QPointF(m_rect.left(), m_rect.height() * (1.0 - m_curIndex3D.z() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->zSize));
    //	lineX.p2 = QPointF(m_rect.right(), m_rect.height() * (1.0 - m_curIndex3D.z() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->zSize));
    //	lineY.p1 = QPointF(m_rect.width() * (0.5 + m_curIndex3D.x() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->xSize), m_rect.top());
    //	lineY.p2 = QPointF(m_rect.width() * (0.5 + m_curIndex3D.x() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->xSize), m_rect.bottom());
    //	axesX = "X";
    //	axesY = "Z";
    //	lineX.clr = QColor("#0C89EB");
    //	lineY.clr = QColor("#5F974B");
    //}
    //break;
    //case AimLibDefine::E_SAGITTAL:
    //{
    //	typeStr = "Sagittal";
    //	totalSize = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->xSize;
    //	lineX.p1 = QPointF(m_rect.left(), m_rect.height() * (1.0 - m_curIndex3D.z() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->zSize));
    //	lineX.p2 = QPointF(m_rect.right(), m_rect.height() * (1.0 - m_curIndex3D.z() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->zSize));
    //	lineY.p1 = QPointF(m_rect.width() * m_curIndex3D.y() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->ySize, m_rect.top());
    //	lineY.p2 = QPointF(m_rect.width() * m_curIndex3D.y() / ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->ySize, m_rect.bottom());
    //	axesX = "Y";
    //	axesY = "Z";
    //	lineX.clr = QColor("#764B97");
    //	lineY.clr = QColor("#5F974B");
    //}
    //break;
    //case AimLibDefine::E_SHOW3D:
    //	break;
    //default:
    //	break;
    //}
    //Utils::drawTextEx(painter, typeStr, typeStrRect, QColor("#CAE1FB"), 18, QFont::Bold, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

    //// 绘制当前切片编号
    //QString indexStr = QString("%1/%2").arg(m_curIndex).arg(totalSize);
    //Utils::drawTextEx(painter, indexStr, indexStrRect, QColor("#C7C9D0"), 18, QFont::Bold, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

    //// 绘制当前标识线
    //drawLine(painter, lineX);
    //drawLine(painter, lineY);

    //// 绘制当前指示方向
    //Utils::drawTextEx(painter, axesX, QRect(lineX.p2.x() < 30 ? lineX.p2.x() : lineX.p2.x() - 30, lineX.p2.y() < 30 ? lineX.p2.y() : lineX.p2.y() - 30, 30, 30),
    //	QColor("#FFBF00"), 18, QFont::Bold, QTextOption(Qt::AlignCenter));
    //Utils::drawTextEx(painter, axesY, QRect(lineY.p1.x() < 30 ? lineY.p1.x() : lineY.p1.x() - 30, lineY.p1.y() < 30 ? lineY.p1.y() : lineY.p1.y() - 30, 30, 30),
    //	QColor("#FFBF00"), 18, QFont::Bold, QTextOption(Qt::AlignCenter));
}

bool GlWdgImpl::drawHighLightRegion()
{
    // 生成高亮区域
/*	if (m_highLightValue > 0 && ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->modality == "REGION")
    {
        m_curMask = QImage(m_curQImg.size(), QImage::Format_RGBA8888);
        if (m_highLightValue == 1 || m_highLightValue == 2)
        {
            for (int i = 0; i < m_curQImg.height(); i++)
                for (int j = 0; j < m_curQImg.width(); j++)
                {
                    int pixelValue = qGray(m_curQImg.pixel(j, i));
                    if (pixelValue == m_highLightValue || pixelValue == (m_highLightValue + 116))
                        m_curMask.setPixel(j, i, m_highLightColor.rgba());
                    else
                        m_curMask.setPixel(j, i, qRgba(0, 0, 0, 0));
                }
        }
        else if (m_highLightValue == 117 || m_highLightValue == 118)
        {
            for (int i = 0; i < m_curQImg.height(); i++)
                for (int j = 0; j < m_curQImg.width(); j++)
                {
                    int pixelValue = qGray(m_curQImg.pixel(j, i));
                    if (pixelValue == 1 || pixelValue == 2)
                        m_curMask.setPixel(j, i, m_highLightColor.rgba());
                    else if (pixelValue == 117 || pixelValue == 118)
                        m_curMask.setPixel(j, i, m_thresholdColor.rgba());
                    else
                        m_curMask.setPixel(j, i, qRgba(0, 0, 0, 0));
                }
        }
        else
        {
            for (int i = 0; i < m_curQImg.height(); i++)
                for (int j = 0; j < m_curQImg.width(); j++)
                {
                    int pixelValue = qGray(m_curQImg.pixel(j, i));

                    if (pixelValue == m_highLightValue)
                        m_curMask.setPixel(j, i, m_highLightColor.rgba());
                    else
                        m_curMask.setPixel(j, i, qRgba(0, 0, 0, 0));
                }
        }
        return true;
    }
    else*/ if (m_highLightValueList.size() > 0 && ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->modality == "REGION")
    {
        m_curMask = QImage(m_curQImg.size(), QImage::Format_RGB888);
        if (m_highLightValueList[0] == 117 || m_highLightValueList[0] == 118)
        {
            for (int i = 0; i < m_curQImg.height(); i++)
                for (int j = 0; j < m_curQImg.width(); j++)
                {
                    int pixelValue = qGray(m_curQImg.pixel(j, i));
                    if (pixelValue == 1 || pixelValue == 2)
                        m_curMask.setPixel(j, i, m_highLightColor.rgba());
                    else if (pixelValue == 117 || pixelValue == 118)
                        m_curMask.setPixel(j, i, m_thresholdColor.rgba());
                    else
                        m_curMask.setPixel(j, i, qRgba(0, 0, 0, 0));
                }
        }
        else
        {
            for (int i = 0; i < m_curQImg.height(); i++)
                for (int j = 0; j < m_curQImg.width(); j++)
                {
                    int pixelValue = qGray(m_curQImg.pixel(j, i));

                    if (m_highLightValueList.contains(pixelValue))
                        m_curMask.setPixel(j, i, m_highLightColor.rgba());
                    else
                        m_curMask.setPixel(j, i, qRgba(0, 0, 0, 0));
                }
        }
        return true;
    }
    else
        return false;
}

QPainterPath GlWdgImpl::createClosedCurveWithTension(const QVector<QPointF>& points, double tension, bool closePath)
{
    QPainterPath path;
    if (points.size() < 2) return path;

    // 扩展点集使曲线闭合
    QVector<QPointF> extendedPoints;
    extendedPoints.reserve(points.size() + 3);
    extendedPoints << points.last(); // 添加最后一个点作为起始
    extendedPoints << points;        // 添加原始点集
    extendedPoints << points.first() << points[1]; // 添加首两个点作为结尾

    // 移动到第一个点
    path.moveTo(points.first());

    // 计算每段曲线
    const int n = points.size();
    const double s = (1.0 - tension) / 6.0;

    for (int i = 0; i < n; ++i) {
        const QPointF& p0 = extendedPoints[i];
        const QPointF& p1 = extendedPoints[i + 1];
        const QPointF& p2 = extendedPoints[i + 2];
        const QPointF& p3 = extendedPoints[i + 3];

        // 计算贝塞尔控制点
        QPointF cp1 = p1 + (p2 - p0) * s;
        QPointF cp2 = p2 - (p3 - p1) * s;

        // 添加曲线段
        path.cubicTo(cp1, cp2, p2);
    }

    if (closePath) path.closeSubpath();
    return path;
}

void GlWdgImpl::handleLicense(RegisterTimer::ErrCode err) {
    if (err != RegisterTimer::ErrCode::EC_OK) {
        HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, QApplication::applicationPid());
        ::TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        QApplication::quit();
        exit(0);
    }
}


int GlWdgImpl::fromSpaceGetIndex(QVector3D p)
{
    resizeEvent(nullptr);
    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[getGlWidgetType()];
    auto xspace = dicom.xSpacing;
    auto yspace = dicom.ySpacing;
    auto zspace = dicom.zSpacing;

    int index = 0;
    switch (m_type) {
    case AimLibDefine::E_AXIAL: {
        index = p.z() / zspace;
    }break;
    case AimLibDefine::E_CORONAL: {
        index = p.y() / yspace;
    }break;
    case AimLibDefine::E_SAGITTAL: {
        index = p.x() / xspace;
    }break;
    case AimLibDefine::E_SHOW3D:
        break;
    default:
        break;
    }

    if (index < 0)
        index = 0;
    index = index > dicom.total ? (dicom.total - 1) : index;

    return index;
}

QVector3D GlWdgImpl::fromAimglgetSpace(QPoint p) {
    auto ret = QVector3D();
    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[getGlWidgetType()];
    auto xspace = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->xSpacing;
    auto yspace = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->ySpacing;
    auto zspace = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->zSpacing;
    // 先将p按照painter的缩放还原
    auto px = (p.x() - m_translate.x()) / m_scale;
    auto py = (p.y() - m_translate.y()) / m_scale;
    auto w = m_curQImg.width();
    auto h = m_curQImg.height();
    float wratio = float(m_curQImg.width()) / m_imgRct.width();
    float hratio = float(m_curQImg.height()) / m_imgRct.height();
    int x, y;
    x = px - m_imgRct.x();
    y = py - m_imgRct.y();
    x *= wratio;
    y *= hratio;
    switch (m_type) {
    case AimLibDefine::E_AXIAL: {
        ret.setZ(m_curIndex * zspace);
        ret.setY((dicom.height - y) * yspace);
        ret.setX(x * xspace);
    }break;
    case AimLibDefine::E_CORONAL: {
        ret.setZ((dicom.height - y) * zspace);
        ret.setY(m_curIndex * yspace);
        ret.setX(x * xspace);
    }break;
    case AimLibDefine::E_SAGITTAL: {
        ret.setZ((dicom.height - y) * zspace);
        ret.setY(x * yspace);
        ret.setX(m_curIndex * xspace);
    }break;
    case AimLibDefine::E_SHOW3D:
        break;
    default:
        break;
    }

    return ret;
}


void GlWdgImpl::setInteractiveMode(bool label)
{
    InteractiveMode = label;
}

bool GlWdgImpl::getInteractiveMode()
{
    return InteractiveMode;
}
