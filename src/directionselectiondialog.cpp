#include "directionselectiondialog.h"

DirectionSelectionDialog::DirectionSelectionDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Select Edge Detection Direction");
    QGridLayout *layout = new QGridLayout(this);

    QStringList directions = {"↖", "↑", "↗", "←", "•", "→", "↙", "↓", "↘"};
    int index = 0;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            QPushButton *btn = new QPushButton(directions[index], this);
            btn->setFixedSize(50, 50);
            if (directions[index] == "•") {
                btn->setEnabled(false);
            }
            layout->addWidget(btn, row, col);
            int direction = index; // Capture index for lambda
            connect(btn, &QPushButton::clicked, this, [this, direction]() {
                setDirection(direction);
            });
            index++;
        }
    }
    setLayout(layout);
}

void DirectionSelectionDialog::setDirection(int direction) {
    emit directionSelected(direction);
    accept();
}
