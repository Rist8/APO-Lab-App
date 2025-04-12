#ifndef HOUGHDIALOG_H
#define HOUGHDIALOG_H

#include <QDialog>

class QDoubleSpinBox;
class QSpinBox;
class QPushButton;

class HoughDialog : public QDialog {
    Q_OBJECT

public:
    HoughDialog(QWidget* parent = nullptr);
    double getRho() const;
    double getThetaDegrees() const;
    int getThreshold() const;

private:
    QDoubleSpinBox* rhoSpin;
    QDoubleSpinBox* thetaSpin;
    QSpinBox* thresholdSpin;
};


#endif // HOUGHDIALOG_H
