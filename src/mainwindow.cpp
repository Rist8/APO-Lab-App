#include "mainwindow.h"
#include "imageviewer.h"
#include <QVBoxLayout>
#include <QActionGroup>

// ==========================================================================
// Application Core & Main Window
// ==========================================================================

// Constructor: Sets up the main window menu bar (File, Info, Options).
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *openAction = new QAction("Open", this);
    connect(openAction, &QAction::triggered, this, &MainWindow::openImage);
    fileMenu->addAction(openAction);

    QMenu *infoMenu = menuBar()->addMenu("Info"); // New Info menu
    QAction *aboutAction = new QAction("About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showInfo);
    infoMenu->addAction(aboutAction);

    QMenu *optionsMenu = menuBar()->addMenu("Options");
    QMenu *borderMenu = optionsMenu->addMenu("Border Handling");

    borderIsolated = new QAction("Isolated", this);
    borderReflect = new QAction("Reflect", this);
    borderReplicate = new QAction("Replicate", this);

    borderIsolated->setCheckable(true);
    borderReflect->setCheckable(true);
    borderReplicate->setCheckable(true);

    QActionGroup *borderGroup = new QActionGroup(this);
    borderGroup->addAction(borderIsolated);
    borderGroup->addAction(borderReflect);
    borderGroup->addAction(borderReplicate);

    connect(borderIsolated, &QAction::triggered, this, [this]() { setBorderOption(cv::BORDER_ISOLATED, borderIsolated); });
    connect(borderReflect, &QAction::triggered, this, [this]() { setBorderOption(cv::BORDER_REFLECT, borderReflect); });
    connect(borderReplicate, &QAction::triggered, this, [this]() { setBorderOption(cv::BORDER_REPLICATE, borderReplicate); });

    borderMenu->addAction(borderIsolated);
    borderMenu->addAction(borderReflect);
    borderMenu->addAction(borderReplicate);

    setBorderOption(cv::BORDER_ISOLATED, borderIsolated); // Default selection

    pyramidScalingToggle = new QAction("Use Pyramid Scaling", this);
    pyramidScalingToggle->setCheckable(true);
    pyramidScalingToggle->setChecked(false); // Default off

    connect(pyramidScalingToggle, &QAction::triggered, this, [this](bool checked) {
        usePyramidScaling = checked;

        // Update all opened image viewers
        for (QWidget* widget : openedImages) {
            auto viewer = qobject_cast<ImageViewer*>(widget);
            if (viewer) {
                viewer->setUsePyramidScaling(checked);
            }
        }
    });

    optionsMenu->addAction(pyramidScalingToggle);
}

// Returns the currently selected border handling option for OpenCV functions.
int MainWindow::getBorderOption() {
    return borderOption;
}

// Returns the current state of pyramid scaling toggle.
bool MainWindow::isPyramidScalingEnabled() const {
    return usePyramidScaling;
}

// Sets the border handling option and updates menu checks.
void MainWindow::setBorderOption(int option, QAction *selectedAction) {
    borderOption = option;
    borderIsolated->setChecked(false);
    borderReflect->setChecked(false);
    borderReplicate->setChecked(false);
    selectedAction->setChecked(true);
}

cv::Mat decompressRLE(const std::vector<std::pair<uchar, int>>& rleData, int width, int height) {
    cv::Mat image(height, width, CV_8UC1);
    uchar* data = image.ptr<uchar>(0);
    int idx = 0;

    for (const auto& [val, count] : rleData) {
        std::fill(data + idx, data + idx + count, val);
        idx += count;
    }

    return image;
}


cv::Mat decompressColorRLE(const std::vector<std::pair<cv::Vec3b, int>>& rleData, int width, int height) {
    cv::Mat image(height, width, CV_8UC3);
    cv::Vec3b* data = image.ptr<cv::Vec3b>(0);
    int idx = 0;

    for (const auto& [val, count] : rleData) {
        std::fill(data + idx, data + idx + count, val);
        idx += count;
    }

    return image;
}

cv::Mat loadRLEFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QTextStream in(&file);
    QString header = in.readLine();
    QStringList headerParts = header.split(" ");
    if (headerParts.size() != 3) return {};

    QString type = headerParts[0];
    int width = headerParts[1].toInt();
    int height = headerParts[2].toInt();

    if (type == "grayscale") {
        std::vector<std::pair<uchar, int>> rleData;
        while (!in.atEnd()) {
            int val, count;
            in >> val >> count;
            rleData.emplace_back(static_cast<uchar>(val), count);
        }
        return decompressRLE(rleData, width, height);
    } else if (type == "color") {
        std::vector<std::pair<cv::Vec3b, int>> rleData;
        while (!in.atEnd()) {
            int b, g, r, count;
            in >> b >> g >> r >> count;
            rleData.emplace_back(cv::Vec3b(b, g, r), count);
        }
        return decompressColorRLE(rleData, width, height);
    }

    return {};
}


// Opens a file dialog to load an image and creates an ImageViewer for it.
void MainWindow::openImage() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.rle)");

    if (filePath.isEmpty())
        return;

    cv::Mat image;

    if (filePath.endsWith(".rle", Qt::CaseInsensitive)) {
        image = loadRLEFile(filePath);  // <-- custom RLE loader
        if (image.empty()) {
            QMessageBox::critical(this, "Load Error", "Failed to load RLE image.");
            return;
        }
    } else {
        QByteArray imageData;
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            imageData = file.readAll();
            file.close();
        }

        std::vector<uchar> buffer(imageData.begin(), imageData.end());
        image = cv::imdecode(buffer, cv::IMREAD_UNCHANGED);

        if (image.empty()) {
            QMessageBox::critical(this, "Load Error", "Failed to decode image.");
            return;
        }
    }

    ImageViewer *viewer = new ImageViewer(image, filePath, nullptr, QPoint(100,100), this);
    viewer->show();
}


// Displays an 'About' message box.
void MainWindow::showInfo() {
    QMessageBox::information(this, "About",
                             "Application for APO subject in WIT academy\n"
                             "Autor: Rist8\n");
}
