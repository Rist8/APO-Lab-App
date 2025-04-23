#ifndef BITWISEOPERATIONDIALOG_H
#define BITWISEOPERATIONDIALOG_H

#include "previewdialogbase.h"
#include <QDialog>
#include <QMap> // <-- Added include for QMap
#include <QString> // <-- Added include for QString

#include <opencv2/opencv.hpp> // Keep for cv::Mat
#include <qboxlayout.h>
#include <qcheckbox.h>

// Forward declarations for pointer members to reduce header dependencies
class QComboBox;
class QSpinBox;
class QPushButton;
class QListWidget;
class QLabel;
class ImageViewer; // Forward declare ImageViewer

class BitwiseOperationDialog : public PreviewDialogBase {
    Q_OBJECT

public:
    // Constructor now takes parent and the list of available image viewers
    explicit BitwiseOperationDialog(QWidget *parent, const QVector<QWidget*>& openedImages);

    // Getter for the resulting image after processing
    cv::Mat getResult() const { return resultImage; }


    QCheckBox* getPreviewCheckBox() const { return previewCheckBox; }

    // Getters for dialog parameters (optional, might not be needed externally)
    // int getSelectedImageIndex() const; // Index might be fragile, title/pointer better
    // QString getOperationType() const;
    // double getAlpha() const;

private slots:
    // Slot connected to the "Process" button click
    void processOperation();
    // Slot to update UI elements based on selected operation (e.g., show/hide alpha)
    void updateUi();

signals:
    void previewRequested();

private:
    // --- UI Elements (Declared as members) ---
    QVBoxLayout *mainLayout;
    QCheckBox* previewCheckBox;
    QComboBox *operationCombo;
    QComboBox *imageCombo1;
    QComboBox *imageCombo2;
    QHBoxLayout *alphaLayout; // Layout containing alpha controls
    QLabel *alphaLabel;       // <-- Added declaration
    QSpinBox *alphaSpin;
    QPushButton *processButton;
    QPushButton *cancelButton;  // <-- Added declaration

    // --- Data Members ---
    cv::Mat resultImage; // Stores the result of the operation
    // Map to associate list widget item text (window title) with the actual viewer
    QMap<QString, ImageViewer*> imageViewerMap; // <-- Added declaration

    // --- Helper Methods ---
    // Sets up the UI layout and widgets
    void setupUi();
    // Performs the actual image processing based on selections
    // Now takes pointers to the relevant viewers as arguments
    void performOperation(ImageViewer* currentViewer, ImageViewer* secondViewer); // <-- Updated signature

};

#endif // BITWISEOPERATIONDIALOG_H
