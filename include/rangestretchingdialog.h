#ifndef RANGESTRETCHINGDIALOG_H
#define RANGESTRETCHINGDIALOG_H

#include "previewdialogbase.h"
#include <QDialog>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <qcheckbox.h>

class RangeStretchingDialog : public PreviewDialogBase {
    Q_OBJECT

public:
    explicit RangeStretchingDialog(QWidget *parent = nullptr);
    int getP1() const;
    int getP2() const;
    int getQ3() const;
    int getQ4() const;
    QCheckBox* getPreviewCheckBox() const { return previewCheckBox; }

signals:
    void previewRequested();

private:
    QSpinBox *p1SpinBox, *p2SpinBox, *q3SpinBox, *q4SpinBox;
    QSlider *p1Slider, *p2Slider, *q3Slider, *q4Slider;
    QCheckBox* previewCheckBox;
};

#endif // RANGESTRETCHINGDIALOG_H
