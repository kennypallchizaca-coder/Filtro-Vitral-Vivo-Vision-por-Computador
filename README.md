# Filtro-Spiderman-Vision-por-Computador

Aplicación de escritorio en C++17 y OpenCV que transforma video de cámara o archivo en un efecto cómic inspirado en *Spider-Man: Un nuevo universo*. Combina colores planos, sombras azul-violeta, luces cálidas, puntos halftone, rayado diagonal, contornos negros, tonos neón y una separación RGB suave. Una sola ventana blanca contiene original y efecto, controles deslizantes y mediciones. La comparación se muestra como textura dinámica en un panel VTK integrado, sin abrir otra ventana.

## Cómo funciona el filtro

Cada fotograma pasa por estas etapas:

| Etapa | Técnica utilizada | Función en el efecto |
|---|---|---|
| Contraste | Conversión BGR → Lab y CLAHE sobre el canal L | Ajusta el contraste local de la luminosidad antes de recuperar la imagen en color. |
| Limpieza | Mediana de 3×3 | Reduce valores aislados que podrían generar puntos o bordes no deseados. |
| Detección de contornos | Escala de grises, Gaussiano, Sobel horizontal/vertical y kernel propio | Suaviza detalles pequeños y calcula una máscara de cambios de intensidad mediante un umbral. |
| Colores planos | Tabla de consulta `LUT` | Reduce las intensidades a un número ajustable de niveles por canal. El valor inicial es ocho niveles por canal. |
| Paleta | Máscaras de luminosidad y mezclas con `addWeighted` | Tiñe las sombras de azul-violeta y aporta calidez a las luces, conservando parte del color original. |
| Halftone | Celdas de 8×8 y puntos de radio variable | Oscurece un 35 % los puntos seleccionados en tonos medios y protege las luces. |
| Rayado | Líneas diagonales repetidas y máscara de sombras | Oscurece un 30 % líneas finas únicamente en zonas oscuras. |
| Tinta | Máscara de contornos aplicada sobre la imagen con puntos | Pinta de negro los píxeles señalados por la máscara de contornos. |
| Neón | Conversión a HSV y ajustes de S/V mediante LUT | Aumenta saturación y brillo; aplica un refuerzo de brillo a los tonos seleccionados de azul, morado y rosa. |
| Glitch RGB | Separación de canales y traslaciones con `warpAffine` | Desplaza azul a la derecha y rojo a la izquierda; conserva el verde sin desplazar. Se aplica al final. |

Los contornos se calculan antes del halftone y del rayado para evitar que sus patrones se conviertan en nuevos bordes. La LUT de color procesa la imagen con mediana. La paleta y las texturas toman el brillo de su versión en gris, antes del Gaussiano. La máscara de tinta se aplica después de esas mezclas para conservar contornos negros.

### Kernel propio

El filtro utiliza una matriz ajustable de 3×3, de tipo laplaciano:

```text
           [ -a    -1       0 ]
K(a) =     [ -1   4+2a     -1 ] / (4+2a)
           [  0    -1      -a ]
```

`a` corresponde al parámetro `diagonal`, inicialmente 1 y ajustable entre 0 y 3. Modifica el peso de dos vecinos diagonales. La suma de los coeficientes es cero: una zona de intensidad constante produce respuesta cero, mientras que los cambios espaciales pueden producir una respuesta distinta de cero.

`filter2D` aplica el kernel sobre la imagen gris con suavizado Gaussiano y conserva su respuesta en `CV_32F`. Después, `convertScaleAbs` obtiene su magnitud y la convierte a ocho bits. Esa respuesta se combina con los bordes de Sobel usando `pesoPropio`, inicialmente 0,3: 70 % Sobel y 30 % kernel propio. Un umbral, inicialmente 40, selecciona dónde pintar la tinta negra.

### Puntos y color

Para cada celda del halftone se toma el brillo `I` de su centro y se calcula:

```text
radio = round((255 - I) * radioMaximo / 255)
```

El radio máximo inicial es 1 píxel. La máscara de puntos se limita a píxeles con gris entre 70 y 180. En ellos se conserva el 65 % del color, equivalente a una tinta negra al 35 % de opacidad. Con radio máximo 0 se desactivan los puntos.

El rayado utiliza diagonales de un píxel de grosor cada ocho píxeles. Su máscara cumple `x mod 8 = y mod 8` y gris entre 0 y 69. En esas posiciones conserva el 70 % del color. Las dos texturas quedan ancladas a las coordenadas de la imagen y se preparan con operaciones compatibles con `Mat` y `UMat`.

