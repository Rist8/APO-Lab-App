#ifndef RANGESTRETCHINGDIALOG_H
#define RANGESTRETCHINGDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class RangeStretchingDialog : public QDialog {
    Q_OBJECT

public:
    explicit RangeStretchingDialog(QWidget *parent = nullptr);
    int getP1() const;
    int getP2() const;
    int getQ3() const;
    int getQ4() const;

private:
    QSpinBox *p1SpinBox, *p2SpinBox, *q3SpinBox, *q4SpinBox;
    QSlider *p1Slider, *p2Slider, *q3Slider, *q4Slider;
};

#endif // RANGESTRETCHINGDIALOG_H
