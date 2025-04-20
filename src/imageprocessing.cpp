#include "imageprocessing.h"
#include <vector>
#include <cmath>
#include <QMessageBox>
#include <algorithm> // For std::find_if, std::max_element

namespace ImageProcessing {

// ==========================================================================
// Group 5: Image Processing - Core Operations
// ==========================================================================

cv::Mat binarise(const cv::Mat& inputImage, double thresholdValue, double maxValue) {
    cv::Mat outputImage;
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Make Binary Error", "Input image is empty or not grayscale.");
        return outputImage; // Return empty if not grayscale
    }
    cv::threshold(inputImage, outputImage, thresholdValue, maxValue, cv::THRESH_BINARY);
    return outputImage;
}

cv::Mat convertToGrayscale(const cv::Mat& inputImage) {
    cv::Mat outputImage;
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Convert To Grayscale Error", "Input image is empty.");
        return outputImage;
    }
    if (inputImage.channels() == 3) {
        cv::cvtColor(inputImage, outputImage, cv::COLOR_BGR2GRAY);
    } else if (inputImage.channels() == 4) {
        cv::cvtColor(inputImage, outputImage, cv::COLOR_BGRA2GRAY);
    } else {
        // Already grayscale or binary, return a clone
        outputImage = inputImage.clone();
    }
    return outputImage;
}

cv::Mat convertToColor(const cv::Mat &input) {
    if (input.empty()) {
        QMessageBox::warning(nullptr, "Convert To Color Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat output;
    if (input.channels() == 1) {
        cv::cvtColor(input, output, cv::COLOR_GRAY2BGR);
    } else if (input.channels() == 4) {
        cv::cvtColor(input, output, cv::COLOR_BGRA2BGR);
    } else if (input.channels() == 3){
        output = input.clone(); // Already color
    } else {
        qWarning("convertToColor: Unsupported channel count %d", input.channels());
        // Return a black image of same size? Or empty?
        output = cv::Mat::zeros(input.size(), CV_8UC3);
    }
    return output;
}

std::vector<cv::Mat> splitColorChannels(const cv::Mat& inputImage) {
    std::vector<cv::Mat> channels;
    if (!inputImage.empty() && inputImage.channels() >= 3) {
        cv::split(inputImage, channels);
    }
    return channels;
}

std::vector<cv::Mat> convertToHSV(const cv::Mat& inputImage) {
    std::vector<cv::Mat> channels;
    if (!inputImage.empty() && inputImage.channels() >= 3) {
        cv::Mat outputImage;
        cv::cvtColor(inputImage, outputImage, cv::COLOR_BGR2HSV);
        channels = splitColorChannels(outputImage);
    }
    return channels;
}

std::vector<cv::Mat> convertToLab(const cv::Mat& inputImage) {
    std::vector<cv::Mat> channels;
    if (!inputImage.empty() && inputImage.channels() >= 3) {
        cv::Mat outputImage;
        cv::cvtColor(inputImage, outputImage, cv::COLOR_BGR2Lab);
        channels = splitColorChannels(outputImage);
    }
    return channels;
}


// ==========================================================================
// Group 6: Image Processing - Point Operations
// ==========================================================================

cv::Mat applyNegation(const cv::Mat& inputImage) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Negation Error", "Input image is empty.");
        return cv::Mat();
    }
    return cv::Scalar::all(255) - inputImage;
}

cv::Mat applyRangeStretching(const cv::Mat& inputImage, int p1, int p2, int q3, int q4) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Range Stretching Error", "Input image is empty or not grayscale.");
        return inputImage.clone(); // Return copy if invalid input
    }
    if (p1 >= p2 || q3 >= q4) {
        QMessageBox::warning(nullptr, "Range Stretching Error", "Invalid params were given.");
        // Consider logging a warning or throwing an exception for invalid params
        return inputImage.clone();
    }

    cv::Mat stretchedImage = inputImage.clone();
    float scale = static_cast<float>(q4 - q3) / (p2 - p1);

    for (int y = 0; y < stretchedImage.rows; y++) {
        uchar* rowPtr = stretchedImage.ptr<uchar>(y);
        for (int x = 0; x < stretchedImage.cols; x++) {
            int pixel = rowPtr[x];
            // Apply stretching only if the pixel is in range [p1, p2]
            if (pixel >= p1 && pixel <= p2) {
                // Apply linear transformation
                pixel = static_cast<int>((pixel - p1) * scale + q3);
                // Clamp result to [0, 255] just in case, although logic should prevent overflow if q3/q4 are valid
                rowPtr[x] = cv::saturate_cast<uchar>(pixel);
            }
            // Pixels outside [p1, p2] remain unchanged
        }
    }
    return stretchedImage;
}

