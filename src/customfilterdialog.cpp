#include "customfilterdialog.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QVector>

CustomFilterDialog::CustomFilterDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Custom Filter Input");
    setFixedSize(500, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Kernel size selection
    kernelSizeBox = new QComboBox(this);
    kernelSizeBox->addItems({"3x3", "5x5", "7x7", "9x9"});
    connect(kernelSizeBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &CustomFilterDialog::updateKernelSize);
    mainLayout->addWidget(kernelSizeBox);

    // Grid for kernel input
    kernelLayout = new QGridLayout();
    mainLayout->addLayout(kernelLayout);

    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    updateKernelSize();
}

void CustomFilterDialog::updateKernelSize() {
    // Clear existing widgets
    QLayoutItem *item;
    while ((item = kernelLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    int size = kernelSizeBox->currentText().split("x")[0].toInt();
    kernelInputs.clear();
    int spinBoxSize = (size > 5) ? 50 : 60; // Increase size for better visibility

    for (int i = 0; i < size; i++) {
        QVector<QSpinBox*> row;
        for (int j = 0; j < size; j++) {
            QSpinBox *spinBox = new QSpinBox(this);
            spinBox->setRange(-10, 10);
            spinBox->setValue(0);
            spinBox->setFixedSize(spinBoxSize, spinBoxSize);
            spinBox->setAlignment(Qt::AlignCenter); // Ensure text is centered
            kernelLayout->addWidget(spinBox, i, j);
            row.append(spinBox);
        }
        kernelInputs.append(row);
    }
}

cv::Mat CustomFilterDialog::getKernel() {
    int size = kernelSizeBox->currentText().split("x")[0].toInt();
    cv::Mat kernel(size, size, CV_32F);

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            kernel.at<float>(i, j) = kernelInputs[i][j]->value();
        }
    }
    return kernel;
}
