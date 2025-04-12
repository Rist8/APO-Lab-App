#include "directionselectiondialog.h"
#include <QGridLayout> // Include necessary header
#include <QPushButton> // Include necessary header

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Constructor: Sets up the dialog with buttons for selecting edge direction.
// ==========================================================================
DirectionSelectionDialog::DirectionSelectionDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Select Edge Detection Direction");
    QGridLayout *layout = new QGridLayout(this);

    // Unicode arrows for directions, plus a disabled center button
    QStringList directions = {"↖", "↑", "↗", "←", "•", "→", "↙", "↓", "↘"};
    int index = 0;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            QPushButton *btn = new QPushButton(directions[index], this);
            btn->setFixedSize(50, 50); // Fixed size for square buttons
            btn->setFont(QFont("Arial", 16)); // Make arrows larger

            if (directions[index] == "•") { // Disable the center button
                btn->setEnabled(false);
            } else {
                // Connect only enabled buttons
                int direction = index; // Capture index for lambda
                connect(btn, &QPushButton::clicked, this, [this, direction]() {
                    setDirection(direction);
                });
            }
            layout->addWidget(btn, row, col);
            index++;
        }
    }
    setLayout(layout);
    adjustSize(); // Adjust dialog size to fit buttons
}

// ==========================================================================
// Group 3: UI Dialogs & Input
// ==========================================================================
// Slot called when a direction button is clicked. Emits the selected
// direction signal and accepts the dialog.
// ==========================================================================
void DirectionSelectionDialog::setDirection(int direction) {
    emit directionSelected(direction); // Emit signal with the selected index
    accept(); // Close the dialog with accept state
}
