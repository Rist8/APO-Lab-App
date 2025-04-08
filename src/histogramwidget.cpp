#include "histogramwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>
#include <QLinearGradient>
#include <cmath>

HistogramWidget::HistogramWidget(QWidget *parent) : QWidget(parent), hoveredBin(-1) {
    setMinimumHeight(276);
    setMaximumHeight(788);
    setMinimumWidth(276);
    setMaximumWidth(1044);
    setMouseTracking(true);
    setVisible(false);
}

void HistogramWidget::computeHistogram(const cv::Mat &grayImage) {
    if (grayImage.empty()) return;

    histogramData.fill(0, 256);
    for (int y = 0; y < grayImage.rows; ++y) {
        for (int x = 0; x < grayImage.cols; ++x) {
            int pixelValue = grayImage.at<uchar>(y, x);
            ++histogramData[pixelValue];
        }
    }

    maxHistogramValue = *std::max_element(histogramData.begin(), histogramData.end());
    setVisible(true);
    update();
}

void HistogramWidget::paintEvent(QPaintEvent *event) {
    if (!isVisible()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();
    painter.fillRect(0, 0, width, height, Qt::white);

    if (maxHistogramValue <= 1) return;

    // Compute precise bar width for smooth scaling
    double barWidth = static_cast<double>(width - 20) / 256.0;

    // Draw gridlines
    painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    for (int i = 1; i <= 4; ++i) {
        int yGrid = height - (i * height / 4);
        painter.drawLine(10, yGrid, width - 10, yGrid);
    }

    // Draw histogram bars
    for (int i = 0; i < 256; ++i) {
        double barHeight = std::sqrt(histogramData[i]) * (height / std::sqrt(maxHistogramValue));
        int xPos = static_cast<int>(i * barWidth) + 10;
        int yPos = height - static_cast<int>(barHeight);

        QLinearGradient gradient(xPos, height, xPos, yPos);
        gradient.setColorAt(0, Qt::black);
        gradient.setColorAt(1, Qt::gray);
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);

        if (i == hoveredBin) {
            painter.setBrush(QColor(255, 0, 0, 150));  // Highlight hovered bar in semi-transparent red
        }

        painter.drawRect(xPos, yPos, std::max(1, static_cast<int>(barWidth)), barHeight);
    }
}

void HistogramWidget::mouseMoveEvent(QMouseEvent *event) {
    int width = this->width();
    double barWidth = static_cast<double>(width - 20) / 256.0;

    int binIndex = static_cast<int>((event->pos().x() - 10) / barWidth);
    if (binIndex < 0 || binIndex >= 256) return;

    if (binIndex != hoveredBin) {
        hoveredBin = binIndex;
        update();  // Redraw to show highlight
    }

    int count = histogramData[binIndex];
    QToolTip::showText(event->globalPos(),
                       QString("Intensity: %1\nCount: %2").arg(binIndex).arg(count), this);
}
