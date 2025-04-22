#include "imageviewer.h"
#include "imageprocessing.h" // Include the new algorithms header
#include "clickablelabel.h"
#include "houghdialog.h"
#include "inpaintingdialog.h"
#include "inputdialog.h"
#include "pointselectiondialog.h"
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


// ======================================================================
// `Constructor & Core Management`
// ======================================================================
// Constructor: Sets up the image viewer window (image label, layout, menu, LUT).
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

    mainLayout = new QVBoxLayout(this);
    // Initialize member layout

    // Create Menu Bar *before* adding widgets that depend on its height for layout
    createMenu();
    // This sets up menuBar and adds it via mainLayout->setMenuBar()

    // Main content area: Just the image label for now
    mainLayout->addWidget(imageLabel);
    // LUT and Zoom Input at the bottom
    mainLayout->addWidget(LUT);
    // Add LUT

    zoomInput = new QLineEdit(this);
    zoomInput->setStyleSheet("QLineEdit { background-color: rgba(0, 0, 0, 100); color: white; padding: 3px; border-radius: 5px; }");
    zoomInput->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    // Keep alignment as is
    // zoomInput->setText(QString::number(static_cast<int>(currentScale * 100)) + "%");
    // Set text in updateZoomLabel
    zoomInput->setMaximumWidth(60);
    zoomInput->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    QRegularExpression regex(R"(^\d{1,3}%?$)");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regex, this);
    zoomInput->setValidator(validator);
    connect(zoomInput, &QLineEdit::returnPressed, this, &ImageViewer::setZoomFromInput);

    // Add zoomInput using addWidget with alignment
    mainLayout->addWidget(zoomInput, 0, Qt::AlignRight | Qt::AlignBottom);
    // Set the main layout for the widget
    setLayout(mainLayout);
    // Explicitly set layout

    // Display initial image and set size
    if (!originalImage.empty()) {
        imageLabel->setPixmap(QPixmap::fromImage(MatToQImage(originalImage)));
    }
    int initialWidth = originalImage.cols > 0 ? originalImage.cols + 30 : 400;
    // Simplified size calculation
    int initialHeight = originalImage.rows > 0 ?
                            originalImage.rows + (menuBar ? menuBar->height() : 0) + 80 : 300;
    // Adjusted height for menu and bottom widgets
    setGeometry(position.x(), position.y(), initialWidth, initialHeight);


    updateZoomLabel();
    // Set initial zoom text
    updateImage(); // Initial image display, scaling, and dependent updates
}

// Handle window close event
void ImageViewer::closeEvent(QCloseEvent *event) {
    // 1. Close associated histogram window (if open)
    if (histogramWindow) {
        // Closing the window will trigger its WA_DeleteOnClose attribute,
        // which emits destroyed(), calling onHistogramClosed() to nullptr the pointer.
        histogramWindow->close();
        // We don't delete histogramWindow here, WA_DeleteOnClose handles it.
        histogramWindow = nullptr;
        // Defensive nullification
    }

    // 2. Remove this viewer from the MainWindow's list
    if (mainWindow) {
        mainWindow->openedImages.erase(
            std::remove(mainWindow->openedImages.begin(), mainWindow->openedImages.end(), this),
            mainWindow->openedImages.end()
            );
    }

    // 3. Clean up other owned resources (like LUT if it wasn't parented)
    if (LUT && LUT->parent() != this) { // Example check if parenting isn't guaranteed
        delete LUT;
        // Or LUT->deleteLater();
        LUT = nullptr;
    }
    // Note: Widgets parented to 'this' (like LUT, imageLabel, zoomInput, menuBar)
    // will be deleted automatically by Qt's parent-child mechanism when 'this' is deleted.
    // Operations in operationsList might need explicit deletion if not parented.
    qDeleteAll(operationsList);
    // If operations are dynamically allocated and not parented elsewhere. Be cautious.
    operationsList.clear();
    // 4. Accept the event to allow the window to close
    QWidget::closeEvent(event); // Call base class implementation
}


