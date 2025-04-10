#ifndef TWOSTEPFILTERDIALOG_H
#define TWOSTEPFILTERDIALOG_H

#include <QDialog>
#include <QVector>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <opencv2/opencv.hpp>

class TwoStepFilterDialog : public QDialog {
    Q_OBJECT
public:
    explicit TwoStepFilterDialog(QWidget *parent = nullptr);

    cv::Mat getKernel1();
    cv::Mat getKernel2();
    cv::Mat getKernel3();

private:
    QVector<QVector<QDoubleSpinBox*>> kernelInputs1;
    QVector<QVector<QDoubleSpinBox*>> kernelInputs2;
    QVector<QVector<QDoubleSpinBox*>> kernel5x5Labels;

    void initKernels(QGridLayout *grid1, QGridLayout *grid2, QGridLayout *grid5x5);
    QDoubleSpinBox* createSpinBox(QGridLayout *layout, int row, int col);
    cv::Mat extractKernel(const QVector<QVector<QDoubleSpinBox*>> &inputs);
    void updateKernel5x5();
};

#endif // TWOSTEPFILTERDIALOG_H
