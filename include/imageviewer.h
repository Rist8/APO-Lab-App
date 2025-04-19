#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QLabel>
#include <qcheckbox.h>
#include <stack>
#include <vector>
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
#include "previewdialogbase.h"

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
    // ======================================================================
    // `Constructor & Core Management`
    // ======================================================================
    explicit ImageViewer(const cv::Mat &image, const QString &title, QWidget *parent = nullptr,
                         QPoint position = QPoint(100, 100), MainWindow *mainWindow = nullptr);

    // ======================================================================
    // `Getters & Setters`
    // ======================================================================
    // --- Getters ---
    cv::Mat getOriginalImage() const { return originalImage; }
    MainWindow* getMainWindow() const { return mainWindow; }

    // --- Setters ---
    void setUsePyramidScaling(bool enable) { usePyramidScaling = enable; updateImage();}

    // ======================================================================
    // `UI Interaction & Control`
    // ======================================================================
    void setZoom(double scale) { currentScale = scale; updateImage(); }
    std::vector<cv::Point> getSelectedPoints() const { return selectedPoints; }
    void enableInteractivePointSelection();
    void disableInteractivePointSelection();

    // ======================================================================
    // `Undo/Redo Management`
    // ======================================================================
    void undo();
    void redo();
    void pushToUndoStack(); // Call before modifying `originalImage` via an operation

    // ======================================================================
    // `Dialog Preview Helper`
    // ======================================================================
    template<typename Func>
    void setupPreview(PreviewDialogBase* dialog, QCheckBox* previewCheckBox, Func generator) {
        connect(dialog, &QDialog::finished, this, [&]() {
            if (dialog->result() == QDialog::Accepted) {
                pushToUndoStack();
                originalImage = generator();
            }
            updateImage();
        });
        connect(dialog, &PreviewDialogBase::previewRequested, this, [=]() {
            if (previewCheckBox->isChecked()) {
                cv::Mat preview = generator();
                showTempImage(preview);
            } else {
                updateImage();
            }
        });
    }

protected:
    // ======================================================================
    // `Event Handlers`
    // ======================================================================
    void wheelEvent(QWheelEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    // ======================================================================
    // `Internal UI Slots`
    // ======================================================================
    void onImageClicked(QMouseEvent* event); // Handles clicks for point selection
    void setZoomFromInput(); // Handles zoom input from QLineEdit
    void showHistogram(); // Slot to show the histogram in a separate window
    void onHistogramClosed(); // Slot connected to histogram window's destroyed signal
    void toggleLUT(); // Shows/hides the LUT table
    void enablePointSelection(); // Renamed from enableInteractivePointSelection
    void disablePointSelection(); // Renamed from disableInteractivePointSelection

    // ======================================================================
    // `File & View Operations Slots`
    // ======================================================================
    void duplicateImage();
    void saveImageAs();

    // ======================================================================
    // `Image Type Conversion Slots`
    // ======================================================================
    void convertToGrayscale();
    void binarise();
    void splitColorChannels();
    void convertToHSVLab(); // Splits into HSV and Lab viewers

    // ======================================================================
    // `Histogram Operations Slots`
    // ======================================================================
    void stretchHistogram();
    void equalizeHistogram();

    // ======================================================================
    // `Point Operations Slots`
    // ======================================================================
    void applyNegation();
    void rangeStretching();
    void applyPosterization();
    void applyBitwiseOperation();
    void showLineProfile(); // Also includes drawing helpers
    void applyGlobalThreshold();
    void applyAdaptiveThreshold();
    void applyOtsuThreshold();
    void activateMagicWandTool(); // Related to magic wand segmentation trigger
    void applyMagicWandSegmentation();
    void applyGrabCutSegmentation();

    // ======================================================================
    // `Filtering & Edge Detection Slots`
    // ======================================================================
    void applyBlur();
    void applyGaussianBlur();
    void applySobelEdgeDetection();
    void applyLaplacianEdgeDetection();
    void applyCannyEdgeDetection();
    void applyHoughLineDetection(); // Feature detection, but often follows edge detection
    void applySharpening(int option); // Option selects kernel type
    void applyPrewittEdgeDetection(); // Opens direction dialog
    void applyCustomFilter();
    void applyMedianFilter();
    void applyTwoStepFilter();


    // ======================================================================
    // `Morphology Operations Slots`
    // ======================================================================
    void applyErosion(StructuringElementType type);
    void applyDilation(StructuringElementType type);
    void applyOpening(StructuringElementType type);
    void applyClosing(StructuringElementType type);
    void applySkeletonization();


private:
    // ======================================================================
    // `UI Elements`
    // ======================================================================
    ClickableLabel *imageLabel;
    QLineEdit *zoomInput;
    HistogramWidget *histogramWindow = nullptr; // Pointer to the separate histogram window
    QTableWidget *LUT; // Table widget for histogram data (remains embedded)
    QVBoxLayout *mainLayout;
    QMenuBar *menuBar;
    QAction* showHistogramAction; // Action to show histogram window (was histogramAction)

    // ======================================================================
    // `Core State & Data`
    // ======================================================================
    cv::Mat originalImage; // The currently displayed image data
    std::stack<cv::Mat> undoStack;
    std::stack<cv::Mat> redoStack;
    double currentScale; // Current zoom level (1.0 = 100%)
    MainWindow *mainWindow; // Pointer to the main application window
    QList<ImageOperation*> operationsList; // List of registered operations for state updates
    bool usePyramidScaling = false;

    // ======================================================================
    // `Interaction Mode State`
    // ======================================================================
    bool magicWandMode = false;
    bool rectangleMode = false;
    bool selectingPoints = false;
    int pointsToSelect = 0;
    int draggedPointIndex = -1; // Seems unused but kept
    std::vector<cv::Point> selectedPoints;

    // ======================================================================
    // `Morphology State`
    // ======================================================================
    // Morphology element types (could be moved to dialogs if needed per-operation)
    StructuringElementType erosionElement = Diamond;
    StructuringElementType dilationElement = Diamond;
    StructuringElementType openingElement = Diamond;
    StructuringElementType closingElement = Diamond;

    // ======================================================================
    // `Initialization & Setup`
    // ======================================================================
    void createMenu();          // Sets up the menu bar and actions
    void registerOperation(ImageOperation *op); // Adds an operation for state management
    void updateOperationsEnabledState(); // Enables/disables menu actions based on image type

    // ======================================================================
    // `Internal UI Update Helpers`
    // ======================================================================
    void updateImage();         // Updates the displayed pixmap, resizes window
    void updateZoomLabel();     // Updates the text in the zoom input field
    void updateHistogram();     // Recalculates and updates the histogram window (if exists)
    void updateHistogramTable(); // Updates the LUT QTableWidget
    void showTempImage(const cv::Mat &temp); // Temporarily displays an image (e.g., for line profile/preview)
    void drawTemporaryPoints(); // Draws points/lines during selection
    void drawLineProfile(const cv::Point& p1, const cv::Point& p2); // Draws the line profile chart


    // ======================================================================
    // `Helper Functions`
    // ======================================================================
    void clearRedoStack();      // Clears the redo stack (used after a new operation)
    QImage MatToQImage(const cv::Mat &mat); // Converts cv::Mat to QImage
};

#endif // IMAGEVIEWER_H
