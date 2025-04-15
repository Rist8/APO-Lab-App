#include "imageviewer.h"
#include "imageprocessing.h" // Include the new algorithms header
#include "clickablelabel.h"
#include "houghdialog.h"
#include "rangestretchingdialog.h"
#include "customfilterdialog.h"
#include "twostepfilterdialog.h"
#include "directionselectiondialog.h"
#include "bitwiseoperationdialog.h"
#include "histogramwidget.h" // Included for histogramWindow member
#include "imageoperation.h" // Included for ImageOperation member
#include "mainwindow.h" // Included for mainWindow member

#include <QVBoxLayout>
#include <QHBoxLayout> // Still needed for maybe other layouts, but not histogram
#include <QMenuBar>
#include <QMessageBox>
#include <QRadioButton>
#include <QWidgetAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QTableWidget>
#include <QRegularExpressionValidator>
#include <QEventLoop>
#include <QtCharts/QtCharts>
#include <QActionGroup>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// ==========================================================================
// Group 2: Image Viewer Core & Management
// ==========================================================================
// Constructor: Sets up the image viewer window (image label, layout, menu, LUT).
// ==========================================================================
ImageViewer::ImageViewer(const cv::Mat &image, const QString &title, QWidget *parent, QPoint position, MainWindow *mainWindow)
    : QWidget(parent), originalImage(image.clone()), currentScale(1.0f), histogramWindow(nullptr), mainWindow(mainWindow) { // Initialize histogramWindow to nullptr

    if (!mainWindow) {
        qCritical("ImageViewer created without a valid MainWindow pointer!");
        // Decide how to handle this - maybe throw an exception or return early?
    } else {
        mainWindow->openedImages.push_back(this);
    }

    LUT = new QTableWidget(this);
    LUT->setEditTriggers(QAbstractItemView::NoEditTriggers);
    LUT->setMinimumHeight(64);
    LUT->setMaximumHeight(64);
    LUT->setRowCount(1);
    LUT->setColumnCount(256);
    LUT->setVerticalHeaderLabels({"Count"});

    QStringList columnLabels;
    for (int i = 0; i < 256; ++i) {
        columnLabels.append(QString::number(i));
    }
    LUT->setHorizontalHeaderLabels(columnLabels);
    LUT->hide(); // Initially hidden

    setWindowTitle(title);

    imageLabel = new ClickableLabel(this);
    connect(imageLabel, &ClickableLabel::mouseClicked, this, &ImageViewer::onImageClicked);
    imageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    // Display initial image later in constructor after layout is set

    mainLayout = new QVBoxLayout(this); // Initialize member layout

    // Create Menu Bar *before* adding widgets that depend on its height for layout
    createMenu(); // This sets up menuBar and adds it via mainLayout->setMenuBar()

    // Main content area: Just the image label for now
    mainLayout->addWidget(imageLabel);

    // LUT and Zoom Input at the bottom
    mainLayout->addWidget(LUT); // Add LUT

    zoomInput = new QLineEdit(this);
    zoomInput->setStyleSheet("QLineEdit { background-color: rgba(0, 0, 0, 100); color: white; padding: 3px; border-radius: 5px; }");
    zoomInput->setAlignment(Qt::AlignRight | Qt::AlignBottom); // Keep alignment as is
    // zoomInput->setText(QString::number(static_cast<int>(currentScale * 100)) + "%"); // Set text in updateZoomLabel
    zoomInput->setMaximumWidth(60);
    zoomInput->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    QRegularExpression regex(R"(^\d{1,3}%?$)");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regex, this);
    zoomInput->setValidator(validator);
    connect(zoomInput, &QLineEdit::returnPressed, this, &ImageViewer::setZoomFromInput);

    // Add zoomInput using addWidget with alignment
    mainLayout->addWidget(zoomInput, 0, Qt::AlignRight | Qt::AlignBottom);

    // Set the main layout for the widget
    setLayout(mainLayout); // Explicitly set layout

    // Display initial image and set size
    if (!originalImage.empty()) {
        imageLabel->setPixmap(QPixmap::fromImage(MatToQImage(originalImage)));
    }
    int initialWidth = originalImage.cols > 0 ? originalImage.cols + 30 : 400; // Simplified size calculation
    int initialHeight = originalImage.rows > 0 ? originalImage.rows + (menuBar ? menuBar->height() : 0) + 80 : 300; // Adjusted height for menu and bottom widgets
    setGeometry(position.x(), position.y(), initialWidth, initialHeight);


    updateZoomLabel(); // Set initial zoom text
    updateImage(); // Initial image display, scaling, and dependent updates
}

// ==========================================================================
// Group 11: Operation Management & Abstraction
// ==========================================================================
void ImageViewer::registerOperation(ImageOperation *op) {
    operationsList.append(op);
}

// ==========================================================================
// Group 11: Operation Management & Abstraction
// ==========================================================================
void ImageViewer::updateOperationsEnabledState() {
    using ImageType = ImageOperation::ImageType;
    ImageType type = ImageType::None;

    if (!originalImage.empty()) {
        if (originalImage.channels() == 1) {
            // Check if binary or grayscale
            cv::Mat diff1 = (originalImage != 0);
            cv::Mat diff2 = (originalImage != 255);
            cv::Mat nonBinaryPixels = diff1 & diff2;
            if (cv::countNonZero(nonBinaryPixels) == 0) {
                type = ImageType::Binary;
            } else {
                type = ImageType::Grayscale;
            }
        } else if (originalImage.channels() == 3 || originalImage.channels() == 4) {
            type = ImageType::Color;
        }
    }

    // Update registered operations
    for (ImageOperation* op : operationsList) {
        if(op) op->updateActionState(type);
    }

    // Update specific actions like histogram and LUT
    bool histogramEnabled = (type == ImageType::Grayscale || type == ImageType::Binary);
    if (showHistogramAction) { // Use the new action name
        showHistogramAction->setEnabled(histogramEnabled);
        // The button remains enabled even if the window is open; clicking it will raise the window.
    }

    QAction* lutAction = nullptr;
    for(ImageOperation* op : operationsList) {
        if(op && op->getAction() && op->getAction()->text() == "Show LUT") {
            lutAction = op->getAction();
            break;
        }
    }
    if(lutAction) {
        lutAction->setEnabled(type == ImageType::Grayscale); // LUT only for grayscale
    }
}

