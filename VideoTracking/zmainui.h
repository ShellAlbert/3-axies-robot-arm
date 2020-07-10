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
    void ZSlotPDO(qint32 iSlave,qint32 iActPos,qint32 iTarPos,qint32 iActVel,qint32 iStatusWord);
    void ZSlotLog(bool bErrFlag,QString log);
public slots:
    void ZSlotCtrlLR();
    void ZSlotCtrlUD();
protected:
    void paintEvent(QPaintEvent *e);
    void closeEvent(QCloseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    QSize sizeHint() const;
private:
    //left part: Left Right direction servo motors.
    QToolButton *m_tbCtrlLR;

    QLabel *m_llActVelLR;
    QLineEdit *m_leActVelLR;

    QLabel *m_llActPosLR;
    QLineEdit *m_leActPosLR;

    QLabel *m_llTarPosLR;
    QLineEdit *m_leTarPosLR;

    QGridLayout *m_gLayoutLft;

    //right part: Up Down direction servo motors.
    QToolButton *m_tbCtrlUD;

    QLabel *m_llActVelUD;
    QLineEdit *m_leActVelUD;

    QLabel *m_llActPosUD;
    QLineEdit *m_leActPosUD;

    QLabel *m_llTarPosUD;
    QLineEdit *m_leTarPosUD;

    QGridLayout *m_gLayoutRht;

    //the main layout.
    QHBoxLayout *m_hLayoutMain;

    QTextEdit *m_teLog;
    QVBoxLayout *m_vLayoutMain;

private:
    QImage m_img;
};

#endif // ZMAINUI_H
