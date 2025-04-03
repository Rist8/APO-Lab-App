#ifndef DIRECTIONSELECTIONDIALOG_H
#define DIRECTIONSELECTIONDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QGridLayout>

class DirectionSelectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit DirectionSelectionDialog(QWidget *parent = nullptr);

signals:
    void directionSelected(int);

private slots:
    void setDirection(int direction);
};

#endif // DIRECTIONSELECTIONDIALOG_H
