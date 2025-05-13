#include "mainwindow.h"
#include "bitwiseoperationdialog.h"
#include "imageviewer.h"
#include <QVBoxLayout>
#include <QActionGroup>
#include <qcombobox.h>
#include <qmimedata.h>

// ==========================================================================
// Application Core & Main Window
// ==========================================================================

// Constructor: Sets up the main window menu bar (File, Info, Options).
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(800, 30);
    setTabPosition(Qt::DockWidgetArea::TopDockWidgetArea, QTabWidget::TabPosition::North);
    move(400, 0);
    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *openAction = new QAction("Open", this);
    connect(openAction, &QAction::triggered, this, [=](){ openImage(); });
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



    QMenu *imagesInteractionMenu = menuBar()->addMenu("Images Interaction");

    QAction* mergeGrayscale = new QAction("Merge Grayscale Channels...", this);
    connect(mergeGrayscale, &QAction::triggered, this, &MainWindow::mergeGrayscaleChannels);
    imagesInteractionMenu->addAction(mergeGrayscale);

    QAction* bitwiseOperations = new QAction("Bitwise operations...", this);
    connect(bitwiseOperations, &QAction::triggered, this, &MainWindow::showBitwiseOperationDialog);
    imagesInteractionMenu->addAction(bitwiseOperations);
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

cv::Mat decompressRLE(const std::vector<std::pair<uchar, uchar>>& rleData, int width, int height) {
    cv::Mat image(height, width, CV_8UC1);
    uchar* data = image.ptr<uchar>(0);
    uchar idx = 0;

    for (const auto& [val, count] : rleData) {
        std::fill(data + idx, data + idx + count, val);
        idx += count;
    }

    return image;
}


cv::Mat decompressColorRLE(const std::vector<std::pair<cv::Vec3b, uchar>>& rleData, int width, int height) {
    cv::Mat image(height, width, CV_8UC3);
    cv::Vec3b* data = image.ptr<cv::Vec3b>(0);
    uchar idx = 0;

    for (const auto& [val, count] : rleData) {
        std::fill(data + idx, data + idx + count, val);
        idx += count;
    }

    return image;
}

cv::Mat loadRLEFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return {};

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    char type[3];
    if (file.read(type, 3) != 3) return {};

    int width, height;
    if (file.read(reinterpret_cast<char*>(&width), 4) != 4 ||
        file.read(reinterpret_cast<char*>(&height), 4) != 4) return {};

    QString format = QString::fromLatin1(type, 3);
    if (format == "GRE") {
        cv::Mat image(height, width, CV_8UC1);
        uchar* data = image.ptr<uchar>(0);
        int idx = 0;

        while (!file.atEnd() && idx < width * height) {
            uchar val, count;
            if (file.read(reinterpret_cast<char*>(&val), 1) != 1 ||
                file.read(reinterpret_cast<char*>(&count), 1) != 1) break;

            std::fill(data + idx, data + idx + count, val);
            idx += count;
        }

        return image;
    } else if (format == "COL") {
        // Delegate to color loading function (as shown earlier)
        // Or copy in the "COL" logic here
    }

    return {};
}





// Wrapper for menu action
void MainWindow::openImage() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff *.rle)");
    if (!filePath.isEmpty()) {
        openImage(filePath);  // call the main logic
    }
}

// Core image opening logic
void MainWindow::openImage(const QString &filePath) {
    cv::Mat image;

    if (filePath.endsWith(".rle", Qt::CaseInsensitive)) {
        image = loadRLEFile(filePath);
        if (image.empty()) {
            QMessageBox::critical(this, "Load Error", "Failed to load RLE image.");
            return;
        }
    } else {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray imageData = file.readAll();
            std::vector<uchar> buffer(imageData.begin(), imageData.end());
            image = cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
        }
        if (image.empty()) {
            QMessageBox::critical(this, "Load Error", "Failed to decode image.");
            return;
        }
    }

    ImageViewer *viewer = new ImageViewer(image, filePath, nullptr, QPoint(100, 100), this);
    viewer->show();
    if(!openedImages.contains(viewer))
        openedImages.append(viewer);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QList<QUrl> urls = event->mimeData()->urls();
    for (const QUrl &url : urls) {
        QString filePath = url.toLocalFile();
        if (!filePath.isEmpty()) {
            openImage(filePath);  // ← new, cleaner usage
        }
    }
    event->acceptProposedAction();
}