// ======================================================================
// `Menu & Operation Management`
// ======================================================================
// Creates the main menu bar and registers all image operations.
void ImageViewer::createMenu() {
    menuBar = new QMenuBar(this);

    // --- File Menu ---
    QMenu *fileMenu = new QMenu("File", this);
    registerOperation(new ImageOperation("Save As...", this, fileMenu,
                                         ImageOperation::All,
                                         [this]() { this->saveImageAs(); }, false,
                                         QKeySequence::Save));

    // --- Edit Menu ---
    QMenu *editMenu = new QMenu("Edit", this);
    QAction *undoAction = new QAction("Undo", this);
    QAction *redoAction = new QAction("Redo", this);
    QAction *duplicateAction = new QAction("Duplicate Image", this);
    QAction *drawMaskAction = new QAction("Draw Mask", this);

    undoAction->setShortcut(QKeySequence::Undo);
    redoAction->setShortcut(QKeySequence::Redo);
    duplicateAction->setShortcut(QKeySequence("Ctrl+Shift+D"));

    connect(undoAction, &QAction::triggered, this, &ImageViewer::undo);
    connect(redoAction, &QAction::triggered, this, &ImageViewer::redo);
    connect(duplicateAction, &QAction::triggered, this, &ImageViewer::duplicateImage);
    connect(drawMaskAction, &QAction::triggered, this, &ImageViewer::drawMask);

    editMenu->addAction(undoAction);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    editMenu->addAction(duplicateAction);
    editMenu->addAction(drawMaskAction);

    // --- View Menu ---
    QMenu *viewMenu = new QMenu("View", this);
    showHistogramAction = new QAction("Show Histogram", this);
    showHistogramAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(showHistogramAction, &QAction::triggered, this, &ImageViewer::showHistogram);
    viewMenu->addAction(showHistogramAction);
    viewMenu->addSeparator();
    registerOperation(new ImageOperation("Show LUT", this, viewMenu,
                                         ImageOperation::Grayscale,
                                         [this]() { this->toggleLUT(); }, true));

    // --- Processing Menu ---
    QMenu *processingMenu = new QMenu("Processing", this);

    // -- Image Type Submenu --
    QMenu *imageTypeMenu = processingMenu->addMenu("Image Type");
    registerOperation(new ImageOperation("Convert to Grayscale", this, imageTypeMenu,
                                         ImageOperation::Color, [this]() { this->convertToGrayscale(); }));
    registerOperation(new ImageOperation("Remove alpha channel", this, imageTypeMenu,
                                         ImageOperation::RGBA, [this]() { this->removeAlphaChannel(); }));
    registerOperation(new ImageOperation("Convert to HSV", this, imageTypeMenu,
                                         ImageOperation::Color, [this]() { this->convertToHSV(); }));
    registerOperation(new ImageOperation("Convert to Lab", this, imageTypeMenu,
                                         ImageOperation::Color, [this]() { this->convertToLab(); }));
    registerOperation(new ImageOperation("Split Color Channels", this, imageTypeMenu,
                                         ImageOperation::Color, [this]() { this->splitColorChannels(); }));

    // -- Point Operations Submenu --
    QMenu *pointOpsMenu = processingMenu->addMenu("Point Operations");
    registerOperation(new ImageOperation("Apply Negation", this, pointOpsMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyNegation(); }));
    registerOperation(new ImageOperation("Range Stretching...", this, pointOpsMenu,
                                         ImageOperation::Grayscale, [this]() { this->rangeStretching(); }));
    registerOperation(new ImageOperation("Apply Posterization...", this, pointOpsMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyPosterization(); }));
    registerOperation(new ImageOperation("Bitwise Operations...", this, pointOpsMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyBitwiseOperation(); }));
    registerOperation(new ImageOperation("Show Line Profile", this, pointOpsMenu,
                                         ImageOperation::Grayscale, [this]() { this->showLineProfile(); }));

    // -- Thresholding Submenu --
    QMenu *thresholdMenu = processingMenu->addMenu("Thresholding");
    registerOperation(new ImageOperation("Make Binary", this, thresholdMenu,
                                         ImageOperation::Grayscale, [this]() { this->binarise(); }));
    registerOperation(new ImageOperation("Global Threshold...", this, thresholdMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyGlobalThreshold(); }));
    registerOperation(new ImageOperation("Adaptive Threshold", this, thresholdMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyAdaptiveThreshold(); }));
    registerOperation(new ImageOperation("Otsu Threshold", this, thresholdMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyOtsuThreshold(); }));

    // -- Segmentation Submenu --
    QMenu *segmentationMenu = processingMenu->addMenu("Segmentation");
    registerOperation(new ImageOperation("Magic wand...", this, segmentationMenu,
                                         ImageOperation::All, [this]() { this->applyMagicWandSegmentation(); }));
    registerOperation(new ImageOperation("Grab cut...", this, segmentationMenu,
                                         ImageOperation::All, [this]() { this->applyGrabCutSegmentation(); }));
    registerOperation(new ImageOperation("Watershed Segmentation", this, segmentationMenu,
                                         ImageOperation::All, [this]() { this->applyWatershedSegmentation(); }));
    registerOperation(new ImageOperation("Inpaint Image...", this, segmentationMenu,
                                         ImageOperation::All, [this]() { this->applyInpainting(); }));

    // -- Histogram Operations Submenu --
    QMenu *histogramMenu = processingMenu->addMenu("Histogram Operations");
    registerOperation(new ImageOperation("Stretch Histogram", this, histogramMenu,
                                         ImageOperation::Grayscale, [this]() { this->stretchHistogram(); }));
    registerOperation(new ImageOperation("Equalize Histogram", this, histogramMenu,
                                         ImageOperation::Grayscale, [this]() { this->equalizeHistogram(); }));

    // -- Filters Submenu --
    QMenu *filterMenu = processingMenu->addMenu("Filtering");
    registerOperation(new ImageOperation("Apply Blur (3x3)", this, filterMenu,
                                         ImageOperation::All, [this]() { this->applyBlur(); }));
    registerOperation(new ImageOperation("Apply Gaussian Blur (3x3)", this, filterMenu,
                                         ImageOperation::All, [this]() { this->applyGaussianBlur(); }));
    registerOperation(new ImageOperation("Apply Median Filter...", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyMedianFilter(); }));
    filterMenu->addSeparator();
    registerOperation(new ImageOperation("Apply Custom Filter...", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyCustomFilter(); }));
    registerOperation(new ImageOperation("Two-Step Filter (5x5)...", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyTwoStepFilter(); }));

    // -- Edge Detection Submenu --
    QMenu *detectionMenu = processingMenu->addMenu("Edge Detection");
    registerOperation(new ImageOperation("Sobel Edge Detection", this, detectionMenu,
                                         ImageOperation::Grayscale, [this]() { this->applySobelEdgeDetection(); }));
    registerOperation(new ImageOperation("Laplacian Edge Detection", this, detectionMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyLaplacianEdgeDetection(); }));
    registerOperation(new ImageOperation("Canny Edge Detection", this, detectionMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyCannyEdgeDetection(); }));
    registerOperation(new ImageOperation("Prewitt Edge Detection...", this, detectionMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyPrewittEdgeDetection(); }));
    registerOperation(new ImageOperation("Detect Lines (Hough)...", this, detectionMenu,
                                         ImageOperation::All, [this]() { this->applyHoughLineDetection(); }));

    // -- Morphology Submenu --
    QMenu *morphologyMenu = processingMenu->addMenu("Morphology");

    auto createElementSelector = [this](QMenu *parentMenu, StructuringElementType &targetVar, const QString& title) {
        QMenu *elementMenu = new QMenu(title, parentMenu);
        QWidget *widget = new QWidget(elementMenu);
        QVBoxLayout *layout = new QVBoxLayout(widget);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->setSpacing(5);

        QRadioButton *diamondRadio = new QRadioButton("Diamond (4-conn)", widget);
        QRadioButton *squareRadio = new QRadioButton("Square (8-conn)", widget);
        if (targetVar == Diamond) diamondRadio->setChecked(true);
        else squareRadio->setChecked(true);

        layout->addWidget(diamondRadio);
        layout->addWidget(squareRadio);

        connect(diamondRadio, &QRadioButton::toggled, this, [&targetVar](bool checked) {
            if (checked) targetVar = Diamond;
        });
        connect(squareRadio, &QRadioButton::toggled, this, [&targetVar](bool checked) {
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
                                         ImageOperation::Binary, [this]() { this->applyErosion(this->erosionElement); }));

    QMenu *dilationMenu = morphologyMenu->addMenu("Dilation");
    dilationMenu->addMenu(createElementSelector(dilationMenu, dilationElement, "Structuring Element"));
    registerOperation(new ImageOperation("Apply Dilation", this, dilationMenu,
                                         ImageOperation::Binary, [this]() { this->applyDilation(this->dilationElement); }));

    QMenu *openingMenu = morphologyMenu->addMenu("Opening");
    openingMenu->addMenu(createElementSelector(openingMenu, openingElement, "Structuring Element"));
    registerOperation(new ImageOperation("Apply Opening", this, openingMenu,
                                         ImageOperation::Binary, [this]() { this->applyOpening(this->openingElement); }));

    QMenu *closingMenu = morphologyMenu->addMenu("Closing");
    closingMenu->addMenu(createElementSelector(closingMenu, closingElement, "Structuring Element"));
    registerOperation(new ImageOperation("Apply Closing", this, closingMenu,
                                         ImageOperation::Binary, [this]() { this->applyClosing(this->closingElement); }));

    morphologyMenu->addSeparator();
    registerOperation(new ImageOperation("Skeletonize", this, morphologyMenu,
                                         ImageOperation::Binary, [this]() { this->applySkeletonization(); }));

    QMenu *analysisMenu = processingMenu->addMenu("Analysis");
    registerOperation(new ImageOperation("Analyze Shape Features", this, analysisMenu,
                                         ImageOperation::Binary, [this]() { this->analyzeShapeFeatures(); }));

    // --- Add Top-Level Menus to MenuBar ---
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(editMenu);
    menuBar->addMenu(viewMenu);
    menuBar->addMenu(processingMenu);

    mainLayout->setMenuBar(menuBar);

    updateOperationsEnabledState();
}


// Registers an operation for automatic state updates (enabled/disabled).
void ImageViewer::registerOperation(ImageOperation *op) {
    operationsList.append(op);
}

// Updates the enabled/disabled state of all registered operations based on the current image type.
void ImageViewer::updateOperationsEnabledState() {
    using ImageType = ImageOperation::ImageType;
    ImageType type = ImageType::None;

    if (!originalImage.empty()) {
        // Step 1: Handle 4-channel images (e.g., RGBA)
        if (originalImage.channels() == 4) {
            std::vector<cv::Mat> channels(4);
            cv::split(originalImage, channels);

            // Check if the alpha channel is all zeros
            if (cv::countNonZero(channels[3]) == 0) {
                // Discard the alpha channel
                cv::merge(std::vector<cv::Mat>{channels[0], channels[1], channels[2]}, originalImage);
            }
        }

        // Step 2: Check if it's a grayscale image (all channels are equal)
        if (originalImage.channels() == 3) {
            std::vector<cv::Mat> channels(3);
            cv::split(originalImage, channels);

            // Check if all three channels are exactly the same
            cv::Mat cmp1, cmp2;
            cv::compare(channels[0], channels[1], cmp1, cv::CMP_NE); // Non-equal comparison
            cv::compare(channels[0], channels[2], cmp2, cv::CMP_NE);

            if (cv::countNonZero(cmp1) == 0 && cv::countNonZero(cmp2) == 0) {
                // All channels are the same, convert to single-channel grayscale
                originalImage = channels[0];
            }
        }


        // Step 3: Classify the image
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
        } else if (originalImage.channels() == 3) {
            type = ImageType::Color;
        } else if (originalImage.channels() == 4) {
            type = ImageType::RGBA;
        } else {
            type = ImageType::None;
        }
    }


    // Update registered operations
    for (ImageOperation* op : operationsList) {
        if(op) op->updateActionState(type);
    }

    // Update specific actions like histogram and LUT
    bool histogramEnabled = (type & ImageType::Grayscale);
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
        lutAction->setEnabled(type & ImageType::Grayscale);
    }
}


