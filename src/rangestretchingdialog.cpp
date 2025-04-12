#include "rangestretchingdialog.h"
#include <QVBoxLayout>   // Include necessary header
#include <QHBoxLayout>   // Include necessary header
#include <QLabel>        // Include necessary header
#include <QSpinBox>      // Include necessary header
#include <QSlider>       // Include necessary header
#include <QPushButton>   // Include necessary header

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Constructor: Sets up the dialog for range stretching parameters (p1, p2, q3, q4).
// ==========================================================================
RangeStretchingDialog::RangeStretchingDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Range Stretching Parameters");
    // setFixedSize(300, 250); // Avoid fixed size

    // Create SpinBoxes & Sliders
    p1SpinBox = new QSpinBox(this);
    p2SpinBox = new QSpinBox(this);
    q3SpinBox = new QSpinBox(this);
    q4SpinBox = new QSpinBox(this);

    p1Slider = new QSlider(Qt::Horizontal, this);
    p2Slider = new QSlider(Qt::Horizontal, this);
    q3Slider = new QSlider(Qt::Horizontal, this);
    q4Slider = new QSlider(Qt::Horizontal, this);

    // Set ranges for SpinBoxes and Sliders
    p1SpinBox->setRange(0, 255); p1Slider->setRange(0, 255);
    p2SpinBox->setRange(0, 255); p2Slider->setRange(0, 255);
    q3SpinBox->setRange(0, 255); q3Slider->setRange(0, 255);
    q4SpinBox->setRange(0, 255); q4Slider->setRange(0, 255);

    // Set reasonable default values
    p1SpinBox->setValue(50);
    p2SpinBox->setValue(200);
    q3SpinBox->setValue(0);
    q4SpinBox->setValue(255);

    // Sync slider values with spinbox defaults
    p1Slider->setValue(p1SpinBox->value());
    p2Slider->setValue(p2SpinBox->value());
    q3Slider->setValue(q3SpinBox->value());
    q4Slider->setValue(q4SpinBox->value());


    // Connect Sliders and SpinBoxes bidirectionally
    connect(p1Slider, &QSlider::valueChanged, p1SpinBox, &QSpinBox::setValue);
    connect(p1SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), p1Slider, &QSlider::setValue);
    // Add validation for p1 < p2
    connect(p1SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val){ if(val >= p2SpinBox->value()) p2SpinBox->setValue(val + 1); });
    connect(p1Slider, &QSlider::valueChanged, this, [this](int val){ if(val >= p2Slider->value()) p2Slider->setValue(val + 1); });


    connect(p2Slider, &QSlider::valueChanged, p2SpinBox, &QSpinBox::setValue);
    connect(p2SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), p2Slider, &QSlider::setValue);
    // Add validation for p2 > p1
    connect(p2SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val){ if(val <= p1SpinBox->value()) p1SpinBox->setValue(val - 1); });
    connect(p2Slider, &QSlider::valueChanged, this, [this](int val){ if(val <= p1Slider->value()) p1Slider->setValue(val - 1); });


    connect(q3Slider, &QSlider::valueChanged, q3SpinBox, &QSpinBox::setValue);
    connect(q3SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), q3Slider, &QSlider::setValue);
        // Add validation for q3 < q4
    connect(q3SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val){ if(val >= q4SpinBox->value()) q4SpinBox->setValue(val + 1); });
    connect(q3Slider, &QSlider::valueChanged, this, [this](int val){ if(val >= q4Slider->value()) q4Slider->setValue(val + 1); });


    connect(q4Slider, &QSlider::valueChanged, q4SpinBox, &QSpinBox::setValue);
    connect(q4SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), q4Slider, &QSlider::setValue);
    // Add validation for q4 > q3
    connect(q4SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val){ if(val <= q3SpinBox->value()) q3SpinBox->setValue(val - 1); });
    connect(q4Slider, &QSlider::valueChanged, this, [this](int val){ if(val <= q3Slider->value()) q3Slider->setValue(val - 1); });


    // Layouts
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Helper lambda to create a labeled row with SpinBox and Slider
    auto createRow = [](const QString &labelText, QSpinBox *spinBox, QSlider *slider) {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *label = new QLabel(labelText);
        label->setFixedWidth(30); // Adjust width as needed
        rowLayout->addWidget(label);
        rowLayout->addWidget(spinBox);
        rowLayout->addWidget(slider);
        return rowLayout;
    };

    mainLayout->addWidget(new QLabel("Input Range (p1, p2):"));
    mainLayout->addLayout(createRow("p1:", p1SpinBox, p1Slider));
    mainLayout->addLayout(createRow("p2:", p2SpinBox, p2Slider));
    mainLayout->addSpacing(10); // Add space between input/output ranges
    mainLayout->addWidget(new QLabel("Output Range (q3, q4):"));
    mainLayout->addLayout(createRow("q3:", q3SpinBox, q3Slider));
    mainLayout->addLayout(createRow("q4:", q4SpinBox, q4Slider));

    // Buttons
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // Push buttons to the right
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
    adjustSize(); // Adjust size to content
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Getter for the p1 parameter.
// ==========================================================================
int RangeStretchingDialog::getP1() const { return p1SpinBox->value(); }

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Getter for the p2 parameter.
// ==========================================================================
int RangeStretchingDialog::getP2() const { return p2SpinBox->value(); }

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Getter for the q3 parameter.
// ==========================================================================
int RangeStretchingDialog::getQ3() const { return q3SpinBox->value(); }

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Getter for the q4 parameter.
// ==========================================================================
int RangeStretchingDialog::getQ4() const { return q4SpinBox->value(); }
