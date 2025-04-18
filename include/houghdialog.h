#ifndef HOUGHDIALOG_H
#define HOUGHDIALOG_H

#include "previewdialogbase.h"
#include <QDialog>
#include <qcheckbox.h>

class QDoubleSpinBox;
class QSpinBox;
class QPushButton;

class HoughDialog : public PreviewDialogBase {
    Q_OBJECT

public:
    HoughDialog(QWidget* parent = nullptr);
    double getRho() const;
    double getThetaDegrees() const;
    int getThreshold() const;

    QCheckBox* getPreviewCheckBox() const { return previewCheckBox; }

signals:
    void previewRequested();

private:
    QDoubleSpinBox* rhoSpin;
    QDoubleSpinBox* thetaSpin;
    QSpinBox* thresholdSpin;
    QCheckBox* previewCheckBox;
};


#endif // HOUGHDIALOG_H