La paleta mezcla un 25 % del color BGR `(150,55,95)` en sombras con gris entre 25 y 120. En luces con gris entre 175 y 245 mezcla un 10 % de BGR `(90,195,245)`. Los tonos medios y los extremos quedan fuera de estas dos máscaras. El rayado continúa activo cuando se desactivan los puntos.

Los valores iniciales usan ocho niveles por canal, radio 1, brillo 1,00 y límite CLAHE 1,50. Las técnicas de impresión y rayado toman como referencia el trabajo descrito por el equipo de la película. [Sony Pictures Imageworks](https://dev.imageworks.com/node/1371)

En HSV, la máscara de neón selecciona H entre 90 y 170 y S mayor que 50. Se refuerza V dentro de esa máscara y luego se ajustan la saturación y el brillo general. Las transformaciones conservan el recorte de intensidades a 0..255.

### CPU, OpenCL y visualización

La plantilla `Efecto<Imagen>` contiene una sola secuencia de procesamiento: se instancia con `Mat` para CPU y con `UMat` para la ruta OpenCL. `UMat` permite utilizar la GPU en las operaciones compatibles; algunas pueden ejecutarse en CPU según el soporte disponible.

La ventana usa `Lienzo` con `WINDOW_AUTOSIZE`, como el Ejemplo 7. Original y efecto se copian con `copyTo` a dos regiones `Rect` de un mismo `Mat`, siguiendo los Ejemplos 6 y 7. Las barras se crean con `createTrackbar` y se leen con `getTrackbarPos`, como en el Ejemplo 3.

Ese `Mat` se aplica como textura a un plano `WImage3D`. `setImage` la actualiza en cada vuelta y `spinOnce` atiende eventos sin detener el ciclo de cámara. El visor VTK real se integra como ventana hija de `Lienzo` mediante las API nativas de Windows; no se reemplaza por una captura de pantalla. Este puente no procede de los ejemplos del profesor. No se reconstruye el widget ni se restablece su cámara entre fotogramas. Arrastrar permite rotar la escena y `R` vuelve a encuadrarla. Esc o cerrar `Lienzo` termina toda la aplicación. El plano no reconstruye la profundidad de la escena capturada.

## Compilar y ejecutar

Requisitos: Windows, MSYS2 UCRT64 con su compilador C++, CMake y OpenCV con el módulo `viz`/VTK. Para utilizar GPU también se necesita un controlador OpenCL compatible.

Al clonar el repositorio por primera vez, configura CMake desde PowerShell. Estas rutas corresponden a una instalación de MSYS2 en `C:\msys64`:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
cmake -S . -B build -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe -DOpenCV_DIR=C:/msys64/ucrt64/lib/cmake/opencv5
```

En cada nueva terminal PowerShell, añade las bibliotecas de MSYS2 a la ruta de esa sesión antes de compilar o ejecutar.

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
cmake --build build --parallel 2
.\build\interciclo_vision.exe
```

Después de esa configuración, `build` queda en Release. Cierra la ventana con Esc antes de recompilar. Los archivos de compilación se generan localmente y no forman parte del repositorio.

Sin argumentos se abre la cámara 0 a 640×480 y se comparan CPU y OpenCL si se encuentra la GPU configurada; si no, se continúa en CPU. La selección predeterminada es `NVIDIA:GPU:RTX 4070`, modificable con `--device`. `--mode=gpu` exige una GPU compatible; `--mode=cpu` omite su selección. Todas las opciones aparecen con `.\build\interciclo_vision.exe --help`.

## Transformación de realidad a cómic

La barra **Transformacion %** revela el efecto de izquierda a derecha en el panel procesado: **0** muestra la imagen real, **50** mitad cómic y mitad real, y **100** el efecto completo. Al iniciar está en 100. El panel original permanece intacto; las etapas intermedias tampoco cambian. Se calcula siempre el filtro completo: el barrido no reduce artificialmente la carga ni los tiempos del filtro. `aplicarBarrido` usa `Rect` y `copyTo`, como los lienzos de clase.

El acabado aplica mediana a la máscara de tinta, limita los puntos a luminosidad 70–155 y oscurece esos puntos solo un 20 %. El rayado actúa entre 25–69. El desplazamiento RGB se copia únicamente cerca de los contornos; fuera de esa máscara conserva los colores. Son ajustes del mismo pipeline, no detección facial ni realidad aumentada.

## Controles

- Sliders: ajustar los parámetros del efecto con el ratón.
- Espacio: CPU Mat, OpenCL UMat y comparación de ambos sobre el mismo fotograma.
- `T`: efecto final, gris, Gaussiano, respuesta del kernel propio y máscara de contornos.
- Arrastrar sobre las imágenes: girar la comparación 3D. `R`: restaurar su encuadre.
- `Esc` o `q` minúscula: salir.

Los controles incluyen umbral, tamaño y sigma de Gaussiano, peso del filtro propio, diagonal del kernel, radio de puntos, niveles de color, saturación, brillo, neón, desplazamiento RGB y CLAHE. El tamaño Gaussiano siempre es impar entre 3 y 51.

La barra `Kernel` muestra el tamaño del Gaussiano entre 3 y 51. Cuando se selecciona un número par, la barra avanza al impar vecino en la dirección del ajuste. En las barras `x100`, 125 corresponde a 1,25. `Peso propio %` muestra el porcentaje aplicado al filtro propio.

Los resultados se muestran en vivo; no se guardan mediciones ni se exporta CSV. Para alternar CPU/OpenCL/comparación con Espacio, inicia con `--mode=both` (predeterminado) y OpenCL disponible.

## Archivos del proyecto

| Archivo | Responsabilidad |
|---|---|
| `Principal.cpp` | Cámara/video, comparación, selección OpenCL, tiempos en vivo y vista VTK. |
| `Principal.hpp` | Parámetros y el mismo algoritmo compilado para Mat y UMat. |
| `Vista3D.hpp` | Composición de ambas imágenes y visor VTK integrado en Lienzo, sin reiniciar su cámara. |
| `Interfaz.hpp` | Crear y leer trackbars, convertir sus valores a parámetros y gestionar las teclas. |
| `Mediciones.hpp/.cpp` | RAM del proceso y contadores CPU/GPU de Windows. |

La base de procesamiento adapta los ejemplos de clase 3 (HSV/Lab), 5 (CLAHE y mediana), 6 (Gaussiano y Sobel) y 9 (mediana frente a ruido impulsivo) de `Vision-Computador/Codigos`. El bloque Gaussiano → Sobel → `convertScaleAbs` → `addWeighted` conserva la secuencia del Ejemplo 6. OpenCL, VTK, el kernel ajustable y las mediciones amplían esa base para el efecto cómic.

La integración con la versión Android de la Parte I se describe en [docs/ANDROID.md](docs/ANDROID.md).

## Mediciones en pantalla

La interfaz muestra tiempos CPU y OpenCL, transferencias GPU, FPS del ciclo visible, RAM del proceso y muestras de CPU/GPU de Windows. La opción de comparación procesa el mismo fotograma con los mismos parámetros.

- Los tiempos OpenCL esperan `ocl::finish()`, no solo el envío del trabajo.
- RAM es el Working Set del proceso, no VRAM. La comparación conserva búferes de ambas rutas.
- El contador GPU del proceso incluye VTK, incluso si el filtro usa CPU; no aísla cómputo OpenCL.
- Una raya indica una ruta no ejecutada o una muestra aún no disponible.
- El FPS de ventana incluye captura, procesamiento y presentación; no es lo mismo que la capacidad del filtro.

`--benchmark --source=entrada.avi --mode=cpu` sigue permitiendo una comprobación sin ventanas, pero imprime solo un resumen en consola. `--output` se reserva para una captura PNG de la prueba `--smoke`; no se usa para guardar mediciones.

## Informe y comprobaciones de Parte I

- [Informe con esquema, matemática, gráficas y referencias](docs/INFORME_PARTE_I.html). Se abre en un navegador y se puede imprimir a PDF.
- [Datos históricos y metadatos del informe](docs/mediciones/metadatos.json): seis casos CPU/OpenCL, 120 muestras por caso y kernels 5, 31 y 51 sobre el mismo clip sintético. No son mediciones del teléfono ni de la cámara en vivo.
- [Guion del videoblog en inglés](docs/GUION_VIDEO_EN.md). La grabación la realizará el usuario; el guion no sustituye al video.
- `tests/prueba_vista3d.cpp` verifica texturas dinámicas, pose, reapertura, integración en una sola ventana, posición del panel, trackbars y cierre del padre mediante renderizado real. Se habilita con `-DBUILD_TESTING=ON` al configurar CMake y se ejecuta con `ctest --test-dir build --output-on-failure`; no es necesario para abrir la aplicación.

En los benchmarks sin ventanas, PDH devolvió 0 % de motor GPU incluso bajo carga; en vivo sí registró actividad, pero incluye el renderizado VTK. No lo interpretes como una medición aislada del filtro: `nvidia-smi --query-gpu=utilization.gpu,memory.used,power.draw --format=csv -l 1` permite observar la GPU completa mientras corre OpenCL. Se registró hasta 23 % en la prueba documentada; ese dato es global del dispositivo, no exclusivo del filtro.