cv::Mat applyPosterization(const cv::Mat& inputImage, int levels) {
    if (inputImage.empty() || inputImage.channels() != 1 || levels < 2 || levels > 256) {
        QMessageBox::warning(nullptr, "Posterization Error", "Input image is empty or not grayscale.");
        return inputImage.clone(); // Return copy if invalid input
    }

    cv::Mat outputImage = inputImage.clone();
    uchar lookupTable[256];
    double step = 255.0 / (levels - 1);

    for (int i = 0; i < 256; ++i) {
        lookupTable[i] = cv::saturate_cast<uchar>(std::round(i / step) * step);
    }

    // Apply LUT efficiently
    cv::LUT(inputImage, cv::Mat(1, 256, CV_8U, lookupTable), outputImage);

    return outputImage;
}

cv::Mat applyBitwiseAnd(const cv::Mat& img1, const cv::Mat& img2) {
    cv::Mat result;
    if (!img1.empty() && !img2.empty() && img1.size() == img2.size() && img1.type() == img2.type()) {
        cv::bitwise_and(img1, img2, result);
    } else {
        // Handle error: incompatible images
        // Consider logging or throwing
        result = img1.clone(); // Or return empty
    }
    return result;
}

cv::Mat applyBitwiseOr(const cv::Mat& img1, const cv::Mat& img2) {
    cv::Mat result;
    if (!img1.empty() && !img2.empty() && img1.size() == img2.size() && img1.type() == img2.type()) {
        cv::bitwise_or(img1, img2, result);
    } else {
        result = img1.clone();
    }
    return result;
}

cv::Mat applyBitwiseXor(const cv::Mat& img1, const cv::Mat& img2) {
    cv::Mat result;
    if (!img1.empty() && !img2.empty() && img1.size() == img2.size() && img1.type() == img2.type()) {
        cv::bitwise_xor(img1, img2, result);
    } else {
        result = img1.clone();
    }
    return result;
}

cv::Mat applyAddition(const cv::Mat& img1, const cv::Mat& img2) {
    cv::Mat result;
    if (!img1.empty() && !img2.empty() && img1.size() == img2.size() && img1.type() == img2.type()) {
        cv::add(img1, img2, result);
    } else {
        result = img1.clone();
    }
    return result;
}

cv::Mat applySubtraction(const cv::Mat& img1, const cv::Mat& img2) {
    cv::Mat result;
    if (!img1.empty() && !img2.empty() && img1.size() == img2.size() && img1.type() == img2.type()) {
        cv::subtract(img1, img2, result);
    } else {
        result = img1.clone();
    }
    return result;
}

cv::Mat applyBlending(const cv::Mat& img1, const cv::Mat& img2, double alpha, double gamma) {
    cv::Mat result;
    if (!img1.empty() && !img2.empty() && img1.size() == img2.size() && img1.type() == img2.type()) {
        double beta = 1.0 - alpha;
        cv::addWeighted(img1, alpha, img2, beta, gamma, result);
    } else {
        result = img1.clone();
    }
    return result;
}

// ==========================================================================
// Group 7: Image Processing - Histogram Operations
// ==========================================================================

cv::Mat stretchHistogram(const cv::Mat& inputImage) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Stretch Histogram Error", "Input image is empty or not grayscale.");
        return inputImage.clone();
    }
    cv::Mat outputImage;
    double minVal, maxVal;
    cv::minMaxLoc(inputImage, &minVal, &maxVal);

    // Avoid division by zero if image is uniform
    if (maxVal - minVal > DBL_EPSILON) {
        // Use normalize function for potentially better precision/handling
        // cv::normalize(inputImage, outputImage, 0, 255, cv::NORM_MINMAX, CV_8U);
        // Or manual conversion:
        inputImage.convertTo(outputImage, CV_8U, 255.0 / (maxVal - minVal), -minVal * 255.0 / (maxVal - minVal));
    } else {
        // Handle uniform image (e.g., set all to 0 or 128, or return clone)
        outputImage = inputImage.clone(); // Or cv::Mat(inputImage.size(), CV_8U, cv::Scalar(128));
    }
    return outputImage;
}