void MainWindow::mergeGrayscaleChannels() {
    QDialog dialog(this);
    dialog.setWindowTitle("Merge Grayscale Channels");
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* instructions = new QLabel("Assign grayscale images to each channel:");
    layout->addWidget(instructions);

    QStringList channelLabels = {"Blue", "Green", "Red", "Alpha"};
    QMap<QString, QComboBox*> channelSelectors;

    QStringList imageTitles;
    QMap<QString, ImageViewer*> imageMap;

    for (QWidget* widget : openedImages) {
        ImageViewer* viewer = qobject_cast<ImageViewer*>(widget);
        if (viewer && viewer->getOriginalImage().channels() == 1) {
            QString title = viewer->windowTitle();
            imageTitles.append(title);
            imageMap[title] = viewer;
        }
    }

    for (const QString& label : channelLabels) {
        QHBoxLayout* row = new QHBoxLayout;
        row->addWidget(new QLabel(label + ":"));
        QComboBox* comboBox = new QComboBox;
        comboBox->addItems(imageTitles);
        row->addWidget(comboBox);
        layout->addLayout(row);
        channelSelectors[label] = comboBox;
    }

    QCheckBox* includeAlpha = new QCheckBox("Include Alpha Channel");
    layout->addWidget(includeAlpha);
    channelSelectors["Alpha"]->setEnabled(false);

    connect(includeAlpha, &QCheckBox::toggled, [&](bool checked){
        channelSelectors["Alpha"]->setEnabled(checked);
    });

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    for(int i = 0; i < 3; ++i) {
        if(channelSelectors[channelLabels[i]]->currentText().isEmpty()) {
            QMessageBox::warning(this, "Merge Error", "Choose all 3 channels");
            return;
        }
    }

    std::vector<cv::Mat> channels(3);
    for (int i = 0; i < 3; ++i) {
        auto viewer = imageMap[channelSelectors[channelLabels[i]]->currentText()];
        channels[i] = viewer->getOriginalImage();
    }

    if (includeAlpha->isChecked()) {
        if(!channelSelectors["Alpha"]->currentText().isEmpty()) {
            auto alphaViewer = imageMap[channelSelectors["Alpha"]->currentText()];
            channels.push_back(alphaViewer->getOriginalImage());
        }
    }

    cv::Mat merged;
    cv::merge(channels, merged);

    auto viewer = new ImageViewer(merged, "Merged Image", nullptr, QPoint(100, 100), this);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->show();
    openedImages.append(viewer);
}

void MainWindow::showBitwiseOperationDialog() {
    // Show the dialog and pass list of currently opened images
    BitwiseOperationDialog dialog(nullptr, openedImages);

    if (dialog.exec() == QDialog::Accepted) {
        cv::Mat result = dialog.getResult(); // Add a getter if not present
        if (!result.empty()) {
            auto viewer = new ImageViewer(result, "Bitwise Result", nullptr, QPoint(100, 100), this);
            viewer->show();
            openedImages.append(viewer);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    for(auto image : openedImages) image->close();
    QWidget::closeEvent(event); // Call base class implementation
}

// Displays an 'About' message box.
void MainWindow::showInfo() {
    QMessageBox::information(this, "About",
                             "Aplikacja zbiorcza z ćwiczeń laboratoryjnych\n"
                             "Autor: Mikhail Harbuz\n"
                             "Prowadzący: dr inż. Łukasz Roszkowiak\n"
                             "Algorytmy Przetwarzania Obrazów 2024\n"
                             "WIT grupa ID: ID06IO1");
}
