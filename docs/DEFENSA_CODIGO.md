# Guía de estudio y sustentación — Parte I

Vitral Vivo convierte el video en piezas triangulares de color con uniones oscuras. El kernel propio contribuye a la máscara de detalle. Android conserva la vista «Portal Vitral».

## Cómo estudiar esta guía

Sigue un solo fotograma. Primero responde **qué entra**, luego **qué se calcula**, y al final **qué se muestra**. Lee `Efecto::procesar` antes de `dibujarVitral`. Cuando entiendas el núcleo, revisa quién lo llama en escritorio y en Android. Deja los detalles de renderizado y de hilos para una segunda lectura.

Si tienes el editor abierto, ve al [recorrido por líneas del código](#7-recorrido-por-líneas-del-código). Los números corresponden al código fuente, no a este documento.

Para explicarlo en 30 segundos: «Del fotograma saco dos cosas: su color original y un mapa que indica dónde hay detalle. El color pinta los triángulos y los bordes hacen más visibles las subdivisiones. Las posiciones de los triángulos permanecen fijas, así que el vitral no salta entre fotogramas».

## 1. Cómo se relacionan las aplicaciones

Escritorio y Android mantienen una copia autónoma del mismo núcleo `Principal.hpp`. La plantilla C++ `Efecto<Imagen>` permite usar el mismo algoritmo con `cv::Mat` en CPU o con `cv::UMat` en la ruta OpenCL. Una plantilla es código que se especializa para un tipo; no son dos filtros distintos. Se conserva esta duplicación entre repositorios para que cada proyecto compile sin depender de una ruta absoluta al otro.

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

```text
Fotograma BGR ─┬─ reducción por área ───────────────────→ mapa de color ────┐
              └─ gris → mediana → CLAHE → Gaussiano                        │
                        → Sobel + kernel propio → umbral → mapa de bordes ─┤
                                                                              ↓
                                                  dibujarVitral → imagen BGR
```

El color no se reconstruye desde el gris. La rama gris decide dónde hay detalle; la otra conserva los colores del fotograma para pintar. En escritorio el resultado va a VTK. En Android se convierte a RGBA antes de devolverlo a la vista de cámara.

| Función | Recibe | Produce o cambia |
|---|---|---|
| `Parametros::validar` | Estado de los siete controles | Rechaza valores fuera de rango, incluido un Gaussiano de tamaño par. |
| `Efecto::procesar` | Fotograma BGR no vacío de tipo `CV_8UC3` y parámetros | Guarda etapas intermedias y prepara ambos mapas reducidos. No altera la entrada. |
| `Efecto::cuadricula` | Los mapas ya calculados | Devuelve color, bordes y escala en una estructura `CuadriculaVitral` de matrices `Mat`. |
| `dibujarVitral` | Cuadrícula reducida y tamaño solicitado | Devuelve una imagen BGR a ese tamaño, con piezas de color y uniones oscuras. |

`CV_8UC3` significa tres canales de ocho bits sin signo. En BGR el canal 0 es azul, el 1 verde y el 2 rojo. Una máscara de bordes tiene un canal; sus valores antes de reducirla son 0 o 255.

En ambos sistemas el núcleo recibe una imagen BGR. `Efecto::procesar` aplica estos pasos en orden:

1. `cvtColor` calcula una luminancia aproximada: `Y ≈ 0.114 B + 0.587 G + 0.299 R`. Ejemplo: para BGR = (20, 100, 200), se obtiene aproximadamente 121. El verde aporta más por la ponderación, no porque se haya borrado el color original.
2. `medianBlur` usa una ventana 3×3: ordena los nueve valores y conserva el central. Por ejemplo, si los nueve valores ordenados son 98, 99, 100, 100, 100, 101, 101, 102 y 255, queda 100. El 255 aislado no arrastra el resultado como haría con el promedio.
3. `CLAHE` ajusta contraste localmente por regiones y limita el refuerzo de contraste para no amplificarlo sin control. El código usa el parámetro `contraste`; no hace falta memorizar una fórmula interna de OpenCV.
4. `GaussianBlur` suaviza la imagen antes de medir bordes. Cada vecino recibe un peso Gaussiano discreto proporcional a `exp(-(x²+y²)/(2σ²))`; OpenCV normaliza los pesos para que sumen uno. El kernel inicial es 5×5 y `sigma` inicial es 1.73.

### Bordes y filtro propio

`detectarContornos` calcula las derivadas aproximadas `Gx` y `Gy` con Sobel en tipo con signo `CV_16S`. Al omitir `ksize`, OpenCV usa el tamaño por defecto 3. `convertScaleAbs` toma el valor absoluto y lo limita a ocho bits. `addWeighted` promedia esas magnitudes ya limitadas: `S≈(min(255,|Gx|)+min(255,|Gy|))/2`. No calcula la magnitud euclidiana `sqrt(Gx²+Gy²)`. `Gx` detecta cambios al desplazarse a izquierda/derecha, por ejemplo un límite vertical; `Gy` detecta cambios al desplazarse arriba/abajo.

El kernel propio se define en `crearKernel(a)`:

```text
            [ -a     -1       0 ]
K(a) = 1/(4+2a) * [ -1   4+2a     -1 ]
            [  0     -1      -a ]
```

La suma de los nueve coeficientes es cero. Por eso, una región constante `c` produce `c·sum(K)=0`; una diferencia entre el centro y sus vecinos produce respuesta. `a` regula dos vecinos diagonales, entre 0 y 3. En el valor inicial `a=1`, el centro pesa `6/6=1` y los otros seis vecinos pesan en total `-6/6=-1`.

`filter2D` conserva la respuesta con signo en `CV_32F`. La matriz es simétrica por rotación de 180°, así que la correlación coincide aquí con la convolución. Se calcula `H = min(255, 12·|K*Y|)` para acercar su magnitud a Sobel. La mezcla `E = (1−w)S + wH` usa `w=0.30` inicialmente. Umbral 40 y mediana convierten la mezcla a una máscara limpia.

### Ejemplo de cálculo a mano

Estos valores son un ejercicio, no una medición de la app:

1. Con `a=1`, toma un vecindario del gris suavizado cuyo centro vale 112 y cuyos vecinos valen 100. La respuesta es `(6×112 − 6×100) / 6 = 12`. Si el centro también valiera 100, daría cero.
2. `convertScaleAbs` toma la magnitud y aplica la ganancia: `H = 12×12 = 144`. Si el resultado superara 255, se limitaría a 255. Esa ganancia fija ajusta la escala relativa del kernel; no es parte del cálculo de Sobel.
3. Supón que Sobel dio `S=20`. Con `w=0.30`, la mezcla es `0.70×20 + 0.30×144 = 57.2`, aproximadamente 57 al guardar a ocho bits.
4. Con umbral 40, 57 pasa a 255 porque es mayor que 40. Con `w=0`, quedaría 20 y no pasaría. Una última mediana de la máscara puede eliminar un punto de borde aislado.

### Qué cambia con cada control

| Control (inicio) | Qué mirar al moverlo |
|---|---|
| Umbral (40) | Un valor mayor exige más respuesta para marcar un borde. Ensáyalo en «Máscara de bordes». |
| Tamaño Gaussiano (5) | Cambia la ventana de convolución: 5 significa 5×5. Solo admite impares de 3 a 51 y permite variar la carga de cálculo. |
| Sigma (1.73) | Cambia cómo se reparten los pesos del Gaussiano dentro de esa ventana. No es el tamaño de las piezas. |
| Peso propio (0.30) | Con 0 queda Sobel; con 1 queda el kernel propio. |
| Diagonal (1) | Cambia la importancia de dos vecinos diagonales dentro del kernel 3×3. |
| Escala del vitral (9) | Cambia el tamaño de las piezas. El lado objetivo de cada bloque es tres veces la escala: inicialmente, unos 27 píxeles. |
| Contraste CLAHE (1.5) | Cambia el límite con el que se ajusta el contraste local antes de detectar bordes. |

### De mapas reducidos a un vitral

`Efecto::procesar` reduce el BGR original y la máscara con `INTER_AREA`. Para tamaño `W×H` y escala `c`, cada mapa mide aproximadamente `max(1,W/c) × max(1,H/c)`.

Aquí hay dos grillas distintas. Los **mapas reducidos** almacenan datos. La **malla** describe la geometría que se dibuja. Por ejemplo, en 640×480 con escala `c=9`, los mapas miden 71×53 por división entera, mientras que la malla distribuye aproximadamente 24×18 bloques de lado objetivo 27. No se dibuja un triángulo por cada celda del mapa.

1. `dibujarVitral` genera puntos; desplaza los interiores con un cálculo basado en sus coordenadas. Dos puntos con la misma fila y columna y los mismos parámetros tienen el mismo desplazamiento en todos los fotogramas.
2. Cada bloque se divide en dos triángulos alternando la diagonal. Para pintar uno, el código calcula el centro promediando las coordenadas de sus tres vértices y busca allí el color en el mapa reducido.
3. El centro del triángulo genera tres subdivisiones fijas. La intensidad `α = min(1, detalle/56)` controla su contraste. Si `detalle=28`, se mezcla mitad color base y mitad color local. Si vale 0, no se ven subdivisiones internas.
4. `fillConvexPoly` rellena las caras y `line` dibuja las uniones. La cámara cambia los colores y el detalle, pero no las posiciones. La malla no sigue objetos ni calcula profundidad.

Android convierte RGBA a BGR para el núcleo común y vuelve a RGBA para mostrar original y vitral completos. Permite inspeccionar gris, mediana, Gaussiano, gradiente, respuesta propia y máscara. «Portal Vitral» solo sustituye el interior de un rectángulo.

## 3. Escritorio: del video a VTK

En [Principal.cpp](../Principal.cpp), `main` valida las opciones y abre una cámara si `--source` es un número, o un archivo si es una ruta. `leerFrame` devuelve la imagen en el tamaño configurado; si el video termina en la vista en vivo, el bucle termina. `visualizar` lee los controles en cada vuelta, llama a `procesarFrame`, dibuja el panel y actualiza VTK.

| Modo | Qué se calcula sobre el fotograma | Qué resultado usa la vista |
|---|---|---|
| CPU (`Mat`) | Solo la ruta CPU | El de CPU. |
| OpenCL (`UMat`) | Solo la ruta OpenCL | El de OpenCL. |
| `both` en escritorio | Primero CPU y luego OpenCL, sobre el mismo fotograma | El de OpenCL; los dos tiempos aparecen en el panel. |

**Izquierda y derecha no significan CPU y GPU:** son el original y la salida del modo elegido. Si la GPU no está disponible, la vista en modo `both` puede continuar en CPU; solicitar explícitamente `--mode=gpu` da un error en vez de hacerse pasar por GPU. Con **Espacio** se cambia de modo, **T** recorre vistas, **R** restaura el plano y **Esc** cierra. Ver un intermedio no evita que `Efecto::procesar` calcule la cadena; cambia la imagen que se presenta.

En [Vista3D.hpp](../Vista3D.hpp), `componer` copia las dos imágenes en áreas `Rect` de un mismo `Mat`. `WImage3D` usa ese resultado como textura sobre un plano. Cuatro `WCube` forman el marco sólido. `actualizar` cambia la textura con `setImage` y atiende eventos con `spinOnce`, sin recrear el marco ni mover la cámara en cada vuelta. El plano está en un visor 3D, pero la imagen no contiene profundidad reconstruida.

### Qué mide cada pantalla

- **CPU:** se cronometra el filtro y, por separado, sacar la cuadrícula y dibujar el vitral. **OpenCL:** se separan subida, filtro, bajada y dibujo en CPU. Para el vitral, la descarga usa los mapas reducidos; una etapa intermedia puede descargar su imagen entera. `ocl::finish()` se espera antes de cerrar los cronómetros de la cola: si no, se podría medir solo el tiempo de enviar el trabajo.
- **FPS de la vista:** usa la duración del ciclo, que incluye lectura, procesamiento, dibujo, eventos y renderizado, y suaviza el valor entre fotogramas. **FPS de proceso del benchmark:** `1000 / tiempo total medio en ms`. Por ejemplo, 4 ms equivalen a 250 FPS de proceso; eso no quiere decir que la cámara entregue 250 FPS.
- **Benchmark:** recibe un archivo fijo y un modo separado por ejecución, no una cámara ni `both`. Calienta al menos 15 fotogramas y 1,1 segundos, vuelve al inicio y mide las muestras. Reporta media y mediana del filtrado, promedio del total y RAM máxima observada. El total de procesamiento no incluye lectura de video ni VTK.
- **Memoria y actividad:** [Mediciones.cpp](../Mediciones.cpp) lee el *Working Set* de Windows en MiB y el uso de CPU del proceso. El porcentaje GPU muestra su motor más ocupado; puede incluir VTK. No es un contador de VRAM ni una medida aislada de OpenCL.

`UMat` permite que OpenCV use OpenCL donde hay soporte, pero no convierte automáticamente todo el programa en código GPU. En particular, el dibujo de la malla se hace en CPU.

## 4. Android: cámara, JNI y muestras de rendimiento

En el repositorio Android, los archivos Kotlin están en `app/src/main/java/com/example/interciclovision` y los nativos en `app/src/main/cpp`.

1. **`MainActivity.kt`:** pide permiso y abre la cámara con `JavaCamera2View`. `onCameraFrame` recibe un `Mat` RGBA prestado que no debe liberar ni alterar. La actividad sí crea su propio `Mat` de salida. Lee una instantánea del modo, vista y parámetros para que un cambio en pantalla no mezcle dos configuraciones dentro del mismo fotograma.
2. **`native-lib.cpp`:** JNI conecta Kotlin con C++. Se pasan direcciones de los `Mat` y un arreglo de siete enteros, no una copia píxel por píxel a Java. JNI valida direcciones no nulas y longitud, serializa procesamiento/reinicio y traduce excepciones de C++ para Kotlin. `parametrosDesdeControles` reconstruye los decimales: por ejemplo, 173 se convierte en sigma 1.73.
3. **`ProcesadorAndroid.hpp`:** valida RGBA, vistas y que entrada y salida sean distintas. Convierte a BGR, ejecuta el núcleo de CPU u OpenCL y devuelve RGBA del mismo tamaño. En CPU el filtro usa `Mat`; para OpenCL comprueba un dispositivo GPU y usa `UMat`. Si OpenCL no está disponible, el resultado es de CPU y se etiqueta como respaldo, no como una medición de GPU.
4. **Vistas:** «Comparación completa» pone el original a la izquierda y el vitral a la derecha en ambas orientaciones. `encajar` usa el menor entre los factores de ancho y alto para conservar proporciones. Ejemplo: en una entrada vertical de 480×640, cada panel tiene 240×640; cada imagen se reduce a 240×320 y queda centrada. El espacio sobrante no es un recorte. «Gris» muestra el gris después de CLAHE y «Sobel combinado» muestra la mezcla con el kernel propio, no `Gx` o `Gy` solos. «Gaussiano» y «Máscara de bordes» permiten mostrar otras etapas. «Portal Vitral» aplica el efecto dentro de un rectángulo.
5. **`Mediciones.kt`:** usa ventanas de al menos un segundo. El primer fotograma tras reiniciar fija una referencia; los siguientes alimentan tiempo promedio, FPS procesados y CPU del proceso. Al cerrar la ventana, Kotlin suma memoria PSS y RAM disponible. PSS cuenta la memoria atribuible al proceso; no es VRAM.

Android pasa `comparar=false`: corre un modo por fotograma, no ambos a la vez. Conserva la última muestra completa de cada modo. Cambiar de CPU a GPU reinicia la ventana y los búferes, pero no debe borrar la muestra anterior. Cambiar parámetros, vista o reiniciar de verdad la cámara invalida las muestras para no comparar configuraciones diferentes. Por eso los textos de métricas se colocaron en el panel de controles: cambiar su altura ya no redimensiona el visor de la cámara.

**Si te preguntan qué devuelve JNI:** el arreglo trae, en orden, tiempo CPU, tiempo OpenCL, subida, filtrado GPU, bajada y una bandera de salida GPU (1 si sí, 0 si no). Kotlin muestra el total del modo activo. Ese total incluye preparación y dibujo de la vista, pero excluye captura de cámara, renderizado Android y la copia final a la matriz devuelta a JNI. No lo llames «tiempo puro de GPU».

## 5. Qué viene de clase y qué estudiar aparte

Los códigos docentes están bajo `Vision-Computador/Codigos`:

- Unidad 1, Ejemplo 2 (`Unidad-1/Ejemplo-2-Conformación-Imágenes-Escala-Grises-Video-OpenCV/Principal.cpp`): video y conversión a gris. Ejemplo 3 (`Unidad-1/Ejemplo-3-Espacio-Color-HSV-Binarización-por-Umbral-de-Color-OpenCV/Principal.cpp`): HSV, umbrales y trackbars; aquí se reutiliza el patrón de controles, no la segmentación HSV del ejemplo.
- Unidad 2, Ejemplo 5 (`Unidad-2/Ejemplo-5-Filtro-CLAHE-Convolución-Imágenes-OpenCV/Principal.cpp`): CLAHE y procesamiento espacial.
- Unidad 2, Ejemplo 6 (`Unidad-2/Ejemplo-6-Filtro-Gaussiano-Filtro-Sobel-Detección-Bordes-Suavizado-OpenCV/Principal.cpp`): Gaussiano, Sobel y composición de imágenes con `Rect`/`copyTo`.
- Unidad 2, Ejemplo 9 (`Unidad-2/Ejemplo-9-Ruido-Sal-y-Pimienta-OpenCV/Principal.cpp`): filtro mediano para reducir ruido impulsivo.

El kernel y la malla de color son decisiones propias. Para defenderlos, estudia OpenCL/T-API y sus transferencias, JNI y propiedad del Mat de cámara, texturas VTK, `INTER_AREA`, `fillConvexPoly` y `line`. Parte I no incluye FFT, ASL, Telegram ni web aparte.

La guía del curso permite asistencia de LLM, pero exige comprender y citar el código generado o de terceros. Presenta la adaptación de ejemplos del profesor como tal; confirma cualquier procedencia de bloques que no puedas verificar. Para la defensa, explica cada operación y variable con tus palabras.

## 6. Preguntas de práctica

1. **¿Por qué mediana y no promedio?** La mediana reemplaza el centro por el valor central ordenado de una ventana 3×3; suele quitar ruido impulsivo sin difundir un valor extremo a todos los vecinos. Referencia: `Efecto::procesar` en `Principal.hpp`.
2. **¿Qué significa que el kernel sume cero?** En una región uniforme la suma ponderada cancela y la salida es cero. En un borde los vecinos y el centro no son iguales y queda una respuesta. Referencia: `crearKernel` y `detectarContornos`.
3. **¿Qué cambia al aumentar `a` o `peso propio`?** `a` cambia la contribución de las dos diagonales del kernel. El peso controla la mezcla entre la respuesta propia y el promedio de magnitudes Sobel; no cambia Sobel.
4. **¿Cómo aporta Sobel al vitral?** Se combina con el kernel propio para decidir dónde acentuar las piezas pequeñas.
5. **¿Qué mantiene estable la malla?** Los vértices solo dependen de coordenadas, resolución y escala. El fotograma cambia colores y contraste, no las posiciones.
6. **¿Por qué OpenCL puede ser más lento?** Hay transferencias y operaciones que pueden caer a CPU; el dibujo también es CPU. Hay que medir cada ruta.
7. **¿La vista VTK es una reconstrucción 3D?** No. Es una textura dinámica sobre un plano que se puede girar; la cámara no calcula profundidad. Referencia: `Vista3D::componer` y `actualizar`.
8. **¿Por qué cambiar RGBA a BGR en Android?** La cámara/OpenCV entrega cuatro canales, el núcleo compartido espera tres canales BGR y la vista necesita RGBA otra vez. La salida conserva las dimensiones que espera la cámara.

### Practica con el programa abierto

1. Sin leer la respuesta, explica por qué da cero `crearKernel(1)` en una zona uniforme. Luego repite el cálculo de la sección 2 con centro 112 y predice qué pasa si bajas el peso a 0.
2. Selecciona «Gaussiano» y después «Máscara de bordes». Explica qué sale de cada paso. Vuelve al vitral: sube la **escala** para ver piezas más grandes, y mueve el **umbral** para cambiar el detalle sin mover la malla.
3. En la vitrina, muestra original y vitral, gira el plano y pulsa **R**. Explica por qué el marco tiene volumen y el video sigue siendo una textura.
4. En el S25, espera una muestra CPU y luego selecciona OpenCL sin cambiar escena, vista ni controles. Explica por qué dos filas visibles no significan dos rutas simultáneas. Si aparece respaldo CPU, di qué pasó.

Si puedes explicar esos cuatro ejercicios sin leer el código línea por línea, ya tienes el recorrido para el [video en inglés](GUION_VIDEO_EN.md). Usa el [borrador del informe](ENTREGA_PARTE_I.md) para distinguir los resultados medidos de los ejemplos de aprendizaje.

## 7. Recorrido por líneas del código

La explicación detallada de escritorio está en [CODIGO_ESCRITORIO.md](CODIGO_ESCRITORIO.md). Sigue el código en este orden:

1. [Principal.cpp](CODIGO_ESCRITORIO.md#1-principalcpp): cámara, ciclo de video, CPU/OpenCL y benchmark.
2. [Principal.hpp](CODIGO_ESCRITORIO.md#2-principalhpp): parámetros, kernel, máscara y dibujo de triángulos.
3. [Interfaz.hpp](CODIGO_ESCRITORIO.md#3-interfazhpp): cómo los deslizadores y el teclado cambian el estado.
4. [Vista3D.hpp](CODIGO_ESCRITORIO.md#4-vista3dhpp): cómo se pegan las imágenes, dónde se crea el marco, cómo se integra el visor y qué hace `R`.
5. [Mediciones](CODIGO_ESCRITORIO.md#5-medicioneshpp-y-medicionescpp) y [CMake](CODIGO_ESCRITORIO.md#6-cmakeliststxt): recursos de Windows, compilación y pruebas.
