#ifndef BITWISEOPERATIONDIALOG_H
#define BITWISEOPERATIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QListWidget>
#include <opencv2/opencv.hpp>

class BitwiseOperationDialog : public QDialog {
    Q_OBJECT

public:
    explicit BitwiseOperationDialog(QWidget *parent, const QVector<QWidget*>& openedImages);
    cv::Mat getResult() const { return resultImage; }
    int getSelectedImageIndex() const { return imageList->currentRow(); }
    QString getOperationType() const { return operationCombo->currentText(); }
    double getAlpha() const { return alphaSpin->value() / 100.0; }

private slots:
    void processOperation();
    void updateUi();

private:
    QComboBox *operationCombo;
    QListWidget *imageList;
    QSpinBox *alphaSpin;
    QPushButton *processButton;
    cv::Mat resultImage;

    void setupUi();
    void performOperation();
};

#endif // BITWISEOPERATIONDIALOG_H
