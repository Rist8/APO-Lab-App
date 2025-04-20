#ifndef IMAGE_ALGORITHMS_H
#define IMAGE_ALGORITHMS_H

#include <opencv2/opencv.hpp>
#include <vector>
#include "imageviewer.h" // Include for StructuringElementType enum

namespace ImageProcessing {

// ==========================================================================
// Group 5: Image Processing - Core Operations
// ==========================================================================

/**
     * @brief Converts an image to binary using a fixed threshold.
     * @param inputImage The input grayscale image.
     * @param thresholdValue The threshold value.
     * @param maxValue The value to assign to pixels above the threshold.
     * @return The binary image.
     */
cv::Mat binarise(const cv::Mat& inputImage, double thresholdValue = 127.0, double maxValue = 255.0);

/**
     * @brief Converts a color image to grayscale.
     * @param inputImage The input BGR or BGRA image.
     * @return The grayscale image, or the original if already grayscale/binary.
     */
cv::Mat convertToGrayscale(const cv::Mat& inputImage);

/**
     * @brief Converts a grayscale image to color.
     * @param inputImage The input grayscale image.
     * @return The BGR image, or the original if BGR.
     */
cv::Mat convertToColor(const cv::Mat &input);

/**
     * @brief Splits a color image into its B, G, R channels.
     * @param inputImage The input BGR image.
     * @return A vector containing the B, G, and R channel Mats. Returns empty if input is not 3-channel.
     */
std::vector<cv::Mat> splitColorChannels(const cv::Mat& inputImage);

/**
     * @brief Converts a color image to HSV color space.
     * @param inputImage The input BGR image.
     * @return The HSV image. Returns empty Mat if input is not 3-channel.
     */
std::vector<cv::Mat> convertToHSV(const cv::Mat& inputImage);

/**
     * @brief Converts a color image to Lab color space.
     * @param inputImage The input BGR image.
     * @return The Lab image. Returns empty Mat if input is not 3-channel.
     */
std::vector<cv::Mat> convertToLab(const cv::Mat& inputImage);

// ==========================================================================
// Group 6: Image Processing - Point Operations
// ==========================================================================

/**
     * @brief Applies image negation (inversion).
     * @param inputImage The input image (grayscale or color).
     * @return The negated image.
     */
cv::Mat applyNegation(const cv::Mat& inputImage);

/**
     * @brief Applies contrast stretching based on input/output ranges.
     * @param inputImage The input grayscale image.
     * @param p1 Lower bound of input range.
     * @param p2 Upper bound of input range.
     * @param q3 Lower bound of output range.
     * @param q4 Upper bound of output range.
     * @return The contrast-stretched image.
     */
cv::Mat applyRangeStretching(const cv::Mat& inputImage, int p1, int p2, int q3, int q4);

/**
     * @brief Applies posterization to reduce the number of intensity levels.
     * @param inputImage The input grayscale image.
     * @param levels The desired number of intensity levels (2-256).
     * @return The posterized image.
     */
cv::Mat applyPosterization(const cv::Mat& inputImage, int levels);

/**
     * @brief Performs bitwise AND operation between two images.
     * @param img1 First input image.
     * @param img2 Second input image (must be same size and type as img1).
     * @return Result of img1 & img2.
     */
cv::Mat applyBitwiseAnd(const cv::Mat& img1, const cv::Mat& img2);

/**
     * @brief Performs bitwise OR operation between two images.
     * @param img1 First input image.
     * @param img2 Second input image (must be same size and type as img1).
     * @return Result of img1 | img2.
     */
cv::Mat applyBitwiseOr(const cv::Mat& img1, const cv::Mat& img2);

/**
     * @brief Performs bitwise XOR operation between two images.
     * @param img1 First input image.
     * @param img2 Second input image (must be same size and type as img1).
     * @return Result of img1 ^ img2.
     */
cv::Mat applyBitwiseXor(const cv::Mat& img1, const cv::Mat& img2);

/**
     * @brief Adds two images.
     * @param img1 First input image.
     * @param img2 Second input image (must be same size and type as img1).
     * @return Result of saturated addition img1 + img2.
     */
cv::Mat applyAddition(const cv::Mat& img1, const cv::Mat& img2);

/**
     * @brief Subtracts the second image from the first.
     * @param img1 First input image.
     * @param img2 Second input image (must be same size and type as img1).
     * @return Result of saturated subtraction img1 - img2.
     */
cv::Mat applySubtraction(const cv::Mat& img1, const cv::Mat& img2);

/**
     * @brief Blends two images using a weighted sum.
     * @param img1 First input image.
     * @param img2 Second input image (must be same size and type as img1).
     * @param alpha Weight for the first image (0.0 to 1.0). Weight for second is (1.0 - alpha).
     * @param gamma Value added to the sum (usually 0).
     * @return Result of alpha*img1 + (1-alpha)*img2 + gamma.
     */
cv::Mat applyBlending(const cv::Mat& img1, const cv::Mat& img2, double alpha, double gamma = 0.0);


// ==========================================================================
// Group 7: Image Processing - Histogram Operations
// ==========================================================================

/**
     * @brief Stretches the histogram to the full 0-255 range.
     * @param inputImage The input grayscale image.
     * @return The histogram-stretched image.
     */
cv::Mat stretchHistogram(const cv::Mat& inputImage);

/**
     * @brief Applies histogram equalization manually using CDF.
     * @param inputImage The input grayscale image.
     * @return The histogram-equalized image.
     */
cv::Mat equalizeHistogram(const cv::Mat& inputImage);

// ==========================================================================
// Group 8: Image Processing - Filtering & Edge Detection
// ==========================================================================

/**
     * @brief Applies a simple box blur filter.
     * @param inputImage The image to blur.
     * @param kernelSize The size of the blur kernel (e.g., 3 for 3x3).
     * @param borderOption OpenCV border handling flag (e.g., cv::BORDER_DEFAULT).
     * @return The blurred image.
     */
cv::Mat applyBoxBlur(const cv::Mat& inputImage, int kernelSize, int borderOption);

/**
     * @brief Applies a Gaussian blur filter.
     * @param inputImage The image to blur.
     * @param kernelSize The size of the Gaussian kernel (must be odd, e.g., 3 for 3x3).
     * @param sigmaX Gaussian kernel standard deviation in X direction.
     * @param sigmaY Gaussian kernel standard deviation in Y direction (if 0, it's set to sigmaX).
     * @param borderOption OpenCV border handling flag.
     * @return The Gaussian-blurred image.
     */
cv::Mat applyGaussianBlur(const cv::Mat& inputImage, int kernelSize, double sigmaX, double sigmaY, int borderOption);

/**
     * @brief Applies Sobel edge detection (combining X and Y gradients).
     * @param inputImage The input grayscale image.
     * @param kernelSize Size of the extended Sobel kernel (1, 3, 5, or 7).
     * @param scale Scale factor for the computed derivative values.
     * @param delta Delta value added to the results before storing them.
     * @param borderOption OpenCV border handling flag.
     * @return The edge magnitude image (CV_8U).
     */
cv::Mat applySobelEdgeDetection(const cv::Mat& inputImage, int kernelSize, double scale, double delta, int borderOption);

/**
     * @brief Applies Laplacian edge detection.
     * @param inputImage The input grayscale image.
     * @param kernelSize Aperture size used to compute the second-derivative filters (must be positive and odd).
     * @param scale Optional scale factor for the computed Laplacian values.
     * @param delta Optional delta value added to the results before storing them.
     * @param borderOption OpenCV border handling flag.
     * @return The Laplacian edge image (CV_8U).
     */
cv::Mat applyLaplacianEdgeDetection(const cv::Mat& inputImage, int kernelSize, double scale, double delta, int borderOption);

/**
     * @brief Applies Canny edge detection.
     * @param inputImage The input grayscale image.
     * @param threshold1 First threshold for the hysteresis procedure.
     * @param threshold2 Second threshold for the hysteresis procedure.
     * @param apertureSize Aperture size for the Sobel operator (3, 5, or 7).
     * @param L2gradient Flag indicating whether to use a more accurate L2 norm for gradient magnitude.
     * @return The binary edge map (CV_8U).
     */
cv::Mat applyCannyEdgeDetection(const cv::Mat& inputImage, double threshold1, double threshold2, int apertureSize = 3, bool L2gradient = false);

/**
     * @brief Applies a sharpening filter using a predefined kernel.
     * @param inputImage The input grayscale image.
     * @param option 1: Basic Sharpening, 2: Strong Sharpening, 3: Edge Enhancement.
     * @param borderOption OpenCV border handling flag.
     * @return The sharpened image.
     */
cv::Mat applySharpening(const cv::Mat& inputImage, int option, int borderOption);

/**
     * @brief Applies a Prewitt edge detection filter for a specific direction.
     * @param inputImage The input grayscale image.
     * @param direction The direction index (0-8, excluding 4).
     * @param borderOption OpenCV border handling flag.
     * @return The edge detected image for the specified direction.
     */
cv::Mat applyPrewittEdgeDetection(const cv::Mat& inputImage, int direction, int borderOption);

/**
     * @brief Applies a custom user-defined filter kernel.
     * @param inputImage The input grayscale image.
     * @param kernel The custom filter kernel (CV_32F).
     * @param normalize If true, the kernel will be normalized by dividing by its sum (if sum is non-zero).
     * @param borderOption OpenCV border handling flag.
     * @return The filtered image.
     */
cv::Mat applyCustomFilter(const cv::Mat& inputImage, cv::Mat kernel, bool normalize, int borderOption);

/**
     * @brief Applies a median filter.
     * @param inputImage The input grayscale image.
     * @param kernelSize Aperture linear size (must be odd and greater than 1, e.g., 3, 5).
     * @param borderOption OpenCV border handling flag (Note: medianBlur itself doesn't use borderOption, custom implementation needed for that).
     * @return The median-filtered image.
     */
cv::Mat applyMedianFilter(const cv::Mat& inputImage, int kernelSize, int borderOption);

/**
     * @brief Applies a 5x5 filter derived from the convolution of two 3x3 kernels.
     * @param inputImage The input grayscale image.
     * @param kernel1 The first 3x3 kernel (CV_32F).
     * @param kernel2 The second 3x3 kernel (CV_32F).
     * @param borderOption OpenCV border handling flag.
     * @return The filtered image using the combined 5x5 kernel.
     */
cv::Mat applyTwoStepFilter(const cv::Mat& inputImage, const cv::Mat& kernel1, const cv::Mat& kernel2, int borderOption);

// ==========================================================================
// Group 9: Image Processing - Morphology
// ==========================================================================

/**
     * @brief Creates a 3x3 structuring element for morphological operations.
     * @param type Diamond (4-connected) or Square (8-connected).
     * @return The 3x3 structuring element (CV_8U).
     */
cv::Mat getStructuringElement(StructuringElementType type);

/**
     * @brief Applies morphological erosion.
     * @param inputImage The input binary or grayscale image.
     * @param elementType Diamond or Square structuring element.
     * @param iterations Number of times erosion is applied.
     * @param borderOption OpenCV border handling flag.
     * @return The eroded image.
     */
cv::Mat applyErosion(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption);

/**
     * @brief Applies morphological dilation.
     * @param inputImage The input binary or grayscale image.
     * @param elementType Diamond or Square structuring element.
     * @param iterations Number of times dilation is applied.
     * @param borderOption OpenCV border handling flag.
     * @return The dilated image.
     */
cv::Mat applyDilation(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption);

/**
     * @brief Applies morphological opening (erosion followed by dilation).
     * @param inputImage The input binary or grayscale image.
     * @param elementType Diamond or Square structuring element.
     * @param iterations Number of times opening is applied.
     * @param borderOption OpenCV border handling flag.
     * @return The opened image.
     */
cv::Mat applyOpening(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption);

/**
     * @brief Applies morphological closing (dilation followed by erosion).
     * @param inputImage The input binary or grayscale image.
     * @param elementType Diamond or Square structuring element.
     * @param iterations Number of times closing is applied.
     * @param borderOption OpenCV border handling flag.
     * @return The closed image.
     */
cv::Mat applyClosing(const cv::Mat& inputImage, StructuringElementType elementType, int iterations, int borderOption);

/**
     * @brief Applies morphological skeletonization iteratively.
     * @param inputImage The input binary image (non-zero pixels treated as foreground).
     * @param elementType The structuring element type to use (typically Diamond).
     * @return The skeletonized image (CV_8U).
     */
cv::Mat applySkeletonization(const cv::Mat& inputImage, StructuringElementType elementType);


// ==========================================================================
// Group 10: Image Processing - Feature Detection
// ==========================================================================

/**
     * @brief Detects lines in a binary image using the standard Hough transform.
     * @param binaryEdgeImage The input edge map (binary image, typically from Canny).
     * @param rho Distance resolution of the accumulator in pixels.
     * @param theta Angle resolution of the accumulator in radians.
     * @param threshold Accumulator threshold parameter. Only lines receiving more votes than threshold are returned.
     * @return An image with the detected lines drawn.
     */
cv::Mat detectHoughLines(const cv::Mat& binaryEdgeImage, double rho, double theta, int threshold);

// ==========================================================================
// Group 11: Image Segmentation - Thresholding
// ==========================================================================


/**
     * @brief If the pixel value is smaller than or equal to the threshold, it is set to 0, otherwise it is set to a maximum value.
     * @param inputImage The input grayscale image.
     * @param threshold Threshold value.
     * @return An image with the threshold applied.
     */
cv::Mat applyGlobalThreshold(const cv::Mat& inputImage, int threshold);

/**
     * @brief Determines the threshold for a pixel based on a small region around it and thresholds it.
     * @param inputImage The input grayscale image.
     * @return An image with the adaptive threshold applied.
     */
cv::Mat applyAdaptiveThreshold(const cv::Mat& inputImage);

/**
     * @brief Otsu's method determines an optimal global threshold value from the image histogram and then thresholds an image.
     * @param inputImage The input grayscale image.
     * @return An image with the Otsu Threshold applied.
     */
cv::Mat applyOtsuThreshold(const cv::Mat& inputImage);

/**
     * @brief Magic wand segmentation algorithm which puts all nearby pixels with values within tolerance into one group.
     * @param inputImageRaw The input image.
     * @param seed Starting point.
     * @param tolerance Tolerance value.
     * @return A magic wand segmented image.
     */
cv::Mat magicWandSegmentation(const cv::Mat& inputImageRaw, const cv::Point& seed, int tolerance);

/**
     * @brief Grab cut segmentation algorithm which cuts the object from the background.
     * @param inputImage The input grayscale image.
     * @param rect The rectangle area on which the operation will be applied.
     * @param iterCount Operation iterations count.
     * @return A grab cutted image.
     */
cv::Mat grabCutSegmentation(const cv::Mat& inputImage, const cv::Rect& rect, int iterCount = 5);

/**
     * @brief Watershed algorithm with all preprocessing steps included.
     * @param inputImage The input image.
     * @return An image with the Watershed applied.
     */
cv::Mat applyWatershedSegmentation(const cv::Mat &inputImage);

/**
     * @brief Inpaitning algorithm.
     * @param inputImage The input image.
     * @param mask The inpainting mask.
     * @param radius Radius value.
     * @param method Used method.
     * @return An image with the Inpainting applied.
     */
cv::Mat applyInpainting(const cv::Mat& inputImage, const cv::Mat& mask, double radius, int method);

} // namespace ImageProcessing

#endif // IMAGE_ALGORITHMS_H
