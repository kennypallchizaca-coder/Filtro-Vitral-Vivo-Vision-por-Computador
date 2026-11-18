# Vitral Vivo — Parte I

Vitral Vivo reemplaza el ASCII por una malla estable de triángulos de color y uniones oscuras. Sobel y el kernel propio, calibrado antes de la mezcla, realzan subdivisiones sin mover sus vértices. Se conservan mediana, CLAHE, Gaussiano, los controles dinámicos, la textura VTK y la comparación original/efecto con vistas intermedias en Android. Ambos repositorios contienen el mismo `Principal.hpp`.

## Evidencia vigente (PC, 17 de noviembre de 2026)

- Escritorio Release: 3/3 pruebas sin ventana. Se compiló la prueba VTK pero no se ejecutó porque obtiene una captura del visor. Falta repetir la demostración VTK y el uso de GPU en vivo sin capturas.
- Android: APK ARM64 y APK de instrumentación compilados offline, 3/3 pruebas JVM, 11/11 comprobaciones estáticas y de empaquetado, 1/1 prueba nativa en Windows con OpenCL y prueba adicional de respaldo CPU. Lint: 0 errores, 8 advertencias. No se ejecutó instrumentación en el teléfono.
- En Android, CPU/Mat y GPU/OpenCL tienen ventanas separadas; la pantalla conserva la última muestra completa de cada ruta y no cuenta el respaldo como OpenCL. Falta probar cámara, disponibilidad efectiva de OpenCL y rendimiento de Vitral Vivo en el S25 Ultra.
- Asistencia: OpenAI Codex implementó la malla y el ajuste del kernel, integró las ventanas de medición y añadió pruebas de estabilidad y rutas separadas. Petición: «Sí, cambia el ASCII por el vitral de triángulos con malla estable en los dos proyectos.» Falta contrastar la política complementaria del AVAC.

## Benchmark del vitral en escritorio

Mismo clip sintético MJPG de 180 fotogramas, 640×480, SHA-256 `5A79D866C2AFD443C5219F1E174723BBE9C34E10B462D2D0E15FD99EB8D35AE0`. OpenCV 5.0.0, compilación Release; GPU NVIDIA GeForce RTX 4070 Laptop GPU, controlador 617.42. Se midieron 120 fotogramas por combinación en procesos separados tras calentar. El clip se generó en temporal con figuras geométricas y se eliminó; no es una captura ni se exportó CSV.

| Gaussiano | Modo | Filtro medio (ms) | Proceso medio (ms) | FPS de proceso | RAM máxima (MiB) |
|---:|---|---:|---:|---:|---:|
| 5 | CPU | 2,57 | 3,52 | 284,43 | 81,47 |
| 5 | OpenCL | 1,47 | 2,72 | 367,88 | 191,46 |
| 31 | CPU | 3,30 | 4,28 | 233,66 | 80,22 |
| 31 | OpenCL | 1,33 | 2,49 | 402,20 | 193,05 |
| 51 | CPU | 4,55 | 5,50 | 181,81 | 80,01 |
| 51 | OpenCL | 1,35 | 2,52 | 396,18 | 193,18 |

El total incluye dibujo del vitral en CPU y, en OpenCL, transferencias. Los FPS son de procesamiento, no de cámara ni de la ventana con VTK; la RAM es el Working Set del proceso, no VRAM. Para repetir la comparación con un clip fijo, usa `--benchmark --source=ruta.avi --frames=120 --kernel=5 --mode=cpu`; cambia el modo a `gpu` y repite con kernels 31 y 51.

## Registro histórico: versión ASCII sustituida

Las cifras, pruebas y prompts que siguen corresponden al filtro anterior. Se conservan para trazabilidad y no se deben presentar como evidencia de Vitral Vivo.

La guía vigente es `Guia-Proyecto-Integrador-Visión-Artificial-G9_.pdf`, en `Vision-Computador/Proyectos/Proyecto-Integrador-G9`. Las páginas 3–8 contienen el alcance y la rúbrica del interciclo. El ejemplo tipo Sketch no obliga a copiar un estilo. El efecto elegido para esta entrega es ASCII.

## Requisitos y estado

