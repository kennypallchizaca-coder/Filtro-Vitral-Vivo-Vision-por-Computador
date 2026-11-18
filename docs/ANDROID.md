# Android — Parte I

La versión móvil está en el repositorio independiente `App-movil-vision`. Mantiene su propia copia de `Principal.hpp`, idéntica a la del escritorio, para compilar sin depender de una ruta absoluta a otro proyecto.

## Qué se reutiliza

- `Parametros`, su validación y `Efecto<Mat>` / `Efecto<UMat>`: mismo núcleo Vitral Vivo.
- Mediana, CLAHE, Gaussiano, Sobel, kernel propio y triángulos de color cuyo detalle depende del borde.
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
5. La comparación encaja original y resultado completos. Portal Vitral mantiene la cámara original fuera del marco.
6. La app ejecuta y mide cada modo por separado; OpenCL descarga los mapas reducidos de color y borde, y el dibujo CPU cuenta en el total.

## VTK de escritorio

`Vista3D.hpp` combina original y efecto con `Mat`, `Rect` y `copyTo`, y actualiza la textura de un plano mediante `setImage` y `spinOnce`. El visor VTK real está integrado en la única ventana blanca Lienzo, junto a las barras y métricas. Arrastrar gira la escena, `R` restaura el encuadre y Esc cierra la aplicación. La prueba verifica colores, pose, reapertura, posición del panel, una sola ventana y cierre seguro. Esta integración es exclusiva del escritorio Windows; no modifica Android.

## Compilación y comprobación móvil

En Android Studio se abre el repositorio Android y se ejecuta `app` sobre un dispositivo ARM64. Se utiliza OpenCV 5.0.0 con OpenCL, SDK37, NDK28.2.13676358 y CMake3.22.1.

La compilación de APK, pruebas nativas/JVM y lint no sustituye comprobar permisos, cámaras, orientación y rendimiento de Vitral Vivo en el S25 Ultra. No se conectó el teléfono. El APK es ARM64 y los emuladores locales son x86_64.

La [evidencia y declaración de IA](ENTREGA_PARTE_I.md) quedan para la futura plantilla. El guion en inglés describe Vitral Vivo. La Parte II no se incluye.
