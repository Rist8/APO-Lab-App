#include "bitwiseoperationdialog.h"
#include "imageviewer.h"        // Need full definition for getOriginalImage, windowTitle
#include "imageprocessing.h"   // Include for processing functions
#include "mainwindow.h"         // Need full definition for accessing openedImages (can't easily avoid this)

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QListWidgetItem>
#include <opencv2/opencv.hpp> // For cv::resize, cv::Exception

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Constructor: Sets up the dialog for bitwise operations.
// ==========================================================================
BitwiseOperationDialog::BitwiseOperationDialog(QWidget *parent, const QVector<QWidget*>& openedImages)
    : PreviewDialogBase(parent), resultImage() {

    setupUi(); // Create UI elements

    // Populate image list, excluding the parent viewer itself
    ImageViewer* parentViewer = qobject_cast<ImageViewer*>(parent);
    imageViewerMap.clear(); // Clear map before populating

    for (QWidget* widget : openedImages) {
        ImageViewer* viewer = qobject_cast<ImageViewer*>(widget);
        // Only add other valid viewers to the list
        if (viewer && viewer != parentViewer) {
            QString title = viewer->windowTitle();
            // Ensure unique titles if multiple duplicates exist? For now, assume titles are unique enough.
            QListWidgetItem* item = new QListWidgetItem(title, imageList);
            // item->setData(Qt::UserRole, QVariant::fromValue(viewer)); // Alternative: store pointer in item data
            imageList->addItem(item);
            imageViewerMap[title] = viewer; // Map title to viewer pointer
        }
    }

    connect(imageList, &QListWidget::currentItemChanged, this, &PreviewDialogBase::previewRequested);
    // Connect signals and slots
    connect(operationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BitwiseOperationDialog::updateUi);
    connect(operationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PreviewDialogBase::previewRequested);
    connect(this, &PreviewDialogBase::previewRequested, this, &BitwiseOperationDialog::processOperation);
    connect(processButton, &QPushButton::clicked, this, &BitwiseOperationDialog::processOperation);
    connect(processButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject); // Connect cancel button

    updateUi(); // Set initial UI state (e.g., hide alpha)
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Creates and arranges the UI elements for the dialog.
// ==========================================================================
void BitwiseOperationDialog::setupUi() {
    setWindowTitle("Bitwise Operations");
    setMinimumSize(350, 250);

    mainLayout = new QVBoxLayout(this); // Ensure mainLayout is accessible if needed later

    // Operation selection
    QHBoxLayout *opLayout = new QHBoxLayout();
    opLayout->addWidget(new QLabel("Operation:"));
    operationCombo = new QComboBox(this);
    operationCombo->addItems({"Add", "Subtract", "Blend", "Bitwise AND", "Bitwise OR", "Bitwise XOR"});
    opLayout->addWidget(operationCombo);
    mainLayout->addLayout(opLayout);

    // Image selection label and list
    mainLayout->addWidget(new QLabel("Select second image:"));
    imageList = new QListWidget(this);
    imageList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(imageList);

    // Alpha parameter layout (for blending)
    alphaLayout = new QHBoxLayout();
    alphaLabel = new QLabel("Alpha (0-100%):"); // Assign to member variable
    alphaSpin = new QSpinBox(this);
    alphaSpin->setRange(0, 100);
    alphaSpin->setValue(50);
    alphaSpin->setSuffix("%");
    connect(alphaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
    alphaLayout->addWidget(alphaLabel);
    alphaLayout->addWidget(alphaSpin);
    alphaLayout->addStretch();
    mainLayout->addLayout(alphaLayout);


    previewCheckBox = new QCheckBox("Preview");
    previewCheckBox->setChecked(false); // default off
    connect(previewCheckBox, &QCheckBox::checkStateChanged, this, &PreviewDialogBase::previewRequested);
    mainLayout->addWidget(previewCheckBox);


    // Buttons layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    processButton = new QPushButton("Process", this);
    cancelButton = new QPushButton("Cancel", this); // Assign to member variable
    buttonLayout->addWidget(processButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);


    setLayout(mainLayout);
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Updates the UI visibility based on the selected operation (e.g., shows alpha for Blend).
// ==========================================================================
void BitwiseOperationDialog::updateUi() {
    bool showAlpha = (operationCombo->currentText() == "Blend");
    // Use the member pointers to show/hide
    alphaLabel->setVisible(showAlpha);
    alphaSpin->setVisible(showAlpha);
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Slot called when the 'Process' button is clicked. Validates input and calls performOperation.
// ==========================================================================
void BitwiseOperationDialog::processOperation() {
    // Validate selection
    if (imageList->currentRow() == -1 || !imageList->currentItem()) {
        QMessageBox::warning(this, "Input Error", "Please select a second image from the list.");
        return;
    }

    // Get selected viewer using the map
    QString selectedTitle = imageList->currentItem()->text();
    ImageViewer* secondViewer = imageViewerMap.value(selectedTitle, nullptr);

    if (!secondViewer) {
        QMessageBox::critical(this, "Internal Error", "Could not find the selected image viewer. The window might have been closed.");
        // Optionally, refresh the list here if a window was closed?
        return;
    }

    // Get the source viewer (the parent of this dialog)
    ImageViewer* currentViewer = qobject_cast<ImageViewer*>(parent());
    if(!currentViewer) {
        QMessageBox::critical(this, "Internal Error", "Could not identify the source image viewer.");
        return;
    }

    // Perform the operation by calling the private helper
    performOperation(currentViewer, secondViewer); // Pass pointers

    // If performOperation failed, it should have shown a message, so we just stay in the dialog.
}

// ==========================================================================
// Group 5: Image Processing - Core Operations (Delegation)
// ==========================================================================
// Performs the selected bitwise/arithmetic operation using the ImageProcessing module.
// Definition now matches the declaration in the header.
// ==========================================================================
void BitwiseOperationDialog::performOperation(ImageViewer* currentViewer, ImageViewer* secondViewer) {
    resultImage.release(); // Clear previous result

    // Basic null checks (should have been caught earlier, but good practice)
    if (!currentViewer || !secondViewer) {
        QMessageBox::warning(this, "Error", "Invalid image viewer references passed to performOperation.");
        return;
    }

    cv::Mat img1 = currentViewer->getOriginalImage();
    cv::Mat img2 = secondViewer->getOriginalImage();

    if (img1.empty() || img2.empty()) {
        QMessageBox::warning(this, "Error", "One or both selected images are empty.");
        return;
    }

    // --- Handle different sizes or types ---
    if (img1.size() != img2.size() || img1.type() != img2.type()) {
        QString sizeMsg = QString("Image sizes or types differ:\nSource: %1x%2 (%3 channels)\nSecond: %4x%5 (%6 channels)\n\n")
        .arg(img1.cols).arg(img1.rows).arg(img1.channels())
            .arg(img2.cols).arg(img2.rows).arg(img2.channels());

        if (img1.type() != img2.type()) {
            QMessageBox::warning(this, "Type Mismatch", sizeMsg + "Image types must match for this operation.");
            return;
        }

        int response = QMessageBox::question(
            this, "Size Mismatch",
            sizeMsg + "Do you want to resize the second image to match the source image size?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (response == QMessageBox::Yes) {
            try {
                cv::resize(img2, img2, img1.size(), 0, 0, cv::INTER_LINEAR);
            } catch (const cv::Exception& e) {
                QMessageBox::critical(this, "Resize Error", QString("Failed to resize image: %1").arg(e.what()));
                return;
            }
        } else {
            QMessageBox::information(this, "Operation Cancelled", "Operation cancelled due to size mismatch.");
            return;
        }
    }
    // --- End size/type handling ---

    // --- Perform selected operation using ImageProcessing functions ---
    QString operation = operationCombo->currentText();
    try {
        if (operation == "Add") {
            resultImage = ImageProcessing::applyAddition(img1, img2);
        } else if (operation == "Subtract") {
            resultImage = ImageProcessing::applySubtraction(img1, img2);
        } else if (operation == "Blend") {
            double alpha = static_cast<double>(alphaSpin->value()) / 100.0;
            resultImage = ImageProcessing::applyBlending(img1, img2, alpha);
        } else if (operation == "Bitwise AND") {
            resultImage = ImageProcessing::applyBitwiseAnd(img1, img2);
        } else if (operation == "Bitwise OR") {
            resultImage = ImageProcessing::applyBitwiseOr(img1, img2);
        } else if (operation == "Bitwise XOR") {
            resultImage = ImageProcessing::applyBitwiseXor(img1, img2);
        } else {
            QMessageBox::warning(this, "Error", "Unknown operation selected.");
            return; // Don't proceed if operation is unknown
        }

        if (resultImage.empty() && operation != "") { // Check if result is empty after a known operation
            QMessageBox::warning(this, "Operation Failed", "The selected operation did not produce a result image. Check image compatibility.");
        }

    } catch (const cv::Exception& e) {
        QMessageBox::critical(this, "OpenCV Error", QString("Operation failed due to OpenCV error:\n%1").arg(e.what()));
        resultImage.release();
    } catch (...) {
        QMessageBox::critical(this, "Unknown Error", "An unexpected error occurred during the operation.");
        resultImage.release();
    }
}