// ======================================================================
// `Internal UI Update Helpers`
// ======================================================================
// Updates the displayed pixmap based on the original image and current scale. Also updates dependent UI elements.
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

    cv::Mat displayImage;
    if (showingMaskMode && !drawnMask.empty() && drawnMask.size() == originalImage.size()) {
        cv::Mat display;

        if (originalImage.channels() == 4) {
            cv::cvtColor(originalImage, display, cv::COLOR_BGRA2BGR);
        } else if (originalImage.channels() == 1) {
            cv::cvtColor(originalImage, display, cv::COLOR_GRAY2BGR);
        } else {
            display = originalImage.clone();
        }

        cv::Mat maskColored;
        cv::cvtColor(drawnMask, maskColored, cv::COLOR_GRAY2BGR);
        cv::addWeighted(display, 1.0, maskColored, 0.5, 0, display);
        displayImage = display;
    } else {
        displayImage = originalImage.clone();
    }

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
        pixmap = QPixmap::fromImage(qimg);
        // Already scaled, no additional scaling
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
    if(selectingPoints) drawTemporaryPoints();
}

// Updates the text in the zoom percentage input field.
void ImageViewer::updateZoomLabel() {
    if (!zoomInput) return;
    // Safety check
    int zoomPercentage = static_cast<int>(std::round(currentScale * 100.0));
    zoomInput->setText(QString::number(zoomPercentage) + "%");
    // zoomInput->adjustSize();
    // Let layout handle size, just set max width
}

// Updates the histogram data in the separate HistogramWidget window, if it exists.
void ImageViewer::updateHistogram() {
    // Check if the histogram window pointer is valid (i.e., window exists)
    if (histogramWindow) {
        // Ensure image is grayscale before computing histogram
        cv::Mat grayImage;
        if (originalImage.channels() == 1) {
            grayImage = originalImage;
            // Already grayscale/binary
        } else if (originalImage.channels() >= 3) {
            grayImage = ImageProcessing::convertToGrayscale(originalImage);
        } else {
            // Handle unexpected channel count - clear histogram
            histogramWindow->computeHistogram(cv::Mat());
            // Pass empty Mat
            return;
        }

        if (!grayImage.empty()) {
            histogramWindow->computeHistogram(grayImage);
        } else {
            // Handle case where grayscale conversion failed
            histogramWindow->computeHistogram(cv::Mat());
            // Pass empty Mat
        }
    }
}

