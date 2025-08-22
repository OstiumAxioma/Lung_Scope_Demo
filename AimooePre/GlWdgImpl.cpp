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

//ת��cv::Mat��������������������
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

    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;//ԭ�����˱䶯
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
    if (ct_width > ct_height)//������Ϊ��
    {
        unitView = ct_height / m_scale / m_glRect.height();//��λ��ͼ��������
    }
    else
    {
        unitView = ct_width / m_scale / m_glRect.width();//��λ��ͼ��������
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

    //����λ�ý����ƶ���
    ImgVmFactory::stGetInstance()->setViewRect(m_type, QRect(x * unitView / ctWidthSpacing, y * unitView / ctHeightSpacing, 0, 0));
}

QVector3D GlWdgImpl::pointFromGlToSpace(QPoint p) {
    resizeEvent(nullptr);
    auto ret = QVector3D();
    auto pCtInfo = ImgVmFactory::stGetInstance()->getCtInfo();
    if (!pCtInfo || !pCtInfo->pCtInfo)
        return ret;
        
    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[getGlWidgetType()];
    auto xspace = dicom.xSpacing;
    auto yspace = dicom.ySpacing;
    auto zspace = dicom.zSpacing;
    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;
    
    // Step 1: Convert screen point to GL widget coordinates
    QPointF screenPoint(p.x() - m_glRect.x(), p.y() - m_glRect.y());
    
    // Step 2: Get center of the view
    QPointF center = m_glRect.center() - QPointF(m_glRect.x(), m_glRect.y());
    
    // Step 3: Apply inverse transform (reverse of paintEvent transform)
    // The paintEvent transform order is:
    // 1. translate(center)
    // 2. scale(m_scale, m_scale)
    // 3. translate(m_translate)
    // So inverse should be:
    // 1. subtract center
    // 2. divide by scale  
    // 3. subtract user translation
    
    // First, move origin to center (inverse of translate(center))
    screenPoint -= center;
    // Then apply inverse scale
    screenPoint /= m_scale;
    // Then apply inverse user translation
    screenPoint -= QPointF(m_translate.x(), m_translate.y());
    
    // Step 4: Convert to physical coordinates
    float physicalWidth, physicalHeight;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL:
        physicalWidth = dicom.width * dicom.xSpacing;
        physicalHeight = dicom.height * dicom.ySpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        physicalWidth = dicom.width * dicom.xSpacing;
        physicalHeight = dicom.height * dicom.zSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        physicalWidth = dicom.width * dicom.ySpacing;
        physicalHeight = dicom.height * dicom.zSpacing;
        break;
    default:
        physicalWidth = dicom.width;
        physicalHeight = dicom.height;
        break;
    }
    
    // Add back the offset to get physical position
    float physX = screenPoint.x() + physicalWidth / 2.0;
    float physY = screenPoint.y() + physicalHeight / 2.0;
    
    // Step 5: Convert physical coordinates to pixel coordinates
    float pixelX, pixelY;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL:
        pixelX = physX / dicom.xSpacing;
        pixelY = physY / dicom.ySpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        pixelX = physX / dicom.xSpacing;
        pixelY = physY / dicom.zSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        pixelX = physX / dicom.ySpacing;
        pixelY = physY / dicom.zSpacing;
        break;
    default:
        pixelX = physX;
        pixelY = physY;
        break;
    }
    
    // Step 6: Convert to 3D space coordinates
    switch (m_type) {
    case AimLibDefine::E_AXIAL: {
        ret.setX(imgPos[0] + pixelX * xspace);
        ret.setY(imgPos[1] + pixelY * yspace);
        ret.setZ(imgPos[2] + m_curIndex * zspace);
    }break;
    case AimLibDefine::E_CORONAL: {
        ret.setX(imgPos[0] + pixelX * xspace);
        ret.setY(imgPos[1] + m_curIndex * yspace);
        ret.setZ(imgPos[2] + (dicom.height - 1 - pixelY) * zspace);
    }break;
    case AimLibDefine::E_SAGITTAL: {
        ret.setX(imgPos[0] + m_curIndex * xspace);
        ret.setY(imgPos[1] + pixelX * yspace);
        ret.setZ(imgPos[2] + (dicom.height - 1 - pixelY) * zspace);
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

    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;//ԭ�����˱䶯
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
    if (ct_width > ct_height)//������Ϊ��
    {
        unitView = ct_height / m_scale / m_glRect.height();//��λ��ͼ��������
    }
    else
    {
        unitView = ct_width / m_scale / m_glRect.width();//��λ��ͼ��������
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
    //����ƽ��
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

    std::vector<double> imgPos = pCtInfo->pCtInfo->pSliceInfo->imgPosition;//ԭ�����˱䶯
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
    // Do NOT set m_needResample - scaling doesn't require resampling
    update();
}

void GlWdgImpl::translate(QPoint translate)
{
    // Simply update the translation offset and repaint
    // No longer modify viewRect - translation is handled by QPainter transform
    m_translate = m_translate + translate;  // Accumulate translation
    // Do NOT set m_needResample - translation doesn't require resampling
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

void GlWdgImpl::sampleCTData() {
    if (!ImgVmFactory::stGetInstance()->getCtInfo() || !ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo)
        return;

    //��ȡCT����
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

    m_glRect = QRect(90, 30, glRectWidth, glRectHeight);//�Զ�����󴰿�
    //��Ҫ����ͼ��ռ����
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

    //���Ʊ�ǩ��Ӧ��ɫ��Ȩ�����
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

    //���㴰����λ����
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

    // ��ȡ��ǰҪ��Ⱦ��ͼ��/�ں�ͼ��
    std::unique_lock<std::mutex> lk(m_loadCtMtx);

    int labelValue;
    QColor color;
    bool isFoundColor;

    QRect viewRect = ImgVmFactory::stGetInstance()->getViewRect(m_type);

    // Always create QImage with full CT slice dimensions
    // No longer adjust image size based on scale
    int img_width = dicom.width;
    int img_height = dicom.height;
    
    // Calculate unit view for coordinate conversion (but doesn't affect sampling)
    if (ct_width > ct_height)//������Ϊ��
    {
        m_unitView = ct_height / m_scale / m_glRect.height();//��λ��ͼ��������
    }
    else
    {
        m_unitView = ct_width / m_scale / m_glRect.width();//��λ��ͼ��������
    }

    // Keep drawing area constant
    m_imgRct = QRect(m_glRect.x(), m_glRect.y(), m_glRect.width(), m_glRect.height());

    // Always sample complete CT slice, not affected by viewRect
    int iterx = 0;  // Always start from 0
    int itery = 0;  // Always start from 0
    int iterHeight = dicom.height;  // Full height
    int iterWidth = dicom.width;    // Full width

    m_curQImg = QImage(img_width, img_height, QImage::Format_RGB32);
    m_curQImg.fill(qRgb(0, 0, 0));//Ĭ�����Ϊ0

    std::vector<int16> ctVecData;

    if (m_type == AimLibDefine::ViewNameEnum::E_AXIAL) {
        long zlocation = m_curIndex * dicom.width * dicom.height;
        //#pragma omp parallel for collapse(2)
        for (int y = itery; y < iterHeight; ++y) {
            for (int x = iterx; x < iterWidth; ++x) {
                int x1 = x;  // Use x directly, no longer subtract viewRect
                int y1 = y;  // Use y directly, no longer subtract viewRect

                //��ȡCtֵ
                signed short ctData = pCTData[zlocation + y * dicom.width + x];	//������֮����ڴ�ֵ����data
                int16_t pixelValue;
                if (isUsingHURange)
                {
                    float range = m_curMaxHU - m_curMinHU;
                    if (range <= 0) range = 1.0f;  // ���������
                    float temp1 = (ctData - m_curMinHU) * 255.0f / range;
                    pixelValue = DataMinMax(temp1);
                }
                else
                {
                    float temp1 = (ctData - Min) * 255.0f / (Max - Min);
                    // ��ӳ����ֵ���뻺����
                    pixelValue = DataMinMax(temp1);
                }
                ctVecData.emplace_back(pixelValue);
                //��ȡLabelֵ
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
                int x1 = x;  // Use x directly
                int y1 = y;  // Use y directly
                signed short ctData = pCTData[(dicom.height - 1 - y) * locationx + locationy + x];
                int16_t pixelValue;
                if (isUsingHURange)
                {
                    float range = m_curMaxHU - m_curMinHU;
                    if (range <= 0) range = 1.0f;  // ���������
                    float temp1 = (ctData - m_curMinHU) * 255.0f / range;
                    pixelValue = DataMinMax(temp1);
                }
                else
                {
                    float temp1 = (ctData - Min) * 255.0f / (Max - Min);
                    // ��ӳ����ֵ���뻺����
                    pixelValue = DataMinMax(temp1);
                }
                //��ɫ�ұ�����Ϊ0
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
                int x1 = x;  // Use x directly
                int y1 = y;  // Use y directly
                signed short ctData = pCTData[locationx * (dicom.height - y - 1) + m_curIndex + dicom.total * x];
                int16_t pixelValue;
                if (isUsingHURange)
                {
                    float range = m_curMaxHU - m_curMinHU;
                    if (range <= 0) range = 1.0f;  // ���������
                    float temp1 = (ctData - m_curMinHU) * 255.0f / range;
                    pixelValue = DataMinMax(temp1);
                }
                else
                {
                    float temp1 = (ctData - Min) * 255.0f / (Max - Min);
                    // ��ӳ����ֵ���뻺����
                    pixelValue = DataMinMax(temp1);
                }

                //��ɫ�ұ�����Ϊ0
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
    
    // Mark that sampling is complete
    m_needResample = false;
}

void GlWdgImpl::paintEvent(QPaintEvent* event) {
    QPainter paint(this);
    paint.setRenderHint(QPainter::Antialiasing);
    paint.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!ImgVmFactory::stGetInstance()->getCtInfo() || !ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo)
        return;
    
    // Only resample when needed
    if (m_needResample) {
        sampleCTData();
    }
    
    // Get dicom info for drawing
    auto dicom = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[m_type];
    
    // Calculate m_glRect (display area)
    int glRectWidth = this->width() - 144, glRectHeight = this->height() - 67;
    if (m_glRect.width() != 0 && m_glRect.width() < glRectWidth)
    {
        full_ratio = (glRectWidth * 1.0) / m_glRect.width();
    }
    else if (m_glRect.width() > glRectWidth)
    {
        full_ratio = 1.0;
    }
    m_glRect = QRect(90, 30, glRectWidth, glRectHeight);
    m_imgRct = QRect(m_glRect.x(), m_glRect.y(), m_glRect.width(), m_glRect.height());
    
    // Calculate unit view for coordinate conversion
    float ct_width, ct_height;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL: {
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.ySpacing;
    } break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        ct_width = (dicom.width - 1) * dicom.xSpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        ct_width = (dicom.width - 1) * dicom.ySpacing;
        ct_height = (dicom.height - 1) * dicom.zSpacing;
        break;
    default:
        break;
    }
    
    if (ct_width > ct_height) {
        m_unitView = ct_height / m_scale / m_glRect.height();
    } else {
        m_unitView = ct_width / m_scale / m_glRect.width();
    }
    
    // Use QPainter transform for scaling and translation
    paint.save();
    
    // Set clipping to ensure image doesn't draw outside m_glRect
    paint.setClipRect(m_glRect);
    
    // Calculate center point for scaling (view center)
    QPointF center = m_glRect.center();
    
    // Translate to center point
    paint.translate(center);
    
    // Apply scaling
    paint.scale(m_scale, m_scale);
    
    // Apply user translation
    paint.translate(m_translate.x(), m_translate.y());
    
    // Calculate physical dimensions considering spacing
    float physicalWidth, physicalHeight;
    switch (m_type) {
    case AimLibDefine::ViewNameEnum::E_AXIAL:
        physicalWidth = dicom.width * dicom.xSpacing;
        physicalHeight = dicom.height * dicom.ySpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_CORONAL:
        physicalWidth = dicom.width * dicom.xSpacing;
        physicalHeight = dicom.height * dicom.zSpacing;
        break;
    case AimLibDefine::ViewNameEnum::E_SAGITTAL:
        physicalWidth = dicom.width * dicom.ySpacing;
        physicalHeight = dicom.height * dicom.zSpacing;
        break;
    default:
        physicalWidth = dicom.width;
        physicalHeight = dicom.height;
        break;
    }
    
    // Use physical dimensions for drawing rect to maintain correct aspect ratio
    QRectF scaledRect(-physicalWidth / 2.0, -physicalHeight / 2.0, physicalWidth, physicalHeight);
    
    paint.setOpacity(1.0);
    
    // Draw the complete CT image
    // Due to transforms, image will automatically scale and translate
    paint.drawImage(scaledRect, m_curQImg);
    
    // Draw crosshair while transform is still active
    // This makes the crosshair follow the image
    if (InteractiveMode)
    {
        // Convert from space to image pixel coordinates
        QVector3D spacePos = curcenter;
        std::vector<double> imgPos = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->imgPosition;
        
        float pixelX = 0, pixelY = 0;
        switch (m_type) {
        case AimLibDefine::E_AXIAL: {
            pixelX = (spacePos.x() - imgPos[0]) / dicom.xSpacing;
            pixelY = (spacePos.y() - imgPos[1]) / dicom.ySpacing;
        }break;
        case AimLibDefine::E_CORONAL: {
            pixelX = (spacePos.x() - imgPos[0]) / dicom.xSpacing;
            pixelY = dicom.height - 1 - (spacePos.z() - imgPos[2]) / dicom.zSpacing;
        }break;
        case AimLibDefine::E_SAGITTAL: {
            pixelX = (spacePos.y() - imgPos[1]) / dicom.ySpacing;
            pixelY = dicom.height - 1 - (spacePos.z() - imgPos[2]) / dicom.zSpacing;
        }break;
        default:
            break;
        }
        
        // Map to physical coordinates (considering spacing)
        float physX = pixelX * (m_type == AimLibDefine::E_SAGITTAL ? dicom.ySpacing : dicom.xSpacing);
        float physY = pixelY * (m_type == AimLibDefine::E_AXIAL ? dicom.ySpacing : dicom.zSpacing);
        
        // Offset to match the image rect
        physX -= physicalWidth / 2.0;
        physY -= physicalHeight / 2.0;
        
        // Draw ellipse
        float radius = curradius * full_ratio / m_scale;
        QPainterPath path;
        path.addEllipse(QPointF(physX, physY), radius, radius);
        paint.setPen(QPen(QColor("#91A2AE"), 2));
        paint.drawPath(path);
        
        // Draw center point
        QPen pen(Qt::red);
        pen.setWidth(5 / m_scale);  // Adjust pen width for scale
        paint.setPen(pen);
        paint.drawPoint(QPointF(physX, physY));
    }
    else // Crosshair mode
    {
        // Convert from space to image pixel coordinates
        QVector3D spacePos = curcenter;
        std::vector<double> imgPos = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->imgPosition;
        
        float pixelX = 0, pixelY = 0;
        switch (m_type) {
        case AimLibDefine::E_AXIAL: {
            pixelX = (spacePos.x() - imgPos[0]) / dicom.xSpacing;
            pixelY = (spacePos.y() - imgPos[1]) / dicom.ySpacing;
        }break;
        case AimLibDefine::E_CORONAL: {
            pixelX = (spacePos.x() - imgPos[0]) / dicom.xSpacing;
            pixelY = dicom.height - 1 - (spacePos.z() - imgPos[2]) / dicom.zSpacing;
        }break;
        case AimLibDefine::E_SAGITTAL: {
            pixelX = (spacePos.y() - imgPos[1]) / dicom.ySpacing;
            pixelY = dicom.height - 1 - (spacePos.z() - imgPos[2]) / dicom.zSpacing;
        }break;
        default:
            break;
        }
        
        // Map to physical coordinates (considering spacing)
        float physX = pixelX * (m_type == AimLibDefine::E_SAGITTAL ? dicom.ySpacing : dicom.xSpacing);
        float physY = pixelY * (m_type == AimLibDefine::E_AXIAL ? dicom.ySpacing : dicom.zSpacing);
        
        // Offset to match the image rect
        physX -= physicalWidth / 2.0;
        physY -= physicalHeight / 2.0;
        
        // Draw crosshair lines
        int linelength = 15 / m_scale;  // Adjust line length for scale
        QPen pen2(QColor("#FFFFFF"));
        pen2.setWidth(1.0 / m_scale);
        paint.setPen(pen2);
        paint.drawLine(QPointF(physX - linelength, physY), QPointF(physX + linelength, physY));
        paint.drawLine(QPointF(physX, physY - linelength), QPointF(physX, physY + linelength));
        
        // Draw center point
        QPen pen(Qt::red);
        pen.setWidth(5 / m_scale);  // Adjust pen width for scale
        paint.setPen(pen);
        paint.drawPoint(QPointF(physX, physY));
    }
    
    paint.restore();
    
    // Draw edge indicators (lines that stay at window edges)
    // These lines follow the crosshair but stay at the edges
    // Need to get dicom info again after restore
    auto dicom2 = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->dicoms[m_type];
    QPointF center2 = m_glRect.center();
    
    std::for_each(m_drawInfos.begin(), m_drawInfos.end(), [&](DrawInfo* info) {
        switch (info->type) {
        case DrawType::DT_Line: {
            // Get the line endpoints
            QVector3D p1 = info->info->line.p1;
            QVector3D p2 = info->info->line.p2;
            std::vector<double> imgPos = ImgVmFactory::stGetInstance()->getCtInfo()->pCtInfo->pSliceInfo->imgPosition;
            
            // Convert space coordinates to pixel coordinates for the crosshair position
            float crosshairPixelX = 0, crosshairPixelY = 0;
            QVector3D crosshairPos = curcenter;  // Use the current crosshair position
            
            switch (m_type) {
            case AimLibDefine::E_AXIAL:
                crosshairPixelX = (crosshairPos.x() - imgPos[0]) / dicom2.xSpacing;
                crosshairPixelY = (crosshairPos.y() - imgPos[1]) / dicom2.ySpacing;
                break;
            case AimLibDefine::E_CORONAL:
                crosshairPixelX = (crosshairPos.x() - imgPos[0]) / dicom2.xSpacing;
                crosshairPixelY = dicom2.height - 1 - (crosshairPos.z() - imgPos[2]) / dicom2.zSpacing;
                break;
            case AimLibDefine::E_SAGITTAL:
                crosshairPixelX = (crosshairPos.y() - imgPos[1]) / dicom2.ySpacing;
                crosshairPixelY = dicom2.height - 1 - (crosshairPos.z() - imgPos[2]) / dicom2.zSpacing;
                break;
            }
            
            // Convert to physical coordinates
            float physicalWidth2, physicalHeight2;
            switch (m_type) {
            case AimLibDefine::ViewNameEnum::E_AXIAL:
                physicalWidth2 = dicom2.width * dicom2.xSpacing;
                physicalHeight2 = dicom2.height * dicom2.ySpacing;
                break;
            case AimLibDefine::ViewNameEnum::E_CORONAL:
                physicalWidth2 = dicom2.width * dicom2.xSpacing;
                physicalHeight2 = dicom2.height * dicom2.zSpacing;
                break;
            case AimLibDefine::ViewNameEnum::E_SAGITTAL:
                physicalWidth2 = dicom2.width * dicom2.ySpacing;
                physicalHeight2 = dicom2.height * dicom2.zSpacing;
                break;
            default:
                physicalWidth2 = dicom2.width;
                physicalHeight2 = dicom2.height;
                break;
            }
            
            float physX = crosshairPixelX * (m_type == AimLibDefine::E_SAGITTAL ? dicom2.ySpacing : dicom2.xSpacing);
            float physY = crosshairPixelY * (m_type == AimLibDefine::E_AXIAL ? dicom2.ySpacing : dicom2.zSpacing);
            physX -= physicalWidth2 / 2.0;
            physY -= physicalHeight2 / 2.0;
            
            // Apply transform to get screen position
            QPointF screenPos;
            screenPos.setX(center2.x() + (physX + m_translate.x()) * m_scale);
            screenPos.setY(center2.y() + (physY + m_translate.y()) * m_scale);
            
            // Determine if this is a vertical or horizontal line
            bool isVertical = (abs(p1.x() - p2.x()) < 0.001 && abs(p1.y() - p2.y()) > 0.001) ||
                            (abs(p1.x() - p2.x()) < 0.001 && abs(p1.z() - p2.z()) > 0.001) ||
                            (abs(p1.y() - p2.y()) < 0.001 && abs(p1.z() - p2.z()) > 0.001);
            
            QPen pen(info->info->line.clr, info->info->line.width);
            if (info->info->line.bDash) {
                QVector<qreal>dashes;
                qreal space = 4;
                dashes << 3 << space;
                pen.setDashPattern(dashes);
            }
            paint.setPen(pen);
            
            if (isVertical) {
                // Vertical line (purple) - stays at left/right edges, follows Y
                paint.drawLine(QPointF(m_glRect.left(), screenPos.y()), 
                             QPointF(m_glRect.left() + 10, screenPos.y()));  // Left edge
                paint.drawLine(QPointF(m_glRect.right() - 10, screenPos.y()), 
                             QPointF(m_glRect.right(), screenPos.y()));  // Right edge
            } else {
                // Horizontal line (green) - stays at top/bottom edges, follows X
                paint.drawLine(QPointF(screenPos.x(), m_glRect.top()), 
                             QPointF(screenPos.x(), m_glRect.top() + 10));  // Top edge
                paint.drawLine(QPointF(screenPos.x(), m_glRect.bottom() - 10), 
                             QPointF(screenPos.x(), m_glRect.bottom()));  // Bottom edge
            }
        }
        break;
        case DrawType::DT_Point:
            // Draw points normally (outside transform)
            drawPoint(&paint, info->info->point);
            break;
        case DrawType::DT_Polygon:
            // Draw polygons normally (outside transform)
            drawPolygon(&paint, info->info->polygon);
            break;
        case DrawType::DT_Curve:
            // Draw curves normally (outside transform)
            drawCurve(&paint, info->info->curveInfo);
            break;
        }
    });

    // ���ɸ�������
    /*if (drawHighLightRegion())
    {
        paint.setOpacity(m_highLightOpacity);
        paint.drawImage(m_imgRct, m_curMask);
    }*/

    // Drawing of lines, points, polygons, curves has been moved inside transform above
    //��������
    for (auto item : m_drawText)
    {
        drawText(&paint, item);
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
        m_needResample = true;  // Need to resample when these values change
        update();
    }
}

void GlWdgImpl::mouseReleaseEvent(QMouseEvent* event) {
    // No need to transform the position anymore
    // pointFromGlToSpace will handle all transformations
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
    // ���ϽǵĽ�������
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

    //// ���Ƶ�ǰ��Ƭ���
    //QString indexStr = QString("%1/%2").arg(m_curIndex).arg(totalSize);
    //Utils::drawTextEx(painter, indexStr, indexStrRect, QColor("#C7C9D0"), 18, QFont::Bold, QTextOption(Qt::AlignLeft | Qt::AlignVCenter));

    //// ���Ƶ�ǰ��ʶ��
    //drawLine(painter, lineX);
    //drawLine(painter, lineY);

    //// ���Ƶ�ǰָʾ����
    //Utils::drawTextEx(painter, axesX, QRect(lineX.p2.x() < 30 ? lineX.p2.x() : lineX.p2.x() - 30, lineX.p2.y() < 30 ? lineX.p2.y() : lineX.p2.y() - 30, 30, 30),
    //	QColor("#FFBF00"), 18, QFont::Bold, QTextOption(Qt::AlignCenter));
    //Utils::drawTextEx(painter, axesY, QRect(lineY.p1.x() < 30 ? lineY.p1.x() : lineY.p1.x() - 30, lineY.p1.y() < 30 ? lineY.p1.y() : lineY.p1.y() - 30, 30, 30),
    //	QColor("#FFBF00"), 18, QFont::Bold, QTextOption(Qt::AlignCenter));
}

bool GlWdgImpl::drawHighLightRegion()
{
    // ���ɸ�������
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

    // ��չ�㼯ʹ���߱պ�
    QVector<QPointF> extendedPoints;
    extendedPoints.reserve(points.size() + 3);
    extendedPoints << points.last(); // �������һ������Ϊ��ʼ
    extendedPoints << points;        // ����ԭʼ�㼯
    extendedPoints << points.first() << points[1]; // ��������������Ϊ��β

    // �ƶ�����һ����
    path.moveTo(points.first());

    // ����ÿ������
    const int n = points.size();
    const double s = (1.0 - tension) / 6.0;

    for (int i = 0; i < n; ++i) {
        const QPointF& p0 = extendedPoints[i];
        const QPointF& p1 = extendedPoints[i + 1];
        const QPointF& p2 = extendedPoints[i + 2];
        const QPointF& p3 = extendedPoints[i + 3];

        // ���㱴�������Ƶ�
        QPointF cp1 = p1 + (p2 - p0) * s;
        QPointF cp2 = p2 - (p3 - p1) * s;

        // �������߶�
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
    // �Ƚ�p����painter�����Ż�ԭ
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
