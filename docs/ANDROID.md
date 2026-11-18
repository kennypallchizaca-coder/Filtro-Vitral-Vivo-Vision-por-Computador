# Android — Parte I

La versión móvil está en el repositorio independiente `App-movil-vision`. Mantiene su propia copia de `Principal.hpp`, idéntica a la del escritorio, para compilar sin depender de una ruta absoluta a otro proyecto.

## Qué se reutiliza

- `Parametros`, su validación y `Efecto<Mat>` / `Efecto<UMat>`: mismo kernel y misma secuencia ASCII.
- Mediana, CLAHE, Gaussiano, Sobel y selección de caracteres según brillo y borde.
- La lógica matemática procede de los ejemplos de clase; cámara, interfaz y mediciones se adaptan a cada plataforma.

## Qué cambia por plataforma

| Escritorio | Android |
|---|---|
| VideoCapture y HighGUI, siete trackbars | JavaCamera2View, Kotlin/XML y siete SeekBars |
| Mat BGR | Cámara RGBA, conversión a BGR en C++ y regreso a RGBA |
| PSAPI / PDH de Windows | PSS, memoria disponible y tiempo CPU del proceso |
| VTK interactivo con video en texturas | VTK no se exige en móvil |
| Ejecutable CMake | JNI, NDK y APK ARM64 |

## Contratos que se conservan

1. El filtro recibe imágenes BGR `CV_8UC3` no vacías y parámetros válidos.
2. Cada `Efecto` posee sus búferes; no se procesa en paralelo sobre una misma instancia.
3. La interfaz entrega una copia de los parámetros por fotograma.
4. La salida Android es RGBA y mantiene las dimensiones de entrada requeridas por el Bitmap de cámara.
5. La comparación encaja original y resultado completos sin recorte ni deformación. Las vistas intermedias siguen disponibles. El Portal ASCII mantiene la cámara original fuera del marco.
6. Se sincroniza OpenCL antes de leer sus tiempos. La descarga ASCII transfiere la cuadrícula en lugar de la imagen completa. El dibujo con CPU entra en el total de cada ruta. Una ruta no disponible se informa como tal.

## VTK de escritorio

`Vista3D.hpp` combina original y efecto con `Mat`, `Rect` y `copyTo`, y actualiza la textura de un plano mediante `setImage` y `spinOnce`. El visor VTK real está integrado en la única ventana blanca Lienzo, junto a las barras y métricas. Arrastrar gira la escena, `R` restaura el encuadre y Esc cierra la aplicación. La prueba verifica colores, pose, reapertura, posición del panel, una sola ventana y cierre seguro. Esta integración es exclusiva del escritorio Windows; no modifica Android.

## Compilación y comprobación móvil

En Android Studio se abre el repositorio Android y se ejecuta `app` sobre un dispositivo ARM64. Se utiliza OpenCV 5.0.0 con OpenCL, SDK37, NDK28.2.13676358 y CMake3.22.1.

La compilación de APK, pruebas nativas/JVM y lint no sustituye una prueba física de permisos, cámaras, orientación y rendimiento. La versión ASCII todavía requiere prueba de cámara y rendimiento en el S25 Ultra. En esta revisión no se transfirieron archivos del teléfono. Los emuladores locales instalados son x86_64 y no ejecutan directamente el APK ARM64; no se modificó la arquitectura ni se fabricaron resultados móviles.

La [evidencia y declaración de IA](ENTREGA_PARTE_I.md) están preparadas para la futura plantilla; el guion del videoblog describe ASCII. La grabación hablada en inglés la realizará el usuario después. La Parte II no está incluida.
