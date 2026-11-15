# Android continuation — Part I

Checkpoint verified on 2026-11-14. The next implementation stage is the Android version of the Part I visual-effects application.

## Reusable code

| File | Role in the Android implementation |
|---|---|
| `Principal.hpp` | Reuse the existing `Parametros` and `Efecto<Imagen>` processing code. It requires OpenCV core/imgproc and standard C++. |
| `Mediciones.hpp` | Review the measurement interface when implementing Android resource reporting. |
| `Principal.cpp` | Reference for timing, warm-up, comparison and CSV behavior. Its camera, window and VTK integration belongs to desktop. |
| `Interfaz.hpp` | Desktop HighGUI controls. Android needs controls connected to the same parameters. |
| `Mediciones.cpp` | Windows implementation using PDH and PSAPI. Android needs its own platform implementation. |
| `CMakeLists.txt` | Desktop build. The Android native build needs its own OpenCV Android configuration. |

## Native image contract

- The filter input is a nonempty BGR image with type `CV_8UC3`.
- Convert the camera's actual pixel format to BGR before processing. Convert the result to the format required by the Android preview.
- Use `Parametros::validar()` and the current parameter defaults and ranges.
- Keep the processing order in `Efecto::procesar`, including the contour branch before printed textures.
- An `Efecto` instance owns mutable buffers and caches. Process frames sequentially on its owning thread.
- Take a parameter snapshot for each frame when UI updates can occur concurrently.
- `procesar()` returns a reference to an internal output buffer. Copy or convert that result into the preview output before processing another frame.

## Verified desktop checkpoint

- `cmake --build build --parallel 2` succeeds.
- The executable's `--help` entry point exits successfully.
- Focused checks pass for the palette, translucent halftone, highlight protection, dark hatching, radius zero, resolution changes and invalid kernels.
- CPU and UMat output similarity passed on the NVIDIA GeForce RTX 4070 Laptop GPU.
- The camera preview ran at approximately 30 FPS during interactive checks.
- The kernel trackbar changed from 5 to 7 and back to 5 with fixed preview positions.
- Stage selection and CPU/OpenCL mode switching worked.
- V opened the VTK inspector. Escape returned to the camera, and a second Escape closed the application successfully.

### Benchmark verification

These checks used the same generated synthetic 640×480 MJPG clip, 120 recorded samples per case, and separate CPU/OpenCL processes. The application performed its normal warm-up before sampling.

| Mode | Gaussian kernel | Mean processing time, including GPU transfers |
|---|---:|---:|
| CPU Mat | 5 | 5.289 ms |
| OpenCL UMat | 5 | 5.239 ms |
| CPU Mat | 31 | 5.548 ms |
| OpenCL UMat | 31 | 4.627 ms |

All four CSVs contained frame indices 0 through 119 and positive filtering time, processing FPS and process RAM readings. These desktop synthetic checks do not establish phone performance. Use the reproducible procedure in the main README for the final measurements on each platform.

## Open desktop requirement

The current VTK inspector displays a paused frame. The teacher's current Part I guide requires the desktop processed video to update as an interactive 3D texture. That requirement remains open before claiming complete desktop delivery.

## Android environment checkpoint

The Android SDK is installed, with NDK `28.2.13676358` and Android CMake `3.22.1` available. The desktop uses OpenCV `5.0.0`.

The Android OpenCV package/version, target ABI, native compilation and physical-device OpenCL support still need verification. Select compatible Android headers and libraries before assuming that the desktop OpenCV installation can be reused.

## Next implementation sequence

1. Identify the target Android device and verify its Android OpenCV/NDK configuration.
2. Create the Android camera and JNI integration around `Principal.hpp`, starting with CPU `Mat` processing.
3. Display the original image, final result and selectable intermediate stages.
4. Connect Android controls to the existing parameter ranges and add mobile timing/resource reporting.
5. Detect the phone's OpenCL device and verify the `UMat` path. The desktop NVIDIA device selector is machine-specific.
6. Run CPU/OpenCL comparisons on the same mobile input and record the selected device, resolution, parameters and transfer costs.

GPU availability must be checked on the phone. A `UMat` pipeline can fall back to CPU operations, so report the effective execution mode and device explicitly.
