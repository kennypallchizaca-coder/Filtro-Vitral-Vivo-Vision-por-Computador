# Explicación del código de escritorio

## Archivos y responsabilidades

| Archivo | Responsabilidad |
|---|---|
| Principal.hpp | Parámetros, validación y algoritmo Spiderman compartido por Mat y UMat. |
| Principal.cpp | Fuente de video, selección OpenCL, tiempos, interfaz y ejecución principal. |
| Interfaz.hpp | Doce trackbars nativos, lectura de valores y teclado. |
| Mediciones.hpp / Mediciones.cpp | RAM y porcentajes CPU/GPU del proceso Windows. |
| Vista3D.hpp | Comparación como textura de un plano VTK integrado en Lienzo. |

No hay registro de muestras ni exportación CSV. Las cifras de la demostración se muestran en pantalla. El informe conserva datos históricos de pruebas anteriores, no archivos generados por la versión actual.

## 1. Capturar y procesar

`abrirFuente` abre una cámara numérica o un archivo. Intenta DirectShow y después el backend disponible. `leerFrame` comprueba la lectura y ajusta la resolución cuando hace falta. En visualización, un archivo termina normalmente al llegar a su final; una cámara que deja de entregar imágenes produce un error. No se procesa un fotograma vacío.

Cada vuelta lee los controles, toma los parámetros del fotograma, procesa y muestra el resultado. `waitKey` atiende HighGUI y `spinOnce` atiende VTK. No se abre un segundo bucle bloqueante para inspeccionar la escena.

## 2. Efecto Spiderman

1. **CLAHE:** convertir BGR a Lab, ajustar el canal L y recuperar color. Evita ecualizar separadamente cada canal de color.
2. **Mediana 3×3:** reduce valores aislados antes de detectar bordes y reducir colores.
3. **Gris y Gaussiano:** obtener luminosidad y suavizar detalles pequeños.
4. **Sobel X/Y:** calcular cambios horizontales y verticales en CV_16S, obtener magnitudes y combinarlas con pesos 0.5/0.5, como el Ejemplo 6.
5. **Kernel propio:** aplicar filter2D en CV_32F para no perder signo ni decimales antes de obtener su magnitud.
6. **Umbral:** combinar Sobel con el detector propio y seleccionar la máscara de tinta negra.
7. **LUT:** reducir el número de niveles por canal para formar colores planos.
8. **Paleta y texturas:** teñir sombras/luces y aplicar puntos y rayado mediante máscaras.
9. **Tinta:** pintar de negro los píxeles de la máscara de contornos.
10. **HSV y glitch:** modificar saturación/brillo y desplazar azul/rojo en sentidos opuestos, conservando verde.

Los contornos se calculan antes de dibujar la trama para no detectar los puntos como bordes. Los valores se redondean y saturan a 0..255 cuando se convierten a ocho bits.

La LUT aproxima intensidad I a n niveles mediante `round(I·(n−1)/255)·255/(n−1)`. Los puntos usan celdas 8×8 y radio `round((255−I)·R/255)`, limitado a tonos medios 70..180; conservan 65 % del color. El rayado afecta sombras 0..69 y conserva 70 %. Los patrones se reutilizan mientras no cambian resolución o parámetros, evitando reconstruirlos por fotograma.

## 3. Kernel propio

```text
           [ -a    -1       0 ]
K(a) =     [ -1   4+2a     -1 ] / (4+2a)
           [  0    -1      -a ]
```

El parámetro a vale inicialmente 1 y varía entre 0 y 3. La suma `(4+2a−4−2a)/(4+2a)` es cero: una región de intensidad constante c da `c·sum(K)=0`. Los cambios entre el centro y sus vecinos producen una respuesta. La simetría de 180° hace coincidir aquí correlación y convolución.

El peso w mezcla las magnitudes: `D=(1−w)·Sobel+w·abs(propio)`. Inicialmente w=0.30: 70 % Sobel y 30 % propio. El umbral 40 selecciona los contornos. La matriz propia es 3×3; el tamaño Gaussiano ajustable 3..51 permite aumentar la carga espacial. Spiderman es el nombre del efecto combinado, no un kernel estándar publicado.

## 4. CPU y GPU sin duplicar el algoritmo

`Efecto<Imagen>` utiliza la misma secuencia con `Mat` o `UMat`. Mat usa CPU; UMat habilita T-API/OpenCL en las operaciones compatibles. No significa que todas las operaciones estén garantizadas en GPU.

`prepararOpenCL` selecciona un dispositivo GPU explícito y comprueba el contexto. La ruta GPU no se etiqueta como válida si realmente se está usando un dispositivo CPU.