cv::Mat equalizeHistogram(const cv::Mat& inputImage) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Equalize Histogram Error", "Input image is empty or not grayscale.");
        return inputImage.clone();
    }

        cv::Mat outputImage = inputImage.clone();
        // Step 1: Compute Histogram
        std::vector<int> histogram(256, 0);
        for (int y = 0; y < outputImage.rows; y++) {
            for (int x = 0; x < outputImage.cols; x++) {
                histogram[outputImage.at<uchar>(y, x)]++;
            }
        }

        // Step 2: Compute CDF
        std::vector<int> cdf(256, 0);
        cdf[0] = histogram[0];
        for (int i = 1; i < 256; i++) {
            cdf[i] = cdf[i - 1] + histogram[i];
        }

        // Find the first non-zero CDF value
        int minCDF = 0;
        for(int val : cdf) {
            if (val > 0) {
                minCDF = val;
                break;
            }
        }
        if (outputImage.total() == minCDF) return outputImage; // Avoid division by zero if all pixels are the same value > 0

        // Step 3: Create LUT and Apply Equalization
        uchar equalizationMap[256];
        float scale = 255.0f / (outputImage.total() - minCDF);
        for (int i = 0; i < 256; i++) {
             if (cdf[i] < minCDF) { // Handle cases where cdf[i] might be 0 before minCDF
                 equalizationMap[i] = 0;
             } else {
                 equalizationMap[i] = cv::saturate_cast<uchar>((cdf[i] - minCDF) * scale);
             }
        }
        cv::LUT(inputImage, cv::Mat(1, 256, CV_8U, equalizationMap), outputImage);

        return outputImage;
}

// ==========================================================================
// Group 8: Image Processing - Filtering & Edge Detection
// ==========================================================================

cv::Mat applyBoxBlur(const cv::Mat& inputImage, int kernelSize, int borderOption) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Box Blur Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat outputImage;
    cv::blur(inputImage, outputImage, cv::Size(kernelSize, kernelSize), cv::Point(-1, -1), borderOption);
    return outputImage;
}

cv::Mat applyGaussianBlur(const cv::Mat& inputImage, int kernelSize, double sigmaX, double sigmaY, int borderOption) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Gaussian Blur Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat outputImage;
    cv::GaussianBlur(inputImage, outputImage, cv::Size(kernelSize, kernelSize), sigmaX, sigmaY, borderOption);
    return outputImage;
}

cv::Mat applySobelEdgeDetection(const cv::Mat& inputImage, int kernelSize, double scale, double delta, int borderOption) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Sobel Edge Detection Error", "Input image is empty or not grayscale.");
        return inputImage.clone();
    }
    cv::Mat gradX, gradY;
    cv::Mat absGradX, absGradY;
    cv::Mat outputImage;

    // Gradient X
    cv::Sobel(inputImage, gradX, CV_16S, 1, 0, kernelSize, scale, delta, borderOption);
    // Gradient Y
    cv::Sobel(inputImage, gradY, CV_16S, 0, 1, kernelSize, scale, delta, borderOption);

    // Convert gradients to absolute versions
    cv::convertScaleAbs(gradX, absGradX);
    cv::convertScaleAbs(gradY, absGradY);

    // Total Gradient (approximate)
    cv::addWeighted(absGradX, 0.5, absGradY, 0.5, 0, outputImage);

    return outputImage;
}

cv::Mat applyLaplacianEdgeDetection(const cv::Mat& inputImage, int kernelSize, double scale, double delta, int borderOption) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Laplacian Edge Detection Error", "Input image is empty or not grayscale.");
        return inputImage.clone();
    }
    cv::Mat laplacian_16s, outputImage;
    cv::Laplacian(inputImage, laplacian_16s, CV_16S, kernelSize, scale, delta, borderOption);
    cv::convertScaleAbs(laplacian_16s, outputImage);
    return outputImage;
}

cv::Mat applyCannyEdgeDetection(const cv::Mat& inputImage, double threshold1, double threshold2, int apertureSize, bool L2gradient) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Canny Edge Detection Error", "Input image is empty or not grayscale.");
        return inputImage.clone();
    }
    cv::Mat outputImage;
    cv::Canny(inputImage, outputImage, threshold1, threshold2, apertureSize, L2gradient);
    return outputImage;
}

cv::Mat applySharpening(const cv::Mat& inputImage, int option, int borderOption) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Sharpening Filter Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat kernel;
    switch (option) {
    case 1: // Basic Sharpening
        kernel = (cv::Mat_<float>(3, 3) <<
                      0, -1,  0,
                  -1,  5, -1,
                  0, -1,  0);
        break;
    case 2: // Strong Sharpening
        kernel = (cv::Mat_<float>(3, 3) <<
                      -1, -1, -1,
                  -1,  9, -1,
                  -1, -1, -1);
        break;
    case 3: // Edge Enhancement
        kernel = (cv::Mat_<float>(3, 3) <<
                      1, -2,  1,
                  -2,  5, -2,
                  1, -2,  1);
        break;
    default:
        return inputImage.clone(); // Return copy if invalid option
    }

    cv::Mat outputImage;
    cv::filter2D(inputImage, outputImage, -1, kernel, cv::Point(-1, -1), 0, borderOption);
    return outputImage;
}

