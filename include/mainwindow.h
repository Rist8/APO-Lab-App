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
    QVector<QWidget*> openedImages;

private slots:
    void openImage();
    void showInfo(); // New slot for displaying info
    void setBorderOption(int option, QAction *selectedAction);

private:
    int borderOption;
    QAction *borderIsolated;
    QAction *borderReflect;
    QAction *borderReplicate;
};

#endif // MAINWINDOW_H
