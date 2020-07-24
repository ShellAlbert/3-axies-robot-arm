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
#include <QLabel>
typedef struct{
    bool bErrFlag;
    QString log;
}ZVectorLog;
class ZMainUI : public QWidget
{
    Q_OBJECT

public:
    ZMainUI(QWidget *parent = 0);
    ~ZMainUI();

    bool ZDoInit();
public slots:
    void ZSlotUpdateImg(const QImage &img);
    void ZSlotPDO(qint32 iSlave,qint32 iActPos,qint32 iTarPos,qint32 iActVel);
    void ZSlotLog(bool bErrFlag,QString log);
    void ZSlotModeChanged();
    void ZSlotLocked(bool,QRect rect);
    void ZSlotInitBox(const QImage &img);
public slots:
    void ZSlotMoveToLeft();
    void ZSlotMoveToRight();
    void ZSlotMoveToUp();
    void ZSlotMoveToDown();
private slots:
    void ZSlotHome();
    void ZSlotScan();
    void ZSlotCalibrate();
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

    //slave 0 Position Actual Value.
    qint32 m_iS0PosActVal;
    //slave 0 Target Position.
    qint32 m_iS0TarPos;
    //slave 0 Actual Velocity.
    qint32 m_iS0ActVel;

    //slave 1 Position Actual Value.
    qint32 m_iS1PosActVal;
    //slave 1 Target Position.
    qint32 m_iS1TarPos;
    //slave 1 Actual Velocity.
    qint32 m_iS1ActVel;

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
private:
    QVector<ZVectorLog> m_vecLog;

private:
    qint32 m_iFrmCounter;
};

#endif // ZMAINUI_H