cv::Mat applyPrewittEdgeDetection(const cv::Mat& inputImage, int direction, int borderOption) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        QMessageBox::warning(nullptr, "Prewitt Edge Detection Error", "Input image is empty or not grayscale.");
        return inputImage.clone();
    }

    cv::Mat kernel;
    switch (direction) {
    case 0: kernel = (cv::Mat_<float>(3,3) << -1,-1, 0, -1, 0, 1,  0, 1, 1); break; // TL
    case 1: kernel = (cv::Mat_<float>(3,3) << -1,-1,-1,  0, 0, 0,  1, 1, 1); break; // T
    case 2: kernel = (cv::Mat_<float>(3,3) <<  0,-1,-1,  1, 0,-1,  1, 1, 0); break; // TR
    case 3: kernel = (cv::Mat_<float>(3,3) << -1, 0, 1, -1, 0, 1, -1, 0, 1); break; // L
    // case 4: center - no kernel
    case 5: kernel = (cv::Mat_<float>(3,3) <<  1, 0,-1,  1, 0,-1,  1, 0,-1); break; // R
    case 6: kernel = (cv::Mat_<float>(3,3) <<  0, 1, 1, -1, 0, 1, -1,-1, 0); break; // BL
    case 7: kernel = (cv::Mat_<float>(3,3) <<  1, 1, 1,  0, 0, 0, -1,-1,-1); break; // B
    case 8: kernel = (cv::Mat_<float>(3,3) <<  1, 1, 0,  1, 0,-1,  0,-1,-1); break; // BR
    default: return inputImage.clone(); // Invalid direction
    }

    cv::Mat outputImage_16s, outputImage;
    cv::filter2D(inputImage, outputImage_16s, CV_16S, kernel, cv::Point(-1,-1), 0, borderOption);
    cv::convertScaleAbs(outputImage_16s, outputImage); // Convert to 8U for display
    return outputImage;
}

cv::Mat applyCustomFilter(const cv::Mat& inputImage, cv::Mat kernel, bool normalize, int borderOption) {
    if (inputImage.empty() || kernel.empty()) {
        QMessageBox::warning(nullptr, "Custom Filter Error", "Input image or kernel is empty.");
        return inputImage.clone();
    }
    // Ensure kernel is float
    if(kernel.type() != CV_32F) {
        kernel.convertTo(kernel, CV_32F);
    }

    if (normalize) {
        double sum = cv::sum(kernel)[0];
        if (std::abs(sum) > DBL_EPSILON) { // Avoid division by zero or near-zero
            kernel /= sum;
        }
    }

    cv::Mat outputImage;
    cv::filter2D(inputImage, outputImage, -1, kernel, cv::Point(-1, -1), 0, borderOption);
    return outputImage;
}

// Custom implementation of Median Filtering to support border handling
cv::Mat applyMedianFilter(const cv::Mat& inputImage, int kernelSize, int borderType) {
    if (inputImage.empty() || inputImage.channels() != 1 || kernelSize <= 1 || kernelSize % 2 == 0) {
        QMessageBox::warning(nullptr, "Median Filter Error", "Input image is empty.");
        return inputImage.clone();
    }

    cv::Mat borderedImage;
    int border = kernelSize / 2;
    cv::copyMakeBorder(inputImage, borderedImage, border, border, border, border, borderType);

    cv::Mat filteredImage = inputImage.clone(); // Initialize with correct size

    std::vector<uchar> neighbors(kernelSize * kernelSize);

    for (int y = 0; y < inputImage.rows; ++y) {
        uchar* filteredRowPtr = filteredImage.ptr<uchar>(y);
        for (int x = 0; x < inputImage.cols; ++x) {
            // Collect neighbors from the bordered image
            int k = 0;
            for (int ky = -border; ky <= border; ++ky) {
                const uchar* borderedRowPtr = borderedImage.ptr<uchar>(y + border + ky);
                for (int kx = -border; kx <= border; ++kx) {
                    neighbors[k++] = borderedRowPtr[x + border + kx];
                }
            }
            // Find the median using nth_element (efficient)
            std::nth_element(neighbors.begin(), neighbors.begin() + neighbors.size() / 2, neighbors.end());
            filteredRowPtr[x] = neighbors[neighbors.size() / 2];
        }
    }
    return filteredImage;
}