// ==========================================================================
// Group 2: Image Viewer Core & Management
// ==========================================================================
void ImageViewer::createMenu() {
    menuBar = new QMenuBar(this);

    // --- File Menu ---
    QMenu *fileMenu = new QMenu("File", this);
    QAction *undoAction = new QAction("Undo", this);
    QAction *redoAction = new QAction("Redo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(undoAction, &QAction::triggered, this, &ImageViewer::undo);
    connect(redoAction, &QAction::triggered, this, &ImageViewer::redo);
    fileMenu->addAction(undoAction);
    fileMenu->addAction(redoAction);
    fileMenu->addSeparator();
    registerOperation(new ImageOperation("Duplicate", this, fileMenu,
                                         ImageOperation::All,
                                         [this]() { this->duplicateImage(); }, false,
                                         QKeySequence("Ctrl+Shift+D")));
    registerOperation(new ImageOperation("Save As...", this, fileMenu,
                                         ImageOperation::All,
                                         [this]() { this->saveImageAs(); }, false,
                                         QKeySequence::Save));

    // --- View Menu ---
    QMenu *viewMenu = new QMenu("View", this);
    // Replace checkable action with a standard button/action
    showHistogramAction = new QAction("Show Histogram", this); // New action name
    showHistogramAction->setShortcut(QKeySequence("Ctrl+H")); // Optional: add shortcut
    connect(showHistogramAction, &QAction::triggered, this, &ImageViewer::showHistogram); // Connect to new slot
    viewMenu->addAction(showHistogramAction); // Add the new action
    viewMenu->addSeparator();
    registerOperation(new ImageOperation("Show LUT", this, viewMenu,
                                         ImageOperation::Grayscale, // Only for grayscale
                                         [this]() { this->toggleLUT(); }, true)); // Keep LUT toggle checkable

    // --- Image Type Menu ---
    QMenu *imageProcessingMenu = new QMenu("Image Type", this);
    registerOperation(new ImageOperation("Convert to Grayscale", this, imageProcessingMenu,
                                         ImageOperation::Color, [this]() { this->convertToGrayscale(); }));
    registerOperation(new ImageOperation("Make Binary", this, imageProcessingMenu,
                                         ImageOperation::Grayscale, [this]() { this->binarise(); }));
    registerOperation(new ImageOperation("Split Color Channels", this, imageProcessingMenu,
                                         ImageOperation::Color, [this]() { this->splitColorChannels(); }));
    registerOperation(new ImageOperation("Convert to HSV/Lab", this, imageProcessingMenu,
                                         ImageOperation::Color, [this]() { this->convertToHSVLab(); }));

    // --- Histogram Menu --- (Keep separate for consistency)
    QMenu *histogramMenu = new QMenu("Histogram Ops", this); // Renamed slightly
    registerOperation(new ImageOperation("Stretch Histogram", this, histogramMenu,
                                         ImageOperation::Grayscale, [this]() { this->stretchHistogram(); }));
    registerOperation(new ImageOperation("Equalize Histogram", this, histogramMenu,
                                         ImageOperation::Grayscale, [this]() { this->equalizeHistogram(); }));

    // --- Point Operations Menu ---
    QMenu *pointOperationsMenu = new QMenu("Point", this);
    registerOperation(new ImageOperation("Apply Negation", this, pointOperationsMenu,
                                         ImageOperation::Grayscale,
                                         [this]() { this->applyNegation(); }));
    registerOperation(new ImageOperation("Range Stretching...", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, [this]() { this->rangeStretching(); }));
    registerOperation(new ImageOperation("Apply Posterization...", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyPosterization(); }));
    registerOperation(new ImageOperation("Bitwise Operations...", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, // Bitwise usually on binary/gray
                                         [this]() { this->applyBitwiseOperation(); }));
    registerOperation(new ImageOperation("Show Line Profile", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, // Line profile on grayscale
                                         [this]() { this->showLineProfile(); }));

    // --- Filters Menu ---
    QMenu *filterMenu = new QMenu("Filters", this);
    registerOperation(new ImageOperation("Apply Blur (3x3)", this, filterMenu,
                                         ImageOperation::Grayscale | ImageOperation::Color,
                                         [this]() { this->applyBlur(); }));
    registerOperation(new ImageOperation("Apply Gaussian Blur (3x3)", this, filterMenu,
                                         ImageOperation::Grayscale | ImageOperation::Color,
                                         [this]() { this->applyGaussianBlur(); }));
    filterMenu->addSeparator();
    registerOperation(new ImageOperation("Sobel Edge Detection", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applySobelEdgeDetection(); }));
    registerOperation(new ImageOperation("Laplacian Edge Detection", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyLaplacianEdgeDetection(); }));
    registerOperation(new ImageOperation("Canny Edge Detection", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyCannyEdgeDetection(); }));
    registerOperation(new ImageOperation("Prewitt Edge Detection...", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyPrewittEdgeDetection(); }));
    filterMenu->addSeparator();
    QMenu *sharpenMenu = filterMenu->addMenu("Sharpening");
    registerOperation(new ImageOperation("Basic Sharpening", this, sharpenMenu,
                                         ImageOperation::Grayscale | ImageOperation::Color,
                                         [this]() { this->applySharpening(1); }));
    registerOperation(new ImageOperation("Strong Sharpening", this, sharpenMenu,
                                         ImageOperation::Grayscale | ImageOperation::Color,
                                         [this]() { this->applySharpening(2); }));
    registerOperation(new ImageOperation("Edge Enhancement", this, sharpenMenu,
                                         ImageOperation::Grayscale | ImageOperation::Color,
                                         [this]() { this->applySharpening(3); }));
    filterMenu->addSeparator();
    registerOperation(new ImageOperation("Apply Median Filter...", this, filterMenu,
                                         ImageOperation::Grayscale, // Median typically on grayscale
                                         [this]() { this->applyMedianFilter(); }));
    filterMenu->addSeparator();
    registerOperation(new ImageOperation("Apply Custom Filter...", this, filterMenu,
                                         ImageOperation::Grayscale, // Custom usually on grayscale
                                         [this]() { this->applyCustomFilter(); }));
    registerOperation(new ImageOperation("Two-Step Filter (5x5)...", this, filterMenu,
                                         ImageOperation::Grayscale, // Two-step usually on grayscale
                                         [this]() { this->applyTwoStepFilter(); }));
    filterMenu->addSeparator();
    registerOperation(new ImageOperation("Detect Lines (Hough)...", this, filterMenu,
                                         ImageOperation::Binary, // Hough requires binary
                                         [this]() { this->applyHoughLineDetection(); }));

    // --- Morphology Menu ---
    QMenu *morphologyMenu = new QMenu("Morphology", this);
    auto createElementSelector = [this](QMenu *parentMenu, StructuringElementType &targetVar, const QString& title) {
        QMenu *elementMenu = new QMenu(title, parentMenu);
        QWidget *widget = new QWidget(elementMenu);
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->setSpacing(5);
        QRadioButton *diamondRadio = new QRadioButton("Diamond (4-conn)", widget);
        QRadioButton *squareRadio = new QRadioButton("Square (8-conn)", widget);
        if (targetVar == Diamond) diamondRadio->setChecked(true); else squareRadio->setChecked(true);
        layout->addWidget(diamondRadio);
        layout->addWidget(squareRadio);
        // Use lambda capture to ensure the correct targetVar is modified
        connect(diamondRadio, &QRadioButton::toggled, this, [&targetVar, this](bool checked) {
            if (checked) targetVar = Diamond;
        });
        connect(squareRadio, &QRadioButton::toggled, this, [&targetVar, this](bool checked) {
            if (checked) targetVar = Square;
        });
        QWidgetAction *widgetAction = new QWidgetAction(elementMenu);
        widgetAction->setDefaultWidget(widget);
        elementMenu->addAction(widgetAction);
        return elementMenu;
    };
    QMenu *erosionMenu = morphologyMenu->addMenu("Erosion");
    erosionMenu->addMenu(createElementSelector(erosionMenu, erosionElement, "Structuring Element"));
    registerOperation(new ImageOperation("Apply Erosion", this, erosionMenu,
                                         ImageOperation::Binary,
                                         [this]() { this->applyErosion(this->erosionElement); })); // Explicitly capture this if necessary, or ensure member access works
    QMenu *dilationMenu = morphologyMenu->addMenu("Dilation");
    dilationMenu->addMenu(createElementSelector(dilationMenu, dilationElement, "Structuring Element"));
    registerOperation(new ImageOperation("Apply Dilation", this, dilationMenu,
                                         ImageOperation::Binary,
                                         [this]() { this->applyDilation(this->dilationElement); }));
    QMenu *openingMenu = morphologyMenu->addMenu("Opening");
    openingMenu->addMenu(createElementSelector(openingMenu, openingElement, "Structuring Element"));
    registerOperation(new ImageOperation("Apply Opening", this, openingMenu,
                                         ImageOperation::Binary,
                                         [this]() { this->applyOpening(this->openingElement); }));
    QMenu *closingMenu = morphologyMenu->addMenu("Closing");
    closingMenu->addMenu(createElementSelector(closingMenu, closingElement, "Structuring Element"));
    registerOperation(new ImageOperation("Apply Closing", this, closingMenu,
                                         ImageOperation::Binary,
                                         [this]() { this->applyClosing(this->closingElement); }));
    morphologyMenu->addSeparator();
    registerOperation(new ImageOperation("Skeletonize", this, morphologyMenu,
                                         ImageOperation::Binary,
                                         [this]() { this->applySkeletonization(); }));

    // --- Add Menus to Bar ---
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(viewMenu);
    menuBar->addMenu(imageProcessingMenu);
    menuBar->addMenu(pointOperationsMenu);
    menuBar->addMenu(histogramMenu);
    menuBar->addMenu(filterMenu);
    menuBar->addMenu(morphologyMenu);

    // Set the menu bar for the main layout (already done if passed to QVBoxLayout)
    mainLayout->setMenuBar(menuBar);

    updateOperationsEnabledState(); // Initial state update
}