| Requisito | Estado comprobado |
|---|---|
| Escritorio C++/OpenCV y Android con C++ nativo por JNI (pp. 4 y 7) | Ejecutable y APK ARM64 compilados; el núcleo `Principal.hpp` es idéntico en ambos repositorios. |
| Cuatro filtros de clase, kernel propio justificado y modificación de píxeles (pp. 3–4 y 7) | Mediana, CLAHE, Gaussiano, Sobel y `filter2D`; se usa brillo para elegir símbolos y se aumenta un nivel al detectar borde. [Explicación y preguntas](DEFENSA_CODIGO.md). |
| Mat/UMat con OpenCL, controles dinámicos de tamaño y peso, tiempo/RAM/recursos (pp. 3–4 y 7–8) | Se comparó con video sintético en Windows y se seleccionó la RTX 4070 con OpenCL. En Android están los controles y métricas; se comprobó el núcleo móvil en el PC, no el OpenCL del teléfono. |
| Escritorio VTK: textura de video que se actualiza sobre una primitiva 3D (pp. 4 y 7–8) | Original y ASCII en un mismo lienzo con un plano VTK. Prueba de textura/pose/cierre y revisión visual con video sintético. No reconstruye profundidad. |
| Móvil: original y final simultáneos, selector de intermedios (p. 8) | Prueba nativa de composición horizontal/vertical y vistas; selector y controles comprobados por código/XML. Falta revisar la interfaz renderizada, permisos, reinicio, cámaras y rotación en el S25 Ultra. |
| Informe con diagrama, justificación, gráficas CPU/GPU de FPS y memoria, fuentes y conclusiones (p. 8) | La tabla y las fuentes siguientes alimentan la futura plantilla. El HTML se retiró a petición del usuario. No hay una aplicación web exigida en la Parte I; «Web 2.0 o similar» se refiere al informe (p. 1). |
| Videoblog hablado en inglés (pp. 1 y 8) | [Guion](GUION_VIDEO_EN.md) disponible; grabación pendiente. |

No se ha comprobado que otro grupo no utilice la misma combinación artística. Durante la demostración, mostrar el peso propio y tamaño del Gaussiano con la misma fuente en ambos modos. La actividad GPU del proceso visible en el panel incluye VTK y no mide por separado la utilización de un kernel OpenCL.

## Verificaciones de la revisión

- **Escritorio:** CMake Release y CTest **4/4**. La CLI rechaza opciones desconocidas y conserva `--help`/`-h`. La prueba de benchmark elimina su propio video temporal. Antes del ajuste de las letras se comprobaron Lienzo, `T`, Espacio y el deslizador ASCII. Para el acabado de terminal se verificó el resultado desde una foto de prueba, sin capturar la pantalla. No se usó webcam ni se probaron otros monitores o escalas DPI.
- **Android:** pruebas estáticas **11/11**, CTest nativo **1/1** en Windows con la RTX 4070, Gradle debug y APK de instrumentación compilados, **2/2** pruebas JVM y lint sin errores (8 advertencias). La prueba estática de reinicio falló antes de restablecer la visibilidad del mensaje de espera y pasó después. La consulta JNI de estado GPU ahora se realiza cuando la interfaz va a actualizarse, no en todos los fotogramas. No se ejecutó instrumentación ni se midieron tiempos en un teléfono.
- **Alcance web:** no se construyó ni se probó una aplicación web. El único HTML identificado era el informe sustituido por la futura plantilla.

## Datos de escritorio para las gráficas

Medición del 17 de noviembre de 2026. Mismo clip sintético local, MJPG, 240 fotogramas, 640×480; SHA-256 `D280B824B383003DA2398498782E921DC80B7D6FC27D8F624AF3D74FB90DC118`. OpenCV 5.0.0, CPU AMD Ryzen 9 7845HX, NVIDIA GeForce RTX 4070 Laptop GPU con controlador 617.42. Cada fila corresponde a un proceso separado, con calentamiento y 120 muestras; valores restantes predeterminados. No se exportó CSV.

| Gaussiano | Modo | Filtrado medio (ms) | Proceso medio (ms) | FPS de proceso | RAM máxima (MiB) |
|---:|---|---:|---:|---:|---:|
| 5 | CPU | 2,00 | 2,74 | 364,55 | 89,25 |
| 5 | OpenCL | 1,29 | 2,17 | 460,10 | 204,87 |
| 31 | CPU | 2,70 | 3,67 | 272,34 | 88,97 |
| 31 | OpenCL | 1,28 | 2,17 | 461,82 | 205,45 |
| 51 | CPU | 4,16 | 5,13 | 194,91 | 89,15 |
| 51 | OpenCL | 1,29 | 2,18 | 459,53 | 205,03 |

El tiempo total incluye el dibujo ASCII en CPU; OpenCL incluye además subida y descarga. FPS de proceso es `1000 / ms medio` antes del redondeo; no es FPS real de cámara ni incluye VTK. RAM es el Working Set del proceso, no VRAM. En este clip la ruta OpenCL procesó más rápido y usó más RAM; no se extrapola al S25. Los datos en `docs/mediciones` son de la versión anterior, no se mezclan con esta tabla.

Para reproducir, usar `--benchmark --source=ruta-del-video.avi --frames=120 --kernel=5 --mode=cpu`, repetir con `--mode=gpu` y cambiar el tamaño de 5 a 31 y 51. En las gráficas usar el tamaño horizontal y series separadas CPU/OpenCL para FPS de proceso y RAM máxima.