// Updates the embedded QTableWidget (LUT) with histogram data for grayscale images.
void ImageViewer::updateHistogramTable() {
    if (!LUT || !LUT->isVisible() || originalImage.empty() || originalImage.channels() != 1) {
        // Clear table if not visible, no image, or not grayscale
        if (LUT) {
            LUT->clearContents();
            // Clear data but keep headers
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
    LUT->setColumnCount(256);
    // Ensure correct column count
    for (int i = 0; i < 256; ++i) {
        QTableWidgetItem *item = LUT->item(0, i);
        // Try to reuse existing item
        if (!item) {
            item = new QTableWidgetItem(QString::number(histogramDataVec[i]));
            LUT->setItem(0, i, item);
        } else {
            item->setText(QString::number(histogramDataVec[i]));
        }
    }
}

// Temporarily displays an image (scaled to current view) without adding to undo stack. Used for previews.
void ImageViewer::showTempImage(const cv::Mat &temp) {
    if (temp.empty() || !imageLabel) return;
    // Safety checks

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

// Draws currently selected points and lines/rectangles on a temporary image overlay.
void ImageViewer::drawTemporaryPoints() {
    cv::Mat displayImage;
    if (originalImage.channels() == 1)
        cv::cvtColor(originalImage, displayImage, cv::COLOR_GRAY2BGR);
    else
        displayImage = originalImage.clone();

    int lineThickness = std::max(1, static_cast<int>(std::round(std::max(displayImage.cols, displayImage.rows) / 500.0)));
    for (const auto& pt : selectedPoints) {
        cv::circle(displayImage, pt, lineThickness, cv::Scalar(0, 255, 0), -1);
    }

    if (selectedPoints.size() == 2) {
        if (!rectangleMode) {
            cv::line(displayImage, selectedPoints[0], selectedPoints[1], cv::Scalar(0, 0, 255), lineThickness);
            // Red line
        } else {
            // Calculate top-left and size
            cv::Point p1 = selectedPoints[0];
            cv::Point p2 = selectedPoints[1];
            cv::Point topLeft(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
            cv::Point bottomRight(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
            // Draw rectangle
            cv::rectangle(displayImage, topLeft, bottomRight, cv::Scalar(0, 255, 0), lineThickness);
            // Green rectangle
        }
    }

    showTempImage(displayImage);
}


// ======================================================================
// `Internal UI Slots`
// ======================================================================
// Handles mouse clicks on the image label, primarily for point selection modes.
void ImageViewer::onImageClicked(QMouseEvent* event) {
    if (magicWandMode) {
        QPoint clickPos = event->pos();
        QSize pixmapSize = imageLabel->pixmap(Qt::ReturnByValue).size();
        if (pixmapSize.isEmpty()) return;

        double xScale = static_cast<double>(originalImage.cols) / pixmapSize.width();
        double yScale = static_cast<double>(originalImage.rows) / pixmapSize.height();
        int imgX = std::clamp(static_cast<int>(std::round(clickPos.x() * xScale)), 0, originalImage.cols - 1);
        int imgY = std::clamp(static_cast<int>(std::round(clickPos.y() * yScale)), 0, originalImage.rows - 1);
        // Wykonaj segmentację
        cv::Mat mask = ImageProcessing::magicWandSegmentation(originalImage, cv::Point(imgX, imgY), 15);
        // tolerancja 15
        showTempImage(mask * 255);
        // maska jako obraz binarny

        magicWandMode = false;
        // Wyłącz po jednym użyciu
    }


    if (!selectingPoints || pointsToSelect <= 0 || originalImage.empty())
        return;
    QPoint clickPos = event->pos();
    QSize pixmapSize = imageLabel->pixmap(Qt::ReturnByValue).size();
    if (pixmapSize.isEmpty()) return;

    double xScale = static_cast<double>(originalImage.cols) / pixmapSize.width();
    double yScale = static_cast<double>(originalImage.rows) / pixmapSize.height();

    int imgX = std::clamp(static_cast<int>(std::round(clickPos.x() * xScale)), 0, originalImage.cols - 1);
    int imgY = std::clamp(static_cast<int>(std::round(clickPos.y() * yScale)), 0, originalImage.rows - 1);

    selectedPoints.push_back(cv::Point(imgX, imgY));
    if (selectedPoints.size() > pointsToSelect) {
        selectedPoints.erase(selectedPoints.begin());
        // Optionally disable more clicks here if you don't want extra points
        //selectingPoints = false;
    }

    drawTemporaryPoints(); // <- VISUAL FEEDBACK

}

void ImageViewer::mousePressEvent(QMouseEvent* event) {
    if (drawingMaskMode && event->button() == Qt::LeftButton && imageLabel && imageLabel->underMouse()) {
        // Map position from ImageViewer widget coordinates to imageLabel coordinates
        QPoint relativePos = imageLabel->mapFrom(this, event->pos());

        // Check if the mapped position is within the actual pixmap bounds displayed in the label
        QPixmap currentPixmapPtr = imageLabel->pixmap();
        if (!currentPixmapPtr.isNull() && currentPixmapPtr.rect().contains(relativePos)) {
            lastDrawPos = QPoint(); // Reset last position for the start of a new stroke
            drawOnMask(relativePos); // Draw the first point/circle
            event->accept(); // Indicate the event was handled
            return; // Don't call base class if handled
        }
    }
    // Handle other modes (point selection, etc.) or call base class
    else if (selectingPoints) {
        // Handle point selection logic...
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event); // Call base class for other cases
}

void ImageViewer::mouseReleaseEvent(QMouseEvent* event) {
    if (drawingMaskMode && event->button() == Qt::LeftButton) {
        lastDrawPos = QPoint(); // Invalidate last position when mouse is released, ending the stroke
        event->accept();
        return;
    }
    // Handle other modes or call base class
    else if (selectingPoints && event->button() == Qt::LeftButton) {
        // Finalize point placement/drag...
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void ImageViewer::mouseMoveEvent(QMouseEvent* event) {
    if (drawingMaskMode && (event->buttons() & Qt::LeftButton) && imageLabel && imageLabel->underMouse()) {
        QPoint relativePos = imageLabel->mapFrom(this, event->pos());

        // Only draw if the cursor is over the displayed pixmap area
        QPixmap currentPixmapPtr = imageLabel->pixmap();
        if (!currentPixmapPtr.isNull() && currentPixmapPtr.rect().contains(relativePos)) {
            // Check if lastDrawPos is valid (mouse didn't leave/re-enter image)
            if (!lastDrawPos.isNull()) {
                drawOnMask(relativePos);
            } else {
                // If lastDrawPos is null but button is down, treat as start of new stroke
                lastDrawPos = QPoint(); // Ensure it's reset before drawing first point
                drawOnMask(relativePos);
            }
            event->accept();
            return;
        } else {
            // If mouse moves off the image while button is held, invalidate last pos
            // so drawing stops until it re-enters.
            lastDrawPos = QPoint();
        }
    }
    // Handle other modes or call base class
    else if (selectingPoints && (event->buttons() & Qt::LeftButton)) {
        // Handle point dragging logic...
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}


bool ImageViewer::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QMenu *menu = qobject_cast<QMenu *>(watched);
        if (menu) {
            QAction *action = menu->actionAt(helpEvent->pos());
            if (action && !action->toolTip().isEmpty()) {
                QToolTip::showText(helpEvent->globalPos(), action->toolTip());
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}


void ImageViewer::drawOnMask(const QPoint& widgetPos) {
    // Guard clauses
    if (!drawingMaskMode || originalImage.empty() || drawnMask.empty() || widgetPos.isNull() || !imageLabel) return;

    QPixmap currentPixmap = imageLabel->pixmap(Qt::ReturnByValue);
    if (currentPixmap.isNull()) return;

    // Calculate scale factors based on the displayed pixmap size vs original image size
    QSize pixmapSize = currentPixmap.size();
    if (pixmapSize.width() == 0 || pixmapSize.height() == 0) return; // Prevent division by zero

    double xScale = static_cast<double>(originalImage.cols) / pixmapSize.width();
    double yScale = static_cast<double>(originalImage.rows) / pixmapSize.height();

    // Convert current widget position to image coordinates, clamping to bounds
    int imgX = std::clamp(static_cast<int>(std::round(widgetPos.x() * xScale)), 0, originalImage.cols - 1);
    int imgY = std::clamp(static_cast<int>(std::round(widgetPos.y() * yScale)), 0, originalImage.rows - 1);
    cv::Point current(imgX, imgY);

    // Use lastDrawPos for drawing lines; handle initial point separately
    if (!lastDrawPos.isNull()) {
        // Convert previous widget position to image coordinates
        int lastImgX = std::clamp(static_cast<int>(std::round(lastDrawPos.x() * xScale)), 0, originalImage.cols - 1);
        int lastImgY = std::clamp(static_cast<int>(std::round(lastDrawPos.y() * yScale)), 0, originalImage.rows - 1);
        cv::Point previous(lastImgX, lastImgY);

        // Draw line on the mask using currentBrushThickness
        // LINE_8 is generally faster for thicker lines, LINE_AA is anti-aliased
        cv::line(drawnMask, previous, current, cv::Scalar(255), currentBrushThickness, cv::LINE_8);
    } else {
        // For the very first point of a stroke, draw a filled circle to make it visible immediately
        cv::circle(drawnMask, current, currentBrushThickness / 2, cv::Scalar(255), /*thickness*/ -1, cv::LINE_8);
    }

    // Update last position *after* drawing for the next segment
    lastDrawPos = widgetPos;

    updateImage();
}

void ImageViewer::setBrushThickness(int thickness) {
    currentBrushThickness = std::max(1, thickness); // Ensure thickness is at least 1 pixel
}

// Sets the zoom level based on the value entered in the zoom input field.
void ImageViewer::setZoomFromInput() {
    if (!zoomInput) return;
    // Safety check
    QString text = zoomInput->text();
    text.remove('%');
    // Remove percentage sign if present
    bool ok;
    int zoomValue = text.toInt(&ok);

    zoomInput->clearFocus();
    // Remove focus after pressing Enter

    if (ok && zoomValue >= 10 && zoomValue <= 500) { // Zoom range 10% to 500%
        double newScale = zoomValue / 100.0;
        if (std::abs(newScale - currentScale) > 1e-6) { // Only update if scale actually changes
            currentScale = newScale;
            updateImage();
        } else {
            updateZoomLabel();
            // Restore text if value was same but format different (e.g., added %)
        }
    } else {
        QMessageBox::warning(this, "Invalid Input", "Please enter a zoom value between 10% and 500%.");
        updateZoomLabel(); // Reset text to current valid zoom
    }
}

// Creates or raises the separate histogram window.
void ImageViewer::showHistogram() {
    if (!histogramWindow) { // If no window exists or it was destroyed
        histogramWindow = new HistogramWidget();
        // Create as a top-level window

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

// Slot called when the separate histogram window is closed/destroyed.
void ImageViewer::onHistogramClosed() {
    // The signal is emitted when the object is about to be destroyed,
    // or already is.
    // Just nullify the pointer.
    histogramWindow = nullptr;
    // Update the button state (if needed, though current logic doesn't disable when open)
    // updateOperationsEnabledState();
    // Uncomment if button state depends on window existence
}

// Toggles the visibility of the embedded LUT (histogram table).
void ImageViewer::toggleLUT() {
    QAction* lutAction = nullptr;
    for(ImageOperation* op : operationsList) {
        if(op && op->getAction() && op->getAction()->text() == "Show LUT") {
            lutAction = op->getAction();
            break;
        }
    }

    if (!LUT) return;
    // Safety check

    if (LUT->isVisible()) {
        LUT->hide();
        if(lutAction) lutAction->setChecked(false);
    } else {
        LUT->show();
        if(lutAction) lutAction->setChecked(true);
        updateHistogramTable();
    }
    // Adjusting size might still be needed if LUT significantly changes window height needs
    adjustSize();
}

// Enables point selection mode and sets the cursor.
void ImageViewer::enablePointSelection() {
    selectingPoints = true;
    menuBar->setEnabled(false);
    setCursor(Qt::CrossCursor);
}

// Disables point selection mode and resets the cursor.
void ImageViewer::disablePointSelection() {
    selectingPoints = false;
    menuBar->setEnabled(true);
    unsetCursor();
}

void ImageViewer::enableMaskDrawing() {
    if (originalImage.empty()) {
        qWarning("ImageViewer::enableMaskDrawing: Cannot draw mask, no image loaded.");
        return;
    }

    drawingMaskMode = true;
    QApplication::setOverrideCursor(Qt::CrossCursor); // Use application override for consistency
    if(menuBar) menuBar->setEnabled(false); // Disable menu during drawing

    // Initialize mask if it's empty or has the wrong size
    if (drawnMask.empty() || drawnMask.size() != originalImage.size()) {
        drawnMask = cv::Mat::zeros(originalImage.size(), CV_8UC1); // Ensure 8-bit single channel
    }
    // Reset last position for a new drawing session
    lastDrawPos = QPoint();
    updateImage();
}

void ImageViewer::disableMaskDrawing() {
    if (drawingMaskMode) {
        drawingMaskMode = false;
        QApplication::restoreOverrideCursor(); // Restore normal cursor
        if(menuBar) menuBar->setEnabled(true); // Re-enable menu
        lastDrawPos = QPoint(); // Clear last position
    }
}

void ImageViewer::clearDrawnMask() {
    if (!drawnMask.empty()) {
        drawnMask.setTo(cv::Scalar(0)); // Set all pixels to 0
        // If drawing mode is active, update the display immediately to show cleared mask
        if (drawingMaskMode) {
            updateImage();
        }
    }
}

void ImageViewer::enableMaskShowing() {
    if(!showingMaskMode) {
        showingMaskMode = true;
        updateImage();
    }
}

void ImageViewer::disableMaskShowing() {
    if(showingMaskMode) {
        showingMaskMode = false;
        updateImage();
    }
}


// ======================================================================
// `Event Handlers`
// ======================================================================
// Handles mouse wheel events for zooming the image.
void ImageViewer::wheelEvent(QWheelEvent *event) {
    double oldScale = currentScale;
    if (usePyramidScaling) {
        // Only allow switching between 0.25, 0.5, 1.0, 2.0, 4.0
        static const std::vector<double> pyramidScales = {0.25, 0.5, 1.0, 2.0, 4.0};
        auto it = std::find(pyramidScales.begin(), pyramidScales.end(), currentScale);
        if (it != pyramidScales.end()) {
            if (event->angleDelta().y() > 0 && it + 1 != pyramidScales.end()) {
                currentScale = *(it + 1);
                // Zoom in
            } else if (event->angleDelta().y() < 0 && it != pyramidScales.begin()) {
                currentScale = *(it - 1);
                // Zoom out
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


// ======================================================================
// `Undo/Redo Management`
// ======================================================================
// Pushes the current image state onto the undo stack.
void ImageViewer::pushToUndoStack() {
    if (!originalImage.empty()) {
        // Consider using QStack for potentially better Qt integration, though std::stack is fine
        const int MAX_UNDO_LEVELS = 20;
        // Keep limit reasonable
        // If using std::stack, we need to manage the size manually if a limit is desired
        if (undoStack.size() >= MAX_UNDO_LEVELS) {
            // To implement a limit with stack, we'd need to pop from the "bottom",
            // which stack doesn't support.
            // Easiest is to use std::deque or QQueue/QStack
            // Or just let it grow, assuming memory isn't a huge concern for typical use.
            qWarning() << "Undo stack limit potentially exceeded. Oldest states not automatically removed.";
        }
        undoStack.push(originalImage.clone());
        clearRedoStack();
        // Clear redo whenever a new action is performed
    }
}

// Reverts to the previous image state from the undo stack.
void ImageViewer::undo() {
    if (!undoStack.empty()) {
        redoStack.push(originalImage.clone());
        // Push current state to redo
        originalImage = undoStack.top();
        undoStack.pop();
        updateImage();
    }
}

// Re-applies an undone image state from the redo stack.
void ImageViewer::redo() {
    if (!redoStack.empty()) {
        undoStack.push(originalImage.clone());
        // Push current state to undo
        originalImage = redoStack.top();
        redoStack.pop();
        updateImage();
    }
}

// Clears the redo stack.
void ImageViewer::clearRedoStack() {
    // Use QStack's clear() or std::stack's loop pop
    while (!redoStack.empty()) redoStack.pop();
    // redoStack.clear(); // If using QStack
}


// ======================================================================
// `File & View Operations Slots`
// ======================================================================
// Creates a new ImageViewer instance with a copy of the current image.
void ImageViewer::duplicateImage() {
    if (originalImage.empty() || !mainWindow) return;
    static int duplicateCount = 1;
    QString newTitle = QString("%1 - Copy %2").arg(this->windowTitle()).arg(duplicateCount++);
    // Use this->windowTitle()
    QPoint newPos = this->pos() + QPoint(150, 150);
    ImageViewer *newViewer = new ImageViewer(originalImage.clone(),
                                             newTitle, nullptr, // Parent is null for new top-level window
                                             newPos, mainWindow);
    newViewer->setZoom(currentScale); // Apply current zoom to duplicate
    newViewer->show();
    newViewer->undoStack = std::stack<cv::Mat>(undoStack);
    newViewer->redoStack = std::stack<cv::Mat>(redoStack);
    newViewer->setBrushThickness(currentBrushThickness);
    newViewer->setUsePyramidScaling(usePyramidScaling);
    newViewer->drawnMask = drawnMask;
    newViewer->lastDrawPos = lastDrawPos;
    newViewer->updateImage();
}

void ImageViewer::drawMask() {
    InpaintingDialog* inpaintDialog = new InpaintingDialog(this, mainWindow->openedImages, mainWindow);
    inpaintDialog->setAttribute(Qt::WA_DeleteOnClose);
    enableMaskShowing();
    inpaintDialog->show();

    connect(inpaintDialog, &InpaintingDialog::maskChanged, this, [=]() {
        drawnMask = inpaintDialog->getSelectedMask();
        updateImage();
    });

    connect(inpaintDialog, &QDialog::accepted, this, [=]() {
        disableMaskDrawing();
        disableMaskShowing();
        clearMask();
        updateImage();
    });

    connect(inpaintDialog, &QDialog::rejected, this, [=]() {
        disableMaskDrawing();
        disableMaskShowing();
        clearMask();
        updateImage();
    });
}


std::vector<std::pair<uchar, int>> compressRLE(const cv::Mat& image) {
    std::vector<std::pair<uchar, int>> rleData;

    for (int i = 0; i < image.rows; ++i) {
        const uchar* row = image.ptr<uchar>(i);
        int count = 1;
        uchar prev = row[0];

        for (int j = 1; j < image.cols; ++j) {
            if (row[j] == prev) {
                ++count;
            } else {
                rleData.emplace_back(prev, count);
                prev = row[j];
                count = 1;
            }
        }
        rleData.emplace_back(prev, count);
    }

    return rleData;
}

std::vector<std::pair<cv::Vec3b, int>> compressColorRLE(const cv::Mat& image) {
    std::vector<std::pair<cv::Vec3b, int>> rleData;

    for (int i = 0; i < image.rows; ++i) {
        const cv::Vec3b* row = image.ptr<cv::Vec3b>(i);
        cv::Vec3b prev = row[0];
        int count = 1;

        for (int j = 1; j < image.cols; ++j) {
            if (row[j] == prev) {
                ++count;
            } else {
                rleData.emplace_back(prev, count);
                prev = row[j];
                count = 1;
            }
        }
        rleData.emplace_back(prev, count);
    }

    return rleData;
}

bool saveRLEToFile(const std::vector<std::pair<uchar, int>>& rleData,
                   const QString& filePath, int width, int height) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << "grayscale " << width << " " << height << "\n";
    for (const auto& [val, count] : rleData) {
        out << static_cast<int>(val) << " " << count << "\n";
    }

    return true;
}


bool saveColorRLEToFile(const std::vector<std::pair<cv::Vec3b, int>>& rleData,
                        const QString& filePath, int width, int height) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    out << "color " << width << " " << height << "\n";
    for (const auto& [val, count] : rleData) {
        out << static_cast<int>(val[0]) << " "
            << static_cast<int>(val[1]) << " "
            << static_cast<int>(val[2]) << " "
            << count << "\n";
    }

    return true;
}


double computeCompressionRatio(const cv::Mat& original, const std::vector<std::pair<uchar, int>>& rleData) {
    int originalSize = original.rows * original.cols;
    int compressedSize = rleData.size() * (sizeof(uchar) + sizeof(int));
    return static_cast<double>(originalSize) / compressedSize;
}

double computeColorCompressionRatio(const cv::Mat& original, const std::vector<std::pair<cv::Vec3b, int>>& rleData) {
    int originalSize = original.rows * original.cols * 3; // każdy piksel = 3 bajty
    int compressedSize = rleData.size() * (3 * sizeof(uchar) + sizeof(int));
    return static_cast<double>(originalSize) / compressedSize;
}

// Opens a file dialog to save the current image.
void ImageViewer::saveImageAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Image As", "",
                                                    "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.rle)");
    if (filePath.isEmpty()) return;

#ifdef _WIN32
    std::string path = filePath.toLocal8Bit().constData();
#else
    std::string path = filePath.toUtf8().constData();
#endif

    std::vector<int> compression_params;

    if (filePath.endsWith(".rle", Qt::CaseInsensitive)) {
        if (originalImage.channels() == 1) {
            auto rleData = compressRLE(originalImage);
            double sk = computeCompressionRatio(originalImage, rleData);
            if (!saveRLEToFile(rleData, filePath, originalImage.cols, originalImage.rows)) {
                QMessageBox::critical(this, "Save Error", "Could not save grayscale RLE data to file.");
            } else {
                QMessageBox::information(this, "Compression Done",
                                         QString("Grayscale image compressed.\nCompression ratio: %1").arg(sk, 0, 'f', 2));
            }
        } else if (originalImage.channels() == 3) {
            auto rleData = compressColorRLE(originalImage);
            double sk = computeColorCompressionRatio(originalImage, rleData);
            if (!saveColorRLEToFile(rleData, filePath, originalImage.cols, originalImage.rows)) {
                QMessageBox::critical(this, "Save Error", "Could not save color RLE data to file.");
            } else {
                QMessageBox::information(this, "Compression Done",
                                         QString("Color image compressed.\nCompression ratio: %1").arg(sk, 0, 'f', 2));
            }
        } else {
            QMessageBox::warning(this, "Unsupported Format", "Only grayscale and 3-channel color images are supported for RLE.");
        }
        return;
    }

    // Standard image formats
    if (filePath.endsWith(".jpg", Qt::CaseInsensitive) || filePath.endsWith(".jpeg", Qt::CaseInsensitive)) {
        compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        compression_params.push_back(95);
    } else if (filePath.endsWith(".png", Qt::CaseInsensitive)) {
        compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(3);
    }

    try {
        if (!cv::imwrite(path, originalImage, compression_params)) {
            QMessageBox::warning(this, "Save Error", "Failed to save the image. Check file path and permissions.");
        }
    } catch (const cv::Exception& ex) {
        QMessageBox::critical(this, "OpenCV Save Error", QString("Error saving image: %1").arg(ex.what()));
    }
}


// ======================================================================
// `Image Type Conversion Slots`
// ======================================================================
// Converts the current image to grayscale.
void ImageViewer::convertToGrayscale() {
    pushToUndoStack();
    originalImage = ImageProcessing::convertToGrayscale(originalImage);
    updateImage();
}


void ImageViewer::removeAlphaChannel() {
    pushToUndoStack();
    originalImage = ImageProcessing::removeAlphaChannel(originalImage);
    updateImage();
}

// Converts the current grayscale image to binary using a default threshold.
void ImageViewer::binarise() {
    pushToUndoStack();
    originalImage = ImageProcessing::binarise(originalImage);
    updateImage();
}

// Splits a color image into its B, G, R channels, displaying each in a new window.
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

// Converts a color image to HSV color space, displaying channels each in a new window.
void ImageViewer::convertToHSV() {
    if (!mainWindow || originalImage.channels() < 3) { // Added channel check
        QMessageBox::warning(this, "Split Error", "Cannot split channels. Image must be 3 or 4 channel color.");
        return;
    }
    std::vector<cv::Mat> channels = ImageProcessing::convertToHSV(originalImage);
    if (channels.size() >= 3) { // Allow for 4 channels (ignore alpha maybe?)
        (new ImageViewer(channels[0], windowTitle() + " - Hue", nullptr, pos() + QPoint(30, 30), mainWindow))->show();
        (new ImageViewer(channels[1], windowTitle() + " - Saturation", nullptr, pos() + QPoint(60, 60), mainWindow))->show();
        (new ImageViewer(channels[2], windowTitle() + " - Value", nullptr, pos() + QPoint(90, 90), mainWindow))->show();
    } else {
        QMessageBox::warning(this, "Split Error", "Failed to split channels.");
    }
}

// Converts a color image to Lab color space, displaying each channel in a new window.
void ImageViewer::convertToLab() {
    if (!mainWindow || originalImage.channels() < 3) { // Added channel check
        QMessageBox::warning(this, "Split Error", "Cannot split channels. Image must be 3 or 4 channel color.");
        return;
    }
    std::vector<cv::Mat> channels = ImageProcessing::convertToLab(originalImage);
    if (channels.size() >= 3) { // Allow for 4 channels (ignore alpha maybe?)
        (new ImageViewer(channels[0], windowTitle() + " - Lightness", nullptr, pos() + QPoint(30, 30), mainWindow))->show();
        (new ImageViewer(channels[1], windowTitle() + " - a*", nullptr, pos() + QPoint(60, 60), mainWindow))->show();
        (new ImageViewer(channels[2], windowTitle() + " - b*", nullptr, pos() + QPoint(90, 90), mainWindow))->show();
    } else {
        QMessageBox::warning(this, "Split Error", "Failed to split channels.");
    }
}

// ======================================================================
// `Histogram Operations Slots`
// ======================================================================
// Applies histogram stretching to the grayscale image.
void ImageViewer::stretchHistogram() {
    pushToUndoStack();
    originalImage = ImageProcessing::stretchHistogram(originalImage);
    updateImage();
}

// Applies histogram equalization to the grayscale image.
void ImageViewer::equalizeHistogram() {
    pushToUndoStack();
    originalImage = ImageProcessing::equalizeHistogram(originalImage);
    updateImage();
}


// ======================================================================
// `Point Operations Slots`
// ======================================================================
// Applies negation (inversion) to the grayscale image.
void ImageViewer::applyNegation() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyNegation(originalImage);
    updateImage();
}

// Opens a dialog for applying range stretching to the grayscale image.
void ImageViewer::rangeStretching() {
    RangeStretchingDialog dialog(this);
    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::applyRangeStretching(originalImage, dialog.getP1(),
                                                     dialog.getP2(), dialog.getQ3(), dialog.getQ4());
    });
    dialog.exec();
}

// Opens a dialog for applying posterization to the grayscale image.
void ImageViewer::applyPosterization() {
    InputDialog dialog(this);
    auto *levelSpin = new QSpinBox;
    levelSpin->setRange(2, 256);
    levelSpin->setValue(4);
    dialog.addInput("Levels", levelSpin);
    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::applyPosterization(originalImage, dialog.getValue("Levels").toInt());
    });

    dialog.exec();
}

// Opens a dialog for applying bitwise operations between the current image and another open image.
void ImageViewer::applyBitwiseOperation() {
    if (originalImage.empty() || !mainWindow) return;
    BitwiseOperationDialog dialog(this, mainWindow->openedImages);
    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return dialog.getResult();
    });
    dialog.exec();
}

// Initiates the process for selecting two points and displaying the line profile between them.
void ImageViewer::showLineProfile() {
    if (originalImage.empty() || originalImage.channels() != 1) {
        QMessageBox::warning(this, "Invalid Image", "Line profile requires a grayscale image.");
        return;
    }

    selectedPoints.clear();
    pointsToSelect = 2;
    enablePointSelection();

    PointSelectionDialog* dialog = new PointSelectionDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    connect(dialog, &QDialog::accepted, this, [=]() {
        disablePointSelection();
        updateImage();
        if (selectedPoints.size() == 2) {
            drawLineProfile(selectedPoints[0], selectedPoints[1]);
        }
    });
    connect(dialog, &QDialog::rejected, this, [=]() {
        disablePointSelection();
        updateImage();
    });
}

// Extracts pixel values along a line and displays them in a chart dialog.
void ImageViewer::drawLineProfile(const cv::Point& p1, const cv::Point& p2) {
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
    showTempImage(displayImage);
    // Show the image with the line TEMPORARILY
    // --- End temporary line ---

    // --- Extract pixel values along the line from the ORIGINAL image ---
    std::vector<uchar> values;
    try {
        // Use LineIterator on the original CV_8UC1 image
        cv::LineIterator it(originalImage, p1, p2, 8);
        // 8-connected iterator
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
        axisX->setRange(0, values.size() > 0 ? values.size() - 1 : 0);
        // Handle empty values case
        axisX->setTitleText("Distance along line (pixels)");
        axisX->setLabelFormat("%d");
        // Integer labels
    }
    if(axisY) {
        axisY->setRange(0, 255);
        // Intensity range
        axisY->setTitleText("Pixel Intensity");
        axisY->setLabelFormat("%d");
    }

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    // --- End plot setup ---

    // --- Show plot in a Dialog ---
    QDialog *dialog = new QDialog(this);
    // Parent dialog to image viewer
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    // Ensure dialog is deleted
    dialog->setWindowTitle("Line Profile Plot");
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(chartView);
    dialog->resize(600, 400);
    // Position dialog relative to main window or image viewer
    dialog->move(this->mapToGlobal(QPoint(200, 50)));
    // Connect dialog finished signal to restore the original image view
    // Using lambda to capture 'this' safely
    connect(dialog, &QDialog::finished, this, [this]() {
        this->updateImage(); // Restore original image display
    });
    dialog->show(); // Show non-modally
}

// Opens a dialog for applying global thresholding to the grayscale image.
void ImageViewer::applyGlobalThreshold() {
    InputDialog dialog(this);
    auto *levelSpin = new QSpinBox;
    levelSpin->setRange(0, 255);
    levelSpin->setValue(128);
    dialog.addInput("Threshold", levelSpin);
    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::applyGlobalThreshold(originalImage, dialog.getValue("Threshold").toInt());
    });

    dialog.exec();
}

// Applies adaptive thresholding to the grayscale image.
void ImageViewer::applyAdaptiveThreshold() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyAdaptiveThreshold(originalImage);
    updateImage();
}

// Applies Otsu's thresholding to the grayscale image.
void ImageViewer::applyOtsuThreshold() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyOtsuThreshold(originalImage);
    updateImage();
}

