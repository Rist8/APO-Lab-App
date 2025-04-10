#include "imageviewer.h"
#include "rangestretchingdialog.h"
#include "customfilterdialog.h"
#include "twostepfilterdialog.h"
#include "directionselectiondialog.h"
#include "bitwiseoperationdialog.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QInputDialog>

ImageViewer::ImageViewer(const cv::Mat &image, const QString &title, QWidget *parent, QPoint position, MainWindow *mainWindow)
    : QWidget(parent), originalImage(image), currentScale(1.0f), histogramWidget(nullptr), mainWindow(mainWindow) {

    mainWindow->openedImages.push_back(this);
    LUT = new QTableWidget(this);
    LUT->setEditTriggers(QAbstractItemView::NoEditTriggers);
    LUT->setMinimumHeight(64);
    LUT->setMaximumHeight(64);
    LUT->setRowCount(1);
    LUT->setColumnCount(256);  // Ensure 256 bins for grayscale histogram
    LUT->setVerticalHeaderLabels({"Count"});  // Row label

    // Set column headers to start from 0
    QStringList columnLabels;
    for (int i = 0; i < 256; ++i) {
        columnLabels.append(QString::number(i));  // Number columns from 0 to 255
    }
    LUT->setHorizontalHeaderLabels(columnLabels);

    LUT->hide();  // Hide by default



    setWindowTitle(title);
    setGeometry(position.x(), position.y(), image.cols + 50, image.rows + 50);  // Set initial size and position
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    imageLabel->setPixmap(QPixmap::fromImage(MatToQImage(originalImage)));

    mainLayout = new QVBoxLayout(this);

    // Create Menu Bar
    createMenu();

    mainLayout->addWidget(imageLabel);
    zoomInput = new QLineEdit(this);
    zoomInput->setStyleSheet("QLineEdit { background-color: rgba(0, 0, 0, 100); color: white; padding: 3px; border-radius: 5px; }");
    zoomInput->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    zoomInput->setText(QString::number(static_cast<int>(currentScale * 100)) + "%");
    zoomInput->setMaximumWidth(60);
    // Use regex to allow numbers with or without a '%' sign (e.g., "150" or "150%")
    QRegularExpression regex(R"(^\d{1,3}%?$)");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(regex, this);
    zoomInput->setValidator(validator);

    // Connect user input to zoom function
    connect(zoomInput, &QLineEdit::returnPressed, this, &ImageViewer::setZoomFromInput);
    mainLayout->addWidget(zoomInput, 0, Qt::AlignRight | Qt::AlignBottom);

    updateZoomLabel();
    mainLayout->addWidget(LUT);
    setLayout(mainLayout);
    updateImage();
}

void ImageViewer::registerOperation(ImageOperation *op) {
    operationsList.append(op);
}

void ImageViewer::updateOperationsEnabledState() {
    ImageOperation::ImageType type = (originalImage.channels() == 1)
    ? ImageOperation::Grayscale
    : ImageOperation::Color;

    for (ImageOperation* op : operationsList) {
        op->updateActionState(type);
    }
}

