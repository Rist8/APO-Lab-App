#include "inpaintingdialog.h"
#include "imageviewer.h"
// #include "imageprocessing.h" // Include if you have prepareMask or similar helpers
#include "mainwindow.h" // Include your MainWindow header if needed for context casting

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QListWidgetItem>
#include <QGroupBox>
#include <QDialogButtonBox> // Can be used for standard buttons, but manual layout is fine
#include <QPoint> // For positioning new window

#include <opencv2/imgproc.hpp> // For cvtColor, threshold
#include <qtimer.h>

/**
 * @brief Constructor implementation.
 */
InpaintingDialog::InpaintingDialog(ImageViewer *parent, const QVector<QWidget*>& openedViewers, QWidget* mainWindowCtx)
    : QDialog(parent), // Set parent QWidget for modality etc.
    parentViewer(parent), // Store the specific ImageViewer pointer
    mainWindowContext(mainWindowCtx),
    openedImages(openedViewers)
{
    // Ensure parentViewer is valid (important for drawing interaction)
    if (!parentViewer) {
        // This is a critical error, the dialog cannot function correctly.
        // Show message and potentially close immediately or disable controls.
        QMessageBox::critical(this, "Initialization Error",
                              "InpaintingDialog requires a valid parent ImageViewer instance to function.");
        QTimer::singleShot(0, this, &InpaintingDialog::reject); // Close after constructor finishes
    }

    setupUi(); // Create UI elements

    // Populate image list (excluding the parent viewer itself)
    refreshImageList();


    // --- Connections ---
    connect(imageList, &QListWidget::currentItemChanged, this, [=](){
        if(imageList->currentItem())
            emit maskChanged();
    });

    connect(maskSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](){
        if(maskSourceCombo->currentText() == "Use another image")
            drawnMask = parentViewer->getDrawnMask().clone();
        else
            emit maskChanged();
        updateUi();
    });

    // Connect drawing controls (only if parentViewer is valid)
    if (parentViewer) {
        connect(brushThicknessSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &InpaintingDialog::onBrushThicknessChanged);
        // Connect clear button directly to the viewer's slot
        connect(clearMaskButton, &QPushButton::clicked, parentViewer, &ImageViewer::clearDrawnMask);
        connect(openMaskButton, &QPushButton::clicked, this, &InpaintingDialog::openMaskInNewViewer);
    } else {
        // If parentViewer was invalid, disable the drawing group explicitly
        if(drawingControlsGroup) drawingControlsGroup->setEnabled(false);
    }

    // Connect main action buttons to custom slots for pre-accept/reject actions
    connect(processButton, &QPushButton::clicked, this, &InpaintingDialog::handleAccepted);
    connect(cancelButton, &QPushButton::clicked, this, &InpaintingDialog::handleRejected);

    // --- Initial State ---
    setWindowTitle("Inpainting Options");
    setMinimumSize(400, 350); // Adjusted minimum size
    updateUi(); // Set initial visibility and enable/disable drawing mode

    // Set initial values in the viewer if starting in draw mode AND parentViewer is valid
    if (parentViewer && maskSourceCombo->currentText() == "Draw on image") {
        parentViewer->setBrushThickness(brushThicknessSpin->value());
    }
}

void InpaintingDialog::refreshImageList() {
    imageList->clear();
    imageViewerMap.clear();

    for (QWidget* widget : openedImages) {
        ImageViewer* viewer = qobject_cast<ImageViewer*>(widget);
        if (viewer && viewer != parentViewer && !viewer->getOriginalImage().empty()) {
            QString title = viewer->windowTitle();
            if (!title.isEmpty()) {
                QListWidgetItem* item = new QListWidgetItem(title, imageList);
                imageList->addItem(item);
                imageViewerMap[title] = viewer;

                // Handle removal when viewer is destroyed
                connect(viewer, &QObject::destroyed, this, [this, title]() {
                    imageViewerMap.remove(title);
                    for (int i = 0; i < imageList->count(); ++i) {
                        if (imageList->item(i)->text() == title) {
                            delete imageList->takeItem(i);
                            break;
                        }
                    }
                });
            }
        }
    }

    if (imageList->count() > 0) {
        imageList->setCurrentRow(0);
    }
}

/**
 * @brief Sets up the UI elements and layout.
 */
