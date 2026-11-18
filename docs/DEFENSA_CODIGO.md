# Guía de estudio y sustentación — Parte I

La salida es una imagen ASCII verde sobre fondo oscuro. Todos los caracteres representan distintos niveles de brillo; el kernel propio aporta a la máscara que realza los bordes. La aplicación móvil añade una vista «Portal ASCII».

## 1. Cómo se relacionan las aplicaciones

Escritorio y Android mantienen una copia autónoma del mismo núcleo `Principal.hpp`. La plantilla `Efecto<Imagen>` ejecuta el algoritmo una vez con `cv::Mat` para CPU y otra con `cv::UMat` para la ruta OpenCL. Se conserva esta duplicación entre repositorios para que cada proyecto compile sin depender de una ruta absoluta al otro.

| Parte | Función principal |
|---|---|
| [Principal.hpp](../Principal.hpp) | Parámetros, validación, filtro, reducción a cuadrícula y dibujo ASCII. |
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

`filter2D` conserva la respuesta con signo en `CV_32F`; `convertScaleAbs` obtiene su magnitud `H=|K*Y|`. Aunque `filter2D` aplica correlación, la matriz es simétrica por rotación de 180°, así que aquí da el mismo resultado que una convolución. La combinación usa `E=(1−w)S+wH`, con `w=0.30`: 70% Sobel y 30% filtro propio. `threshold` convierte `E` en una máscara con umbral inicial 40 y `medianBlur` limpia puntos aislados de esa máscara.

Los siete controles son: umbral 40; tamaño Gaussiano 5; sigma 1.73; peso propio 0.30; diagonal `a=1`; celda ASCII 9; contraste CLAHE 1.5. Los rangos se validan en `Parametros::validar`; los decimales de las barras se representan como enteros escalados por 100.

### De imágenes pequeñas a caracteres

`Efecto::procesar` reduce dos mapas con `INTER_AREA`: brillo y máscara de bordes. Si el fotograma mide `W×H` y el tamaño de celda es `c`, la cuadrícula es aproximadamente `max(1,W/c) × max(1,H/(c·3/2))`. El parámetro `celda` inicia en 9 y varía entre 6 y 24. INTER_AREA promedia el área que representa cada celda; no selecciona solo un píxel.

`caracterAscii` normaliza el brillo `L`: `b=clamp((L−9)/204,0,1)`. Elige entre once caracteres con `i=round(10·b^0.77)`. El exponente menor que uno aclara tonos medios. Si la cobertura de borde llega a 25 sobre 255, suma uno al índice sin superar el máximo. `dibujarAscii` prepara fichas con `putText` al doble de tamaño y las reduce para obtener trazos legibles. La ruta UMat solo descarga los dos mapas reducidos; el texto se dibuja en CPU.

En Android la cámara entrega RGBA. `ProcesadorAndroid` convierte `RGBA→BGR` para usar el núcleo común y convierte el resultado `BGR→RGBA` para devolverlo a la cámara. En la comparación, las imágenes completas original y ASCII se encajan sin deformación, en horizontal o vertical según la orientación. Las vistas intermedias permiten inspeccionar gris, mediana, Gaussiano, gradiente, respuesta propia y máscara. «Portal ASCII» deja el video original fuera de un rectángulo central y muestra ASCII dentro; no detecta a una persona ni reconstruye AR.

## 3. Escritorio, GPU y mediciones

`main` valida opciones y parámetros, abre cámara o video y selecciona modo. `visualizar` procesa cada frame y mantiene una sola ventana. `Vista3D::componer` copia original y ASCII en dos regiones `Rect` de una matriz; `WImage3D` usa esa matriz como textura sobre un plano 3D. `actualizar` reemplaza la textura por frame y `spinOnce` atiende eventos. La cámara puede girarse, pero el plano no reconstruye profundidad ni geometría de la escena.

`procesarFrame` cronometra la ruta Mat y UMat. `ocl::finish()` espera a que termine el trabajo OpenCL antes de detener el cronómetro. Muestra filtrado, transferencias y FPS de la ventana; el ciclo visible incluye lectura, dibujo, eventos y renderizado. `benchmark` calienta, rebobina un video fijo y calcula media, mediana y recursos. Es una medición de procesamiento, no la misma cifra que FPS de cámara.

Android ejecuta CPU y GPU en el mismo frame para comparar sus tiempos; el selector elige qué salida mostrar. El tiempo total de cada ruta incluye conversiones y preparación de la vista; el dibujo ASCII se hace en CPU. La app muestra FPS, RAM PSS, CPU y RAM disponible. La medición de recursos es del proceso, no dos procesos separados. `UMat` permite que OpenCV use OpenCL en las operaciones compatibles: no garantiza que toda operación vaya a GPU ni que GPU sea más rápida. Las transferencias, el tamaño del frame, el dispositivo y el dibujo pueden cambiar el resultado. RAM/PSS no es VRAM. No atribuyas una cifra de GPU al teléfono si no se midió en ese dispositivo.

