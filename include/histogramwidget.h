#ifndef HISTOGRAMWIDGET_H
#define HISTOGRAMWIDGET_H

#include <QWidget>
#include <QVector>
#include <opencv2/opencv.hpp>

// Forward declarations
class QPaintEvent;
class QMouseEvent;
class QEvent;
class QWheelEvent; // Keep forward declaration

class HistogramWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget *parent = nullptr);
    void computeHistogram(const cv::Mat &grayImage);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override; // Keep wheel event override

private:
    int hoveredBin = -1;
    QVector<int> histogramData;
    int maxHistogramValue;
};

#endif // HISTOGRAMWIDGET_H
