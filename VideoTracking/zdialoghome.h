#ifndef ZDIALOGHOME_H
#define ZDIALOGHOME_H

#include <QDialog>
#include <zgblpara.h>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
class ZDialogHome : public QDialog
{
    Q_OBJECT
public:
    ZDialogHome(QWidget *parent=NULL);
    ~ZDialogHome();
private slots:
    void ZSlotS0Homing();
    void ZSlotS1Homing();
protected:
    QSize sizeHint() const;
private:
    QGroupBox *m_grp1;
    QToolButton *m_tbS0Homing;
    QToolButton *m_tbS1Homing;
    QHBoxLayout *m_hLayoutGrp1;

    QVBoxLayout *m_vLayMain;
};

#endif // ZDIALOGHOME_H
