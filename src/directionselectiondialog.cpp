#include "directionselectiondialog.h"
#include <QGridLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QDebug>
#include <QMessageBox>

// Constructor
DirectionSelectionDialog::DirectionSelectionDialog(QWidget *parent)
    : PreviewDialogBase(parent), selectedDirection(-1) {

    setWindowTitle("Select Edge Detection Direction");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *gridLayout = new QGridLayout();

    QStringList directions = {"↖", "↑", "↗", "←", "•", "→", "↙", "↓", "↘"};
    int index = 0;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            QPushButton *btn = new QPushButton(directions[index], this);
            btn->setFixedSize(50, 50);
            btn->setFont(QFont("Segoe UI Symbol", 16));

            if (directions[index] == "•") {
                btn->setEnabled(false);
            } else {
                int direction = index;
                connect(btn, &QPushButton::clicked, this, [this, direction]() {
                    setDirection(direction); // Just set, don’t close
                });
                connect(btn, &QPushButton::clicked, this, &PreviewDialogBase::previewRequested);
            }
            gridLayout->addWidget(btn, row, col);
            ++index;
        }
    }

    previewCheckBox = new QCheckBox("Preview", this);
    previewCheckBox->setChecked(true);
    connect(previewCheckBox, &QCheckBox::checkStateChanged, this, &PreviewDialogBase::previewRequested);

    // Button box for OK/Cancel
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DirectionSelectionDialog::onAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &DirectionSelectionDialog::reject);

    // Build layout
    mainLayout->addLayout(gridLayout);
    mainLayout->addWidget(previewCheckBox);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    adjustSize();
}

// When a direction is clicked, just store it
void DirectionSelectionDialog::setDirection(int direction) {
    selectedDirection = direction;
    emit previewRequested();
}

// On OK, emit if direction selected, else warn
void DirectionSelectionDialog::onAccept() {
    if (selectedDirection == -1) {
        QMessageBox::warning(this, "No Direction Selected", "Please select a direction before clicking OK.");
        return;
    }
    accept();
}