cv::Mat applyTwoStepFilter(const cv::Mat& inputImage, const cv::Mat& kernel1, const cv::Mat& kernel2, int borderOption) {
    if (inputImage.empty() || kernel1.empty() || kernel2.empty() || kernel1.size() != cv::Size(3,3) || kernel2.size() != cv::Size(3,3)) {
        QMessageBox::warning(nullptr, "Two Step Filter Error", "Input image is empty or input kernels are incorrect.");
        return inputImage.clone();
    }

    // Ensure kernels are float
    cv::Mat k1, k2;
    kernel1.convertTo(k1, CV_32F);
    kernel2.convertTo(k2, CV_32F);


    // --- Calculate the equivalent 5x5 kernel ---
    cv::Mat combined5x5 = cv::Mat::zeros(5, 5, CV_32F);

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                for (int l = 0; l < 3; ++l) {
                    combined5x5.at<float>(i + k, j + l) += kernel1.at<float>(i, j) * kernel2.at<float>(k, l);
                }
            }
        }
    }
    // --- End 5x5 kernel calculation ---


    // Normalize the combined kernel
    double sum = cv::sum(combined5x5)[0];
    if (std::abs(sum) > DBL_EPSILON) {
        combined5x5 /= sum;
    }

    // Apply the computed 5x5 kernel
    cv::Mat result;
    cv::filter2D(inputImage, result, -1, combined5x5, cv::Point(-1, -1), 0, borderOption);
    return result;
}

// ==========================================================================
// Group 9: Image Processing - Morphology
// ==========================================================================

cv::Mat getStructuringElement(StructuringElementType type) {
    if (type == Diamond) {
        // 3x3 Diamond (cross shape)
        return cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
        /* Manual Diamond:
            cv::Mat diamond = cv::Mat::zeros(3, 3, CV_8U);
            diamond.at<uchar>(0, 1) = 1;
            diamond.at<uchar>(1, 0) = 1;
            diamond.at<uchar>(1, 1) = 1;
            diamond.at<uchar>(1, 2) = 1;
            diamond.at<uchar>(2, 1) = 1;
            return diamond;
            */
    } else { // Square
        return cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    }
}

cv::Mat applyErosion(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Erosion Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat element = getStructuringElement(elementType);
    cv::Mat outputImage;
    // Note: OpenCV's erode/dilate handle borders internally based on borderType passed to the function,
    // but the borderValue (used for BORDER_CONSTANT) defaults to morphologyDefaultBorderValue().
    // If a different constant border is needed, copyMakeBorder might be required first.
    cv::erode(inputImage, outputImage, element, cv::Point(-1, -1), iterations, borderOption);
    return outputImage;
}

cv::Mat applyDilation(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Dilation Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat element = getStructuringElement(elementType);
    cv::Mat outputImage;
    cv::dilate(inputImage, outputImage, element, cv::Point(-1, -1), iterations, borderOption);
    return outputImage;
}

cv::Mat applyOpening(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Opening Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat element = getStructuringElement(elementType);
    cv::Mat outputImage;
    cv::morphologyEx(inputImage, outputImage, cv::MORPH_OPEN, element, cv::Point(-1, -1), iterations, borderOption);
    return outputImage;
}

cv::Mat applyClosing(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Closing Error", "Input image is empty.");
        return cv::Mat();
    }
    cv::Mat element = getStructuringElement(elementType);
    cv::Mat outputImage;
    cv::morphologyEx(inputImage, outputImage, cv::MORPH_CLOSE, element, cv::Point(-1, -1), iterations, borderOption);
    return outputImage;
}

cv::Mat applySkeletonization(const cv::Mat& inputImage, StructuringElementType elementType) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Skeletonization Error", "Input image is empty.");
        return cv::Mat();
    }
    // Ensure image is binary (0 or 255)
    cv::Mat binary = inputImage.clone();
    if (inputImage.type() != CV_8UC1 || cv::countNonZero((binary != 0) & (binary != 255)) > 0) {
        cv::threshold(binary, binary, 127, 255, cv::THRESH_BINARY);
    }


    cv::Mat skeleton = cv::Mat::zeros(binary.size(), CV_8UC1);
    cv::Mat temp, eroded;
    cv::Mat element = getStructuringElement(elementType);

    bool done;
    do {
        cv::erode(binary, eroded, element);
        cv::dilate(eroded, temp, element); // temp = open(binary)
        cv::subtract(binary, temp, temp); // temp = binary - open(binary)
        cv::bitwise_or(skeleton, temp, skeleton); // skeleton |= temp
        eroded.copyTo(binary);
        done = (cv::countNonZero(binary) == 0); // Check if image is empty
    } while (!done);

    return skeleton;
}

