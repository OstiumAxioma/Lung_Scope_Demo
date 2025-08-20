#ifndef __GL_WDG_IMPL_H__
#define __GL_WDG_IMPL_H__

#include <QWidget>
#include <vector>
#include <mutex>
#include <QImage>
#include <QColor>
#include <QTextOption>
#include <QString>
#include <QPainterPath>

#include "view/gl/base/GlWdgBase.h"
#include "component/register-time/RegisterTimer.h"

// 点信息
struct PointInfo {
    QVector3D p;
    QColor clr;
    // 半径
    float r = 0.0;
};

// 线段信息
struct LineInfo {
    QVector3D p1;
    QVector3D p2;
    float width = 1.0;
    QColor clr;
    // 虚线 
    bool bDash = false;
};

// 多边形信息
struct PolygonInfo {
    std::vector<QVector3D> ps;
    float width = 1.0;
    QColor penClr;
    QColor brushClr;
};

// 曲线信息
struct CurveInfo {
    std::vector<QVector3D> ps;
    float width = 1.0;
    float arc = 1.0;
    QColor penClr;
    QColor brushClr;
};

struct TextInfo {
    QVector3D pos;
    int width;
    int height;
    QString text;
    QColor color;
    uint fontH = 18;
    uint weight = QFont::Bold;
    QTextOption to = QTextOption(Qt::AlignHCenter | Qt::AlignVCenter);
};

// 绘制类型
enum DrawType {
    DT_Point = 0,
    DT_Line,
    DT_Polygon,
    DT_Curve,
    DT_Max
};

// 绘制信息
struct DrawInfo {
    DrawType type = DT_Max;
    union Info {
        PointInfo point;
        LineInfo line;
        PolygonInfo polygon;
        CurveInfo curveInfo;
    } *info;
};

class GlWdgImpl : public QWidget, public InterfaceGlWdg {
    Q_OBJECT

public:
    GlWdgImpl(QWidget* parent = nullptr);
    ~GlWdgImpl();

    std::vector<DrawInfo*> getDrawInfos();

    virtual int fromSpaceGetIndex(QVector3D p);  //获取索引信息

    virtual QRect getCurRect();

    /**
    * 真实距离/视图的比例
    */
    virtual float getUnitView();

    /**
    *将点显示在中心位置
    */
    virtual void showCenter(QVector3D point);

    /**
    * 二维视图类型
    */
    virtual void setGlWidgetType(AimLibDefine::ViewNameEnum type);
    virtual AimLibDefine::ViewNameEnum getGlWidgetType();

    /**
    * 设置ct影像数据序号
    */
    virtual bool loadCtInfo(int index);

    /**
     * 设置当前窗口的窗宽窗位.
     */
    virtual bool setWindowWC(int windowWidth, int windowCenter);
    virtual bool getWindowWC(int& windowWidth, int& windowCenter);
    virtual bool getInitWindowWC(int& windowWidth, int& windowCenter);
    virtual bool setMinMaxHU(int minHu, int maxHU, bool isUsing);
    /**
     * 设置图像融合透明度.
     */
    virtual void setFusionRate(double origRate, double fusionRate);

    /**
    * 将窗口点击的坐标，转换为影像空间坐标
    */
    virtual QVector3D pointFromGlToSpace(QPoint p);
    virtual QPoint pointFromSpaceToGl(QVector3D p);
    virtual int pointFromSpaceToImgIndex(QVector3D p);
    /**
    * 画点
    */
    virtual void drawPoint(QVector3D point, QColor clr, float r);

    /**
    * 画线
    */
    virtual void drawLine(QVector3D point1, QVector3D point2, QColor clr, float w, bool bDash = false);

    /**
    * 画多边形
    */
    virtual void drawPolygon(std::vector<QVector3D> points, QColor penClr, QColor brushClr, float w);

    /**
    * 画封闭曲线
    */
    virtual void drawCurve(std::vector<QVector3D> points, QColor penClr, QColor brushClr, float w, float arc);

    virtual bool isPointInCurve(const QVector3D& point);

    //写字
    virtual void drawText(QString txt, QVector3D pos, int width, int height, QColor clr, uint fontH, uint weight, QTextOption to);

    /**
    * 清空绘制
    */
    virtual void clear();

    /**
    * 刷新
    */
    virtual void flash();

