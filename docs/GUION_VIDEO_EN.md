# Vitral Vivo: live stained-glass video effect

Estimated spoken duration: 6–7 minutes. Record the actual desktop and phone application. The bracketed directions are filming cues, not spoken text. This file is a script, not a completed video.

## 1. Objective

[Show the original camera image beside Vitral Vivo in the live demonstration.]

Hello. Vitral Vivo transforms live video into a stained-glass mosaic of colored triangles.

The application has two versions: a C++ desktop application and an Android application with a native C++ processing library. Both use OpenCV and the same image-processing code.

The goal is to combine the techniques studied in class, design a parameterized edge filter, and compare CPU processing with the OpenCL pipeline. This presentation covers Part One only.

## 2. Processing steps

[Select the intermediate views while describing them.]

First, we convert the frame to grayscale. A three-by-three median filter removes isolated noise, and CLAHE improves local contrast. Then a Gaussian filter smooths small details before we look for edges.

Sobel calculates horizontal and vertical changes in brightness. Their absolute responses are combined. The program also applies our own three-by-three kernel using filter2D.

These steps include four classroom filters: CLAHE, median, Gaussian and Sobel, plus our designed kernel.

## 3. Our kernel

[Show the kernel equation in the report, then move the diagonal and own-weight controls.]

Our kernel is based on a Laplacian edge detector. Its center is positive and selected neighbors are negative. The diagonal parameter changes the contribution of two diagonal neighbors.

The coefficients add up to zero. For example, when every pixel in a neighborhood has the same brightness, its response is zero. A change in brightness can produce a nonzero response.

We keep the signed response in a floating-point image, then take its absolute value and add a fixed gain to bring it closer to Sobel's scale. The weight control mixes the two; the resulting mask reveals smaller facets near edges.

## 4. Drawing the stained glass

[Show the final result and change the stained-glass scale.]

We use a fixed mesh of irregular triangles. Their colors come from the video and dark seams form the glass pattern.

Near edges we gradually reveal smaller facets inside the same triangles. The mesh does not move from one frame to the next.

## 5. Desktop, controls and 3D

[Show the sliders and resource readings. Rotate the VTK scene while the video changes.]

One white desktop window contains seven sliders, resource measurements and both images. A real VTK viewport is embedded in this window, so the processed video is still a dynamic 3D texture rather than a screenshot copied into a two-dimensional display.

Following the classroom examples, copyTo places the original and processed images in two rectangular regions of one Mat. This combined image is the texture of one plane. It updates on every loop. The program keeps the same scene and camera pose, so rotating it does not reset the view on the next frame. R restores the initial view, and Escape closes the application. This is a textured plane, not a three-dimensional reconstruction of the person.

The Gaussian kernel size changes the computational load. It is always an odd number. Other controls change sigma, the custom response weight, the threshold, the cell size and CLAHE contrast.

## 6. CPU and OpenCL measurements

[Show current Vitral Vivo timings. Distinguish processing throughput from camera FPS.]

The CPU version uses Mat. The OpenCL version uses UMat and enables OpenCL explicitly. A shared template avoids writing the effect twice.

OpenCL work can be asynchronous, so we wait before reading the timers. Desktop shows upload, filtering and download separately. We download reduced color and edge maps and draw the stained glass on the CPU; that drawing counts in the total.

For a fair desktop benchmark both modes use the same synthetic video, parameters and warm-up. Earlier effects' figures do not describe Vitral Vivo.

OpenCL is not always faster. Transfers, image size, operation support and kernel size affect the result. The processing throughput is not the camera's capture rate, and process RAM is not dedicated GPU memory.

## 7. Android

[Show the S25 Ultra running the application: complete comparison, an intermediate view, a slider, then CPU/OpenCL selection.]

On Android, Kotlin manages the camera, controls and lifecycle. JNI connects the interface to the same C++ effect.

The camera supplies RGBA images. C++ converts them to BGR for processing and returns RGBA with the original dimensions. The comparison fits both complete images without stretching or cropping them.

Android runs one mode at a time and keeps the latest complete sample for each. We show processing time, preview FPS, process CPU and memory. We do not export results. A CPU fallback is labeled and not reported as an OpenCL sample.

The desktop measurements in this report are not measurements of this phone.

## 8. Conclusion

[Return to the final visual effect and the bibliography.]

Vitral Vivo combines four classroom filters and a custom kernel with a stable triangular mesh on desktop and Android.

The main lessons are to reuse the same mathematics, measure completed work, include transfer costs, and describe the limits of each measurement. The report identifies the adapted classroom examples, published references, library documentation and development assistance.

Thank you.

## Optional live stained-glass portal demonstration on Android

[On Android, select Portal Vitral and move your hand across the frame.]

The camera stays normal outside the rectangle and turns into stained glass inside it. We use a rectangular mask and copyTo, following the classroom examples.

This is image processing, not augmented reality or face recognition.
