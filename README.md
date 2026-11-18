# Video ASCII en tiempo real

La aplicación de escritorio convierte video de cámara o archivo en un retrato de caracteres. Original y ASCII se ven juntos en una sola ventana con controles, mediciones en vivo y una vista VTK interactiva.

## Cómo funciona el filtro

1. Convertir el fotograma a gris y aplicar **mediana 3×3** y **CLAHE** para reducir ruido y mejorar el contraste.
2. Pasar un **Gaussiano** y usar **Sobel** para obtener la intensidad y la dirección de los bordes.
3. Combinar Sobel con el kernel propio para decidir cuáles cambios se consideran bordes.
4. Reducir la imagen a una cuadrícula: cada celda guarda el brillo promedio, la cantidad de borde y la dirección del gradiente.
5. Dibujar un carácter por celda con OpenCV. El brillo bajo se ve como un espacio; el alto, como un símbolo denso, por ejemplo # o @. Donde al menos cerca del 10 % de la celda contiene borde, usar |, -, / o \ según su orientación.

### Kernel propio

El filtro utiliza una matriz ajustable de 3×3, de tipo laplaciano:

```text
           [ -a    -1       0 ]
K(a) =     [ -1   4+2a     -1 ] / (4+2a)
           [  0    -1      -a ]
```

`a` corresponde al parámetro `diagonal`, inicialmente 1 y ajustable entre 0 y 3. Modifica el peso de dos vecinos diagonales. La suma de los coeficientes es cero: una zona de intensidad constante produce respuesta cero, mientras que los cambios espaciales pueden producir una respuesta distinta de cero.

`filter2D` aplica el kernel sobre la imagen gris con suavizado Gaussiano y conserva su respuesta en `CV_32F`. Después, `convertScaleAbs` obtiene su magnitud y la convierte a ocho bits. Esa respuesta se combina con los bordes de Sobel usando `pesoPropio`, inicialmente 0,3: 70 % Sobel y 30 % kernel propio. Un umbral, inicialmente 40, selecciona qué píxeles cuentan como borde al escoger un carácter.

El control **Celda ASCII** ajusta cuántos caracteres caben. Cada celda mide inicialmente 9 píxeles de ancho por 13 de alto. El dibujo reutiliza una ficha para cada símbolo; no vuelve a llamar a putText para cada casilla.

### CPU, OpenCL y visualización

La plantilla `Efecto<Imagen>` contiene una sola secuencia de procesamiento: se instancia con `Mat` para CPU y con `UMat` para la ruta OpenCL. `UMat` permite utilizar la GPU en las operaciones compatibles; algunas pueden ejecutarse en CPU según el soporte disponible. En la vista ASCII solo se descargan las cuatro matrices de celdas. Las dos rutas dibujan los caracteres en CPU; el benchmark incluye ese dibujo en el tiempo total, no en el tiempo de filtrado.

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

## Controles

- Sliders: ajustar los parámetros del efecto con el ratón.
- Espacio: CPU Mat, OpenCL UMat y comparación de ambos sobre el mismo fotograma.
- `T`: efecto final, gris, Gaussiano, respuesta del kernel propio y máscara de contornos.
- Arrastrar sobre las imágenes: girar la comparación 3D. `R`: restaurar su encuadre.
- `Esc` o `q` minúscula: salir.

Los siete controles son el umbral, el tamaño y sigma del Gaussiano, el peso propio, la diagonal del kernel, el ancho de celda ASCII (6–24 píxeles) y el contraste CLAHE. El tamaño Gaussiano siempre es impar entre 3 y 51.

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

### Código adaptado de clase

Los ejemplos están en `Vision-Computador/Codigos`. Se reutilizan las operaciones que encajan en el proyecto, sin importar los programas completos:

| Ejemplo | Adaptación |
|---|---|
| 2, Unidad 1 | Captura de video y conversión a gris; aquí se valida cada fotograma. |
| 3, Unidad 1 | Creación de barras y lectura de sus posiciones. |
| 5 y 9, Unidad 2 | CLAHE y reducción de ruido con mediana. |
| 6, Unidad 2, `Principal.cpp` líneas 65–82 | `detectarContornos` agrupa Gaussiano → Sobel → `convertScaleAbs` → `addWeighted`, con tamaño ajustable. A continuación combina la respuesta del kernel propio. |
| 6 y 7, Unidad 2 | Lienzo dividido en regiones `Rect` y compuesto con `copyTo`. |

La conversión a caracteres, las rutas CPU/OpenCL, la integración VTK y las mediciones en vivo son parte de la adaptación del proyecto. No se incorporó la exportación de histogramas a CSV del Ejemplo 5.

La integración con la versión Android de la Parte I se describe en [docs/ANDROID.md](docs/ANDROID.md).

## Mediciones en pantalla

La interfaz muestra tiempos CPU y OpenCL, transferencias GPU, FPS del ciclo visible, RAM del proceso y muestras de CPU/GPU de Windows. La opción de comparación procesa el mismo fotograma con los mismos parámetros.

- Los tiempos OpenCL esperan `ocl::finish()`, no solo el envío del trabajo.
- RAM es el Working Set del proceso, no VRAM. La comparación conserva búferes de ambas rutas.
- El contador GPU del proceso incluye VTK, incluso si el filtro usa CPU; no aísla cómputo OpenCL.
- Una raya indica una ruta no ejecutada o una muestra aún no disponible.
- El FPS de ventana incluye captura, procesamiento y presentación; no es lo mismo que la capacidad del filtro.

`--benchmark --source=entrada.avi --mode=cpu` permite medir un video existente sin ventanas. El resumen de tiempo, FPS y RAM se muestra en consola, sin crear archivos. Usa el mismo video con `--mode=gpu` para comparar ambos procesos.

## Informe y comprobaciones de Parte I

- [Estado, resultados ASCII y declaración de IA para la plantilla](docs/ENTREGA_PARTE_I.md).
- [Datos históricos y metadatos del informe](docs/mediciones/metadatos.json): seis casos CPU/OpenCL, 120 muestras por caso y kernels 5, 31 y 51 sobre el mismo clip sintético. No son mediciones del teléfono ni de la cámara en vivo.
- [Guion del videoblog en inglés](docs/GUION_VIDEO_EN.md). La grabación la realizará el usuario; el guion no sustituye al video.
- `tests/prueba_vista3d.cpp` verifica texturas dinámicas, pose, reapertura, integración en una sola ventana, posición del panel, trackbars y cierre del padre mediante renderizado real. Se habilita con `-DBUILD_TESTING=ON` al configurar CMake y se ejecuta con `ctest --test-dir build --output-on-failure`; no es necesario para abrir la aplicación.
