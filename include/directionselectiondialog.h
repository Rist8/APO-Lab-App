#ifndef DIRECTIONSELECTIONDIALOG_H
#define DIRECTIONSELECTIONDIALOG_H

#include "previewdialogbase.h"
#include <QDialog>
#include <QPushButton>
#include <QGridLayout>
#include <qcheckbox.h>

class DirectionSelectionDialog : public PreviewDialogBase {
    Q_OBJECT
public:
    explicit DirectionSelectionDialog(QWidget *parent = nullptr);
    QCheckBox* getPreviewCheckBox() const { return previewCheckBox; }
    int getSelectedDirection() const { return selectedDirection; }

private slots:
    void setDirection(int direction);
    void onAccept();

signals:
    void previewRequested();

private:
    QCheckBox* previewCheckBox;
    int selectedDirection = -1; // Stores the chosen direction
};

#endif // DIRECTIONSELECTIONDIALOG_H
