#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <opencv2/opencv.hpp>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    int getBorderOption();
    bool isPyramidScalingEnabled() const;
    QVector<QWidget*> openedImages;

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void openImage();
    void openImage(const QString& filePath);  // replaces previous void openImage()
    void showInfo(); // New slot for displaying info
    void setBorderOption(int option, QAction *selectedAction);
    void mergeGrayscaleChannels();
    void showBitwiseOperationDialog();

private:
    bool usePyramidScaling = false;
    QAction *pyramidScalingToggle;
    int borderOption;
    QAction *borderIsolated;
    QAction *borderReflect;
    QAction *borderReplicate;
};

#endif // MAINWINDOW_H
