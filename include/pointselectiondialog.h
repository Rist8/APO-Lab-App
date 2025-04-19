#ifndef POINTSELECTIONDIALOG_H
#define POINTSELECTIONDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <opencv2/core.hpp>
#include <qlabel.h>

class ImageViewer; // Forward declaration

class PointSelectionDialog : public QDialog {
    Q_OBJECT
public:
    PointSelectionDialog(QWidget* parent = nullptr) : QDialog(parent) {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Click required points on the image."));

        QHBoxLayout* btnLayout = new QHBoxLayout();
        QPushButton* ok = new QPushButton("OK");
        QPushButton* cancel = new QPushButton("Cancel");
        connect(ok, &QPushButton::clicked, this, &QDialog::accept);
        connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        btnLayout->addWidget(ok);
        btnLayout->addWidget(cancel);

        layout->addLayout(btnLayout);
    }
};


#endif // POINTSELECTIONDIALOG_H