// ==========================================================================
// Group 2: Image Viewer Core & Management
// ==========================================================================
void ImageViewer::duplicateImage() {
    if (originalImage.empty() || !mainWindow) return;

    static int duplicateCount = 1;
    QString newTitle = QString("%1 - Copy %2").arg(this->windowTitle()).arg(duplicateCount++); // Use this->windowTitle()
    QPoint newPos = this->pos() + QPoint(150, 150);

    ImageViewer *newViewer = new ImageViewer(originalImage.clone(),
                                             newTitle, nullptr, // Parent is null for new top-level window
                                             newPos, mainWindow);
    newViewer->setZoom(currentScale); // Apply current zoom to duplicate
    newViewer->show();
}

// ==========================================================================
// Group 12: UI Interaction Helpers
// ==========================================================================
std::vector<cv::Point> ImageViewer::getUserSelectedPoints(int count) {
    selectedPoints.clear();
    pointsToSelect = count;

    // Optional: Give visual feedback that point selection is active
    // e.g., change cursor, show status message

    QEventLoop loop;
    pointSelectionCallback = [&]() {
        // Visual feedback for selected point (e.g., draw a temporary marker)
        // ...

        if (selectedPoints.size() >= pointsToSelect) {
            loop.quit();  // Exit loop when enough points are picked
        }
    };

    loop.exec();  // Wait until selection is finished

    pointSelectionCallback = nullptr; // Clear callback
    pointsToSelect = 0;
    // Optional: Clear visual feedback

    return selectedPoints;
}


// ==========================================================================
// Group 12: UI Interaction Helpers
// ==========================================================================
void ImageViewer::onImageClicked(QMouseEvent* event) {
    if (pointsToSelect <= 0 || originalImage.empty()) // Added check for empty image
        return;

    // Translate click position to image coordinates (handle scaling AND label position)
    QPoint clickPos = event->pos(); // Position relative to imageLabel

    // Get the displayed pixmap size
    QSize pixmapSize = imageLabel->pixmap(Qt::ReturnByValue).size(); // Use Qt::ReturnByValue for safety
    if (pixmapSize.isEmpty() || pixmapSize.width() <= 0 || pixmapSize.height() <= 0) return;


    // Calculate scale based on original image vs displayed pixmap size
    double xScale = static_cast<double>(originalImage.cols) / pixmapSize.width();
    double yScale = static_cast<double>(originalImage.rows) / pixmapSize.height();

    // Calculate image coordinates, clamping to bounds
    int imgX = static_cast<int>(std::round(clickPos.x() * xScale));
    int imgY = static_cast<int>(std::round(clickPos.y() * yScale));

    imgX = std::max(0, std::min(originalImage.cols - 1, imgX));
    imgY = std::max(0, std::min(originalImage.rows - 1, imgY));


    selectedPoints.push_back(cv::Point(imgX, imgY));

    if (pointSelectionCallback)
        pointSelectionCallback();  // Notify the event loop or update UI
}


