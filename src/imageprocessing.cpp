#include "imageprocessing.h"
#include <vector>
#include <cmath>
#include <numeric> // For std::accumulate
#include <algorithm> // For std::find_if, std::max_element

namespace ImageProcessing {

// ==========================================================================
// Group 5: Image Processing - Core Operations
// ==========================================================================

cv::Mat binarise(const cv::Mat& inputImage, double thresholdValue, double maxValue) {
    cv::Mat outputImage;
    if (inputImage.empty() || inputImage.channels() != 1) {
        return outputImage; // Return empty if not grayscale
    }
    cv::threshold(inputImage, outputImage, thresholdValue, maxValue, cv::THRESH_BINARY);
    return outputImage;
}

cv::Mat convertToGrayscale(const cv::Mat& inputImage) {
    cv::Mat outputImage;
    if (inputImage.empty()) {
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
    if (input.empty()) return cv::Mat();
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

cv::Mat convertToHSV(const cv::Mat& inputImage) {
    cv::Mat outputImage;
    if (!inputImage.empty() && inputImage.channels() >= 3) {
        cv::cvtColor(inputImage, outputImage, cv::COLOR_BGR2HSV);
    }
    return outputImage;
}

cv::Mat convertToLab(const cv::Mat& inputImage) {
    cv::Mat outputImage;
    if (!inputImage.empty() && inputImage.channels() >= 3) {
        cv::cvtColor(inputImage, outputImage, cv::COLOR_BGR2Lab);
    }
    return outputImage;
}


// ==========================================================================
// Group 6: Image Processing - Point Operations
// ==========================================================================

cv::Mat applyNegation(const cv::Mat& inputImage) {
    if (inputImage.empty()) {
        return cv::Mat();
    }
    return cv::Scalar::all(255) - inputImage;
}

cv::Mat applyRangeStretching(const cv::Mat& inputImage, int p1, int p2, int q3, int q4) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        return inputImage.clone(); // Return copy if invalid input
    }
    if (p1 >= p2 || q3 >= q4) {
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
        return cv::Mat();
    }
    cv::Mat outputImage;
    cv::blur(inputImage, outputImage, cv::Size(kernelSize, kernelSize), cv::Point(-1, -1), borderOption);
    return outputImage;
}

cv::Mat applyGaussianBlur(const cv::Mat& inputImage, int kernelSize, double sigmaX, double sigmaY, int borderOption) {
    if (inputImage.empty()) {
        return cv::Mat();
    }
    cv::Mat outputImage;
    cv::GaussianBlur(inputImage, outputImage, cv::Size(kernelSize, kernelSize), sigmaX, sigmaY, borderOption);
    return outputImage;
}

cv::Mat applySobelEdgeDetection(const cv::Mat& inputImage, int kernelSize, double scale, double delta, int borderOption) {
    if (inputImage.empty() || inputImage.channels() != 1) {
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
        return inputImage.clone();
    }
    cv::Mat laplacian_16s, outputImage;
    cv::Laplacian(inputImage, laplacian_16s, CV_16S, kernelSize, scale, delta, borderOption);
    cv::convertScaleAbs(laplacian_16s, outputImage);
    return outputImage;
}

cv::Mat applyCannyEdgeDetection(const cv::Mat& inputImage, double threshold1, double threshold2, int apertureSize, bool L2gradient) {
    if (inputImage.empty() || inputImage.channels() != 1) {
        return inputImage.clone();
    }
    cv::Mat outputImage;
    cv::Canny(inputImage, outputImage, threshold1, threshold2, apertureSize, L2gradient);
    return outputImage;
}

cv::Mat applySharpening(const cv::Mat& inputImage, int option, int borderOption) {
    if (inputImage.empty()) {
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
        return cv::Mat();
    }
    cv::Mat element = getStructuringElement(elementType);
    cv::Mat outputImage;
    cv::dilate(inputImage, outputImage, element, cv::Point(-1, -1), iterations, borderOption);
    return outputImage;
}

cv::Mat applyOpening(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption) {
    if (inputImage.empty()) {
        return cv::Mat();
    }
    cv::Mat element = getStructuringElement(elementType);
    cv::Mat outputImage;
    cv::morphologyEx(inputImage, outputImage, cv::MORPH_OPEN, element, cv::Point(-1, -1), iterations, borderOption);
    return outputImage;
}

cv::Mat applyClosing(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption) {
    if (inputImage.empty()) {
        return cv::Mat();
    }
    cv::Mat element = getStructuringElement(elementType);
    cv::Mat outputImage;
    cv::morphologyEx(inputImage, outputImage, cv::MORPH_CLOSE, element, cv::Point(-1, -1), iterations, borderOption);
    return outputImage;
}

cv::Mat applySkeletonization(const cv::Mat& inputImage, StructuringElementType elementType) {
    if (inputImage.empty()) {
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

std::vector<cv::Vec2f> detectHoughLines(const cv::Mat& binaryEdgeImage, double rho, double theta, int threshold) {
    std::vector<cv::Vec2f> lines;
    if (!binaryEdgeImage.empty() && binaryEdgeImage.type() == CV_8UC1) {
        cv::HoughLines(binaryEdgeImage, lines, rho, theta, threshold);
    }
    return lines;
}


} // namespace ImageProcessing
