#ifndef ZMAINUI_H
#define ZMAINUI_H

#include <QWidget>
#include <QToolButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
class ZMainUI : public QWidget
{
    Q_OBJECT

public:
    ZMainUI(QWidget *parent = 0);
    ~ZMainUI();

    bool ZDoInit();
private:
    //left part: two servo motors.
    //Left Right direction.
    QToolButton *m_tbCtrlLR;
    QLabel *m_llActVelLR;
    QLabel *m_llActPosLR;
    QLabel *m_llTarPosLR;
    //Up Down direction.
    QToolButton *m_tbCtrlUD;
    QLabel *m_llActVelUD;
    QLabel *m_llActPosUD;
    QLabel *m_llTarPosUD;

    QVBoxLayout *m_vLayoutLft;
    //right part: video.
    QVBoxLayout *m_vLayoutRht;
    QHBoxLayout *m_hLayoutMain;
};

#endif // ZMAINUI_H
