#include "histogramwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent> // Included QWheelEvent header
#include <QToolTip>
#include <QLinearGradient>
#include <cmath>
#include <algorithm> // For std::max_element
#include <QtMath>    // For qBound

// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// Constructor: Initializes the histogram widget properties.
// ==========================================================================
HistogramWidget::HistogramWidget(QWidget *parent)
    : QWidget(parent), hoveredBin(-1), maxHistogramValue(0) { // Removed verticalScale initialization
    // Set attributes for a top-level window that deletes itself on close
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Histogram"); // Default title, can be set by ImageViewer

    // Initial size constraints (used for wheel event clamping)
    setMinimumHeight(100 + 20); // Adjusted minimum reasonable height
    setMaximumHeight(1024 + 20); // Adjusted maximum reasonable height
    setMinimumWidth(128 + 20);  // Adjusted minimum reasonable width
    setMaximumWidth(1024 + 20); // Max width same as before

    // Set initial size based on minimum constraints
    // resize(minimumWidth(), minimumHeight()); // Or a default starting size

    setMouseTracking(true); // Enable mouse tracking for hover effects
}

// ==========================================================================
// Group 7: Image Processing - Histogram Operations
// ==========================================================================
// Calculates histogram data from a grayscale image.
// ==========================================================================
void HistogramWidget::computeHistogram(const cv::Mat &grayImage) {
    if (grayImage.empty() || grayImage.channels() != 1) {
        histogramData.fill(0, 256);
        maxHistogramValue = 0;
        // No need to reset verticalScale anymore
        update(); // Trigger repaint to clear the widget view
        return;
    }

    histogramData.fill(0, 256); // Reset histogram data
    maxHistogramValue = 0; // Reset max value

    // Calculate histogram
    for (int y = 0; y < grayImage.rows; ++y) {
        const uchar* row_ptr = grayImage.ptr<uchar>(y);
        for (int x = 0; x < grayImage.cols; ++x) {
            int pixelValue = row_ptr[x];
            histogramData[pixelValue]++;
        }
    }

    // Find the maximum value in the histogram for scaling
    auto maxIt = std::max_element(histogramData.begin(), histogramData.end());
    if (maxIt != histogramData.end()) {
        maxHistogramValue = *maxIt;
    } else {
        maxHistogramValue = 0; // Ensure it's zero if vector was empty or all zeros
    }

    update(); // Request a repaint with the new data
}

// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// Draws the histogram bars, grid, and highlights the hovered bar.
// Reverted to remove verticalScale usage.
// ==========================================================================
void HistogramWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event); // Call base class implementation

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();
    int padding = 10; // Padding around the histogram drawing area
    int drawingWidth = width - 2 * padding;
    int drawingHeight = height - 2 * padding;

    // Background
    painter.fillRect(rect(), Qt::white);

    // Don't draw histogram if no data or drawing area is invalid
    if (maxHistogramValue <= 0 || drawingWidth <= 0 || drawingHeight <= 0) {
        painter.setPen(Qt::black);
        painter.drawText(rect(), Qt::AlignCenter, "No histogram data");
        return;
    }

    // Calculate bar width
    double barWidth = static_cast<double>(drawingWidth) / 256.0;

    // --- Draw Gridlines ---
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    int numGridLines = 4;
    for (int i = 1; i <= numGridLines; ++i) {
        int yGrid = padding + drawingHeight - (i * drawingHeight / numGridLines);
        painter.drawLine(padding, yGrid, width - padding, yGrid);
    }
    // Draw Axes
    painter.setPen(QPen(Qt::black, 1));
    painter.drawLine(padding, padding, padding, height - padding); // Y-axis
    painter.drawLine(padding, height - padding, width - padding, height - padding); // X-axis


    // --- Draw Histogram Bars ---
    // Calculate scale factor based *only* on max value and drawing height
    // Apply square root scaling for better visibility of small counts
    double scaleFactor = (maxHistogramValue > 0) ? (static_cast<double>(drawingHeight) / std::sqrt(static_cast<double>(maxHistogramValue))) : 0;

    for (int i = 0; i < 256; ++i) {
        if (histogramData[i] == 0) continue; // Skip empty bins

        // Calculate bar height based on sqrt(count) and scale factor
        double barHeightRaw = std::sqrt(static_cast<double>(histogramData[i])) * scaleFactor;
        // Clamp bar height to the drawing area and ensure non-negative
        int barHeight = std::max(0, std::min(drawingHeight, static_cast<int>(std::round(barHeightRaw))));

        int xPos = padding + static_cast<int>(i * barWidth);
        int yPos = height - padding - barHeight; // Y position starts from bottom
        // Calculate width for this specific bar to avoid gaps/overlaps with double barWidth
        int currentBarWidth = std::max(1, static_cast<int>(std::round((i + 1) * barWidth)) - static_cast<int>(std::round(i * barWidth)));

        // Set bar color and highlight
        QColor barColor = (i == hoveredBin) ? QColor(255, 0, 0, 180) : Qt::darkGray; // Highlight red, else dark gray

        // Use gradient for bars
        QLinearGradient gradient(xPos, height - padding, xPos, yPos);
        gradient.setColorAt(0, barColor.darker(120));
        gradient.setColorAt(1, barColor);
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen); // No border for bars

        // Draw the rectangle if height > 0
        if (barHeight > 0) {
            painter.drawRect(xPos, yPos, currentBarWidth, barHeight);
        }
    }
}

// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// Handles mouse wheel events for resizing the window.
// ==========================================================================
void HistogramWidget::wheelEvent(QWheelEvent *event) {
    const double resizeFactor = 1.10; // Factor for resizing (e.g., 10% change per step)
    QSize currentSize = size();
    int newWidth = currentSize.width();
    int newHeight = currentSize.height();

    // Check if Ctrl key is pressed (optional modifier for resizing)
    // if (!(event->modifiers() & Qt::ControlModifier)) {
    //     QWidget::wheelEvent(event); // Pass event up if modifier not held
    //     return;
    // }

    if (event->angleDelta().y() > 0) {
        // Scale up
        newWidth = static_cast<int>(std::round(currentSize.width() * resizeFactor));
        newHeight = static_cast<int>(std::round(currentSize.height() * resizeFactor));
    } else if (event->angleDelta().y() < 0) {
        // Scale down
        newWidth = static_cast<int>(std::round(currentSize.width() / resizeFactor));
        newHeight = static_cast<int>(std::round(currentSize.height() / resizeFactor));
    }

    // Clamp to minimum/maximum sizes set for the widget
    QSize minSize = minimumSize();
    QSize maxSize = maximumSize();

    newWidth = qBound(minSize.width(), newWidth, maxSize.width());
    newHeight = qBound(minSize.height(), newHeight, maxSize.height());

    // Only resize if the size actually changes
    if (newWidth != currentSize.width() || newHeight != currentSize.height()) {
        resize(newWidth, newHeight);
        // Resizing implicitly schedules a paint event, so update() is likely redundant
    }

    event->accept(); // Indicate event was handled
}


// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// Handles mouse hovering to update the highlighted bin and show tooltips.
// (No changes needed here)
// ==========================================================================
void HistogramWidget::mouseMoveEvent(QMouseEvent *event) {
    int width = this->width();
    int padding = 10;
    int drawingWidth = width - 2 * padding;
    double barWidth = (drawingWidth > 0) ? static_cast<double>(drawingWidth) / 256.0 : 0;

    int currentHoveredBin = -1;
    // Check if mouse is within the histogram bars area and bar width is valid
    if (barWidth > 0 && event->pos().x() >= padding && event->pos().x() < (width - padding)) {
        currentHoveredBin = static_cast<int>((event->pos().x() - padding) / barWidth);
        // Clamp the bin index to valid range [0, 255]
        currentHoveredBin = std::max(0, std::min(255, currentHoveredBin));
    }

    if (currentHoveredBin != hoveredBin) {
        hoveredBin = currentHoveredBin;
        update(); // Redraw to show/remove highlight
    }

    // Show tooltip if hovering over a valid bin and data exists
    if (hoveredBin != -1 && hoveredBin < histogramData.size()) {
        int count = histogramData[hoveredBin];
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("Intensity: %1\nCount: %2").arg(hoveredBin).arg(count),
                           this, rect()); // Show tooltip relative to widget
    } else {
        QToolTip::hideText(); // Hide tooltip if not hovering over a bin
    }
    QWidget::mouseMoveEvent(event); // Call base implementation
}

// ==========================================================================
// Group 4: UI Widgets & Visualization
// ==========================================================================
// Optional: Handle mouse leave event to clear hover state
// (No changes needed here)
// ==========================================================================
void HistogramWidget::leaveEvent(QEvent *event) {
    if (hoveredBin != -1) {
        hoveredBin = -1;
        update(); // Redraw to remove highlight
        QToolTip::hideText();
    }
    QWidget::leaveEvent(event);
}