## 4. Qué viene de clase y qué estudiar aparte

Los códigos docentes están bajo `Vision-Computador/Codigos`:

- Unidad 1, Ejemplo 2 (`Unidad-1/Ejemplo-2-Conformación-Imágenes-Escala-Grises-Video-OpenCV/Principal.cpp`): video y conversión a gris. Ejemplo 3 (`Unidad-1/Ejemplo-3-Espacio-Color-HSV-Binarización-por-Umbral-de-Color-OpenCV/Principal.cpp`): HSV, umbrales y trackbars; aquí se reutiliza el patrón de controles, no la segmentación HSV del ejemplo.
- Unidad 2, Ejemplo 5 (`Unidad-2/Ejemplo-5-Filtro-CLAHE-Convolución-Imágenes-OpenCV/Principal.cpp`): CLAHE y procesamiento espacial.
- Unidad 2, Ejemplo 6 (`Unidad-2/Ejemplo-6-Filtro-Gaussiano-Filtro-Sobel-Detección-Bordes-Suavizado-OpenCV/Principal.cpp`): Gaussiano, Sobel y composición de imágenes con `Rect`/`copyTo`.
- Unidad 2, Ejemplo 9 (`Unidad-2/Ejemplo-9-Ruido-Sal-y-Pimienta-OpenCV/Principal.cpp`): filtro mediano para reducir ruido impulsivo.

El kernel artístico y el mapeo de celdas a caracteres son decisiones propias de esta versión. Para defender partes que no aparecen en los ejemplos disponibles, estudia: OpenCL/T-API y transferencias Mat/UMat; JNI y propiedad del `Mat` que presta la cámara; renderizado de textura con VTK `WImage3D`; `resize(INTER_AREA)` y conversión BGR/RGBA; y el uso de `putText`/`copyTo` para dibujar celdas. No se usa FFT, ASL, Telegram ni una app web en esta Parte I.

La guía del curso permite asistencia de LLM, pero exige comprender y citar el código generado o de terceros. Presenta la adaptación de ejemplos del profesor como tal; confirma cualquier procedencia de bloques que no puedas verificar. Para la defensa, explica cada operación y variable con tus palabras.

## 5. Preguntas de práctica

1. **¿Por qué mediana y no promedio?** La mediana reemplaza el centro por el valor central ordenado de una ventana 3×3; suele quitar ruido impulsivo sin difundir un valor extremo a todos los vecinos. Referencia: `Efecto::procesar` en `Principal.hpp`.
2. **¿Qué significa que el kernel sume cero?** En una región uniforme la suma ponderada cancela y la salida es cero. En un borde los vecinos y el centro no son iguales y queda una respuesta. Referencia: `crearKernel` y `detectarContornos`.
3. **¿Qué cambia al aumentar `a` o `peso propio`?** `a` cambia la contribución de las dos diagonales del kernel. El peso controla la mezcla entre la respuesta propia y el promedio de magnitudes Sobel; no cambia Sobel.
4. **¿Cómo aporta Sobel al ASCII?** Sus magnitudes horizontal y vertical se combinan con el kernel propio para crear la máscara. No se dibujan rayas según la dirección del gradiente.
5. **¿Cómo se elige un carácter?** El brillo medio selecciona la densidad. Con suficiente borde se avanza un lugar en la misma escala, sin crear símbolos aparte. Referencias: `caracterAscii` y `dibujarAscii`.
6. **¿Por qué el resultado GPU puede ser más lento?** OpenCL puede requerir subir y bajar datos; algunas operaciones y el dibujo ASCII siguen en CPU. Se sincroniza el trabajo, pero el beneficio depende del equipo y de la carga.
7. **¿La vista VTK es una reconstrucción 3D?** No. Es una textura dinámica sobre un plano que se puede girar; la cámara no calcula profundidad. Referencia: `Vista3D::componer` y `actualizar`.
8. **¿Por qué cambiar RGBA a BGR en Android?** La cámara/OpenCV entrega cuatro canales, el núcleo compartido espera tres canales BGR y la vista necesita RGBA otra vez. La salida conserva las dimensiones que espera la cámara.

### Ejercicio oral

Explica `crearKernel(1)` sobre una ventana uniforme y después sobre una ventana cuyo centro es más brillante que el fondo. Primero muestra que la suma de pesos es cero; luego calcula la respuesta central antes de `convertScaleAbs`. Termina explicando por qué se toma magnitud, cómo interviene el peso 70/30 y qué hace el umbral.