// Activates magic wand mode for a single click selection.
void ImageViewer::activateMagicWandTool() {
    magicWandMode = true;
    QMessageBox::information(this, "Magic Wand", "Click on the image to select a region.");
}

// Initiates the magic wand segmentation process after point selection.
void ImageViewer::applyMagicWandSegmentation() {
    if (originalImage.empty()) {
        QMessageBox::warning(this, "Invalid Image", "Image is empty.");
        return;
    }

    selectedPoints.clear();
    pointsToSelect = 1;
    enablePointSelection();

    PointSelectionDialog* dialog = new PointSelectionDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    connect(dialog, &QDialog::accepted, this, [=]() {
        disablePointSelection();
        updateImage();
        if (selectedPoints.size() == 1) {
            InputDialog dialog(this);
            auto *levelSpin = new QSpinBox;
            levelSpin->setRange(0, 255);
            levelSpin->setValue(15);
            dialog.addInput("Tolerance", levelSpin);
            auto *modesCombo = new QComboBox;
            modesCombo->addItems({"Mask", "Masked image"});
            dialog.addInput("Output mode", modesCombo);

            setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
                cv::Mat previewSource;

                if (originalImage.channels() == 4) {
                    cv::cvtColor(originalImage, previewSource, cv::COLOR_BGRA2BGR);
                }
                else {
                    previewSource = originalImage.clone();
                }
                cv::Mat mask = ImageProcessing::magicWandSegmentation(originalImage, selectedPoints[0], dialog.getValue("Tolerance").toInt());
                if(dialog.getValue("Output mode").toString() == "Masked image") {
                    cv::Mat maskedImage = cv::Mat::zeros(previewSource.size(), previewSource.type());
                    previewSource.copyTo(maskedImage, mask);
                    return maskedImage;
                }
                else {
                    return mask;
                }
            });

            dialog.exec();
        }
    });
    connect(dialog, &QDialog::rejected, this, [=]() {
        disablePointSelection();
        updateImage();
    });
}

