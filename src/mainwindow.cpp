#include "mainwindow.h"
#include "imageviewer.h"
#include <QVBoxLayout>
#include <QActionGroup>

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
}

int MainWindow::getBorderOption() {
    return borderOption;
}

void MainWindow::setBorderOption(int option, QAction *selectedAction) {
    borderOption = option;
    borderIsolated->setChecked(false);
    borderReflect->setChecked(false);
    borderReplicate->setChecked(false);
    selectedAction->setChecked(true);
}

void MainWindow::openImage() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (!filePath.isEmpty()) {
        QByteArray imageData;
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            imageData = file.readAll();
            file.close();
        }

        std::vector<uchar> buffer(imageData.begin(), imageData.end());
        cv::Mat image = cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
        if (!image.empty()) {
            ImageViewer *viewer = new ImageViewer(image, filePath, nullptr, QPoint(100,100), this);
            openedImages.push_back(viewer);
            viewer->show();
        }
    }
}


void MainWindow::showInfo() {
    QMessageBox::information(this, "About",
                             "Aplikacja zbiorcza z ćwiczeń laboratoryjnych\n"
                             "Autor: Mikhail Harbuz\n"
                             "Prowadzący: dr inż. Łukasz Roszkowiak\n"
                             "Algorytmy Przetwarzania Obrazów 2024\n"
                             "WIT grupa ID: ID06IO1");
}
