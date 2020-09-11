#ifndef ZMAINUI_H
#define ZMAINUI_H

#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QVector>
#include "zctrlbar.h"
#include "zdirectionbar.h"
#include "zdifffifo.h"
#include <QLabel>
#include <QTimer>
typedef struct{
    bool bErrFlag;
    QString log;
}ZVectorLog;
class ZMainUI : public QWidget
{
    Q_OBJECT

public:
    ZMainUI(ZDiffFIFO *fifoDIFF,QWidget *parent = 0);
    ~ZMainUI();

    bool ZDoInit();
public slots:
    void ZSlotUpdateImg(const QImage &img);
    void ZSlotPDO(int servoID,int statusWord,int velocity,int position);
    void ZSlotLog(bool bErrFlag,QString log);
    void ZSlotModeChanged();
    void ZSlotLocked(bool,QRect rect);
    void ZSlotInitBox(const QImage &img);
    void ZSlotDiffXY(int diffX,int diffY);
    void ZSlotDiffAvailable(int nums);
    void ZSlotMatAvailable(int nums);
public slots:
    void ZSlotMove2Left();
    void ZSlotMove2Right();
    void ZSlotMove2Up();
    void ZSlotMove2Down();
private slots:
    void ZSlotHome();
    void ZSlotScan();
    void ZSlotCalibrate();
    void ZSlotLostTimeout();
protected:
    void paintEvent(QPaintEvent *e);
    void closeEvent(QCloseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    QSize sizeHint() const;
    void resizeEvent(QResizeEvent *event);
private:
    void ZDrawRectangleIndicator(QPainter &p,QImage &img);
    void ZDrawCircleIndicator(QPainter &p,QImage &img);
    void ZDrawSplitGrid(QPainter &p,QImage &img);
    void ZDrawROIMask(QPainter &p,QImage &img);
    qint32 getFps();
private:
    //the top ctrl bar.
    ZCtrlBar *m_ctrlBar;
    QHBoxLayout *m_hLayCtrlBar;
    ZDirectionBar *m_dirBar;
    QHBoxLayout *m_hLayDirBar;
    QVBoxLayout *m_vLayMain;
private:
    QImage m_img;
private:
    //select ROI.
    bool m_bSelectROI;
    QPoint m_ptStart;
    QPoint m_ptEnd;

    bool m_bLocked;
    QRect m_rectLocked;
    QImage m_initImg;

    int m_diffX;
    int m_diffY;
private:
    qint32 m_statusWord[2];
    qint32 m_velocity[2];
    qint32 m_position[2];
private:
    QVector<ZVectorLog> m_vecLog;

private:
    qint32 m_iFrmCounter;
private:
    QTimer m_timerLost;
    qint32 m_iLostTimeout;
private:
    ZDiffFIFO *m_fifoDIFF;
    int m_diffAvailableNums;
    int m_matAvailableNums;
};

#endif // ZMAINUI_H