// ==========================================================================
// Group 2: Image Viewer Core & Management
// ==========================================================================
// REVERTED saveImageAs function kept as is
// ==========================================================================
void ImageViewer::saveImageAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Image As", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!filePath.isEmpty()) {
        std::vector<int> compression_params;
        if (filePath.endsWith(".jpg", Qt::CaseInsensitive) || filePath.endsWith(".jpeg", Qt::CaseInsensitive)) {
            compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
            compression_params.push_back(95);  // JPEG quality (0-100)
        } else if (filePath.endsWith(".png", Qt::CaseInsensitive)) {
            compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
            compression_params.push_back(3); // PNG compression level (0-9)
        }
#ifdef _WIN32
        std::string path = filePath.toLocal8Bit().constData(); // Safer conversion for Windows paths
#else
        std::string path = filePath.toUtf8().constData();
#endif
        try {
            if (!cv::imwrite(path, originalImage, compression_params)) {
                QMessageBox::warning(this, "Save Error", "Failed to save the image. Check file path and permissions.");
            }
        } catch (const cv::Exception& ex) {
            QMessageBox::critical(this, "OpenCV Save Error", QString("Error saving image: %1").arg(ex.what()));
        }
    }
}

// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// Renamed from toggleHistogram to showHistogram
// Logic changed to create/show a separate window
// ==========================================================================
void ImageViewer::showHistogram() {
    if (!histogramWindow) { // If no window exists or it was destroyed
        histogramWindow = new HistogramWidget(); // Create as a top-level window

        // Connect the destroyed signal to our slot to know when it's closed
        connect(histogramWindow, &HistogramWidget::destroyed, this, &ImageViewer::onHistogramClosed);

        // Set a unique window title based on the image viewer
        histogramWindow->setWindowTitle(this->windowTitle() + " - Histogram");

        // Position the histogram window relative to the image viewer
        histogramWindow->move(this->pos() + QPoint(this->width() + 10, 0));

        updateHistogram(); // Compute initial data before showing
        histogramWindow->show();
    } else {
        // Window already exists, just bring it to the front
        histogramWindow->raise();
        histogramWindow->activateWindow();
    }
    // No need to update button state here, it's handled by updateOperationsEnabledState
}

// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// New slot to handle the histogram window being closed/destroyed
// ==========================================================================
void ImageViewer::onHistogramClosed() {
    // The signal is emitted when the object is about to be destroyed,
    // or already is. Just nullify the pointer.
    histogramWindow = nullptr;
    // Update the button state (if needed, though current logic doesn't disable when open)
    // updateOperationsEnabledState(); // Uncomment if button state depends on window existence
}


// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// Keep toggleLUT as is, it controls the embedded QTableWidget
void ImageViewer::toggleLUT() {
    QAction* lutAction = nullptr;
    for(ImageOperation* op : operationsList) {
        if(op && op->getAction() && op->getAction()->text() == "Show LUT") {
            lutAction = op->getAction();
            break;
        }
    }

    if (!LUT) return; // Safety check

    if (LUT->isVisible()) {
        LUT->hide();
        if(lutAction) lutAction->setChecked(false);
    } else {
        updateHistogramTable(); // Update data *before* showing
        LUT->show();
        if(lutAction) lutAction->setChecked(true);
    }
    // Adjusting size might still be needed if LUT significantly changes window height needs
    adjustSize();
}

// ==========================================================================
// Group 5: Image Processing - Core Operations (Wrappers) - NO CHANGES HERE
// ==========================================================================
void ImageViewer::binarise() {
    pushToUndoStack();
    originalImage = ImageProcessing::binarise(originalImage);
    updateImage();
}

void ImageViewer::convertToGrayscale() {
    pushToUndoStack();
    originalImage = ImageProcessing::convertToGrayscale(originalImage);
    updateImage();
}

void ImageViewer::splitColorChannels() {
    if (!mainWindow || originalImage.channels() < 3) { // Added channel check
        QMessageBox::warning(this, "Split Error", "Cannot split channels. Image must be 3 or 4 channel color.");
        return;
    }
    std::vector<cv::Mat> channels = ImageProcessing::splitColorChannels(originalImage);
    if (channels.size() >= 3) { // Allow for 4 channels (ignore alpha maybe?)
        (new ImageViewer(channels[0], windowTitle() + " - Blue", nullptr, pos() + QPoint(30, 30), mainWindow))->show();
        (new ImageViewer(channels[1], windowTitle() + " - Green", nullptr, pos() + QPoint(60, 60), mainWindow))->show();
        (new ImageViewer(channels[2], windowTitle() + " - Red", nullptr, pos() + QPoint(90, 90), mainWindow))->show();
    } else {
        QMessageBox::warning(this, "Split Error", "Failed to split channels.");
    }
}

void ImageViewer::convertToHSVLab() {
    if (!mainWindow || originalImage.channels() < 3) { // Added channel check
        QMessageBox::warning(this, "Conversion Error", "Cannot convert. Image must be 3 or 4 channel color.");
        return;
    }
    cv::Mat hsv = ImageProcessing::convertToHSV(originalImage);
    cv::Mat lab = ImageProcessing::convertToLab(originalImage);

    if (!hsv.empty()) {
        (new ImageViewer(hsv, windowTitle() + " - HSV", nullptr, pos() + QPoint(30, 30), mainWindow))->show();
    } else {
        QMessageBox::warning(this, "Conversion Error", "Could not convert to HSV.");
    }
    if (!lab.empty()) {
        (new ImageViewer(lab, windowTitle() + " - Lab", nullptr, pos() + QPoint(60, 60), mainWindow))->show();
    } else {
        QMessageBox::warning(this, "Conversion Error", "Could not convert to Lab.");
    }
}

