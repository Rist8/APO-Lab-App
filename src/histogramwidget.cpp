#include "histogramwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <QVBoxLayout>

HistogramWidget::HistogramWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(276);
    setMaximumHeight(532);
    setMinimumWidth(276);
    setMaximumWidth(1044);
    setMouseTracking(true);  // Enable tracking even without clicking
    setVisible(false);  // Start hidden
}

void HistogramWidget::computeHistogram(const cv::Mat &grayImage) {
    if (grayImage.empty()) return;

    histogramData.fill(0, 256);
    for (int y = 0; y < grayImage.rows; y++) {
        for (int x = 0; x < grayImage.cols; x++) {
            int pixelValue = grayImage.at<uchar>(y, x);
            histogramData[pixelValue]++;
        }
    }

    maxHistogramValue = *std::max_element(histogramData.begin(), histogramData.end());
    setVisible(true);  // Show only if computed
    update();
}

void HistogramWidget::paintEvent(QPaintEvent *event) {
    if (!isVisible()) return;  // Do nothing if hidden

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    int width = this->width();
    int height = this->height();
    std::cout << width << ' ' << height << std::endl;

    painter.fillRect(0, 0, width + 20, height, Qt::white);

    if (maxHistogramValue == 1) return;

    int barWidth = std::max(1, width / 256);
    for (int i = 0; i < 256; ++i) {
        int barHeight = (histogramData[i] * height) / maxHistogramValue;
        painter.setPen(Qt::black);
        painter.drawLine(i * barWidth + 10, height, i * barWidth + 10, height - barHeight);
    }
}

void HistogramWidget::mouseMoveEvent(QMouseEvent *event) {
    int width = this->width();
    int barWidth = std::max(1, width / 256);

    int binIndex = (event->pos().x() - 10) / barWidth; // Map x-position to histogram bin
    if (binIndex < 0 || binIndex >= 256) return;

    int count = histogramData[binIndex]; // Get the corresponding histogram value
    QToolTip::showText(event->globalPos(),
                       QString("Intensity: %1\nCount: %2").arg(binIndex).arg(count), this);
}