// ==========================================================================
// Group 10: Image Processing - Feature Detection
// ==========================================================================


cv::Mat detectHoughLines(const cv::Mat& binaryEdgeImage, double rho, double theta, int threshold) {
    if (binaryEdgeImage.empty()) {
        QMessageBox::warning(nullptr, "Hough Lines Error", "Input image is empty.");
        return cv::Mat();
    }
    std::vector<cv::Vec2f> lines;
    cv::Mat outputImage = binaryEdgeImage.clone();
    if (!outputImage.empty() && outputImage.type() == CV_8UC1) {
        cv::HoughLines(outputImage, lines, rho, theta, threshold);
    }
    // Prepare image for drawing lines (convert original to color if needed)
    cv::Mat colorImage;
    if (outputImage.channels() == 1) {
        cv::cvtColor(outputImage, colorImage, cv::COLOR_GRAY2BGR);
    } else if (outputImage.channels() == 3){
        colorImage = outputImage.clone(); // Already BGR
    } else if (outputImage.channels() == 4) {
        cv::cvtColor(outputImage, colorImage, cv::COLOR_BGRA2BGR); // Drop alpha
    } else {
        QMessageBox::warning(nullptr, "Hough Draw Error", "Cannot draw lines on image with unsupported channel count.");
        return cv::Mat();
    }

    // Draw detected lines
    if (!lines.empty()) {
        //QMessageBox::information(nullptr, "Hough Lines", "Detected " + QString::fromStdString(std::to_string(lines.size())) + " lines.");
        for (size_t i = 0; i < lines.size(); i++) {
            float r = lines[i][0], t = lines[i][1];
            double a = std::cos(t), b = std::sin(t);
            double x0 = a * r, y0 = b * r;
            // Calculate points far enough to span the image diagonal for safety
            double imgDiagonal = std::sqrt(colorImage.cols*colorImage.cols + colorImage.rows*colorImage.rows);
            cv::Point pt1(cvRound(x0 + imgDiagonal * (-b)), cvRound(y0 + imgDiagonal * (a)));
            cv::Point pt2(cvRound(x0 - imgDiagonal * (-b)), cvRound(y0 - imgDiagonal * (a)));
            cv::line(colorImage, pt1, pt2, cv::Scalar(0, 0, 255), 1, cv::LINE_AA); // Draw red lines
        }
        outputImage = colorImage; // Update originalImage only if lines were drawn
    } else {
        QMessageBox::information(nullptr, "Hough Lines", "No lines detected with the given parameters.");
        // Don't change originalImage, effectively cancelling the operation visually
        // The state pushed to undo stack is the original image before attempting Hough.
        // No need to pop here, user can undo if they want.
    }
    return outputImage;
}

cv::Mat applyGlobalThreshold(const cv::Mat& inputImage, int threshold) {
    cv::Mat resultImage;
    cv::threshold(inputImage, resultImage, threshold, 255, cv::ThresholdTypes::THRESH_BINARY);
    return resultImage;
}

cv::Mat applyAdaptiveThreshold(const cv::Mat& inputImage) {
    cv::Mat resultImage;
    cv::adaptiveThreshold(inputImage, resultImage, 255,
                          cv::AdaptiveThresholdTypes::ADAPTIVE_THRESH_MEAN_C,
                          cv::ThresholdTypes::THRESH_BINARY, 11, 2);
    return resultImage;
}

cv::Mat applyOtsuThreshold(const cv::Mat& inputImage) {
    cv::Mat resultImage;
    cv::threshold(inputImage, resultImage, 0, 255, cv::ThresholdTypes::THRESH_BINARY | cv::THRESH_OTSU);
    return resultImage;
}

#include <queue>

