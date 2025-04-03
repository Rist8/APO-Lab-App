#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <opencv2/opencv.hpp>
#include <QImage>
#include <QPixmap>
#include "histogramwidget.h"
#include "mainwindow.h"

class ImageViewer : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewer(const cv::Mat &image, const QString &title, QWidget *parent = nullptr,
                         QPoint position = QPoint(100, 100), MainWindow *mainWindow = nullptr);

    cv::Mat getOriginalImage() const { return originalImage; }
    MainWindow* getMainWindow() const { return mainWindow; }
    void setZoom(double scale) { currentScale = scale; updateImage(); }

protected:
    void wheelEvent(QWheelEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    QLabel *imageLabel;
    QLineEdit *zoomInput;
    HistogramWidget *histogramWidget;
    QTableWidget *LUT;  // Table widget for histogram data
    QVBoxLayout *mainLayout;
    MainWindow *mainWindow;
    QMenuBar *menuBar;

    QMenu *sharpenMenu;
    QAction *duplicateAction;
    QAction *saveAsAction;
    QAction *sharpenBasic;
    QAction *sharpenStrong;
    QAction *sharpenEdge;
    QAction *histogramAction;
    QAction *convertToGrayscaleAction;
    QAction *splitChannelsAction;
    QAction *convertToHSVLabAction;
    QAction *stretchHistogramAction;
    QAction *equalizeHistogramAction;
    QAction *negationAction;
    QAction *rangeStretchingAction;
    QAction *posterizationAction;
    QAction *blurAction;
    QAction *gaussianBlurAction;
    QAction *sobelEdgeAction;
    QAction *laplacianEdgeAction;
    QAction *cannyEdgeAction;
    QAction *prewittEdgeAction;
    QAction *customFilterAction;
    QAction *medianFilterAction;
    QAction *bitwiseOperationAction;
    QAction *twoStepFilterAction;

    cv::Mat originalImage;
    double currentScale;

    void createMenu();
    void duplicateImage();
    void saveImageAs();
    void updateImage();
    void updateZoomLabel();
    void setZoomFromInput();
    void updateHistogram();
    void updateHistogramTable();  // Function to update table
    void toggleHistogram();
    void toggleLUT();  // New function to show/hide the table
    void convertToGrayscale();
    void splitColorChannels();
    void convertToHSVLab();
    void applyNegation();
    void stretchHistogram();
    void equalizeHistogram();
    void rangeStretching();
    void applyPosterization();
    void applyBlur();
    void applyGaussianBlur();
    void applySobelEdgeDetection();
    void applyLaplacianEdgeDetection();
    void applyCannyEdgeDetection();
    void applySharpening(int option);
    void applyPrewittEdgeDetection();
    void setPrewittDirection(int direction);
    void applyCustomFilter();
    void applyMedianFilter();
    void applyBitwiseOperation();
    void applyTwoStepFilter();
    cv::Mat applyTwoStepFilterOperation(const cv::Mat& input);
    QImage MatToQImage(const cv::Mat &mat);
};

#endif // IMAGEVIEWER_H
