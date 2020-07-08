#ifndef ZMAINUI_H
#define ZMAINUI_H

#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "zethercatthread.h"
class ZMainUI : public QWidget
{
    Q_OBJECT

public:
    ZMainUI(QWidget *parent = 0);
    ~ZMainUI();

    bool ZDoInit();
public slots:
    void ZSlotCtrlLR();
    void ZSlotCtrlUD();
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

private:
    ZEtherCATThread *m_ecThread;
};

#endif // ZMAINUI_H
