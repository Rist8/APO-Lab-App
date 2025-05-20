#include "twostepfilterdialog.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QVector>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>

TwoStepFilterDialog::TwoStepFilterDialog(QWidget *parent) : PreviewDialogBase(parent) {
    setWindowTitle("Two-Step Filter Input");
    setFixedSize(600, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Top layout with the two 3x3 kernels side-by-side
    QHBoxLayout *kernel3x3Layout = new QHBoxLayout();

    // Left 3x3 kernel layout
    QVBoxLayout *leftKernelLayout = new QVBoxLayout();
    QLabel *label1 = new QLabel("First 3x3 Kernel:");
    QGridLayout *gridLayout1 = new QGridLayout();
    leftKernelLayout->addWidget(label1);
    leftKernelLayout->addLayout(gridLayout1);

    // Right 3x3 kernel layout
    QVBoxLayout *rightKernelLayout = new QVBoxLayout();
    QLabel *label2 = new QLabel("Second 3x3 Kernel:");
    QGridLayout *gridLayout2 = new QGridLayout();
    rightKernelLayout->addWidget(label2);
    rightKernelLayout->addLayout(gridLayout2);

    kernel3x3Layout->addLayout(leftKernelLayout);
    kernel3x3Layout->addSpacing(30);  // Space between two kernels
    kernel3x3Layout->addLayout(rightKernelLayout);

    // Add the top 3x3 kernel layout to main
    mainLayout->addLayout(kernel3x3Layout);

    // 5x5 kernel layout below
    QLabel *label3 = new QLabel("Generated 5x5 Kernel:");
    QGridLayout *gridLayout5x5 = new QGridLayout();
    label3->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(label3);
    mainLayout->addLayout(gridLayout5x5);

    QPushButton *loadButton1 = new QPushButton("Load from File");
    connect(loadButton1, &QPushButton::clicked, this, &TwoStepFilterDialog::loadKernel1FromFile);
    leftKernelLayout->addWidget(loadButton1);

    QPushButton *loadButton2 = new QPushButton("Load from File");
    connect(loadButton2, &QPushButton::clicked, this, &TwoStepFilterDialog::loadKernel2FromFile);
    rightKernelLayout->addWidget(loadButton2);


    previewCheckBox = new QCheckBox("Preview");
    previewCheckBox->setChecked(false); // default off
    connect(previewCheckBox, &QCheckBox::checkStateChanged, this, &PreviewDialogBase::previewRequested);
    mainLayout->addWidget(previewCheckBox);

    // Buttons
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Initialize kernel widgets
    initKernels(gridLayout1, gridLayout2, gridLayout5x5);
}


cv::Mat TwoStepFilterDialog::getKernel1() { return extractKernel(kernelInputs1); }
cv::Mat TwoStepFilterDialog::getKernel2() { return extractKernel(kernelInputs2); }
cv::Mat TwoStepFilterDialog::getKernel3() { return extractKernel(kernel5x5Labels); }

void TwoStepFilterDialog::initKernels(QGridLayout *grid1, QGridLayout *grid2, QGridLayout *grid5x5) {
    kernelInputs1.resize(3);
    for (auto& row : kernelInputs1) {
        row.resize(3);
    }

    kernelInputs2.resize(3);
    for (auto& row : kernelInputs2) {
        row.resize(3);
    }

    kernel5x5Labels.resize(5);
    for (auto& row : kernel5x5Labels) {
        row.resize(5);
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            kernelInputs1[i][j] = createSpinBox(grid1, i, j);
            kernelInputs2[i][j] = createSpinBox(grid2, i, j);
        }
    }

    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
            spinBox->setRange(-99.99, 99.99);
            spinBox->setDecimals(2);  // or more if needed
            spinBox->setValue(0.0);
            spinBox->setFixedSize(85, 45);
            spinBox->setAlignment(Qt::AlignCenter);
            spinBox->setReadOnly(true);
            spinBox->setButtonSymbols(QDoubleSpinBox::NoButtons);
            spinBox->setFocusPolicy(Qt::NoFocus);  // Optional for cleaner UX


            kernel5x5Labels[i][j] = spinBox;
            grid5x5->addWidget(spinBox, i, j);
        }
    }

}

QDoubleSpinBox* TwoStepFilterDialog::createSpinBox(QGridLayout *layout, int row, int col) {
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
    spinBox->setRange(-99.99, 99.99);
    spinBox->setDecimals(2);  // or more if needed
    spinBox->setValue(0.0);
    spinBox->setFixedSize(85, 45);
    spinBox->setAlignment(Qt::AlignCenter);
    connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TwoStepFilterDialog::updateKernel5x5);
    connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
    layout->addWidget(spinBox, row, col);
    return spinBox;
}

cv::Mat TwoStepFilterDialog::extractKernel(const QVector<QVector<QDoubleSpinBox*>> &inputs) {
    cv::Mat kernel(3, 3, CV_32F);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            kernel.at<float>(i, j) = inputs[i][j]->value();
        }
    }
    //kernel /= cv::sum(kernel)[0];
    return kernel;
}

void TwoStepFilterDialog::updateKernel5x5() {
    cv::Mat kernel1 = getKernel1();
    cv::Mat kernel2 = getKernel2();
    cv::Mat combined5x5 = cv::Mat::zeros(5, 5, CV_32F);

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                for (int l = 0; l < 3; ++l) {
                    combined5x5.at<float>(i + k, j + l) += kernel1.at<float>(i, j) * kernel2.at<float>(k, l);
                }
            }
        }
    }

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            kernel5x5Labels[i][j]->setValue(combined5x5.at<float>(i, j));
        }
    }
}

void TwoStepFilterDialog::loadKernelFromFile(QVector<QVector<QDoubleSpinBox*>> &kernelInputs) {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Kernel File", "", "Text Files (*.txt);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "File Error", "Could not open file.");
        return;
    }

    QTextStream in(&file);
    QVector<float> values;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList tokens = line.split(' ', Qt::SkipEmptyParts);
        for (const QString &token : tokens) {
            bool ok;
            float val = token.toFloat(&ok);
            if (ok) values.append(val);
        }
    }

    if (values.size() != 9) {
        QMessageBox::warning(this, "Invalid Kernel", QString("Expected 9 values for a 3x3 kernel, but got %1.").arg(values.size()));
        return;
    }

    int index = 0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            kernelInputs[i][j]->setValue(values[index++]);
        }
    }

    updateKernel5x5();
    emit previewRequested();
}

void TwoStepFilterDialog::loadKernel1FromFile() {
    loadKernelFromFile(kernelInputs1);
}

void TwoStepFilterDialog::loadKernel2FromFile() {
    loadKernelFromFile(kernelInputs2);
}