void ImageViewer::createMenu() {
    menuBar = new QMenuBar(this);

    QMenu *fileMenu = new QMenu("File", this);
    QMenu *viewMenu = new QMenu("View", this);
    QMenu *imageProcessingMenu = new QMenu("Image Processing", this);
    QMenu *histogramMenu = new QMenu("Histogram Operations", this);
    QMenu *pointOperationsMenu = new QMenu("Point Operations", this);
    QMenu *filterMenu = new QMenu("Filter Operations", this);
    QMenu *sharpenMenu = new QMenu("Apply Sharpening", this);

    // File Menu
    auto duplicateOp = new ImageOperation("Duplicate", this, fileMenu,
                                          ImageOperation::Color | ImageOperation::Grayscale,
                                          [this]() { this->duplicateImage(); });
    duplicateOp->getAction()->setShortcut(QKeySequence("Ctrl+Shift+D"));
    registerOperation(duplicateOp);

    auto saveAsOp = new ImageOperation("Save As", this, fileMenu,
                                       ImageOperation::Color | ImageOperation::Grayscale,
                                       [this]() { this->saveImageAs(); });
    saveAsOp->getAction()->setShortcut(QKeySequence("Ctrl+S"));
    registerOperation(saveAsOp);

    // View Menu
    histogramAction = new QAction("Histogram", this);
    histogramAction->setCheckable(true);
    connect(histogramAction, &QAction::toggled, this, &ImageViewer::toggleHistogram);
    viewMenu->addAction(histogramAction);

    auto lutToggleOp = new ImageOperation("Show LUT", this, viewMenu,
                                          ImageOperation::Grayscale,
                                          [this]() { this->toggleLUT(); }, true);
    registerOperation(lutToggleOp);

    // Image Processing Menu
    registerOperation(new ImageOperation("Convert to Grayscale", this, imageProcessingMenu,
                                         ImageOperation::Color, [this]() { this->convertToGrayscale(); }));

    registerOperation(new ImageOperation("Split Color Channels", this, imageProcessingMenu,
                                         ImageOperation::Color, [this]() { this->splitColorChannels(); }));

    registerOperation(new ImageOperation("Convert to HSV/Lab", this, imageProcessingMenu,
                                         ImageOperation::Color, [this]() { this->convertToHSVLab(); }));

    // Histogram Menu
    registerOperation(new ImageOperation("Stretch Histogram", this, histogramMenu,
                                         ImageOperation::Grayscale, [this]() { this->stretchHistogram(); }));

    registerOperation(new ImageOperation("Equalize Histogram", this, histogramMenu,
                                         ImageOperation::Grayscale, [this]() { this->equalizeHistogram(); }));

    // Point Operations
    registerOperation(new ImageOperation("Apply Negation", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyNegation(); }));

    registerOperation(new ImageOperation("Range Stretching", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, [this]() { this->rangeStretching(); }));

    registerOperation(new ImageOperation("Apply Posterization", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyPosterization(); }));

    registerOperation(new ImageOperation("Bitwise Operations", this, pointOperationsMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyBitwiseOperation(); }));

    // Filter Menu
    registerOperation(new ImageOperation("Apply Blur", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyBlur(); }));

    registerOperation(new ImageOperation("Apply Gaussian Blur", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyGaussianBlur(); }));

    registerOperation(new ImageOperation("Sobel Edge Detection", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applySobelEdgeDetection(); }));

    registerOperation(new ImageOperation("Laplacian Edge Detection", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyLaplacianEdgeDetection(); }));

    registerOperation(new ImageOperation("Canny Edge Detection", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyCannyEdgeDetection(); }));

    // Sharpen Submenu
    registerOperation(new ImageOperation("Basic Sharpening", this, sharpenMenu,
                                         ImageOperation::Grayscale, [this]() { this->applySharpening(1); }));

    registerOperation(new ImageOperation("Strong Sharpening", this, sharpenMenu,
                                         ImageOperation::Grayscale, [this]() { this->applySharpening(2); }));

    registerOperation(new ImageOperation("Edge Enhancement", this, sharpenMenu,
                                         ImageOperation::Grayscale, [this]() { this->applySharpening(3); }));

    filterMenu->addMenu(sharpenMenu); // Add sharpening menu to filter menu

    registerOperation(new ImageOperation("Prewitt Edge Detection", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyPrewittEdgeDetection(); }));

    registerOperation(new ImageOperation("Apply Custom Filter", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyCustomFilter(); }));

    registerOperation(new ImageOperation("Apply Median Filter", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyMedianFilter(); }));

    registerOperation(new ImageOperation("Two-Step Filter (5x5)", this, filterMenu,
                                         ImageOperation::Grayscale, [this]() { this->applyTwoStepFilter(); }));

    // Add menus to bar
    menuBar->addMenu(fileMenu);
    menuBar->addMenu(viewMenu);
    menuBar->addMenu(filterMenu);
    menuBar->addMenu(imageProcessingMenu);
    menuBar->addMenu(histogramMenu);
    menuBar->addMenu(pointOperationsMenu);
    mainLayout->setMenuBar(menuBar);
    updateOperationsEnabledState();
}


void ImageViewer::duplicateImage() {
    static int n = 0;
    ImageViewer *newViewer = new ImageViewer(originalImage.clone(),
                                             "Duplicate" + QString(std::to_string(n++).c_str()), nullptr,
                                             QPoint(this->x() + 200, this->y() + 200), mainWindow);
    newViewer->setZoom(currentScale);
    newViewer->show();
}

