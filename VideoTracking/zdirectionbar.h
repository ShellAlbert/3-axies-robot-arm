#ifndef ZDIRECTIONBAR_H
#define ZDIRECTIONBAR_H

#include <QFrame>
#include <QToolButton>
#include <QGridLayout>
class ZDirectionBar : public QFrame
{
    Q_OBJECT
public:
    ZDirectionBar();
    ~ZDirectionBar();
signals:
    void ZSigLeft();
    void ZSigRight();
    void ZSigUp();
    void ZSigDown();
private slots:
    void ZSlotStepMode();
private:
    //left,right,up,down control.
    QToolButton *m_tbMoveLft;
    QToolButton *m_tbMoveRht;
    QToolButton *m_tbMoveUp;
    QToolButton *m_tbMoveDn;
    QToolButton *m_tbStepMode;
    QGridLayout *m_gLayoutMove;
};

#endif // ZDIRECTIONBAR_H
