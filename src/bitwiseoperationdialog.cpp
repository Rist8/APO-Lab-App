#include "bitwiseoperationdialog.h"
#include "imageviewer.h"
#include "imageprocessing.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <opencv2/opencv.hpp>

BitwiseOperationDialog::BitwiseOperationDialog(QWidget *parent, const QVector<QWidget*>& openedImages)
    : PreviewDialogBase(parent), resultImage() {

    setupUi(); // Create UI elements

    // Get parent viewer if any
    ImageViewer* parentViewer = qobject_cast<ImageViewer*>(parent);
    imageViewerMap.clear();

    // Populate combo boxes with grayscale image viewers
    for (QWidget* widget : openedImages) {
        ImageViewer* viewer = qobject_cast<ImageViewer*>(widget);
        if (!viewer || viewer->getOriginalImage().channels() != 1) continue;

        QString title = viewer->windowTitle();
        imageViewerMap[title] = viewer;

        // Always populate second image combo
        if (!parentViewer || viewer != parentViewer) {
            imageCombo2->addItem(title);
        }

        imageCombo1->addItem(title);
    }

    // Pre-select and lock first combo to parent if applicable
    if (parentViewer) {
        QString parentTitle = parentViewer->windowTitle();
        int index = imageCombo1->findText(parentTitle);
        if (index >= 0) imageCombo1->setCurrentIndex(index);
        imageCombo1->setEnabled(false); // Lock first image to parent
    }

    // Connections
    connect(operationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BitwiseOperationDialog::updateUi);
    connect(imageCombo1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewDialogBase::previewRequested);
    connect(imageCombo2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewDialogBase::previewRequested);
    connect(operationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewDialogBase::previewRequested);
    connect(this, &PreviewDialogBase::previewRequested, this, &BitwiseOperationDialog::processOperation);
    connect(processButton, &QPushButton::clicked, this, &BitwiseOperationDialog::processOperation);
    connect(processButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    updateUi(); // Initial alpha visibility
}

void BitwiseOperationDialog::setupUi() {
    setWindowTitle("Bitwise Operations");
    setMinimumSize(350, 250);
    mainLayout = new QVBoxLayout(this);

    // Image selection layout: [combo1] [operationCombo] [combo2]
    QHBoxLayout* selectionLayout = new QHBoxLayout();
    imageCombo1 = new QComboBox(this);
    operationCombo = new QComboBox(this);
    imageCombo2 = new QComboBox(this);

    operationCombo->addItems({"Add", "Subtract", "Blend", "Bitwise AND", "Bitwise OR", "Bitwise XOR"});

    selectionLayout->addWidget(imageCombo1);
    selectionLayout->addWidget(operationCombo);
    selectionLayout->addWidget(imageCombo2);
    mainLayout->addLayout(selectionLayout);

    // Alpha layout (for blending)
    alphaLayout = new QHBoxLayout();
    alphaLabel = new QLabel("Alpha (0-100%):");
    alphaSpin = new QSpinBox(this);
    alphaSpin->setRange(0, 100);
    alphaSpin->setValue(50);
    alphaSpin->setSuffix("%");
    connect(alphaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
    alphaLayout->addWidget(alphaLabel);
    alphaLayout->addWidget(alphaSpin);
    alphaLayout->addStretch();
    mainLayout->addLayout(alphaLayout);

    // Preview checkbox
    if(parent()){
        previewCheckBox = new QCheckBox("Preview");
        previewCheckBox->setChecked(false);
        connect(previewCheckBox, &QCheckBox::checkStateChanged, this, &PreviewDialogBase::previewRequested);
        mainLayout->addWidget(previewCheckBox);
    }

    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    processButton = new QPushButton("Process", this);
    cancelButton = new QPushButton("Cancel", this);
    buttonLayout->addWidget(processButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void BitwiseOperationDialog::updateUi() {
    bool showAlpha = (operationCombo->currentText() == "Blend");
    alphaLabel->setVisible(showAlpha);
    alphaSpin->setVisible(showAlpha);
}

void BitwiseOperationDialog::processOperation() {
    QString title1 = imageCombo1->currentText();
    QString title2 = imageCombo2->currentText();

    if (title1.isEmpty() || title2.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please select both images.");
        return;
    }

    if (title1 == title2) {
        QMessageBox::warning(this, "Input Error", "Please select two *different* images.");
        return;
    }

    ImageViewer* viewer1 = imageViewerMap.value(title1, nullptr);
    ImageViewer* viewer2 = imageViewerMap.value(title2, nullptr);

    if (!viewer1 || !viewer2) {
        QMessageBox::critical(this, "Internal Error", "Could not find selected image viewers.");
        return;
    }

    performOperation(viewer1, viewer2);
}

void BitwiseOperationDialog::performOperation(ImageViewer* currentViewer, ImageViewer* secondViewer) {
    resultImage.release();
    if (!currentViewer || !secondViewer) {
        QMessageBox::warning(this, "Error", "Invalid image viewer references.");
        return;
    }

    cv::Mat img1 = currentViewer->getOriginalImage();
    cv::Mat img2 = secondViewer->getOriginalImage();

    if (img1.empty() || img2.empty()) {
        QMessageBox::warning(this, "Error", "One or both selected images are empty.");
        return;
    }

    // Resize or error if dimensions/types differ
    if (img1.size() != img2.size() || img1.type() != img2.type()) {
        QString sizeMsg = QString("Image sizes/types differ:\nFirst: %1x%2 (%3 channels)\nSecond: %4x%5 (%6 channels)\n\n")
        .arg(img1.cols).arg(img1.rows).arg(img1.channels())
            .arg(img2.cols).arg(img2.rows).arg(img2.channels());

        if (img1.type() != img2.type()) {
            QMessageBox::warning(this, "Type Mismatch", sizeMsg + "Image types must match.");
            return;
        }

        int answer = QMessageBox::question(this, "Size Mismatch", sizeMsg + "Resize second image to match?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            try {
                cv::resize(img2, img2, img1.size(), 0, 0, cv::INTER_LINEAR);
            } catch (const cv::Exception& e) {
                QMessageBox::critical(this, "Resize Error", e.what());
                return;
            }
        } else {
            return;
        }
    }

    // Perform operation
    QString op = operationCombo->currentText();
    try {
        if (op == "Add") resultImage = ImageProcessing::applyAddition(img1, img2);
        else if (op == "Subtract") resultImage = ImageProcessing::applySubtraction(img1, img2);
        else if (op == "Blend") {
            double alpha = static_cast<double>(alphaSpin->value()) / 100.0;
            resultImage = ImageProcessing::applyBlending(img1, img2, alpha);
        }
        else if (op == "Bitwise AND") resultImage = ImageProcessing::applyBitwiseAnd(img1, img2);
        else if (op == "Bitwise OR") resultImage = ImageProcessing::applyBitwiseOr(img1, img2);
        else if (op == "Bitwise XOR") resultImage = ImageProcessing::applyBitwiseXor(img1, img2);
        else {
            QMessageBox::warning(this, "Error", "Unknown operation.");
            return;
        }

        if (resultImage.empty()) {
            QMessageBox::warning(this, "Operation Failed", "Operation did not produce a valid result.");
        }
    } catch (const cv::Exception& e) {
        QMessageBox::critical(this, "OpenCV Error", e.what());
    } catch (...) {
        QMessageBox::critical(this, "Unknown Error", "Unexpected error during operation.");
    }
}