// Initiates the GrabCut segmentation process after rectangle selection.
void ImageViewer::applyGrabCutSegmentation() {
    if (originalImage.empty()) {
        QMessageBox::warning(this, "Invalid Image", "Image is empty.");
        return;
    }
    selectedPoints.clear();
    pointsToSelect = 2;
    enablePointSelection();
    rectangleMode = true;

    PointSelectionDialog* dialog = new PointSelectionDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    connect(dialog, &QDialog::accepted, this, [=]() {
        disablePointSelection();
        rectangleMode = false;
        updateImage();
        if (selectedPoints.size() == 2) {
            InputDialog dialog(this);
            auto *levelSpin = new QSpinBox;
            levelSpin->setRange(0, 20);
            levelSpin->setValue(5);
            dialog.addInput("Iterations", levelSpin);

            setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
                cv::Point p1 = selectedPoints[0];
                cv::Point p2 = selectedPoints[1];
                int x = std::min(p1.x, p2.x);
                int y = std::min(p1.y, p2.y);
                int width = std::abs(p1.x - p2.x);
                int height = std::abs(p1.y - p2.y);
                cv::Rect rect(x, y, width, height);
                return ImageProcessing::grabCutSegmentation(originalImage, rect, dialog.getValue("Iterations").toInt());

            });

            dialog.exec();
        }
    });
    connect(dialog, &QDialog::rejected, this, [=]() {
        disablePointSelection();
        rectangleMode = false;
        updateImage();
    });
}

