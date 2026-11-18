# Vitral Vivo: live stained-glass video effect

Estimated duration with demonstrations: 6–7 minutes. Record the actual desktop and phone application. The bracketed directions are filming cues, not spoken text. This is a script, not a recorded video.

## Before recording

- Open the Release build on the PC and check that the camera or input video runs. Rehearse rotating the VTK frame, pressing **R** to reset it, and using **T** to show a processing stage.
- On the S25, start in **Comparación completa**. Use **Gaussiano** to show an intermediate image, then go back to the comparison before reading measurements.
- Keep the input, view and filter settings unchanged while switching CPU to OpenCL. Wait for both readings and for the app to show **GPU / OpenCL** as the active mode. If it falls back to CPU, say so instead of presenting that sample as GPU.

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

We keep the signed response in a floating-point image, then multiply its absolute value by twelve to bring it closer to Sobel's scale. The weight control mixes the two; the resulting mask reveals smaller facets near edges.

## 4. Drawing the stained glass

[Show the final result and change the stained-glass scale.]

We use a fixed mesh of irregular triangles. Their colors come from the video and dark seams form the glass pattern.

Near edges we gradually reveal smaller facets inside the same triangles. The mesh does not move from one frame to the next.

## 5. Desktop, controls and 3D

[Show the sliders and readings, then rotate the scene so the depth of the frame is visible.]

The desktop window contains seven sliders, resource readings and both images. VTK displays the video as a dynamic texture in that same window.

Following the classroom examples, copyTo places the original and processed images in two rectangular regions of one Mat. This combined image is the texture of one plane. It updates on every loop. The program keeps the same scene and camera pose, so rotating it does not reset the view on the next frame. R restores the initial view, and Escape closes the application. Four solid bars give the frame real depth. The video is a flat texture, not a three-dimensional reconstruction of the person.

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

The camera supplies RGBA images. C++ converts them to BGR for processing and returns RGBA with the original dimensions. The comparison keeps the complete original on the left and the stained-glass result on the right in both orientations, without stretching or cropping.

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