    /**
    * 清空所有视图
    */
    virtual void clearWdg();

    /**
    * 缩放
    */
    virtual void scale(double v);

    /**
     * 移动.
     */
    virtual void translate(QPoint translate);

    /**
    * 设置分区高亮编号0-255.
    */
    virtual void setHighLightValue(int value, QColor color = Qt::green, double Opacity = 0.0);

    /**
     * .设置分区高亮编号0-255(多个分区)
     */
    virtual void setHighLightValues(QList<int> valueList, QColor color = Qt::green, double Opacity = 0.0);

    virtual bool updateEllipse(double radius);

    virtual void setInteractiveMode();

    virtual void setInteractiveMode(bool label);

    virtual bool getInteractiveMode();

    virtual bool updateEllipseCenter(QVector3D center);
private:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void timerEvent(QTimerEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void drawPoint(QPainter* painter, PointInfo& info);
    void drawLine(QPainter* painter, LineInfo& info);
    void drawText(QPainter* painter, TextInfo& info);
    void drawPolygon(QPainter* painter, PolygonInfo& info);
    void drawCurve(QPainter* painter, CurveInfo& info);
    void drawTag(QPainter* painter);     //绘制视图的标签-弃用
    bool drawHighLightRegion();            //生成高亮区域
    QPainterPath createClosedCurveWithTension(
        const QVector<QPointF>& points,
        double tension = 0.5,
        bool closePath = true
    );

    QPoint fromSpacegetAimgl(QVector3D p);
    QVector3D fromAimglgetSpace(QPoint p);

private slots:
    void handleLicense(RegisterTimer::ErrCode err);

private:
    AimLibDefine::ViewNameEnum m_type = AimLibDefine::ViewNameEnum::E_AXIAL;
    //要渲染的图片
    int16* mpCurImgData = nullptr;
    int16* mpCurFusionImgData = nullptr;
    QRect m_imgRct;
    QRect m_rect;           //当前图像区域在整个gl窗口的位置
    QRect m_glRect;
    QImage m_curQImg;
    QImage m_curFusionImg;
    QImage m_curMask;
    int16* m_pImgBuf = nullptr;
    int16* m_pLabelImgBuf = nullptr;
    int16* m_pFusionImgBuf = nullptr;
    int m_imgBufLen = 0;
    int m_fusionBufLen = 0;
    int m_LabelBufLen = 0;
    QVector3D m_curIndex3D = QVector3D(0, 0, 0);
    QVector3D m_needIndex3D = QVector3D(0, 0, 0);
    int m_curIndex = 0;
    int m_needIndex = 0;
    int m_curWindowCenter = 0;
    int m_needWindowCenter = 0;
    int m_curWindowWidth = 0;
    int m_needWindowWidth = 0;
    int m_timerId = 0;

    bool isUsingHURange = false;
    int m_curMinHU = 0;
    int m_needMinHU = 0;
    int m_curMaxHU = 0;
    int m_needMaxHU = 0;
    std::mutex m_infoMtx;
    std::vector<DrawInfo*> m_drawInfos;
    std::vector<TextInfo> m_drawText;

    std::mutex m_loadCtMtx;

    // 缩放
    double m_scale = 1.0;
    QPoint m_translate = QPoint(0, 0);
    float full_ratio = 1.0;

    //图像宽高单位长度
    float m_unitView = 0.0;

    int m_highLightValue = 0;    // 0-255
    QList<int> m_highLightValueList;
    QColor m_highLightColor = Qt::green;
    QColor m_thresholdColor = Qt::blue;
    double m_highLightOpacity = 0.5;

    // 窗宽窗位
    double m_windowWidth = 0;
    double m_windowCenter = 0;

    // 当前是否需要做图像融合
    bool m_bFusion = false;
    double m_needOrigRate = 0.5;
    double m_needFusionRate = 0.5;
    double m_origRate = 0.5;
    double m_fusionRate = 0.5;

    double curradius = 20;  //交互区域半径
    double needradius = 20;

    bool InteractiveMode = false;  //交互模式

    QVector3D curcenter = QVector3D(0, 0, 0);  //交互区域中心
    QVector3D needcenter = QVector3D(0, 0, 0);

    QPainterPath m_curvePainterPath;
};

#endif // !__GL_WDG_IMPL_H__