//Applies Watershed algorithm for current image
void ImageViewer::applyWatershedSegmentation() {
    if (originalImage.empty()) return;

    pushToUndoStack();
    originalImage = ImageProcessing::applyWatershedSegmentation(originalImage);
    updateImage();
}

void ImageViewer::applyInpainting() {
    if (originalImage.empty()) {
        QMessageBox::warning(this, "Inpainting", "No image loaded.");
        return;
    }
    if (!mainWindow) {
        QMessageBox::critical(this, "Inpainting Error", "Main window context is missing. Cannot proceed.");
        return;
    }

    InpaintingDialog* inpaintDialog = new InpaintingDialog(this, mainWindow->openedImages, mainWindow);
    inpaintDialog->setAttribute(Qt::WA_DeleteOnClose);
    enableMaskShowing();
    inpaintDialog->show();

    connect(inpaintDialog, &InpaintingDialog::maskChanged, this, [=]() {
        drawnMask = inpaintDialog->getSelectedMask();
        updateImage();
    });
    connect(inpaintDialog, &QDialog::accepted, this, [=]() {
        cv::Mat mask = inpaintDialog->getSelectedMask();
        updateImage();

        if (mask.empty()) {
            QMessageBox::warning(this, "Inpainting Error", "No valid mask was provided or selected.");
            disableMaskShowing();
            return;
        }
        if (mask.size() != originalImage.size()) {
            QMessageBox::warning(this, "Inpainting Error", "Mask dimensions do not match the image.");
            disableMaskShowing();
            return;
        }
        if (mask.type() != CV_8UC1) {
            QMessageBox::warning(this, "Inpainting Error", "Mask format is invalid (must be 8-bit single channel).");
            disableMaskShowing();
            return;
        }

        InputDialog inputDialog(this);
        inputDialog.setWindowTitle("Inpainting Parameters");

        auto* radiusSpin = new QDoubleSpinBox(&inputDialog);
        radiusSpin->setRange(1.0, 200.0);
        radiusSpin->setValue(5.0);
        radiusSpin->setSingleStep(1.0);
        radiusSpin->setSuffix(" px");

        auto* methodCombo = new QComboBox(&inputDialog);
        methodCombo->addItems({"Telea", "Navier-Stokes"});

        inputDialog.addInput("Inpaint Radius:", radiusSpin);
        inputDialog.addInput("Inpaint Method:", methodCombo);
        setupPreview(&inputDialog, inputDialog.getPreviewCheckBox(), [&]() {
            int methodFlag = (methodCombo->currentText() == "Telea") ? cv::INPAINT_TELEA : cv::INPAINT_NS;
            double radius = radiusSpin->value();

            cv::Mat imageToInpaint;
            if (originalImage.channels() == 4) {
                cv::cvtColor(originalImage, imageToInpaint, cv::COLOR_BGRA2BGR);
            } else {
                imageToInpaint = originalImage.clone();
            }

            return ImageProcessing::applyInpainting(imageToInpaint, mask, radius, methodFlag);
        });

        inputDialog.exec();
        disableMaskDrawing();
        disableMaskShowing();
        clearMask();
        updateImage();
    });

    connect(inpaintDialog, &QDialog::rejected, this, [=]() {
        disableMaskDrawing();
        disableMaskShowing();
        clearMask();
        updateImage();
    });
}



