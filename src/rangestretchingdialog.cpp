#include "rangestretchingdialog.h"

RangeStretchingDialog::RangeStretchingDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Range Stretching Parameters");
    setFixedSize(300, 250);

    // Create SpinBoxes & Sliders
    p1SpinBox = new QSpinBox(this);
    p2SpinBox = new QSpinBox(this);
    q3SpinBox = new QSpinBox(this);
    q4SpinBox = new QSpinBox(this);

    p1Slider = new QSlider(Qt::Horizontal, this);
    p2Slider = new QSlider(Qt::Horizontal, this);
    q3Slider = new QSlider(Qt::Horizontal, this);
    q4Slider = new QSlider(Qt::Horizontal, this);

    // Set ranges
    p1SpinBox->setRange(0, 255); p1Slider->setRange(0, 255);
    p2SpinBox->setRange(0, 255); p2Slider->setRange(0, 255);
    q3SpinBox->setRange(0, 255); q3Slider->setRange(0, 255);
    q4SpinBox->setRange(0, 255); q4Slider->setRange(0, 255);

    // Default values
    p1SpinBox->setValue(50); p2SpinBox->setValue(200);
    q3SpinBox->setValue(0); q4SpinBox->setValue(255);

    p1Slider->setValue(50); p2Slider->setValue(200);
    q3Slider->setValue(0); q4Slider->setValue(255);

    // Connect Sliders and SpinBoxes
    connect(p1Slider, &QSlider::valueChanged, p1SpinBox, &QSpinBox::setValue);
    connect(p1SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), p1Slider, &QSlider::setValue);

    connect(p2Slider, &QSlider::valueChanged, p2SpinBox, &QSpinBox::setValue);
    connect(p2SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), p2Slider, &QSlider::setValue);

    connect(q3Slider, &QSlider::valueChanged, q3SpinBox, &QSpinBox::setValue);
    connect(q3SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), q3Slider, &QSlider::setValue);

    connect(q4Slider, &QSlider::valueChanged, q4SpinBox, &QSpinBox::setValue);
    connect(q4SpinBox, QOverload<int>::of(&QSpinBox::valueChanged), q4Slider, &QSlider::setValue);

    // Layouts
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    auto createRow = [](const QString &labelText, QSpinBox *spinBox, QSlider *slider) {
        QHBoxLayout *rowLayout = new QHBoxLayout();
        QLabel *label = new QLabel(labelText);
        label->setFixedWidth(50);
        rowLayout->addWidget(label);
        rowLayout->addWidget(spinBox);
        rowLayout->addWidget(slider);
        return rowLayout;
    };

    mainLayout->addLayout(createRow("p1:", p1SpinBox, p1Slider));
    mainLayout->addLayout(createRow("p2:", p2SpinBox, p2Slider));
    mainLayout->addLayout(createRow("q3:", q3SpinBox, q3Slider));
    mainLayout->addLayout(createRow("q4:", q4SpinBox, q4Slider));

    QPushButton *okButton = new QPushButton("OK");
    QPushButton *cancelButton = new QPushButton("Cancel");

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

// Getters
int RangeStretchingDialog::getP1() const { return p1SpinBox->value(); }
int RangeStretchingDialog::getP2() const { return p2SpinBox->value(); }
int RangeStretchingDialog::getQ3() const { return q3SpinBox->value(); }
int RangeStretchingDialog::getQ4() const { return q4SpinBox->value(); }
