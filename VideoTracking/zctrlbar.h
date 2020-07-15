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
    void ZSlotTrackBtn();
private:
    QToolButton *m_tbHome;
    QToolButton *m_tbTrack;
    QToolButton *m_tbData;
    QToolButton *m_tbScan;
    QHBoxLayout *m_hLay;
private:
    bool m_bTrackEn;
};

#endif // ZCTRLBAR_H