void ImageViewer::saveImageAs() {
    QString filePath = QFileDialog::getSaveFileName(this, "Save Image As", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!filePath.isEmpty()) {
        std::vector<int> compression_params;
        if (filePath.endsWith(".jpg") || filePath.endsWith(".jpeg")) {
            compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
            compression_params.push_back(95);  // JPEG quality (0-100)
        }
#ifdef _WIN32
        std::string path = QString::fromWCharArray(filePath.toStdWString().c_str()).toLocal8Bit().constData();
#else
        std::string path = filePath.toUtf8().constData();
#endif
        if (!cv::imwrite(path, originalImage, compression_params)) {
            QMessageBox::warning(this, "Save Error", "Failed to save the image.");
        }
    }
}


// Histogram toggle
void ImageViewer::toggleHistogram() {
    if (histogramAction->isChecked()) {
        if (!histogramWidget) {
            histogramWidget = new HistogramWidget(this);

            // Create horizontal layout for image and histogram
            QHBoxLayout *imageHistogramLayout = new QHBoxLayout();
            imageHistogramLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            imageHistogramLayout->addWidget(imageLabel);
            imageHistogramLayout->addWidget(histogramWidget);

            // Create vertical container layout
            QVBoxLayout *containerLayout = new QVBoxLayout();
            containerLayout->addLayout(imageHistogramLayout); // Image & histogram on same row
            containerLayout->addWidget(LUT); // LUT below them
            containerLayout->addWidget(zoomInput, 0, Qt::AlignRight); // Zoom input at bottom-right

            // Add container layout to main layout
            mainLayout->addLayout(containerLayout);
        }
        updateHistogram();
        updateImage();
    } else {
        if (histogramWidget) {
            delete histogramWidget;
            histogramWidget = nullptr;
        }
    }
}


void ImageViewer::toggleLUT() {
    if (LUT->isVisible()) {
        LUT->hide();
    } else {
        updateHistogramTable();
        LUT->show();
    }
}


// Image Processing Functions
void ImageViewer::convertToGrayscale() {
    if (originalImage.channels() == 3 || originalImage.channels() == 4) {
        cv::cvtColor(originalImage, originalImage, cv::COLOR_BGR2GRAY);

        // Disable options that no longer apply
        updateOperationsEnabledState();
        updateImage();
    }
}

void ImageViewer::splitColorChannels() {
    std::vector<cv::Mat> channels;
    cv::split(originalImage, channels);

    // Create child windows for each channel
    (new ImageViewer(channels[0], "Blue Channel", nullptr, QPoint(200, 100), mainWindow))->show();
    (new ImageViewer(channels[1], "Green Channel", nullptr, QPoint(300, 100), mainWindow))->show();
    (new ImageViewer(channels[2], "Red Channel", nullptr, QPoint(400, 100), mainWindow))->show();
}

void ImageViewer::convertToHSVLab() {
    cv::Mat hsv, lab;
    cv::cvtColor(originalImage, hsv, cv::COLOR_BGR2HSV);
    cv::cvtColor(originalImage, lab, cv::COLOR_BGR2Lab);

    (new ImageViewer(hsv, "HSV Image", nullptr, QPoint(200, 100), mainWindow))->show();
    (new ImageViewer(lab, "Lab Image", nullptr, QPoint(300, 100), mainWindow))->show();
}

// Histogram Operations
void ImageViewer::stretchHistogram() {
    double minVal, maxVal;
    cv::minMaxLoc(originalImage, &minVal, &maxVal);
    originalImage.convertTo(originalImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    updateImage();
}

void ImageViewer::equalizeHistogram() {
    if (originalImage.empty() || originalImage.channels() != 1)
        return;

    // Step 1: Compute Histogram
    std::vector<int> histogram(256, 0);
    for (int y = 0; y < originalImage.rows; y++) {
        for (int x = 0; x < originalImage.cols; x++) {
            int pixelValue = originalImage.at<uchar>(y, x);
            histogram[pixelValue]++;
        }
    }

    // Step 2: Compute CDF
    std::vector<int> cdf(256, 0);
    cdf[0] = histogram[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i - 1] + histogram[i];
    }

    // Normalize CDF to range [0, 255]
    int minCDF = *std::find_if(cdf.begin(), cdf.end(), [](int v) { return v > 0; });  // First nonzero CDF
    std::vector<uchar> equalizationMap(256);
    for (int i = 0; i < 256; i++) {
        equalizationMap[i] = static_cast<uchar>((cdf[i] - minCDF) * 255 / (originalImage.total() - minCDF));
    }

    // Step 3: Apply Equalization
    for (int y = 0; y < originalImage.rows; y++) {
        for (int x = 0; x < originalImage.cols; x++) {
            originalImage.at<uchar>(y, x) = equalizationMap[originalImage.at<uchar>(y, x)];
        }
    }

    updateImage();
}

