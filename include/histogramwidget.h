#ifndef HISTOGRAMWIDGET_H
#define HISTOGRAMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTableWidget>
#include <opencv2/opencv.hpp>

class HistogramWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistogramWidget(QWidget *parent = nullptr);
    void computeHistogram(const cv::Mat &grayImage);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    int hoveredBin = -1;
    QVector<int> histogramData;
    int maxHistogramValue;
    QTableWidget *histogramTable;
    void updateTable();
};

#endif // HISTOGRAMWIDGET_H