void InpaintingDialog::setupUi() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10); // Add some spacing

    // 1. Mask Source Selection
    QHBoxLayout* sourceLayout = new QHBoxLayout();
    sourceLayout->addWidget(new QLabel("Mask source:", this));
    maskSourceCombo = new QComboBox(this);
    maskSourceCombo->addItems({"Draw on image", "Use another image"});
    maskSourceCombo->setToolTip("Choose whether to draw the mask directly or use another open image.");
    sourceLayout->addWidget(maskSourceCombo);
    mainLayout->addLayout(sourceLayout);

    // 2. Image List (for "Use another image" mode)
    imageList = new QListWidget(this);
    imageList->setSelectionMode(QAbstractItemView::SingleSelection);
    imageList->setToolTip("Select an image to use as the mask (white/non-zero areas will be inpainted).");
    mainLayout->addWidget(imageList); // Visibility managed by updateUi

    // 3. Drawing Controls Group (for "Draw on image" mode)
    drawingControlsGroup = new QGroupBox("Drawing Options", this);
    QVBoxLayout* drawingLayout = new QVBoxLayout(drawingControlsGroup);
    drawingLayout->setSpacing(8);

    // Brush Thickness
    QHBoxLayout* thicknessLayout = new QHBoxLayout();
    thicknessLayout->addWidget(new QLabel("Brush Thickness:", drawingControlsGroup));
    brushThicknessSpin = new QSpinBox(drawingControlsGroup);
    brushThicknessSpin->setRange(1, 100); // Min 1, Max 100 pixels
    brushThicknessSpin->setValue(10);     // Default thickness
    brushThicknessSpin->setSuffix(" px");
    brushThicknessSpin->setToolTip("Set the diameter of the drawing brush.");
    thicknessLayout->addWidget(brushThicknessSpin);
    thicknessLayout->addStretch();
    drawingLayout->addLayout(thicknessLayout);

    // Buttons within the drawing group
    QHBoxLayout* drawingButtonsLayout = new QHBoxLayout();
    clearMaskButton = new QPushButton("Clear Drawn Mask", drawingControlsGroup);
    clearMaskButton->setToolTip("Erase the mask currently being drawn.");
    openMaskButton = new QPushButton("Open Drawn Mask...", drawingControlsGroup);
    openMaskButton->setToolTip("Open the current drawn mask as a new image window.");
    drawingButtonsLayout->addWidget(clearMaskButton);
    drawingButtonsLayout->addStretch();
    drawingButtonsLayout->addWidget(openMaskButton);
    drawingLayout->addLayout(drawingButtonsLayout);


    drawingControlsGroup->setLayout(drawingLayout);
    mainLayout->addWidget(drawingControlsGroup); // Visibility managed by updateUi

    // 4. Action Buttons (using manual layout)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // Pushes buttons to the right
    processButton = new QPushButton("Apply Inpaint", this); // Renamed for clarity
    cancelButton = new QPushButton("Cancel", this);
    processButton->setDefault(true); // Make "Apply" the default button (Enter key)
    buttonLayout->addWidget(processButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

/**
 * @brief Updates UI element visibility and parent viewer state based on mask source selection.
 */
void InpaintingDialog::updateUi() {
    bool drawMode = (maskSourceCombo->currentText() == "Draw on image");

    imageList->setVisible(!drawMode);
    drawingControlsGroup->setVisible(drawMode);

    // Enable/disable drawing mode in the parent viewer if it's valid
    if (parentViewer) {
        if (drawMode) {
            // Set parameters *before* enabling drawing, ensuring viewer uses current dialog values
            onBrushThicknessChanged(brushThicknessSpin->value());
            parentViewer->enableMaskDrawing();
        } else {
            parentViewer->disableMaskDrawing();
            refreshImageList();
        }
    }
    // Update process button text based on mode? (Optional cosmetic change)
    processButton->setText(drawMode ? "Apply Inpaint" : "Use Image as Mask and Inpaint");
}

// ======================================================================
// `Slot Implementations`
// ======================================================================

/**
 * @brief Updates the parent viewer's brush thickness.
 */
void InpaintingDialog::onBrushThicknessChanged(int value) {
    if (parentViewer) {
        parentViewer->setBrushThickness(value);
    }
}

/**
 * @brief Opens the currently drawn mask in a new ImageViewer window.
 */
void InpaintingDialog::openMaskInNewViewer() {
    if (!parentViewer) {
        QMessageBox::critical(this, "Error", "Cannot open mask, parent viewer is invalid.");
        return;
    }
    if (!mainWindowContext) {
        QMessageBox::critical(this, "Error", "Cannot open new window without main window context.");
        return;
    }

    cv::Mat maskToOpen = parentViewer->getDrawnMask(); // Get clone from getter
    if (maskToOpen.empty()) {
        QMessageBox::warning(this, "Open Mask", "No mask has been drawn yet or the mask is empty.");
        return;
    }

    QString parentTitle = parentViewer->windowTitle();
    QString newTitle = parentTitle.isEmpty() ? "Drawn Mask " : parentTitle + " - Drawn Mask ";
    static int i = 0;
    newTitle += std::to_string(i++);

    // --- Create the new viewer ---
    // Assumes ImageViewer constructor exists as defined in imageviewer.h
    // Assumes ImageViewer can handle a single-channel (CV_8UC1) mask image directly.
    // Assumes mainWindowContext is the MainWindow pointer or can provide it.
    MainWindow* mainWindowPtr = qobject_cast<MainWindow*>(mainWindowContext); // Attempt cast if needed
    if (!mainWindowPtr) {
        QMessageBox::critical(this, "Error", "Invalid main window context provided.");
        return;
    }

    // Calculate a slightly offset position for the new window
    QPoint newPos = parentViewer->pos() + QPoint(20, 20);

    ImageViewer *newViewer = new ImageViewer(maskToOpen, // Pass the mask (ImageViewer takes ownership via clone)
                                             newTitle,
                                             nullptr, // No QWidget parent for top-level window
                                             newPos,    // Position hint
                                             mainWindowPtr); // Pass MainWindow context

    if (newViewer) {
        newViewer->setAttribute(Qt::WA_DeleteOnClose); // Ensure memory is cleaned up when closed
        newViewer->show();
        // Optionally, inform MainWindow about the new viewer if needed for window management
        // mainWindowPtr->addImageViewer(newViewer);
    } else {
        QMessageBox::critical(this, "Error", "Failed to create new image viewer window for the mask.");
    }
}

// ======================================================================
// `Mask Retrieval`
// ======================================================================

/**
 * @brief Gets the selected mask, preparing it if necessary.
 * @return cv::Mat The mask (CV_8UC1, binary) or empty Mat on failure.
 */
cv::Mat InpaintingDialog::getSelectedMask() const {
    if (maskSourceCombo->currentText() == "Draw on image") {
        // Return the mask drawn directly on the parent viewer
        return !drawnMask.empty() ? drawnMask : parentViewer->getDrawnMask(); // Getter returns clone
    } else {
        // Get mask from the selected image in the list
        QListWidgetItem* selectedItem = imageList->currentItem();
        if (!selectedItem) {
            QMessageBox::warning(nullptr, "Mask Selection Error", "No image selected from the list to use as a mask.");
            return cv::Mat();
        }

        ImageViewer* selectedViewer = imageViewerMap.value(selectedItem->text(), nullptr);
        if (!selectedViewer) {
            QMessageBox::critical(nullptr, "Mask Selection Error", "Internal error: Could not find the selected image viewer.");
            return cv::Mat();
        }

        cv::Mat sourceMask = selectedViewer->getOriginalImage(); // Get the image from the selected viewer
        if (sourceMask.empty()) {
            QMessageBox::warning(nullptr, "Mask Selection Error", "The selected image is empty.");
            return cv::Mat();
        }

        // --- Prepare the mask: Ensure it's 8-bit single channel and binary ---
        cv::Mat preparedMask;
        // Convert to grayscale if necessary
        if (sourceMask.channels() == 3) {
            cv::cvtColor(sourceMask, preparedMask, cv::COLOR_BGR2GRAY);
        } else if (sourceMask.channels() == 4) {
            cv::cvtColor(sourceMask, preparedMask, cv::COLOR_BGRA2GRAY);
        } else {
            preparedMask = sourceMask.clone(); // Assume already grayscale or single channel
        }

        // Convert depth to 8U if necessary (e.g., from 16U)
        if (preparedMask.depth() != CV_8U) {
            // Scale appropriately. Example for 16U:
            preparedMask.convertTo(preparedMask, CV_8U, 255.0 / 65535.0);
        }

        // Threshold to ensure binary mask (non-zero becomes 255)
        // Use a threshold slightly above 0 to handle potential noise
        cv::threshold(preparedMask, preparedMask, 1, 255, cv::THRESH_BINARY);

        // Final check
        if (preparedMask.empty() || preparedMask.type() != CV_8UC1) {
            QMessageBox::critical(nullptr, "Mask Preparation Error", "Failed to prepare the selected image as a valid mask (must be convertible to 8-bit single channel).");
            return cv::Mat();
        }

        return preparedMask;
    }
}

// ======================================================================
// `Accept/Reject Handling`
// ======================================================================

/**
 * @brief Handles actions before accepting the dialog.
 */
void InpaintingDialog::handleAccepted() {
    // Ensure drawing mode is turned off in the parent viewer before closing
    if (parentViewer) {
        parentViewer->disableMaskDrawing();
    }
    accept(); // Emit the standard accepted() signal
}

/**
 * @brief Handles actions before rejecting the dialog.
 */
void InpaintingDialog::handleRejected() {
    // Ensure drawing mode is turned off
    if (parentViewer) {
        parentViewer->disableMaskDrawing();
        parentViewer->disableMaskShowing();
    }
    reject(); // Emit the standard rejected() signal
}
