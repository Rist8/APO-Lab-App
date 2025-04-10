#include "bitwiseoperationdialog.h"
#include "imageviewer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

BitwiseOperationDialog::BitwiseOperationDialog(QWidget *parent, const QVector<QWidget*>& openedImages)
    : QDialog(parent), resultImage() {
    setupUi();

    // Populate image list
    for (QWidget* widget : openedImages) {
        if (ImageViewer* viewer = qobject_cast<ImageViewer*>(widget)) {
            imageList->addItem(viewer->windowTitle());
        }
    }

    connect(operationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BitwiseOperationDialog::updateUi);
    connect(processButton, &QPushButton::clicked, this, &BitwiseOperationDialog::processOperation);

    updateUi();
}

void BitwiseOperationDialog::setupUi() {
    setWindowTitle("Bitwise Operations");
    setFixedSize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Operation selection
    QHBoxLayout *opLayout = new QHBoxLayout();
    opLayout->addWidget(new QLabel("Operation:"));
    operationCombo = new QComboBox(this);
    operationCombo->addItems({"Add", "Subtract", "Blend", "Bitwise AND", "Bitwise OR", "Bitwise XOR"});
    opLayout->addWidget(operationCombo);
    mainLayout->addLayout(opLayout);

    // Image selection
    mainLayout->addWidget(new QLabel("Select second image:"));
    imageList = new QListWidget(this);
    mainLayout->addWidget(imageList);

    // Alpha parameter (for blending)
    QHBoxLayout *alphaLayout = new QHBoxLayout();
    alphaLayout->addWidget(new QLabel("Alpha (0-100%):"));
    alphaSpin = new QSpinBox(this);
    alphaSpin->setRange(0, 100);
    alphaSpin->setValue(50);
    alphaLayout->addWidget(alphaSpin);
    mainLayout->addLayout(alphaLayout);

    // Process button
    processButton = new QPushButton("Process", this);
    mainLayout->addWidget(processButton);

    setLayout(mainLayout);
}

void BitwiseOperationDialog::updateUi() {
    bool showAlpha = (operationCombo->currentText() == "Blend");
    alphaSpin->setVisible(showAlpha);
}

void BitwiseOperationDialog::processOperation() {
    if (imageList->currentRow() == -1) {
        QMessageBox::warning(this, "Error", "Please select a second image");
        return;
    }

    performOperation();
    if (!resultImage.empty()) {
        accept();
    }
}

void BitwiseOperationDialog::performOperation() {
    ImageViewer* currentViewer = qobject_cast<ImageViewer*>(parent());
    ImageViewer* secondViewer = qobject_cast<ImageViewer*>(currentViewer->getMainWindow()->openedImages[imageList->currentRow()]);

    if (!currentViewer || !secondViewer) {
        QMessageBox::warning(this, "Error", "Invalid image selection");
        return;
    }

    cv::Mat img1 = currentViewer->getOriginalImage();
    cv::Mat img2 = secondViewer->getOriginalImage();

    // Handle different sizes
    if (img1.size() != img2.size()) {
        int response = QMessageBox::question(
            this,
            "Resize Image",
            "The images are of different sizes. Do you want to automatically resize the second image to match the first?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
            );

        if (response == QMessageBox::Yes) {
            cv::resize(img2, img2, img1.size());
        } else {
            QMessageBox::information(this, "Operation Cancelled", "Bitwise operation was cancelled due to size mismatch.");
            return;
        }
    }

    QString operation = operationCombo->currentText();

    try {
        if (operation == "Add") {
            cv::add(img1, img2, resultImage);
        }
        else if (operation == "Subtract") {
            cv::subtract(img1, img2, resultImage);
        }
        else if (operation == "Blend") {
            double alpha = getAlpha();
            cv::addWeighted(img1, alpha, img2, 1.0 - alpha, 0, resultImage);
        }
        else if (operation == "Bitwise AND") {
            cv::bitwise_and(img1, img2, resultImage);
        }
        else if (operation == "Bitwise OR") {
            cv::bitwise_or(img1, img2, resultImage);
        }
        else if (operation == "Bitwise XOR") {
            cv::bitwise_xor(img1, img2, resultImage);
        }
    }
    catch (const cv::Exception& e) {
        QMessageBox::warning(this, "Error", QString("Operation failed: %1").arg(e.what()));
        resultImage.release();
    }
}

