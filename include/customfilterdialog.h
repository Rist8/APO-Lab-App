#ifndef CUSTOMFILTERDIALOG_H
#define CUSTOMFILTERDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QSpinBox>
#include <QVector>
#include <opencv2/opencv.hpp>

class CustomFilterDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomFilterDialog(QWidget *parent = nullptr);
    cv::Mat getKernel();

private slots:
    void updateKernelSize();

private:
    QComboBox *kernelSizeBox;
    QGridLayout *kernelLayout;
    QVector<QVector<QSpinBox*>> kernelInputs;
};

#endif // CUSTOMFILTERDIALOG_H
