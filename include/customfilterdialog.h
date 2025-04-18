#ifndef CUSTOMFILTERDIALOG_H
#define CUSTOMFILTERDIALOG_H

#include "previewdialogbase.h"
#include <QDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QSpinBox>
#include <QVector>
#include <opencv2/opencv.hpp>
#include <qcheckbox.h>

class CustomFilterDialog : public PreviewDialogBase {
    Q_OBJECT

public:
    explicit CustomFilterDialog(QWidget *parent = nullptr);
    cv::Mat getKernel();
    QCheckBox* getPreviewCheckBox() const { return previewCheckBox; }

private slots:
    void updateKernelSize();

signals:
    void previewRequested();

private:
    QComboBox *kernelSizeBox;
    QCheckBox* previewCheckBox;
    QGridLayout *kernelLayout;
    QVector<QVector<QDoubleSpinBox*>> kernelInputs;
};

#endif // CUSTOMFILTERDIALOG_H