// ==========================================================================
// Group 7: Image Processing - Histogram Operations (Wrappers) - NO CHANGES HERE
// ==========================================================================
void ImageViewer::stretchHistogram() {
    pushToUndoStack();
    originalImage = ImageProcessing::stretchHistogram(originalImage);
    updateImage();
}

void ImageViewer::equalizeHistogram() {
    pushToUndoStack();
    originalImage = ImageProcessing::equalizeHistogram(originalImage);
    updateImage();
}

// ==========================================================================
// Group 6: Image Processing - Point Operations (Wrappers) - NO CHANGES HERE
// ==========================================================================
void ImageViewer::applyNegation() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyNegation(originalImage);
    updateImage();
}

void ImageViewer::rangeStretching() {
    RangeStretchingDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        pushToUndoStack();
        int p1 = dialog.getP1();
        int p2 = dialog.getP2();
        int q3 = dialog.getQ3();
        int q4 = dialog.getQ4();
        originalImage = ImageProcessing::applyRangeStretching(originalImage, p1, p2, q3, q4);
        updateImage();
    }
}

void ImageViewer::applyPosterization() {
    bool ok;
    int levels = QInputDialog::getInt(this, "Posterization", "Enter number of levels (2-256):", 4, 2, 256, 1, &ok);
    if (ok) {
        pushToUndoStack();
        originalImage = ImageProcessing::applyPosterization(originalImage, levels);
        updateImage();
    }
}

void ImageViewer::applyBitwiseOperation() {
    if (originalImage.empty() || !mainWindow) return;
    BitwiseOperationDialog dialog(this, mainWindow->openedImages);
    if (dialog.exec() == QDialog::Accepted) {
        cv::Mat result = dialog.getResult();
        if (!result.empty()) {
            pushToUndoStack();
            originalImage = result;
            updateImage();
        } else {
            QMessageBox::warning(this, "Operation Failed", "Bitwise operation could not be completed (maybe incompatible images?).");
        }
    }
}

// ==========================================================================
// Group 2: Image Viewer Core & Management - UNDO/REDO
// ==========================================================================
void ImageViewer::pushToUndoStack() {
    if (!originalImage.empty()) {
        // Consider using QStack for potentially better Qt integration, though std::stack is fine
        const int MAX_UNDO_LEVELS = 20; // Keep limit reasonable
        // If using std::stack, we need to manage the size manually if a limit is desired
        if (undoStack.size() >= MAX_UNDO_LEVELS) {
            // To implement a limit with stack, we'd need to pop from the "bottom",
            // which stack doesn't support. Easiest is to use std::deque or QQueue/QStack
            // Or just let it grow, assuming memory isn't a huge concern for typical use.
            qWarning() << "Undo stack limit potentially exceeded. Oldest states not automatically removed.";
        }
        undoStack.push(originalImage.clone());
        clearRedoStack(); // Clear redo whenever a new action is performed
    }
}

void ImageViewer::undo() {
    if (!undoStack.empty()) {
        redoStack.push(originalImage.clone()); // Push current state to redo
        originalImage = undoStack.top();
        undoStack.pop();
        updateImage();
    }
}

void ImageViewer::redo() {
    if (!redoStack.empty()) {
        undoStack.push(originalImage.clone()); // Push current state to undo
        originalImage = redoStack.top();
        redoStack.pop();
        updateImage();
    }
}

void ImageViewer::clearRedoStack() {
    // Use QStack's clear() or std::stack's loop pop
    while (!redoStack.empty()) redoStack.pop();
    // redoStack.clear(); // If using QStack
}

