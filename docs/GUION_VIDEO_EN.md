# Spiderman visual effect: English videoblog script

Estimated spoken duration: 6–7 minutes. Record the actual desktop and phone application. The bracketed directions are filming cues, not spoken text. This file is a script, not a completed video.

## 1. Objective

[Show the original camera image and the Spiderman result.]

Hello. This project transforms live video into a comic-book effect inspired by *Spider-Man: Into the Spider-Verse*.

The application has two versions: a C++ desktop application and an Android application with a native C++ processing library. Both use OpenCV and the same image-processing code.

The goal is to combine the techniques studied in class, design a parameterized edge filter, and compare CPU processing with the OpenCL pipeline. This presentation covers Part One only.

## 2. Processing steps

[Select the intermediate views while describing them.]

First, the image is converted to the Lab color space. CLAHE changes the lightness channel to adjust local contrast without directly equalizing the color channels.

Next, a three-by-three median filter reduces isolated values. The image is converted to grayscale, and a Gaussian filter smooths small details before edge detection.

Sobel calculates horizontal and vertical changes in brightness. Their absolute responses are combined. The program also applies our own three-by-three kernel using filter2D.

These steps include four classroom filters: CLAHE, median, Gaussian and Sobel, plus our designed kernel.

## 3. Our kernel

[Show the kernel equation in the report, then move the diagonal and own-weight controls.]

Our kernel is based on a Laplacian edge detector. Its center is positive and selected neighbors are negative. The diagonal parameter changes the contribution of two diagonal neighbors.

The coefficients add up to zero. For example, when every pixel in a neighborhood has the same brightness, its response is zero. A change in brightness can produce a nonzero response.

We keep the signed response in a floating-point image before calculating its absolute value. The own-weight parameter mixes this response with Sobel. Finally, a threshold creates the black contour mask.

The name Spiderman describes the complete artistic effect. It is not the name of a standard convolution kernel.

## 4. Comic appearance

[Show the final result. Adjust color levels, dots, saturation and RGB displacement.]

A lookup table reduces the number of color levels. Color masks add blue-violet shadows and warmer highlights. Halftone dots affect the middle tones, and thin diagonal lines texture the dark areas.

The contour mask paints selected pixels black. HSV adjustments control saturation and brightness, including a stronger treatment of blue, purple and pink tones. The last step shifts the red and blue channels in opposite directions to create a small RGB glitch.

Edges are calculated before the dots and lines are drawn. Otherwise, the texture itself could create unwanted contours.

## 5. Desktop, controls and 3D

[Show the sliders and resource readings. Rotate the VTK scene while the video changes.]

One white desktop window contains twelve sliders, resource measurements and both images. A real VTK viewport is embedded in this window, so the processed video is still a dynamic 3D texture rather than a screenshot copied into a two-dimensional display.

Following the classroom examples, copyTo places the original and processed images in two rectangular regions of one Mat. This combined image is the texture of one plane. It updates on every loop. The program keeps the same scene and camera pose, so rotating it does not reset the view on the next frame. R restores the initial view, and Escape closes the application. This is a textured plane, not a three-dimensional reconstruction of the person.

The Gaussian kernel size changes the computational load. It is always an odd number. Other controls change sigma, the custom response weight, the threshold, the color settings and the printed texture.

## 6. CPU and OpenCL measurements

[Show the report's measured FPS and memory charts; keep their units visible.]

The CPU version uses Mat. The OpenCL version uses UMat and enables OpenCL explicitly. A shared template avoids writing the effect twice.

OpenCL work can be asynchronous, so the program waits for completion before reading its timers. The desktop interface separates upload, filtering and download time. It also records process RAM and system-resource samples.

For the report, the two modes processed the same controlled synthetic video in separate processes, with the same parameters and warm-up procedure. The charts report measured results, not an assumed speedup.

OpenCL is not always faster. Transfers, image size, operation support and kernel size affect the result. The processing throughput is not the camera's capture rate, and process RAM is not dedicated GPU memory.

## 7. Android

[Show the S25 Ultra running the application: complete comparison, an intermediate view, a slider, then CPU/OpenCL selection.]

On Android, Kotlin manages the camera, controls and lifecycle. JNI connects the interface to the same C++ effect.

The camera supplies RGBA images. C++ converts them to BGR for processing and returns RGBA with the original dimensions. The comparison fits both complete images without stretching or cropping them.

The application displays timing, preview FPS, process CPU and memory. Both routes are timed on the same frame, and their measurements stay on screen. No results are recorded or exported. If an actual GPU route is unavailable, the interface reports the CPU fallback instead of inventing GPU results.

The desktop measurements in this report are not measurements of this phone.

## 8. Conclusion

[Return to the final visual effect and the bibliography.]

This project combines familiar classroom operations with a parameterized custom kernel and pixel masks to create a comic-style video effect on desktop and Android.

The main lessons are to reuse the same mathematics, measure completed work, include transfer costs, and describe the limits of each measurement. The report identifies the adapted classroom examples, published references, library documentation and development assistance.

Thank you.

## Optional live comic portal demonstration on Android

[On Android, select Portal cómic and move your hand across the frame.]

The camera stays normal outside the frame and turns into a comic inside it. We use a rectangular mask and copyTo, following our classroom examples. Both processing routes still calculate the whole effect.

The printed dots are lighter, the ink mask is cleaned with a median filter, and RGB displacement is limited to the edges. This is image processing, not augmented reality or face recognition.
