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
protected:
    void paintEvent(QPaintEvent *e);
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    QSize sizeHint() const;
private:
    //left,right,up,down control.
    QToolButton *m_tbMoveLft;
    QToolButton *m_tbMoveRht;
    QToolButton *m_tbMoveUp;
    QToolButton *m_tbMoveDn;
    QGridLayout *m_gLayoutMove;

    //slave 0 Position Actual Value.
    QLabel *m_llS0PosActVal;
    QLineEdit *m_leS0PosActVal;
    //slave 0 Target Position.
    QLabel *m_llS0TarPos;
    QLineEdit *m_leS0TarPos;
    //slave 0 Actual Velocity.
    QLabel *m_llS0ActVel;
    QLineEdit *m_leS0ActVel;

    //slave 1 Position Actual Value.
    QLabel *m_llS1PosActVal;
    QLineEdit *m_leS1PosActVal;
    //slave 1 Target Position.
    QLabel *m_llS1TarPos;
    QLineEdit *m_leS1TarPos;
    //slave 1 Actual Velocity.
    QLabel *m_llS1ActVel;
    QLineEdit *m_leS1ActVel;

    QGridLayout *m_gLayLft;

    //the main layout.
    QHBoxLayout *m_hLayoutTop;

    //the main layout.
    QTextEdit *m_teLog;
    QVBoxLayout *m_vLayoutMain;
private:
    QImage m_img;
};

#endif // ZMAINUI_H
