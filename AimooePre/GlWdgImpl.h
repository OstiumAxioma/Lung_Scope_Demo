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

// ����Ϣ
struct PointInfo {
    QVector3D p;
    QColor clr;
    // �뾶
    float r = 0.0;
};

// �߶���Ϣ
struct LineInfo {
    QVector3D p1;
    QVector3D p2;
    float width = 1.0;
    QColor clr;
    // ���� 
    bool bDash = false;
};

// �������Ϣ
struct PolygonInfo {
    std::vector<QVector3D> ps;
    float width = 1.0;
    QColor penClr;
    QColor brushClr;
};

// ������Ϣ
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

// ��������
enum DrawType {
    DT_Point = 0,
    DT_Line,
    DT_Polygon,
    DT_Curve,
    DT_Max
};

// ������Ϣ
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

    virtual int fromSpaceGetIndex(QVector3D p);  //��ȡ������Ϣ

    virtual QRect getCurRect();

    /**
    * ��ʵ����/��ͼ�ı���
    */
    virtual float getUnitView();

    /**
    *������ʾ������λ��
    */
    virtual void showCenter(QVector3D point);

    /**
    * ��ά��ͼ����
    */
    virtual void setGlWidgetType(AimLibDefine::ViewNameEnum type);
    virtual AimLibDefine::ViewNameEnum getGlWidgetType();

    /**
    * ����ctӰ���������
    */
    virtual bool loadCtInfo(int index);

    /**
     * ���õ�ǰ���ڵĴ�����λ.
     */
    virtual bool setWindowWC(int windowWidth, int windowCenter);
    virtual bool getWindowWC(int& windowWidth, int& windowCenter);
    virtual bool getInitWindowWC(int& windowWidth, int& windowCenter);
    virtual bool setMinMaxHU(int minHu, int maxHU, bool isUsing);
    /**
     * ����ͼ���ں�͸����.
     */
    virtual void setFusionRate(double origRate, double fusionRate);

    /**
    * �����ڵ�������꣬ת��ΪӰ��ռ�����
    */
    virtual QVector3D pointFromGlToSpace(QPoint p);
    virtual QPoint pointFromSpaceToGl(QVector3D p);
    virtual int pointFromSpaceToImgIndex(QVector3D p);
    /**
    * ����
    */
    virtual void drawPoint(QVector3D point, QColor clr, float r);

    /**
    * ����
    */
    virtual void drawLine(QVector3D point1, QVector3D point2, QColor clr, float w, bool bDash = false);

    /**
    * �������
    */
    virtual void drawPolygon(std::vector<QVector3D> points, QColor penClr, QColor brushClr, float w);

    /**
    * ���������
    */
    virtual void drawCurve(std::vector<QVector3D> points, QColor penClr, QColor brushClr, float w, float arc);

    virtual bool isPointInCurve(const QVector3D& point);

    //д��
    virtual void drawText(QString txt, QVector3D pos, int width, int height, QColor clr, uint fontH, uint weight, QTextOption to);

    /**
    * ��ջ���
    */
    virtual void clear();

    /**
    * ˢ��
    */
    virtual void flash();

    /**
    * ���������ͼ
    */
    virtual void clearWdg();

    /**
    * ����
    */
    virtual void scale(double v);

    /**
     * �ƶ�.
     */
    virtual void translate(QPoint translate);

    /**
    * ���÷����������0-255.
    */
    virtual void setHighLightValue(int value, QColor color = Qt::green, double Opacity = 0.0);

    /**
     * .���÷����������0-255(�������)
     */
    virtual void setHighLightValues(QList<int> valueList, QColor color = Qt::green, double Opacity = 0.0);

    virtual bool updateEllipse(double radius);

    virtual void setInteractiveMode();

    virtual void setInteractiveMode(bool label);

    virtual bool getInteractiveMode();

    virtual bool updateEllipseCenter(QVector3D center);
    
private:
    // 新增：变换矩阵相关辅助函数
    void updateViewTransform();
    void initializeViewCenter();
    QRect calculateViewRect();
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
    void drawTag(QPainter* painter);     //������ͼ�ı�ǩ-����
    bool drawHighLightRegion();            //���ɸ�������
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
    //Ҫ��Ⱦ��ͼƬ
    int16* mpCurImgData = nullptr;
    int16* mpCurFusionImgData = nullptr;
    QRect m_imgRct;
    QRect m_rect;           //��ǰͼ������������gl���ڵ�λ��
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

    // ����
    double m_scale = 1.0;
    QPoint m_translate = QPoint(0, 0);
    float full_ratio = 1.0;
    
    // 新增：模拟相机的变换参数
    QPointF m_viewCenter = QPointF(0, 0);    // 视图中心（图像像素坐标）
    double m_zoomFactor = 1.0;               // 缩放因子
    QPointF m_panOffset = QPointF(0, 0);     // 平移偏移

    //ͼ����ߵ�λ����
    float m_unitView = 0.0;

    int m_highLightValue = 0;    // 0-255
    QList<int> m_highLightValueList;
    QColor m_highLightColor = Qt::green;
    QColor m_thresholdColor = Qt::blue;
    double m_highLightOpacity = 0.5;

    // ������λ
    double m_windowWidth = 0;
    double m_windowCenter = 0;

    // ��ǰ�Ƿ���Ҫ��ͼ���ں�
    bool m_bFusion = false;
    double m_needOrigRate = 0.5;
    double m_needFusionRate = 0.5;
    double m_origRate = 0.5;
    double m_fusionRate = 0.5;

    double curradius = 20;  //��������뾶
    double needradius = 20;

    bool InteractiveMode = false;  //����ģʽ

    QVector3D curcenter = QVector3D(0, 0, 0);  //������������
    QVector3D needcenter = QVector3D(0, 0, 0);

    QPainterPath m_curvePainterPath;
};

#endif // !__GL_WDG_IMPL_H__
