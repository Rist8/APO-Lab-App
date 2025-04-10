#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QLabel>
#include <stack>
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
#include "imageoperation.h"

enum StructuringElementType {
    Diamond,
    Square
};

class ImageViewer : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewer(const cv::Mat &image, const QString &title, QWidget *parent = nullptr,
                         QPoint position = QPoint(100, 100), MainWindow *mainWindow = nullptr);

    cv::Mat getOriginalImage() const { return originalImage; }
    MainWindow* getMainWindow() const { return mainWindow; }
    void setZoom(double scale) { currentScale = scale; updateImage(); }

    void undo();
    void redo();
    void pushToUndoStack();  // Call before modifying `originalImage`

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
    QAction* histogramAction;
    QList<ImageOperation*> operationsList;
    void registerOperation(ImageOperation *op);
    void updateOperationsEnabledState();

    cv::Mat originalImage;
    std::stack<cv::Mat> undoStack;
    std::stack<cv::Mat> redoStack;
    double currentScale;

    void createMenu();
    void duplicateImage();
    void saveImageAs();
    void updateImage();
    void clearRedoStack(); // Helper
    void updateZoomLabel();
    void setZoomFromInput();
    void updateHistogram();
    void updateHistogramTable();  // Function to update table
    void toggleHistogram();
    void toggleLUT();  // New function to show/hide the table
    void binarise();
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
    cv::Mat getStructuringElement(StructuringElementType type);
    void applyErosion(StructuringElementType type);
    void applyDilation(StructuringElementType type);
    void applyOpening(StructuringElementType type);
    void applyClosing(StructuringElementType type);
    void applySkeletonization();
    QImage MatToQImage(const cv::Mat &mat);

    StructuringElementType erosionElement = Diamond;
    StructuringElementType dilationElement = Diamond;
    StructuringElementType openingElement = Diamond;
    StructuringElementType closingElement = Diamond;
};

#endif // IMAGEVIEWER_H
