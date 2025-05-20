#ifndef TWOSTEPFILTERDIALOG_H
#define TWOSTEPFILTERDIALOG_H

#include "previewdialogbase.h"
#include <QDialog>
#include <QVector>
#include <QGridLayout>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <opencv2/opencv.hpp>
#include <qcheckbox.h>

class TwoStepFilterDialog : public PreviewDialogBase {
    Q_OBJECT
public:
    explicit TwoStepFilterDialog(QWidget *parent = nullptr);

    cv::Mat getKernel1();
    cv::Mat getKernel2();
    cv::Mat getKernel3();
    QCheckBox* getPreviewCheckBox() const { return previewCheckBox; }

signals:
    void previewRequested();

private slots:
    void loadKernel1FromFile();
    void loadKernel2FromFile();

private:
    QVector<QVector<QDoubleSpinBox*>> kernelInputs1;
    QVector<QVector<QDoubleSpinBox*>> kernelInputs2;
    QVector<QVector<QDoubleSpinBox*>> kernel5x5Labels;
    QCheckBox* previewCheckBox;

    void initKernels(QGridLayout *grid1, QGridLayout *grid2, QGridLayout *grid5x5);
    QDoubleSpinBox* createSpinBox(QGridLayout *layout, int row, int col);
    cv::Mat extractKernel(const QVector<QVector<QDoubleSpinBox*>> &inputs);
    void updateKernel5x5();
    void loadKernelFromFile(QVector<QVector<QDoubleSpinBox*>> &kernelInputs);
};

#endif // TWOSTEPFILTERDIALOG_H