// Point Operations
void ImageViewer::applyNegation() {
    originalImage = 255 - originalImage;
    updateImage();
}

void ImageViewer::rangeStretching() {
    if (originalImage.empty() || originalImage.channels() != 1)
        return;

    RangeStretchingDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        int p1 = dialog.getP1();
        int p2 = dialog.getP2();
        int q3 = dialog.getQ3();
        int q4 = dialog.getQ4();

        if (p1 >= p2 || q3 >= q4) {
            QMessageBox::warning(this, "Invalid Input", "Make sure p1 < p2 and q3 < q4.");
            return;
        }

        cv::Mat stretchedImage = originalImage.clone();

        for (int y = 0; y < stretchedImage.rows; y++) {
            for (int x = 0; x < stretchedImage.cols; x++) {
                int pixel = stretchedImage.at<uchar>(y, x);

                // Apply stretching only if the pixel is in range
                if (pixel < p1)
                    pixel = pixel;  // Keep values below p1 unchanged
                else if (pixel > p2)
                    pixel = pixel;  // Keep values above p2 unchanged
                else
                    pixel = ((pixel - p1) * (q4 - q3)) / (p2 - p1) + q3;

                stretchedImage.at<uchar>(y, x) = pixel;
            }
        }

        originalImage = stretchedImage;
        updateImage();
    }
}