## Herramientas y Asistencia de IA

El informe anterior declaraba:

> Asistencia de desarrollo: OpenAI ChatGPT/Codex, utilizada para propuestas de implementación, revisión, explicación, pruebas y preparación de documentación. Las decisiones y resultados se contrastan con código y ejecuciones; la asistencia no sustituye la comprensión del trabajo.

**Herramienta usada en esta fase:** OpenAI Codex, con el contexto de la conversación. Los siguientes son fragmentos literales comprobables, no una reconstrucción de prompts antiguos.

| Petición | Adaptación y comprobación |
|---|---|
| «IMPLEMETEMOS ESO Y ELIMINES EL COMIC YA QUE NO ME GUSTO» | Se sustituyó el acabado cómic por luminancia y símbolos de borde; se conservaron kernel, filtros de clase, métricas, VTK y vistas Android. Las pruebas comprueban símbolos, tamaño de salida, respuesta del kernel y navegación de resultados. |
| «Revisa, corrige y organiza mi proyecto de Visión por Computador, incluyendo **la aplicación de escritorio, la aplicación web y la aplicación móvil**.» | Se contrastó el alcance con la guía vigente. Se añadieron regresiones para opciones desconocidas y espera al reiniciar cámara, se verificaron compilaciones y se identificó en código la asistencia comprobable. |
| «elimanlo porque ya te voy a paasr la plantilla y ahi va hacerce el informe» | Se retiró el HTML y se conservaron la declaración, las fuentes y los resultados en este material para trasladarlos a la plantilla. No se creó una web nueva. |
| «EL RESULTADO NO ESTA ASI» (con una muestra del retrato ASCII) | Se sustituyeron las rayas direccionales por una escala de caracteres verdes según el brillo. El kernel sigue influyendo en la densidad de celdas con borde. Se compiló y verificó en ambas plataformas y se volvió a medir el rendimiento en el PC. |

Bloques identificados: núcleo ASCII `Principal.hpp` en ambos repositorios; medición del dibujo y validación de opciones en `Principal.cpp`; barras ASCII en `Interfaz.hpp`; vistas, temporización y JNI en `ProcesadorAndroid.hpp`/`native-lib.cpp`; parámetros y estado de la cámara en Kotlin. **Pendiente de confirmar con el estudiante:** prompts de etapas previas no preservados, contribución exacta por bloque antiguo y política separada publicada en el AVAC, no aportada aquí como documento local. No se atribuye automáticamente a IA el material docente ni se afirma una autoría individual solo por el historial Git.

Las pp. 1–2 de la guía permiten asistencia de IA con cita y exigen comprender y explicar el código y su matemática. La declaración de prompts se incluye por petición del estudiante; la rúbrica de la Parte I no la enumera como apartado obligatorio. Antes de entregar, el estudiante debe verificar la política complementaria y ajustar esta declaración si corresponde.

## Fuentes para trasladar a la plantilla

- V. Robles Bykbaev, `Guía Proyecto Integrador G9`, octubre de 2026–febrero de 2027, pp. 1–8. Conformación y captura: Unidad 1, Ejemplo 2 (conversión en línea 37; captura en línea 51); trackbars del Ejemplo 3, líneas 47–53.
- Material de Unidad 2: Ejemplo 5 (CLAHE, mediana y regiones `Rect`, líneas 37, 121–145), Ejemplo 6 (Gaussiano, Sobel y magnitudes, líneas 65–82), Ejemplo 9 (mediana para ruido impulsivo, líneas 130–136). Se adaptan operaciones, no se copia el programa entero.
- K. Pulli et al. (2012), [Realtime Computer Vision with OpenCV](https://research.nvidia.com/publication/2012-06_realtime-computer-vision-opencv), `Communications of the ACM`.
- S. M. Pizer et al. (1987), [Adaptive histogram equalization and its variations](https://doi.org/10.1016/S0734-189X(87)80186-X), `Computer Vision, Graphics, and Image Processing` 39(3), 355–368.
- [OpenCV: soporte OpenCL](https://docs.opencv.org/4.x/dc/d83/group__core__opencl.html). Se contrastaron los encabezados de la versión instalada 5.0.0 para Mat, UMat, `WImage3D::setImage` y `CommandLineParser`. OpenCV Android SDK aporta `JavaCamera2View` y las bibliotecas nativas; Kotlin/AndroidX gestionan interfaz y ciclo de vida.
- Sony Pictures Imageworks, [Spider-Man: Into the Spider-Verse](https://dev.imageworks.com/node/1371), citado en el informe anterior como referencia estética histórica. No aporta código al filtro ASCII.