`procesarFrame` mide CPU cuando el modo lo pide, después activa OpenCL y mide subida, filtrado y bajada. `ocl::finish()` espera el trabajo terminado antes de leer cada intervalo. El total GPU es la suma de esas tres fases. Un tiempo −1 representa una ruta no ejecutada y se muestra como una raya, no como cero.

En comparación se procesa el mismo fotograma en ambas rutas. Los redondeos pueden variar entre Mat y UMat, especialmente cerca del umbral; compartir matemática no garantiza igualdad de todos los píxeles.

## 5. Controles y ventanas

`crearControles` construye una sola vez doce barras con createTrackbar. `leerControles` recupera posiciones con getTrackbarPos y convierte enteros escalados por 100 a decimales. El Gaussiano siempre es impar; al seleccionar un par se salta al impar vecino en la dirección del ajuste.

Los parámetros son umbral, tamaño Gaussiano, sigma, peso propio, diagonal, radio, niveles, saturación, brillo, neón, glitch y CLAHE. Parametros::validar rechaza tamaños/rangos inválidos, incluyendo NaN en los decimales.

- **Espacio:** CPU, OpenCL y comparación, si se inició con ambas rutas disponibles.
- **T:** resultado final, gris, Gaussiano, kernel propio y máscara.
- **Arrastrar las imágenes:** girar la comparación 3D. **R:** restaurar el encuadre.
- **Esc / q:** salir de la aplicación.

Una sola ventana blanca contiene la comparación, trackbars y recursos. Como en los Ejemplos 6 y 7, `copyTo` coloca original y efecto en dos regiones `Rect` de un mismo `Mat`. `Vista3D` usa ese Mat como textura de un `WImage3D` y actualiza `setImage` en cada vuelta. El visor nativo VTK es hijo de Lienzo mediante funciones de Windows; esa integración no viene de los ejemplos. La pose se conserva entre fotogramas. El cierre libera OpenGL antes de destruir la ventana hija. Es un plano con video, no una reconstrucción de la persona.

## 6. Qué significan las cifras

- **ms CPU:** tiempo del efecto Mat.
- **ms OpenCL:** tiempo del efecto UMat sincronizado. Subida y bajada aparecen aparte.
- **FPS de ventana:** incluye lectura, procesamiento y presentación; no equivale a 1000/ms del filtro.
- **RAM MiB:** Working Set del proceso mediante GetProcessMemoryInfo; incluye bibliotecas y búferes, no es VRAM.
- **CPU %:** 100·delta(tiempo CPU)/(delta(tiempo de pared)·procesadores lógicos). GetProcessTimes entrega unidades de 100 ns, que se convierten antes de dividir.
- **GPU proceso:** máximo de los motores PDH del PID actual. Incluye renderizado VTK incluso con filtro CPU. No debe interpretarse como actividad exclusiva de un kernel.

CPU/GPU se muestrean aproximadamente cada segundo y RAM se consulta en cada lectura. Antes de una muestra válida se indica falta de muestra; un cero válido no se sustituye por una cifra inventada. Los recursos de la comparación pertenecen al proceso entero, no a dos programas independientes.

## 7. Funciones auxiliares

`benchmark` puede procesar un video fijo sin ventanas: calienta al menos 15 fotogramas y 1.1 segundos, rebobina y calcula medias, mediana y RAM máxima. Solo imprime un resumen; no guarda archivos. No es la cadencia de cámara ni incluye VTK.

`main` valida opciones, cantidad y resolución; abre la fuente y selecciona el benchmark o el video en vivo. El ejecutable no graba clips ni guarda capturas. Las pruebas de VTK verifican el renderizado sin depender de un modo extra en la aplicación.

## 8. Procedencia y comprobaciones

Se adaptan los ejemplos de clase 3 (HSV, máscaras, barras), 5 (CLAHE/convolución), 6 (Gaussiano/Sobel) y 9 (mediana). El kernel, composición artística, OpenCL, VTK y recursos amplían esa base; no se presenta todo el programa como copia literal del profesor.

Las pruebas opcionales comprueban texturas dinámicas y pose/reapertura, ayuda sin exportación y un benchmark real que deja vacío su directorio de salida. El informe, las gráficas históricas y el guion son entregables separados del programa en vivo.

## Portal en Android y acabado del cómic

En Android, `aplicarPortal` usa `copyTo` con una máscara para recuperar la cámara original fuera del rectángulo; dentro conserva el filtro Spiderman. OpenCV dibuja el marco con `rectangle`. En escritorio, VTK se usa para la comparación interactiva entre el original y el efecto.

Una mediana sobre la máscara reduce motas de tinta. Puntos menos oscuros y restringidos a tonos medios dejan las luces despejadas; el rayado evita el negro profundo. Para el RGB, `dilate` amplía la máscara de contornos lo justo para cubrir el desplazamiento y `copyTo` aplica el resultado solo allí. No hay reconocimiento facial ni realidad aumentada.