// Update displayed image
void ImageViewer::updateImage() {
    if (originalImage.empty()) return;

    QImage qimg = MatToQImage(originalImage);
    if (qimg.isNull()) return;

    // Apply scaling before setting the pixmap
    int newWidth = static_cast<int>(qimg.width() * currentScale);
    int newHeight = static_cast<int>(qimg.height() * currentScale);
    QPixmap scaledPixmap = QPixmap::fromImage(qimg).scaled(newWidth, newHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    imageLabel->setPixmap(scaledPixmap);
    imageLabel->adjustSize();

    if(!isMaximized()){
        // Resize the window to match the image, but ensure a minimum size
        resize(qMax(newWidth + 20, 200), qMax(newHeight + 50, 200));
        adjustSize();
    }

    if (histogramWidget && histogramWidget->isVisible()) {

        histogramWidget->setMinimumWidth(qBound(276, newWidth + 20, 1044));
        updateHistogram();  // Ensure histogram updates dynamically
    }

    updateZoomLabel();  // Update zoom percentage
}

void ImageViewer::updateZoomLabel() {
    int zoomPercentage = static_cast<int>(currentScale * 100);
    zoomInput->setText(QString::number(zoomPercentage) + "%");
    zoomInput->adjustSize();
}

void ImageViewer::setZoomFromInput() {
    QString text = zoomInput->text();
    text.remove("%");  // Remove '%' if user includes it
    bool ok;
    int zoomValue = text.toInt(&ok);

    if (ok && zoomValue >= 10 && zoomValue <= 500) {  // Ensure valid zoom range
        currentScale = zoomValue / 100.0;  // Convert to scale factor
        updateImage();
    } else {
        QMessageBox::warning(this, "Invalid Input", "Please enter a zoom value between 10% and 500%.");
        updateZoomLabel();  // Reset input if invalid
    }
}

void ImageViewer::wheelEvent(QWheelEvent *event) {
    const double scaleFactor = 1.1;  // Zoom in/out step size

    if (event->angleDelta().y() > 0) {
        currentScale *= scaleFactor;  // Zoom in
    } else {
        currentScale /= scaleFactor;  // Zoom out
    }

    currentScale = qBound(0.1, currentScale, 5.0);  // Prevent excessive zooming

    updateImage();  // Apply new scaling
}

void ImageViewer::updateHistogram() {
    if (histogramWidget) {
        histogramWidget->computeHistogram(originalImage);
    }
    updateHistogramTable();
}

void ImageViewer::updateHistogramTable() {
    if (!LUT) return;

    // Compute histogram manually if HistogramWidget is disabled
    std::vector<int> histogramData(256, 0);

    if (!originalImage.empty()) {
        cv::Mat grayImage;
        if (originalImage.channels() == 3) {
            cv::cvtColor(originalImage, grayImage, cv::COLOR_BGR2GRAY);
        } else {
            grayImage = originalImage;
        }

        // Fill histogram data
        for (int y = 0; y < grayImage.rows; y++) {
            for (int x = 0; x < grayImage.cols; x++) {
                int intensity = grayImage.at<uchar>(y, x);
                histogramData[intensity]++;
            }
        }
    }

    // Reset table size
    LUT->setColumnCount(256);

    for (int i = 0; i < 256; ++i) {
        LUT->setItem(0, i, new QTableWidgetItem(QString::number(histogramData[i])));  // Count
    }
}

void ImageViewer::applyPosterization() {
    int levels = QInputDialog::getInt(this, "Posterization", "Enter number of levels (2-256):", 4, 2, 256);
    if (levels < 2 || levels > 256) return;

    cv::Mat tempImage;
    double step = 255.0 / (levels - 1);
    originalImage.convertTo(tempImage, CV_32F);

    for (int y = 0; y < tempImage.rows; y++) {
        for (int x = 0; x < tempImage.cols; x++) {
            tempImage.at<float>(y, x) = std::round(tempImage.at<float>(y, x) / step) * step;
        }
    }

    tempImage.convertTo(originalImage, CV_8U);
    updateImage();
}



void ImageViewer::applyBlur() {
    cv::blur(originalImage, originalImage, cv::Size(3, 3), cv::Point(-1, -1), mainWindow->getBorderOption());
    updateImage();
}

void ImageViewer::applyGaussianBlur() {
    cv::GaussianBlur(originalImage, originalImage, cv::Size(3, 3), 0, 0, mainWindow->getBorderOption());
    updateImage();
}

void ImageViewer::applySobelEdgeDetection() {
    cv::Mat gradX, gradY;
    cv::Sobel(originalImage, gradX, CV_16S, 1, 0, 3, 1, 0, mainWindow->getBorderOption());
    cv::Sobel(originalImage, gradY, CV_16S, 0, 1, 3, 1, 0, mainWindow->getBorderOption());
    cv::convertScaleAbs(gradX + gradY, originalImage);
    updateImage();
}

void ImageViewer::applyLaplacianEdgeDetection() {
    cv::Laplacian(originalImage, originalImage, CV_16S, 1, 1, 0, mainWindow->getBorderOption());
    cv::convertScaleAbs(originalImage, originalImage);
    updateImage();
}

void ImageViewer::applyCannyEdgeDetection() {
    cv::Canny(originalImage, originalImage, 50, 150);
    updateImage();
}

void ImageViewer::applySharpening(int option) {
    cv::Mat kernel;
    switch (option) {
    case 1:
        // Basic Sharpening
        kernel = (cv::Mat_<float>(3, 3) <<
                      0, -1, 0,
                  -1, 5, -1,
                  0, -1, 0);
        break;
    case 2:
        // Strong Sharpening
        kernel = (cv::Mat_<float>(3, 3) <<
                      -1, -1, -1,
                  -1, 9, -1,
                  -1, -1, -1);
        break;
    case 3:
        // Edge Enhancement
        kernel = (cv::Mat_<float>(3, 3) <<
                      1, -2, 1,
                  -2, 5, -2,
                  1, -2, 1);
        break;
    default:
        return; // No action if invalid option
    }

    cv::filter2D(originalImage, originalImage, -1, kernel, cv::Point(-1, -1), 0, mainWindow->getBorderOption());
    updateImage();
}

void ImageViewer::applyPrewittEdgeDetection() {
    DirectionSelectionDialog dialog(this);
    connect(&dialog, &DirectionSelectionDialog::directionSelected, this, &ImageViewer::setPrewittDirection);
    dialog.exec();
}

void ImageViewer::setPrewittDirection(int direction) {
    cv::Mat kernel;
    switch (direction) {
    case 0: // Top-left ↖
        kernel = (cv::Mat_<float>(3,3) << -1, -1, 0,
                  -1,  0, 1,
                  0,  1, 1);
        break;
    case 1: // Up ↑
        kernel = (cv::Mat_<float>(3,3) << -1, -1, -1,
                  0,  0,  0,
                  1,  1,  1);
        break;
    case 2: // Top-right ↗
        kernel = (cv::Mat_<float>(3,3) <<  0, -1, -1,
                  1,  0, -1,
                  1,  1,  0);
        break;
    case 3: // Left ←
        kernel = (cv::Mat_<float>(3,3) << -1,  0,  1,
                  -1,  0,  1,
                  -1,  0,  1);
        break;
    case 5: // Right →
        kernel = (cv::Mat_<float>(3,3) <<  1,  0, -1,
                  1,  0, -1,
                  1,  0, -1);
        break;
    case 6: // Bottom-left ↙
        kernel = (cv::Mat_<float>(3,3) <<  0,  1,  1,
                  -1,  0,  1,
                  -1, -1,  0);
        break;
    case 7: // Down ↓
        kernel = (cv::Mat_<float>(3,3) <<  1,  1,  1,
                  0,  0,  0,
                  -1, -1, -1);
        break;
    case 8: // Bottom-right ↘
        kernel = (cv::Mat_<float>(3,3) <<  1,  1,  0,
                  1,  0, -1,
                  0, -1, -1);
        break;
    }

    if (!originalImage.empty()) {
        cv::Mat result;
        cv::filter2D(originalImage, result, -1, kernel, cv::Point(-1, -1), 0, mainWindow->getBorderOption());
        originalImage = result;
    }
    updateImage();
}

void ImageViewer::applyCustomFilter() {
    CustomFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        cv::Mat kernel = dialog.getKernel();
        double sum = cv::sum(kernel)[0];
        if (sum != 0) {
            kernel /= sum;  // Normalize the kernel
        }
        cv::filter2D(originalImage, originalImage, -1, kernel, cv::Point(-1, -1), 0, mainWindow->getBorderOption());
        updateImage();
    }
}

