#include "customfilterdialog.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QVector>
#include <qdir.h>

CustomFilterDialog::CustomFilterDialog(QWidget *parent) : PreviewDialogBase(parent) {
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

    QPushButton *loadButton = new QPushButton("Load from File");
    connect(loadButton, &QPushButton::clicked, this, &CustomFilterDialog::loadKernelFromFile);
    mainLayout->addWidget(loadButton);


    previewCheckBox = new QCheckBox("Preview");
    previewCheckBox->setChecked(false); // default off
    connect(previewCheckBox, &QCheckBox::checkStateChanged, this, &PreviewDialogBase::previewRequested);
    mainLayout->addWidget(previewCheckBox);

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
    int spinBoxSize = (size > 5) ? 75 : 85; // Increase size for better visibility

    for (int i = 0; i < size; i++) {
        QVector<QDoubleSpinBox*> row;
        for (int j = 0; j < size; j++) {
            QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
            spinBox->setRange(-99.99, 99.99);
            spinBox->setDecimals(2);  // or more if needed
            spinBox->setValue(0);
            spinBox->setFixedSize(spinBoxSize, 45);
            spinBox->setAlignment(Qt::AlignCenter); // Ensure text is centered
            connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
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

void CustomFilterDialog::loadKernelFromFile() {
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

    int size = kernelSizeBox->currentText().split("x")[0].toInt();
    if (values.size() != size * size) {
        QMessageBox::warning(this, "Size Mismatch", QString("Expected %1 values for a %2x%2 kernel, but got %3.")
                                 .arg(size * size).arg(size).arg(values.size()));
        return;
    }

    int index = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            kernelInputs[i][j]->setValue(values[index++]);
        }
    }

    emit previewRequested();
}

