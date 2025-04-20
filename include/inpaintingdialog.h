#ifndef INPAINTINGDIALOG_H
#define INPAINTINGDIALOG_H

#include <QDialog>
#include <QMap>
#include <QVector> // Include necessary header
#include <opencv2/core.hpp> // Include OpenCV header

// Forward declarations
class QComboBox;
class QListWidget;
class QPushButton;
class QLabel;
class ImageViewer; // Use forward declaration
class QVBoxLayout;
class QHBoxLayout; // Added
class QSpinBox;
class QSlider;
class QGroupBox;
class QWidget; // For mainWindowContext type

/**
 * @brief Dialog for configuring and initiating image inpainting.
 * Allows selecting a mask source (drawing or using another image)
 * and provides controls for drawing parameters.
 */
class InpaintingDialog : public QDialog {
    Q_OBJECT

public:

    cv::Mat drawnMask;
    /**
     * @brief Constructor for InpaintingDialog.
     * @param parentViewer The ImageViewer instance this dialog operates on (for drawing).
     * @param openedImages List of other open widgets (potential ImageViewers) to use as masks.
     * @param mainWindowContext Pointer to a context widget (e.g., MainWindow) used for creating new windows.
     */
    explicit InpaintingDialog(ImageViewer *parentViewer, const QVector<QWidget*>& openedImages, QWidget* mainWindowContext);

    /**
     * @brief Gets the selected mask (either drawn or from another image).
     * @return cv::Mat The mask image (CV_8UC1) or an empty Mat if invalid.
     */
    cv::Mat getSelectedMask() const;
    void refreshImageList();

    // signals:
    // No signals needed currently

signals:
    void maskChanged();

private slots:
    /**
     * @brief Updates the UI visibility based on the selected mask source.
     * Also enables/disables drawing mode on the parent viewer.
     */
    void updateUi();

    /**
     * @brief Slot called when the brush thickness spinbox value changes.
     * Updates the parent viewer's brush thickness.
     * @param value The new thickness value.
     */
    void onBrushThicknessChanged(int value);

    /**
     * @brief Slot called when the "Open Mask..." button is clicked.
     * Opens the currently drawn mask in a new ImageViewer window.
     */
    void openMaskInNewViewer();

    /**
     * @brief Slot connected to the main action button (e.g., "Apply Inpaint").
     * Disables drawing mode and accepts the dialog.
     */
    void handleAccepted();

    /**
     * @brief Slot connected to the cancel button.
     * Disables drawing mode, potentially clears mask, and rejects the dialog.
     */
    void handleRejected();

private:
    /**
     * @brief Sets up the UI elements and layout for the dialog.
     */
    void setupUi();

    // Member Variables
    ImageViewer* parentViewer = nullptr; ///< Pointer to the parent ImageViewer for drawing interaction.
    QWidget* mainWindowContext = nullptr; ///< Context for creating new top-level windows.
    const QVector<QWidget*>& openedImages;

    // UI Elements
    QVBoxLayout* mainLayout = nullptr;
    QComboBox* maskSourceCombo = nullptr;
    QListWidget* imageList = nullptr; ///< List to select image mask source.

    // Drawing Controls Group (Managed visibility)
    QGroupBox* drawingControlsGroup = nullptr; ///< GroupBox for drawing-related controls.
    QSpinBox* brushThicknessSpin = nullptr; ///< SpinBox for brush thickness.
    QPushButton* clearMaskButton = nullptr; ///< Button to clear the drawn mask.
    QPushButton* openMaskButton = nullptr; ///< Button to open the drawn mask in a new viewer.

    // Action Buttons
    QPushButton* processButton = nullptr; ///< The main action button (Accept).
    QPushButton* cancelButton = nullptr; ///< The cancel button (Reject).

    QMap<QString, ImageViewer*> imageViewerMap; ///< Maps list widget item text to ImageViewer pointers.
};

#endif // INPAINTINGDIALOG_H
