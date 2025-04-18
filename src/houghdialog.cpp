#include "houghdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDialogButtonBox>

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Constructor: Sets up the dialog for Hough Line Transform parameters.
// ==========================================================================
HoughDialog::HoughDialog(QWidget* parent) : PreviewDialogBase(parent) {
    setWindowTitle("Hough Line Detection Parameters");

    auto layout = new QVBoxLayout(this);

    // Rho
    auto rhoLayout = new QHBoxLayout();
    rhoLayout->addWidget(new QLabel("Rho (distance resolution):"));
    rhoSpin = new QDoubleSpinBox();
    rhoSpin->setRange(0.1, 10.0);
    rhoSpin->setSingleStep(0.1);
    rhoSpin->setValue(1.0);
    connect(rhoSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
    rhoLayout->addWidget(rhoSpin);
    layout->addLayout(rhoLayout);

    // Theta
    auto thetaLayout = new QHBoxLayout();
    thetaLayout->addWidget(new QLabel("Theta (angle resolution in degrees):"));
    thetaSpin = new QDoubleSpinBox();
    thetaSpin->setRange(0.1, 180.0); // Degrees input
    thetaSpin->setSingleStep(1.0);
    thetaSpin->setValue(1.0);        // Default 1 degree
    connect(thetaSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
    thetaLayout->addWidget(thetaSpin);
    layout->addLayout(thetaLayout);

    // Threshold
    auto threshLayout = new QHBoxLayout();
    threshLayout->addWidget(new QLabel("Threshold (min votes to detect a line):"));
    thresholdSpin = new QSpinBox();
    thresholdSpin->setRange(1, 1000);
    thresholdSpin->setValue(160); // Default threshold
    connect(thresholdSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PreviewDialogBase::previewRequested);
    threshLayout->addWidget(thresholdSpin);
    layout->addLayout(threshLayout);

    // Buttons
    previewCheckBox = new QCheckBox("Preview");
    previewCheckBox->setChecked(false); // default off
    connect(previewCheckBox, &QCheckBox::checkStateChanged, this, &PreviewDialogBase::previewRequested);
    layout->addWidget(previewCheckBox);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Getter for the Rho parameter.
// ==========================================================================
double HoughDialog::getRho() const {
    return rhoSpin->value();
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Getter for the Theta parameter (in degrees).
// ==========================================================================
double HoughDialog::getThetaDegrees() const {
    return thetaSpin->value();
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Getter for the Threshold parameter.
// ==========================================================================
int HoughDialog::getThreshold() const {
    return thresholdSpin->value();
}
