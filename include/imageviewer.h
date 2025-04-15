#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <stack>
#include <vector>
#include <functional>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QFileDialog>
#include <QCloseEvent>
#include <opencv2/opencv.hpp>
#include <QImage>
#include <QPixmap>
#include "clickablelabel.h"
// Keep histogramwidget.h include as we need the class definition for the pointer
#include "histogramwidget.h"
#include "mainwindow.h" // Forward declaration might be sufficient if only pointers used
#include "imageoperation.h" // Forward declaration might be sufficient

// Forward declarations
class MainWindow;
class ImageOperation;
class QLineEdit;
class QTableWidget;
class HistogramWidget; // Added forward declaration

// Enum for morphology structuring element type
enum StructuringElementType {
    Diamond, // Typically 4-connected for 3x3
    Square   // Typically 8-connected for 3x3
};

class ImageViewer : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewer(const cv::Mat &image, const QString &title, QWidget *parent = nullptr,
                         QPoint position = QPoint(100, 100), MainWindow *mainWindow = nullptr);

    // --- Getters ---
    cv::Mat getOriginalImage() const { return originalImage; }
    MainWindow* getMainWindow() const { return mainWindow; }

    // --- Setters ---
    void setUsePyramidScaling(bool enable) { usePyramidScaling = enable; updateImage();}

    // --- UI Control ---
    void setZoom(double scale) { currentScale = scale; updateImage(); }
    std::vector<cv::Point> getUserSelectedPoints(int count); // For interactive point selection

    // --- Undo/Redo ---
    void undo();
    void redo();
    void pushToUndoStack();  // Call before modifying `originalImage` via an operation

protected:
    // --- Event Handlers ---
    void wheelEvent(QWheelEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    // --- Internal Slots ---
    void onImageClicked(QMouseEvent* event); // Handles clicks for point selection
    void setZoomFromInput(); // Handles zoom input from QLineEdit
    void showHistogram(); // Slot to show the histogram in a separate window
    void onHistogramClosed(); // Slot connected to histogram window's destroyed signal
    void toggleLUT(); // Shows/hides the LUT table
    void setPrewittDirection(int direction); // Slot to receive direction from dialog

    // --- Operation Slots (Wrappers) ---
    // These slots are connected to menu actions and call the actual processing functions.
    void duplicateImage();
    void saveImageAs();
    void convertToGrayscale();
    void binarise();
    void splitColorChannels();
    void convertToHSVLab(); // Splits into HSV and Lab viewers
    void stretchHistogram();
    void equalizeHistogram();
    void applyNegation();
    void rangeStretching();
    void applyPosterization();
    void applyBitwiseOperation();
    void showLineProfile();
    void applyBlur();
    void applyGaussianBlur();
    void applySobelEdgeDetection();
    void applyLaplacianEdgeDetection();
    void applyCannyEdgeDetection();
    void applyHoughLineDetection();
    void applySharpening(int option); // Option selects kernel type
    void applyPrewittEdgeDetection(); // Opens direction dialog
    void applyCustomFilter();
    void applyMedianFilter();
    void applyTwoStepFilter();
    void applyErosion(StructuringElementType type);
    void applyDilation(StructuringElementType type);
    void applyOpening(StructuringElementType type);
    void applyClosing(StructuringElementType type);
    void applySkeletonization();


private:
    // --- UI Elements ---
    ClickableLabel *imageLabel;
    QLineEdit *zoomInput;
    HistogramWidget *histogramWindow = nullptr; // Pointer to the separate histogram window
    QTableWidget *LUT;          // Table widget for histogram data (remains embedded)
    QVBoxLayout *mainLayout;
    // Removed QHBoxLayout *imageHistogramLayout;
    QMenuBar *menuBar;
    QAction* showHistogramAction; // Action to show histogram window (was histogramAction)

    // --- State ---
    cv::Mat originalImage;      // The currently displayed image data
    std::stack<cv::Mat> undoStack;
    std::stack<cv::Mat> redoStack;
    double currentScale;        // Current zoom level (1.0 = 100%)
    MainWindow *mainWindow;     // Pointer to the main application window
    QList<ImageOperation*> operationsList; // List of registered operations for state updates
    bool usePyramidScaling = false;

    // Morphology element types (could be moved to dialogs if needed per-operation)
    StructuringElementType erosionElement = Diamond;
    StructuringElementType dilationElement = Diamond;
    StructuringElementType openingElement = Diamond;
    StructuringElementType closingElement = Diamond;

    // Point selection state
    int pointsToSelect = 0;
    std::vector<cv::Point> selectedPoints;
    std::function<void()> pointSelectionCallback; // Callback for async point selection

    // --- Initialization ---
    void createMenu();          // Sets up the menu bar and actions
    void registerOperation(ImageOperation *op); // Adds an operation for state management

    // --- UI Updates ---
    void updateImage();         // Updates the displayed pixmap, resizes window
    void updateZoomLabel();     // Updates the text in the zoom input field
    void updateHistogram();     // Recalculates and updates the histogram window (if exists)
    void updateHistogramTable(); // Updates the LUT QTableWidget
    void updateOperationsEnabledState(); // Enables/disables menu actions based on image type
    void showTempImage(const cv::Mat &temp); // Temporarily displays an image (e.g., for line profile)

    // --- Helpers ---
    void clearRedoStack();      // Clears the redo stack (used after a new operation)
    QImage MatToQImage(const cv::Mat &mat); // Converts cv::Mat to QImage
};

#endif // IMAGEVIEWER_H
