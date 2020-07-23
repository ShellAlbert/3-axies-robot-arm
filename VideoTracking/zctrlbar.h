#ifndef ZCTRLBAR_H
#define ZCTRLBAR_H

#include <QFrame>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFrame>
class ZCtrlBar : public QFrame
{
    Q_OBJECT
public:
    ZCtrlBar();
    ~ZCtrlBar();
private slots:
    void ZSlotMode();
    void ZSlotCalibrate();
signals:
    void ZSigHome();
    void ZSigModeChanged();
    void ZSigScan();
    void ZSigCalibrate();
private:
    QToolButton *m_tbHome;
    QToolButton *m_tbMode;
    QToolButton *m_tbData;
    QToolButton *m_tbScan;
    QToolButton *m_tbCalibrate;
    QHBoxLayout *m_hLay;
};

#endif // ZCTRLBAR_H
