# Guía de estudio y sustentación — Parte I

Vitral Vivo convierte el video en piezas triangulares de color con uniones oscuras. El kernel propio contribuye a la máscara de detalle. Android conserva la vista «Portal Vitral».

## 1. Cómo se relacionan las aplicaciones

Escritorio y Android mantienen una copia autónoma del mismo núcleo `Principal.hpp`. La plantilla `Efecto<Imagen>` ejecuta el algoritmo una vez con `cv::Mat` para CPU y otra con `cv::UMat` para la ruta OpenCL. Se conserva esta duplicación entre repositorios para que cada proyecto compile sin depender de una ruta absoluta al otro.

| Parte | Función principal |
|---|---|
| [Principal.hpp](../Principal.hpp) | Parámetros, validación, filtro, mapas reducidos y dibujo del vitral. |
| [Principal.cpp](../Principal.cpp) | Cámara/archivo, ciclo de frames, selección Mat/UMat, tiempos y ventana. |
| [Interfaz.hpp](../Interfaz.hpp) | Siete controles y teclas de escritorio. |
| [Vista3D.hpp](../Vista3D.hpp) | Compone original y resultado y los actualiza como textura VTK. |
| [Mediciones.cpp](../Mediciones.cpp) | Recursos del proceso en Windows. |

En Android, `MainActivity.kt` administra permisos, cámara y controles. `onCameraFrame` toma el `Mat` RGBA prestado por OpenCV y llama a `procesarFrame` de `native-lib.cpp` (JNI). JNI valida y serializa la llamada; `ProcesadorAndroid.hpp` convierte RGBA a BGR, procesa las rutas CPU/GPU, prepara la vista y devuelve una matriz RGBA del mismo tamaño. Kotlin devuelve esa matriz a la vista de cámara y actualiza las métricas. Al pausar se desactiva la cámara; al detenerse se liberan matrices y se reinicia el procesador.

En el repositorio `App-movil-vision`, los archivos Kotlin están en `app/src/main/java/com/example/interciclovision` y el C++ nativo en `app/src/main/cpp`.

No hay una aplicación web separada en el alcance actual. El informe se preparará en la plantilla académica cuando esté disponible.

## 2. Recorrido matemático de un fotograma

En ambos sistemas el núcleo recibe una imagen BGR. `Efecto::procesar` aplica estos pasos en orden:

1. `cvtColor` calcula una luminancia aproximada: `Y ≈ 0.114 B + 0.587 G + 0.299 R`. La conversión no significa que los tres canales tengan el mismo peso.
2. `medianBlur` usa una ventana 3×3: ordena los nueve valores y conserva el central. Es distinto del promedio: un píxel extremo aislado suele desaparecer sin mezclarse con todo el vecindario.
3. `CLAHE` ajusta contraste localmente por regiones y limita el refuerzo de contraste para no amplificarlo sin control. El código usa el parámetro `contraste`; no hace falta memorizar una fórmula interna de OpenCV.
4. `GaussianBlur` suaviza la imagen antes de medir bordes. Cada vecino recibe un peso Gaussiano discreto proporcional a `exp(-(x²+y²)/(2σ²))`; OpenCV normaliza los pesos para que sumen uno. El kernel inicial es 5×5 y `sigma` inicial es 1.73.

### Bordes y filtro propio

`detectarContornos` calcula las derivadas aproximadas `Gx` y `Gy` con Sobel en tipo con signo `CV_16S`. Al omitir `ksize`, OpenCV usa el tamaño por defecto 3. `convertScaleAbs` toma el valor absoluto y lo limita a ocho bits. `addWeighted` promedia esas magnitudes ya limitadas: `S≈(min(255,|Gx|)+min(255,|Gy|))/2`. No calcula la magnitud euclidiana `sqrt(Gx²+Gy²)`.

El kernel propio se define en `crearKernel(a)`:

```text
            [ -a     -1       0 ]
K(a) = 1/(4+2a) * [ -1   4+2a     -1 ]
            [  0     -1      -a ]
```

La suma de los nueve coeficientes es cero. Por eso, una región constante `c` produce `c·sum(K)=0`; una diferencia entre el centro y sus vecinos produce respuesta. `a` regula dos vecinos diagonales, entre 0 y 3. En el valor inicial `a=1`, el centro pesa `6/6=1` y los otros seis vecinos pesan en total `-6/6=-1`.

`filter2D` conserva la respuesta con signo en `CV_32F`. La matriz es simétrica por rotación de 180°, así que la correlación coincide aquí con la convolución. Se calcula `H = min(255, 12·|K*Y|)` para acercar su magnitud a Sobel. La mezcla `E = (1−w)S + wH` usa `w=0.30` inicialmente. Umbral 40 y mediana convierten la mezcla a una máscara limpia.

Los siete controles son: umbral 40; tamaño Gaussiano 5; sigma 1.73; peso propio 0.30; diagonal 1; escala del vitral 9; y CLAHE 1.5. Los rangos se validan en `Parametros::validar`.

### De mapas reducidos a un vitral

`Efecto::procesar` reduce el BGR original y la máscara con `INTER_AREA`. Para tamaño `W×H` y escala `c`, cada mapa mide aproximadamente `max(1,W/c) × max(1,H/c)`.

`dibujarVitral` separa los vértices aproximadamente `3c` píxeles, desplaza los interiores según sus coordenadas y alterna diagonales. Toma el color local cerca de cada centro. Cada triángulo tiene tres subdivisiones fijas, cuyo contraste se mezcla con el color base según `α = min(1, detalle/56)`. La geometría no depende del video.