// Custom Median Filtering (from scratch) with Border Handling
cv::Mat applyCustomMedianFilter(const cv::Mat &image, int kernelSize, int borderType) {
    cv::Mat result;
    cv::copyMakeBorder(image, result, kernelSize / 2, kernelSize / 2, kernelSize / 2, kernelSize / 2, borderType);
    cv::Mat filtered = result.clone();
    int border = kernelSize / 2;
    for (int y = border; y < result.rows - border; ++y) {
        for (int x = border; x < result.cols - border; ++x) {
            std::vector<uchar> neighbors;
            for (int ky = -border; ky <= border; ++ky) {
                for (int kx = -border; kx <= border; ++kx) {
                    neighbors.push_back(result.at<uchar>(y + ky, x + kx));
                }
            }
            std::nth_element(neighbors.begin(), neighbors.begin() + neighbors.size() / 2, neighbors.end());
            filtered.at<uchar>(y, x) = neighbors[neighbors.size() / 2];
        }
    }
    return filtered(cv::Rect(border, border, image.cols, image.rows));
}

void ImageViewer::applyMedianFilter() {
    bool ok;
    QStringList kernelOptions;
    kernelOptions << "3" << "5" << "7";

    QString selected = QInputDialog::getItem(this, "Select Kernel Size", "Kernel Size:", kernelOptions, 0, false, &ok);
    if (!ok) return;

    int kernelSize = selected.toInt();

    int borderType = mainWindow->getBorderOption();
    originalImage = applyCustomMedianFilter(originalImage, kernelSize, borderType);
    updateImage();
}

void ImageViewer::applyBitwiseOperation() {
    if (originalImage.empty()) return;

    BitwiseOperationDialog dialog(this, mainWindow->openedImages);
    if (dialog.exec() == QDialog::Accepted) {
        originalImage = dialog.getResult();
        updateImage();
    }
}

void ImageViewer::closeEvent(QCloseEvent *event){
    mainWindow->openedImages.erase(std::remove_if(mainWindow->openedImages.begin(), mainWindow->openedImages.end(),[this](QWidget* iv){ return qobject_cast<ImageViewer*>(iv) == this; }));
}

void ImageViewer::applyTwoStepFilter() {
    if (originalImage.empty()) return;

    originalImage = applyTwoStepFilterOperation(originalImage);
    updateImage();
}

cv::Mat ImageViewer::applyTwoStepFilterOperation(const cv::Mat& input) {
    if (input.empty()) return input;

    // Open the filter configuration dialog
    TwoStepFilterDialog filterDialog;
    if (filterDialog.exec() != QDialog::Accepted)
        return input; // Return unchanged if user cancels

    // Get the two user-defined 3x3 kernels
    cv::Mat kernel1 = filterDialog.getKernel1();
    cv::Mat kernel2 = filterDialog.getKernel2();

    // Compute the equivalent 5x5 kernel
    cv::Mat combined5x5 = filterDialog.getKernel3();
    combined5x5 /= cv::sum(combined5x5)[0];

    // Apply the computed 5x5 kernel
    cv::Mat result;
    cv::filter2D(input, result, -1, combined5x5, cv::Point(-1, -1), 0, mainWindow->getBorderOption());

    return result;
}

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