// ======================================================================
// `Filtering & Edge Detection Slots`
// ======================================================================
// Applies a 3x3 box blur filter.
void ImageViewer::applyBlur() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyBoxBlur(originalImage, 3, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies a 3x3 Gaussian blur filter.
void ImageViewer::applyGaussianBlur() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyGaussianBlur(originalImage, 3, 0, 0, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies Sobel edge detection (default x-direction).
void ImageViewer::applySobelEdgeDetection() {
    pushToUndoStack();
    originalImage = ImageProcessing::applySobelEdgeDetection(originalImage, 3, 1, 0, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies Laplacian edge detection.
void ImageViewer::applyLaplacianEdgeDetection() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyLaplacianEdgeDetection(originalImage, 1, 1, 0, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies Canny edge detection with default thresholds.
void ImageViewer::applyCannyEdgeDetection() {
    pushToUndoStack();
    originalImage = ImageProcessing::applyCannyEdgeDetection(originalImage, 50, 150); // Default thresholds
    updateImage();
}

// Opens a dialog for Hough line detection on a binary image (prompts for Canny if needed).
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
            pushToUndoStack();
            originalImage = edgeImage;
            updateImage();
        } else {
            return;
            // User chose not to proceed
        }
    } else {
        edgeImage = originalImage.clone();
        // Use the binary image directly
    }

    if (edgeImage.empty() || edgeImage.type() != CV_8UC1) { // Double check edgeImage
        QMessageBox::warning(this, "Hough Error", "Could not obtain a valid binary edge image for Hough transform.");
        return;
    }

    HoughDialog dialog(this);

    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::detectHoughLines(edgeImage, dialog.getRho(),
                                                 dialog.getThetaDegrees() * CV_PI / 180.0, dialog.getThreshold());
    });
    dialog.exec();
}

// Applies a sharpening filter based on the selected option.
void ImageViewer::applySharpening(int option) {
    pushToUndoStack();
    originalImage = ImageProcessing::applySharpening(originalImage, option, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Opens a dialog to select direction for Prewitt edge detection.
void ImageViewer::applyPrewittEdgeDetection() {
    DirectionSelectionDialog dialog(this);
    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::applyPrewittEdgeDetection(originalImage, dialog.getSelectedDirection(),
                                                          mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    });
    dialog.exec();
}

// Opens a dialog to define and apply a custom convolution kernel.
void ImageViewer::applyCustomFilter() {
    CustomFilterDialog dialog(this);
    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::applyCustomFilter(originalImage, dialog.getKernel(), true,
                                                  mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    });
    dialog.exec();
}

// Opens a dialog to select kernel size for the median filter.
void ImageViewer::applyMedianFilter() {
    InputDialog dialog(this);

    // Create kernel size dropdown
    auto *kernelCombo = new QComboBox;
    kernelCombo->addItems({"3", "5", "7", "9"});
    dialog.addInput("Kernel Size", kernelCombo);

    setupPreview(&dialog, dialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::applyMedianFilter(
            originalImage,
            dialog.getValue("Kernel Size").toString().toInt(),
            mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT
            );
    });
    dialog.exec();
}

// Opens a dialog to define and apply a two-step (separable) filter.
void ImageViewer::applyTwoStepFilter() {
    TwoStepFilterDialog filterDialog(this);
    setupPreview(&filterDialog, filterDialog.getPreviewCheckBox(), [&]() {
        return ImageProcessing::applyTwoStepFilter(originalImage.clone(), filterDialog.getKernel1(),
                                                   filterDialog.getKernel2(), mainWindow->getBorderOption());
    });
    filterDialog.exec();
}


// ======================================================================
// `Morphology Operations Slots`
// ======================================================================
// Applies erosion using the selected structuring element type.
void ImageViewer::applyErosion(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyErosion(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies dilation using the selected structuring element type.
void ImageViewer::applyDilation(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyDilation(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies morphological opening using the selected structuring element type.
void ImageViewer::applyOpening(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyOpening(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies morphological closing using the selected structuring element type.
void ImageViewer::applyClosing(StructuringElementType type) {
    pushToUndoStack();
    originalImage = ImageProcessing::applyClosing(originalImage, type, 1, mainWindow ? mainWindow->getBorderOption() : cv::BORDER_DEFAULT);
    updateImage();
}

// Applies skeletonization to the binary image.
void ImageViewer::applySkeletonization() {
    pushToUndoStack();
    // Assuming Diamond is default or appropriate here
    originalImage = ImageProcessing::applySkeletonization(originalImage, Diamond);
    updateImage();
}

// ======================================================================
// `Image Analysis Slot`
// ======================================================================
void ImageViewer::analyzeShapeFeatures() {
    if (originalImage.channels() != 1) {
        QMessageBox::warning(this, "Shape Analysis", "Image must be binary (1-channel).");
        return;
    }

    auto featuresList = ImageProcessing::computeShapeFeatures(originalImage);

    if (featuresList.empty()) {
        QMessageBox::information(this, "Shape Analysis", "No objects found.");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Shape Features");
    dialog->resize(800, 500);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QTableWidget *table = new QTableWidget(dialog);

    QStringList headers = {
        "Object #", "Area", "Perimeter", "Aspect Ratio",
        "Extent", "Solidity", "Equivalent Diameter"
    };

    table->setColumnCount(headers.size());
    table->setRowCount(static_cast<int>(featuresList.size()));
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);

    for (int i = 0; i < featuresList.size(); ++i) {
        const auto& f = featuresList[i];

        class MyTableWidgetItem : public QTableWidgetItem {
        public:
            bool operator <(const QTableWidgetItem &other) const
            {
                return text().toDouble() < other.text().toDouble();
            }
        };

        auto makeItem = [](double val) {
            auto *item = new MyTableWidgetItem;
            item->setData(Qt::DisplayRole, static_cast<double>(val)); // Force double
            return item;
        };


        table->setItem(i, 0, makeItem(i + 1)); // index as string is fine
        table->setItem(i, 1, makeItem(f.area));
        table->setItem(i, 2, makeItem(f.perimeter));
        table->setItem(i, 3, makeItem(f.aspectRatio));
        table->setItem(i, 4, makeItem(f.extent));
        table->setItem(i, 5, makeItem(f.solidity));
        table->setItem(i, 6, makeItem(f.equivalentDiameter));
    }


    table->resizeColumnsToContents();

    QPushButton *closeButton = new QPushButton("Close", dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);

    layout->addWidget(table);
    layout->addWidget(closeButton);
    dialog->show();
}

// ======================================================================
// `Helper Functions`
// ======================================================================
// Converts a cv::Mat to a QImage based on its type.
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