// Magic Wand segmentation supporting both grayscale and RGB images
cv::Mat magicWandSegmentation(const cv::Mat& inputImage, const cv::Point& seed, int tolerance) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Magic Wand Error", "Input image is empty.");
        return cv::Mat();
    }

    cv::Mat resultImage;

    // Convert BGRA to BGR if needed
    if (inputImage.type() == CV_8UC4) {
        cv::cvtColor(inputImage, resultImage, cv::COLOR_BGRA2BGR);
    } else {
        resultImage = inputImage;
    }

    CV_Assert(resultImage.type() == CV_8UC1 || resultImage.type() == CV_8UC3);

    cv::Mat visited = cv::Mat::zeros(resultImage.size(), CV_8U);
    cv::Mat mask = cv::Mat::zeros(resultImage.size(), CV_8U);

    std::queue<cv::Point> queue;
    queue.push(seed);
    visited.at<uchar>(seed) = 255;
    mask.at<uchar>(seed) = 255;

    const int dx[] = {-1, 0, 1, 0};
    const int dy[] = {0, -1, 0, 1};

    if (resultImage.type() == CV_8UC1) {
        uchar seedVal = resultImage.at<uchar>(seed);

        while (!queue.empty()) {
            cv::Point p = queue.front(); queue.pop();

            for (int k = 0; k < 4; ++k) {
                int nx = p.x + dx[k];
                int ny = p.y + dy[k];

                if (nx >= 0 && ny >= 0 && nx < resultImage.cols && ny < resultImage.rows) {
                    if (!visited.at<uchar>(ny, nx)) {
                        uchar val = resultImage.at<uchar>(ny, nx);
                        if (std::abs(val - seedVal) <= tolerance) {
                            visited.at<uchar>(ny, nx) = 255;
                            mask.at<uchar>(ny, nx) = 255;
                            queue.push(cv::Point(nx, ny));
                        }
                    }
                }
            }
        }
    } else {
        cv::Vec3b seedVal = resultImage.at<cv::Vec3b>(seed);

        while (!queue.empty()) {
            cv::Point p = queue.front(); queue.pop();

            for (int k = 0; k < 4; ++k) {
                int nx = p.x + dx[k];
                int ny = p.y + dy[k];

                if (nx >= 0 && ny >= 0 && nx < resultImage.cols && ny < resultImage.rows) {
                    if (!visited.at<uchar>(ny, nx)) {
                        cv::Vec3b val = resultImage.at<cv::Vec3b>(ny, nx);
                        if (tolerance == 0) {
                            if (val == seedVal) {
                                visited.at<uchar>(ny, nx) = 255;
                                mask.at<uchar>(ny, nx) = 255;
                                queue.push(cv::Point(nx, ny));
                            }
                        } else {
                            int dr = val[0] - seedVal[0];
                            int dg = val[1] - seedVal[1];
                            int db = val[2] - seedVal[2];
                            int distSq = dr * dr + dg * dg + db * db;
                            if (distSq <= tolerance * tolerance) {
                                visited.at<uchar>(ny, nx) = 255;
                                mask.at<uchar>(ny, nx) = 255;
                                queue.push(cv::Point(nx, ny));
                            }
                        }

                    }
                }
            }
        }
    }

    return mask;
}

cv::Mat grabCutSegmentation(const cv::Mat& inputImage, const cv::Rect& rect, int iterCount) {
    if (inputImage.empty()) {
        QMessageBox::warning(nullptr, "Grab Cut Error", "Input image is empty.");
        return cv::Mat();
    }

    cv::Mat image;
    // Convert to BGR if inputImage is grayscale or has alpha
    if (inputImage.channels() == 1) {
        cv::cvtColor(inputImage, image, cv::COLOR_GRAY2BGR);
    } else if (inputImage.channels() == 4) {
        cv::cvtColor(inputImage, image, cv::COLOR_BGRA2BGR);
    } else if (inputImage.channels() == 3) {
        image = inputImage.clone();
    } else {
        throw std::invalid_argument("Unsupported image format for GrabCut");
    }

    // Initialize mask and models
    cv::Mat mask(image.size(), CV_8UC1, cv::GC_BGD); // default: background
    cv::Mat bgdModel, fgdModel;

    // Apply GrabCut
    cv::grabCut(image, mask, rect, bgdModel, fgdModel, iterCount, cv::GC_INIT_WITH_RECT);

    // Convert result mask to binary foreground mask
    cv::Mat binMask = (mask == cv::GC_FGD) | (mask == cv::GC_PR_FGD);

    // Create output: keep only foreground
    cv::Mat result(image.size(), image.type(), cv::Scalar(0, 0, 0));
    image.copyTo(result, binMask);

    return result;
}

using namespace cv;
using namespace std;