// ==========================================================================
// Group 2: Image Viewer Core & Management - UPDATE IMAGE
// ==========================================================================
void ImageViewer::updateImage() {
    if (originalImage.empty()) {
        imageLabel->clear();
        setWindowTitle("Image Viewer");
        updateOperationsEnabledState();
        if (histogramWindow) histogramWindow->computeHistogram(cv::Mat());
        if (LUT && LUT->isVisible()) updateHistogramTable();
        return;
    }

    // --- Clamp currentScale to pyramid-friendly values if pyramid is active ---
    if (usePyramidScaling) {
        static const std::vector<double> pyramidScales = {0.25, 0.5, 1.0, 2.0, 4.0};
        auto closest = std::min_element(pyramidScales.begin(), pyramidScales.end(),
                                        [=](double a, double b) {
                                            return std::abs(currentScale - a) < std::abs(currentScale - b);
                                        });
        currentScale = *closest;
    }

    cv::Mat displayImage = originalImage.clone();
    if (usePyramidScaling) {
        if (currentScale == 0.5) {
            cv::pyrDown(displayImage, displayImage);
        } else if (currentScale == 0.25) {
            cv::pyrDown(displayImage, displayImage);
            cv::pyrDown(displayImage, displayImage);
        } else if (currentScale == 2.0) {
            cv::pyrUp(displayImage, displayImage);
        } else if (currentScale == 4.0) {
            cv::pyrUp(displayImage, displayImage);
            cv::pyrUp(displayImage, displayImage);
        }
    }


    QImage qimg = MatToQImage(displayImage);
    if (qimg.isNull()) {
        QMessageBox::warning(this, "Display Error", "Failed to convert image format for display.");
        imageLabel->clear();
        updateOperationsEnabledState();
        return;
    }

    QPixmap pixmap;
    if (usePyramidScaling) {
        pixmap = QPixmap::fromImage(qimg); // Already scaled, no additional scaling
    } else {
        int newWidth = std::max(1, static_cast<int>(std::round(qimg.width() * currentScale)));
        int newHeight = std::max(1, static_cast<int>(std::round(qimg.height() * currentScale)));
        pixmap = QPixmap::fromImage(qimg).scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    imageLabel->setPixmap(pixmap);
    imageLabel->setFixedSize(pixmap.size());

    if (!isMaximized() && !isFullScreen()) {
        adjustSize();
    }

    updateHistogram();
    if (LUT && LUT->isVisible()) {
        updateHistogramTable();
    }

    updateZoomLabel();
    updateOperationsEnabledState();
}



// ==========================================================================
// Group 4: UI Widgets & Visualization - ZOOM CONTROLS
// ==========================================================================
void ImageViewer::updateZoomLabel() {
    if (!zoomInput) return; // Safety check
    int zoomPercentage = static_cast<int>(std::round(currentScale * 100.0));
    zoomInput->setText(QString::number(zoomPercentage) + "%");
    // zoomInput->adjustSize(); // Let layout handle size, just set max width
}

void ImageViewer::setZoomFromInput() {
    if (!zoomInput) return; // Safety check
    QString text = zoomInput->text();
    text.remove('%'); // Remove percentage sign if present
    bool ok;
    int zoomValue = text.toInt(&ok);

    zoomInput->clearFocus(); // Remove focus after pressing Enter

    if (ok && zoomValue >= 10 && zoomValue <= 500) { // Zoom range 10% to 500%
        double newScale = zoomValue / 100.0;
        if (std::abs(newScale - currentScale) > 1e-6) { // Only update if scale actually changes
            currentScale = newScale;
            updateImage();
        } else {
            updateZoomLabel(); // Restore text if value was same but format different (e.g., added %)
        }
    } else {
        QMessageBox::warning(this, "Invalid Input", "Please enter a zoom value between 10% and 500%.");
        updateZoomLabel(); // Reset text to current valid zoom
    }
}

// ==========================================================================
// Group 4: UI Widgets & Visualization - WHEEL EVENT FOR ZOOM
// ==========================================================================
void ImageViewer::wheelEvent(QWheelEvent *event) {
    double oldScale = currentScale;

    if (usePyramidScaling) {
        // Only allow switching between 0.25, 0.5, 1.0, 2.0, 4.0
        static const std::vector<double> pyramidScales = {0.25, 0.5, 1.0, 2.0, 4.0};
        auto it = std::find(pyramidScales.begin(), pyramidScales.end(), currentScale);
        if (it != pyramidScales.end()) {
            if (event->angleDelta().y() > 0 && it + 1 != pyramidScales.end()) {
                currentScale = *(it + 1); // Zoom in
            } else if (event->angleDelta().y() < 0 && it != pyramidScales.begin()) {
                currentScale = *(it - 1); // Zoom out
            }
        }
    } else {
        const double scaleFactor = 1.15;
        if (event->angleDelta().y() > 0) {
            currentScale *= scaleFactor;
        } else {
            currentScale /= scaleFactor;
        }
        currentScale = qBound(0.1, currentScale, 5.0);
    }

    if (std::abs(currentScale - oldScale) > 1e-6) {
        updateImage();
    }

    event->accept();
}


// ==========================================================================
// Group 7: Image Processing - Histogram Operations
// ==========================================================================
// Update histogram SYNCHRONOUSLY - called by updateImage()
void ImageViewer::updateHistogram() {
    // Check if the histogram window pointer is valid (i.e., window exists)
    if (histogramWindow) {
        // Ensure image is grayscale before computing histogram
        cv::Mat grayImage;
        if (originalImage.channels() == 1) {
            grayImage = originalImage; // Already grayscale/binary
        } else if (originalImage.channels() >= 3) {
            grayImage = ImageProcessing::convertToGrayscale(originalImage);
        } else {
            // Handle unexpected channel count - clear histogram
            histogramWindow->computeHistogram(cv::Mat()); // Pass empty Mat
            return;
        }

        if (!grayImage.empty()) {
            histogramWindow->computeHistogram(grayImage);
        } else {
            // Handle case where grayscale conversion failed
            histogramWindow->computeHistogram(cv::Mat()); // Pass empty Mat
        }
    }
}

// Update embedded LUT table - called by updateImage() and toggleLUT()
void ImageViewer::updateHistogramTable() {
    if (!LUT || !LUT->isVisible() || originalImage.empty() || originalImage.channels() != 1) {
        // Clear table if not visible, no image, or not grayscale
        if (LUT) {
            LUT->clearContents(); // Clear data but keep headers
            // Optionally set all counts to 0
            for (int i = 0; i < 256; ++i) {
                QTableWidgetItem *item = LUT->item(0, i);
                if (!item) {
                    item = new QTableWidgetItem("0");
                    LUT->setItem(0, i, item);
                } else {
                    item->setText("0");
                }
            }
        }
        return;
    }

    std::vector<int> histogramDataVec(256, 0); // Use std::vector

    // Calculate histogram directly (only for CV_8UC1)
    for (int y = 0; y < originalImage.rows; ++y) {
        const uchar* row_ptr = originalImage.ptr<uchar>(y);
        for (int x = 0; x < originalImage.cols; ++x) {
            histogramDataVec[row_ptr[x]]++;
        }
    }

    // Update QTableWidget
    LUT->setColumnCount(256); // Ensure correct column count
    for (int i = 0; i < 256; ++i) {
        QTableWidgetItem *item = LUT->item(0, i); // Try to reuse existing item
        if (!item) {
            item = new QTableWidgetItem(QString::number(histogramDataVec[i]));
            LUT->setItem(0, i, item);
        } else {
            item->setText(QString::number(histogramDataVec[i]));
        }
    }
}

// ==========================================================================
// Group 8: Image Processing - Filtering & Edge Detection (Wrappers) - NO CHANGES HERE
// ==========================================================================
void ImageViewer::applyBlur() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyBoxBlur(originalImage, 3, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyGaussianBlur() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyGaussianBlur(originalImage, 3, 0, 0, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applySobelEdgeDetection() {
    pushToUndoStack();
    originalImage = ImageProcessing::applySobelEdgeDetection(originalImage, 3, 1, 0, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyLaplacianEdgeDetection() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyLaplacianEdgeDetection(originalImage, 1, 1, 0, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyCannyEdgeDetection() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyCannyEdgeDetection(originalImage, 50, 150); // Default thresholds
    updateImage();
}

void ImageViewer::applySharpening(int option) {
    pushToUndoStack();
    originalImage = ImageProcessing::applySharpening(originalImage, option, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyPrewittEdgeDetection() {
    DirectionSelectionDialog dialog(this);
    connect(&dialog, &DirectionSelectionDialog::directionSelected, this, &ImageViewer::setPrewittDirection);
    dialog.exec();
}

// Slot remains private, called by applyPrewittEdgeDetection
void ImageViewer::setPrewittDirection(int direction) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyPrewittEdgeDetection(originalImage, direction, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyCustomFilter() {
    CustomFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        pushToUndoStack();
        cv::Mat kernel = dialog.getKernel();
        originalImage = ImageProcessing::applyCustomFilter(originalImage, kernel, true, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
        updateImage();
    }
}

void ImageViewer::applyMedianFilter() {
    bool ok;
    QStringList kernelOptions = {"3", "5", "7", "9"}; // Odd sizes usually
    QString selected = QInputDialog::getItem(this, "Select Median Kernel Size", "Kernel Size:", kernelOptions, 0, false, &ok);
    if (ok && !selected.isEmpty()) {
        int kernelSize = selected.toInt();
        pushToUndoStack();
        originalImage = ImageProcessing::applyMedianFilter(originalImage, kernelSize, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
        updateImage();
    }
}

void ImageViewer::applyTwoStepFilter() {
    TwoStepFilterDialog filterDialog(this);
    if (filterDialog.exec() == QDialog::Accepted) {
        pushToUndoStack();
        cv::Mat kernel1 = filterDialog.getKernel1();
        cv::Mat kernel2 = filterDialog.getKernel2();
        originalImage = ImageProcessing::applyTwoStepFilter(originalImage, kernel1, kernel2, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
        updateImage();
    }
}

// ==========================================================================
// Group 10: Image Processing - Feature Detection (Wrappers) - NO CHANGES HERE
// ==========================================================================
void ImageViewer::applyHoughLineDetection() {
    if (originalImage.empty()) return;

    cv::Mat edgeImage;
    // Check if image is already binary (0s and 255s only)
    bool isBinary = false;
    if (originalImage.type() == CV_8UC1) {
        cv::Mat nonBinary = (originalImage != 0) & (originalImage != 255);
        isBinary = (cv::countNonZero(nonBinary) == 0);
    }

    if (!isBinary) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Hough Lines",
                                      "Hough transform requires a binary image.\nApply Canny edge detection (50, 150) first?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            // Convert to grayscale if necessary before Canny
            cv::Mat grayForCanny;
            if (originalImage.channels() == 1) {
                grayForCanny = originalImage;
            } else {
                grayForCanny = ImageProcessing::convertToGrayscale(originalImage);
            }
            edgeImage = ImageProcessing::applyCannyEdgeDetection(grayForCanny, 50, 150);
        } else {
            return; // User chose not to proceed
        }
    } else {
        edgeImage = originalImage.clone(); // Use the binary image directly
    }

    if (edgeImage.empty() || edgeImage.type() != CV_8UC1) { // Double check edgeImage
        QMessageBox::warning(this, "Hough Error", "Could not obtain a valid binary edge image for Hough transform.");
        return;
    }

    HoughDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return; // User cancelled dialog

    pushToUndoStack(); // Push state *before* drawing lines

    double rho = dialog.getRho();
    double thetaRadians = dialog.getThetaDegrees() * CV_PI / 180.0;
    int threshold = dialog.getThreshold();

    std::vector<cv::Vec2f> lines = ImageProcessing::detectHoughLines(edgeImage, rho, thetaRadians, threshold);

    // Prepare image for drawing lines (convert original to color if needed)
    cv::Mat colorImage;
    if (originalImage.channels() == 1) {
        cv::cvtColor(originalImage, colorImage, cv::COLOR_GRAY2BGR);
    } else if (originalImage.channels() == 3){
        colorImage = originalImage.clone(); // Already BGR
    } else if (originalImage.channels() == 4) {
        cv::cvtColor(originalImage, colorImage, cv::COLOR_BGRA2BGR); // Drop alpha
    } else {
        QMessageBox::warning(this, "Hough Draw Error", "Cannot draw lines on image with unsupported channel count.");
        undo(); // Revert the pushToUndoStack
        return;
    }

    // Draw detected lines
    if (!lines.empty()) {
        qInfo() << "Detected" << lines.size() << "lines.";
        for (size_t i = 0; i < lines.size(); i++) {
            float r = lines[i][0], t = lines[i][1];
            double a = std::cos(t), b = std::sin(t);
            double x0 = a * r, y0 = b * r;
            // Calculate points far enough to span the image diagonal for safety
            double imgDiagonal = std::sqrt(colorImage.cols*colorImage.cols + colorImage.rows*colorImage.rows);
            cv::Point pt1(cvRound(x0 + imgDiagonal * (-b)), cvRound(y0 + imgDiagonal * (a)));
            cv::Point pt2(cvRound(x0 - imgDiagonal * (-b)), cvRound(y0 - imgDiagonal * (a)));
            cv::line(colorImage, pt1, pt2, cv::Scalar(0, 0, 255), 1, cv::LINE_AA); // Draw red lines
        }
        originalImage = colorImage; // Update originalImage only if lines were drawn
    } else {
        QMessageBox::information(this, "Hough Lines", "No lines detected with the given parameters.");
        // Don't change originalImage, effectively cancelling the operation visually
        // The state pushed to undo stack is the original image before attempting Hough.
        // No need to pop here, user can undo if they want.
    }

    updateImage(); // Update display
}

// ==========================================================================
// Group 9: Image Processing - Morphology (Wrappers) - NO CHANGES HERE
// ==========================================================================
void ImageViewer::applyErosion(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyErosion(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyDilation(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyDilation(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyOpening(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyOpening(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applyClosing(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyClosing(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

void ImageViewer::applySkeletonization() {
    pushToUndoStack();
    // Assuming Diamond is default or appropriate here
    originalImage = ImageProcessing::applySkeletonization(originalImage, Diamond);
    updateImage();
}

// ==========================================================================
// Group 4: UI Widgets & Visualization - TEMP IMAGE / LINE PROFILE
// ==========================================================================
// Temporarily display an image without adding to undo stack
void ImageViewer::showTempImage(const cv::Mat &temp) {
    if (temp.empty() || !imageLabel) return; // Safety checks

    QImage qimg = MatToQImage(temp);
    if (qimg.isNull()) return;

    // Scale temp image to fit current label size
    int currentLabelWidth = imageLabel->width();
    int currentLabelHeight = imageLabel->height();
    if (currentLabelWidth <= 0 || currentLabelHeight <= 0) return; // Avoid division by zero if label isn't visible yet

    QPixmap scaledPixmap = QPixmap::fromImage(qimg).scaled(currentLabelWidth, currentLabelHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
    // DO NOT call adjustSize() or updateImage() here, it's temporary
}

void ImageViewer::showLineProfile() {
    if (originalImage.empty() || originalImage.channels() != 1) { // Added empty check
        QMessageBox::warning(this, "Invalid Image", "Line profile requires a non-empty grayscale or binary image.");
        return;
    }

    QMessageBox::information(this, "Select Points", "Please click two points on the image to define the profile line.");
    std::vector<cv::Point> points = getUserSelectedPoints(2); // Get 2 points

    if (points.size() != 2) {
        QMessageBox::warning(this, "Selection Error", "Two points must be selected. Line profile cancelled.");
        // updateImage(); // Restore original view if temp changes were made (none yet)
        return;
    }

    cv::Point p1 = points[0];
    cv::Point p2 = points[1];

    // --- Draw temporary line on a copy ---
    cv::Mat displayImage;
    // Always convert to color for drawing the line, even if original is gray
    if (originalImage.type() == CV_8UC1) {
        cv::cvtColor(originalImage, displayImage, cv::COLOR_GRAY2BGR);
    } else {
        // Should not happen based on initial check, but handle defensively
        QMessageBox::warning(this, "Line Profile Error", "Unexpected image type for line profile.");
        return;
        // displayImage = originalImage.clone(); // If we wanted to handle color images
    }
    // Make line thickness relative to image size, minimum 1
    int lineThickness = std::max(1, static_cast<int>(std::round(std::max(displayImage.cols, displayImage.rows) / 500.0)));
    cv::line(displayImage, p1, p2, cv::Scalar(0, 0, 255), lineThickness); // Red line
    showTempImage(displayImage); // Show the image with the line TEMPORARILY
    // --- End temporary line ---

    // --- Extract pixel values along the line from the ORIGINAL image ---
    std::vector<uchar> values;
    try {
        // Use LineIterator on the original CV_8UC1 image
        cv::LineIterator it(originalImage, p1, p2, 8); // 8-connected iterator
        values.reserve(it.count);
        for (int i = 0; i < it.count; i++, ++it) {
            // Dereference the iterator pointer to get the pixel value pointer, then dereference that
            values.push_back(**it);
        }
    } catch (const cv::Exception& e) {
        QMessageBox::critical(this, "Line Profile Error", QString("Error extracting pixel values using LineIterator: %1").arg(e.what()));
        updateImage(); // Restore original view after error
        return;
    }
    // --- End pixel extraction ---

    if (values.empty()) {
        QMessageBox::warning(this, "Line Profile Error", "No pixel values extracted for the selected line (points might be identical or outside bounds?).");
        updateImage(); // Restore original view
        return;
    }

    // --- Plot values using Qt Charts ---
    QLineSeries *series = new QLineSeries();
    for (int i = 0; i < values.size(); ++i) {
        series->append(i, values[i]);
    }
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("Line Profile (%1,%2) to (%3,%4)")
                        .arg(p1.x).arg(p1.y).arg(p2.x).arg(p2.y));
    chart->createDefaultAxes(); // Creates axes based on series data
    chart->legend()->hide();

    // Customize Axes
    QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());
    if(axisX) {
        axisX->setRange(0, values.size() > 0 ? values.size() - 1 : 0); // Handle empty values case
        axisX->setTitleText("Distance along line (pixels)");
        axisX->setLabelFormat("%d"); // Integer labels
    }
    if(axisY) {
        axisY->setRange(0, 255); // Intensity range
        axisY->setTitleText("Pixel Intensity");
        axisY->setLabelFormat("%d");
    }

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    // --- End plot setup ---

    // --- Show plot in a Dialog ---
    QDialog *dialog = new QDialog(this); // Parent dialog to image viewer
    dialog->setAttribute(Qt::WA_DeleteOnClose); // Ensure dialog is deleted
    dialog->setWindowTitle("Line Profile Plot");
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(chartView);
    dialog->resize(600, 400);
    // Position dialog relative to main window or image viewer
    dialog->move(this->mapToGlobal(QPoint(50, 50)));

    // Connect dialog finished signal to restore the original image view
    // Using lambda to capture 'this' safely
    connect(dialog, &QDialog::finished, this, [this]() {
        this->updateImage(); // Restore original image display
    });

    dialog->show(); // Show non-modally
    // --- End plot dialog ---
}

// ==========================================================================
// Group 2: Image Viewer Core & Management - HELPERS / CLOSE EVENT
// ==========================================================================
QImage ImageViewer::MatToQImage(const cv::Mat &mat) {
    if (mat.type() == CV_8UC3) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888).rgbSwapped();
    } else if (mat.type() == CV_8UC4) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
    } else if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    }
    return QImage(); // Invalid image
}

// Handle window close event
void ImageViewer::closeEvent(QCloseEvent *event) {
    // 1. Close associated histogram window (if open)
    if (histogramWindow) {
        // Closing the window will trigger its WA_DeleteOnClose attribute,
        // which emits destroyed(), calling onHistogramClosed() to nullptr the pointer.
        histogramWindow->close();
        // We don't delete histogramWindow here, WA_DeleteOnClose handles it.
        histogramWindow = nullptr; // Defensive nullification
    }

    // 2. Remove this viewer from the MainWindow's list
    if (mainWindow) {
        mainWindow->openedImages.erase(
            std::remove(mainWindow->openedImages.begin(), mainWindow->openedImages.end(), this),
            mainWindow->openedImages.end()
            );
        // Optional: Update main window state if needed
        // mainWindow->updateSomeUI();
    }

    // 3. Clean up other owned resources (like LUT if it wasn't parented)
    // if (LUT && LUT->parent() != this) { // Example check if parenting isn't guaranteed
    //     delete LUT; // Or LUT->deleteLater();
    //     LUT = nullptr;
    // }
    // Note: Widgets parented to 'this' (like LUT, imageLabel, zoomInput, menuBar)
    // will be deleted automatically by Qt's parent-child mechanism when 'this' is deleted.
    // Operations in operationsList might need explicit deletion if not parented.
    // qDeleteAll(operationsList); // If operations are dynamically allocated and not parented elsewhere. Be cautious.
    // operationsList.clear();

    // 4. Accept the event to allow the window to close
    QWidget::closeEvent(event); // Call base class implementation
}