Android convierte RGBA a BGR para el núcleo común y vuelve a RGBA para mostrar original y vitral completos. Permite inspeccionar gris, mediana, Gaussiano, gradiente, respuesta propia y máscara. «Portal Vitral» solo sustituye el interior de un rectángulo.

## 3. Escritorio, GPU y mediciones

`main` valida opciones y abre cámara o video. `Vista3D::componer` copia original y vitral en dos regiones `Rect`. `WImage3D` usa esa matriz como textura de un plano, `setImage` la actualiza y `spinOnce` atiende eventos. El plano rota, pero no reconstruye profundidad.

`procesarFrame` cronometra la ruta Mat y UMat. `ocl::finish()` espera a que termine el trabajo OpenCL antes de detener el cronómetro. Muestra filtrado, transferencias y FPS de la ventana; el ciclo visible incluye lectura, dibujo, eventos y renderizado. `benchmark` calienta, rebobina un video fijo y calcula media, mediana y recursos. Es una medición de procesamiento, no la misma cifra que FPS de cámara.

Android ejecuta una ruta por vez, reinicia búferes y ventana al cambiar y conserva una muestra completa por modo. Muestra tiempo de proceso, FPS, PSS, CPU y RAM disponible. El respaldo no cuenta como OpenCL; el dibujo se hace en CPU. PSS es memoria del proceso, no VRAM.

## 4. Qué viene de clase y qué estudiar aparte

Los códigos docentes están bajo `Vision-Computador/Codigos`:

- Unidad 1, Ejemplo 2 (`Unidad-1/Ejemplo-2-Conformación-Imágenes-Escala-Grises-Video-OpenCV/Principal.cpp`): video y conversión a gris. Ejemplo 3 (`Unidad-1/Ejemplo-3-Espacio-Color-HSV-Binarización-por-Umbral-de-Color-OpenCV/Principal.cpp`): HSV, umbrales y trackbars; aquí se reutiliza el patrón de controles, no la segmentación HSV del ejemplo.
- Unidad 2, Ejemplo 5 (`Unidad-2/Ejemplo-5-Filtro-CLAHE-Convolución-Imágenes-OpenCV/Principal.cpp`): CLAHE y procesamiento espacial.
- Unidad 2, Ejemplo 6 (`Unidad-2/Ejemplo-6-Filtro-Gaussiano-Filtro-Sobel-Detección-Bordes-Suavizado-OpenCV/Principal.cpp`): Gaussiano, Sobel y composición de imágenes con `Rect`/`copyTo`.
- Unidad 2, Ejemplo 9 (`Unidad-2/Ejemplo-9-Ruido-Sal-y-Pimienta-OpenCV/Principal.cpp`): filtro mediano para reducir ruido impulsivo.

El kernel y la malla de color son decisiones propias. Para defenderlos, estudia OpenCL/T-API y sus transferencias, JNI y propiedad del Mat de cámara, texturas VTK, `INTER_AREA`, `fillConvexPoly` y `line`. Parte I no incluye FFT, ASL, Telegram ni web aparte.

La guía del curso permite asistencia de LLM, pero exige comprender y citar el código generado o de terceros. Presenta la adaptación de ejemplos del profesor como tal; confirma cualquier procedencia de bloques que no puedas verificar. Para la defensa, explica cada operación y variable con tus palabras.

## 5. Preguntas de práctica

1. **¿Por qué mediana y no promedio?** La mediana reemplaza el centro por el valor central ordenado de una ventana 3×3; suele quitar ruido impulsivo sin difundir un valor extremo a todos los vecinos. Referencia: `Efecto::procesar` en `Principal.hpp`.
2. **¿Qué significa que el kernel sume cero?** En una región uniforme la suma ponderada cancela y la salida es cero. En un borde los vecinos y el centro no son iguales y queda una respuesta. Referencia: `crearKernel` y `detectarContornos`.
3. **¿Qué cambia al aumentar `a` o `peso propio`?** `a` cambia la contribución de las dos diagonales del kernel. El peso controla la mezcla entre la respuesta propia y el promedio de magnitudes Sobel; no cambia Sobel.
4. **¿Cómo aporta Sobel al vitral?** Se combina con el kernel propio para decidir dónde acentuar las piezas pequeñas.
5. **¿Qué mantiene estable la malla?** Los vértices solo dependen de coordenadas, resolución y escala. El fotograma cambia colores y contraste, no las posiciones.
6. **¿Por qué OpenCL puede ser más lento?** Hay transferencias y operaciones que pueden caer a CPU; el dibujo también es CPU. Hay que medir cada ruta.
7. **¿La vista VTK es una reconstrucción 3D?** No. Es una textura dinámica sobre un plano que se puede girar; la cámara no calcula profundidad. Referencia: `Vista3D::componer` y `actualizar`.
8. **¿Por qué cambiar RGBA a BGR en Android?** La cámara/OpenCV entrega cuatro canales, el núcleo compartido espera tres canales BGR y la vista necesita RGBA otra vez. La salida conserva las dimensiones que espera la cámara.

### Ejercicio oral

Explica `crearKernel(1)` sobre una ventana uniforme y después sobre una ventana cuyo centro es más brillante que el fondo. Primero muestra que la suma de pesos es cero; luego calcula la respuesta central antes de `convertScaleAbs`. Termina explicando por qué se toma magnitud, cómo interviene el peso 70/30 y qué hace el umbral.