// Peak local max with label support
Mat peakLocalMaxWithLabels(const Mat& image, const Mat& labels, int min_distance = 1) {
    CV_Assert(image.size() == labels.size());
    CV_Assert(labels.type() == CV_32S);

    Mat img;
    image.convertTo(img, CV_64F);
    Mat output = Mat::zeros(image.size(), CV_8U);
    map<int, vector<Point>> label_points;

    for (int y = 0; y < img.rows; ++y) {
        for (int x = 0; x < img.cols; ++x) {
            int label = labels.at<int>(y, x);
            if (label > 0) {
                label_points[label].emplace_back(x, y);
            }
        }
    }

    for (const auto& [label, points] : label_points) {
        if (points.empty()) continue;

        Rect bbox = boundingRect(points);
        Mat roi_img = img(bbox).clone();
        Mat roi_mask = Mat::zeros(bbox.size(), CV_8U);

        for (const auto& pt : points) {
            roi_mask.at<uchar>(pt.y - bbox.y, pt.x - bbox.x) = 255;
        }

        Mat roi_dilated;
        Mat kernel = getStructuringElement(MORPH_RECT, Size(2 * min_distance + 1, 2 * min_distance + 1));
        dilate(roi_img, roi_dilated, kernel);

        for (int y = 0; y < roi_img.rows; ++y) {
            for (int x = 0; x < roi_img.cols; ++x) {
                if (roi_mask.at<uchar>(y, x) &&
                    roi_img.at<double>(y, x) == roi_dilated.at<double>(y, x)) {
                    output.at<uchar>(bbox.y + y, bbox.x + x) = 255;
                }
            }
        }
    }

    return output;
}

cv::Mat applyWatershedSegmentation(const cv::Mat &inputImage) {
    cv::Mat image;
    inputImage.copyTo(image);

    // Ensure 3-channel color image for watershed and drawing
    cv::Mat colorInput;
    if (image.channels() == 1) {
        cv::cvtColor(image, colorInput, cv::COLOR_GRAY2BGR);
    } else {
        colorInput = image.clone();
    }

    // Apply mean shift filtering only for color images
    cv::Mat shifted;
    if (colorInput.channels() == 3) {
        pyrMeanShiftFiltering(colorInput, shifted, 21, 51);
    } else {
        shifted = colorInput.clone();
    }

    // Convert to grayscale if not already
    cv::Mat gray;
    if (shifted.channels() == 3) {
        cv::cvtColor(shifted, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = shifted.clone();
    }

    // Check if already binary
    cv::Mat thresh;
    if (cv::countNonZero(gray == 0) + cv::countNonZero(gray == 255) == gray.total()) {
        thresh = gray.clone();
    } else {
        cv::threshold(gray, thresh, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    }

    // Morphological opening to remove noise
    cv::Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat opening;
    morphologyEx(thresh, opening, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 2);

    // Distance transform
    cv::Mat dist;
    distanceTransform(opening, dist, cv::DIST_L2, 5);
    normalize(dist, dist, 0.0, 1.0, cv::NORM_MINMAX);

    // Adaptive threshold for sure foreground
    double maxVal;
    cv::minMaxLoc(dist, nullptr, &maxVal);
    double sure_fg_threshold = 0.4 * maxVal; // adaptive
    cv::Mat sure_fg_f;
    threshold(dist, sure_fg_f, sure_fg_threshold, 1.0, cv::THRESH_BINARY);
    cv::Mat sure_fg;
    sure_fg_f.convertTo(sure_fg, CV_8U, 255.0);

    // Improved sure background using closing
    cv::Mat sure_bg;
    morphologyEx(opening, sure_bg, cv::MORPH_CLOSE,
                 getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)), cv::Point(-1, -1), 1);

    // Unknown region
    cv::Mat unknown;
    subtract(sure_bg, sure_fg, unknown);

    // Connected components as markers
    cv::Mat markers;
    int numInitialLabels = connectedComponents(sure_fg, markers);
    std::cout << "[INFO] Found " << (numInitialLabels - 1) << " initial markers (potential segments)." << std::endl;

    // Adjust marker labels
    markers += 1;
    markers.setTo(0, unknown); // unknown is 0

    // Apply watershed
    watershed(shifted, markers);

    if (markers.empty() || markers.type() != CV_32S) {
        std::cerr << "Error: Watershed algorithm failed." << std::endl;
        return cv::Mat();
    }

    // Draw final result
    cv::Mat outputImage = colorInput.clone();
    std::set<int> uniqueLabels;
    for (int r = 0; r < markers.rows; ++r) {
        for (int c = 0; c < markers.cols; ++c) {
            int label = markers.at<int>(r, c);
            if (label > 1) {
                uniqueLabels.insert(label);
            }
        }
    }

    std::cout << "[INFO] Found " << uniqueLabels.size() << " final object segments after watershed." << std::endl;
    outputImage.setTo(cv::Scalar(255, 0, 0), markers == -1);

    return outputImage;
}

cv::Mat applyInpainting(const cv::Mat& inputImage, const cv::Mat& mask, double radius, int method) {
    cv::Mat result;
    cv::inpaint(inputImage, mask, result, radius, method);
    return result;
}

} // namespace ImageProcessing
