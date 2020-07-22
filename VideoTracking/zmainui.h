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
    void mouseReleaseEvent(QMouseEvent *event);
    QSize sizeHint() const;
    void resizeEvent(QResizeEvent *event);
private:
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
    QPoint m_ptCenter;
    QPoint m_ptNew;

private:
    QVector<ZVectorLog> m_vecLog;

private:
    qint32 m_iFrmCounter;
private:

};

#endif // ZMAINUI_H
