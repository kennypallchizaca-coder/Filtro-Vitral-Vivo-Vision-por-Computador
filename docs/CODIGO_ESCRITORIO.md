# Código de escritorio — Vitral Vivo

Esta guía sigue el código que ejecuta el programa. Las referencias son números de línea **del archivo fuente**. Abre el archivo que se indica y salta a la línea en el editor. Las filas agrupan instrucciones de una misma operación; no hace falta explicar una llave o una línea vacía.

1. [Principal.cpp: entrada, procesamiento y medición](#1-principalcpp)
2. [Principal.hpp: parámetros, filtros y triángulos](#2-principalhpp)
3. [Interfaz.hpp: barras de control y teclado](#3-interfazhpp)
4. [Vista3D.hpp: composición y visor VTK](#4-vista3dhpp)
5. [Mediciones.hpp y Mediciones.cpp: recursos de Windows](#5-medicioneshpp-y-medicionescpp)
6. [CMakeLists.txt: compilación y pruebas](#6-cmakeliststxt)

**Qué hicimos.** Adaptamos filtros de clase para detectar detalle, diseñamos un kernel de suma cero y usamos el color del fotograma para pintar una malla triangular fija. En escritorio comparamos original y efecto en un visor VTK y separamos el tiempo de filtrado, transferencias y dibujo. La [guía de sustentación](DEFENSA_CODIGO.md) explica la matemática con ejemplos; aquí puedes ubicar cada decisión en el código.

## 1. Principal.cpp

[`Principal.cpp`](../Principal.cpp) coordina el programa. Recibe una cámara o un archivo de video. No contiene la fórmula del vitral: llama al núcleo de `Principal.hpp` y entrega el resultado al visor o al benchmark.

### Inicio: de los argumentos a la fuente

| Líneas | Qué hace el código |
|---|---|
| [1–15](../Principal.cpp#L1-L15) | Reúne el filtro, la vista, los controles y las mediciones. OpenCV aporta video, OpenCL y visualización; la biblioteca estándar aporta el reloj y los contenedores. |
| [307–316](../Principal.cpp#L307-L316) | `main` recibe los argumentos del sistema. `claves` declara opciones y valores iniciales: fuente `0`, modo `both`, 640×480 y 120 muestras de benchmark, entre otros. `CommandLineParser` los interpreta. |
| [318–335](../Principal.cpp#L318-L335) | Quita guiones y la parte posterior a `=` para verificar el nombre de cada opción. Rechaza `capture`, `smoke` y `output`, y convierte la excepción de OpenCV por clave desconocida en un mensaje útil. |
| [337–343](../Principal.cpp#L337-L343) | Si se pidió ayuda, imprime las opciones y atajos y retorna 0. Todavía no se ha abierto la cámara. |
| [344–352](../Principal.cpp#L344-L352) | Lee fuente, modo, dimensiones y cantidad. Crea `p` con valores iniciales y sobrescribe tamaño Gaussiano, sigma, peso propio y diagonal con los valores leídos. |
| [353–362](../Principal.cpp#L353-L362) | Rechaza errores de conversión, tamaños o cantidades fuera del rango, parámetros de filtro inválidos y modos distintos de `cpu`, `gpu` y `both`. |
| [363–368](../Principal.cpp#L363-L368) | En benchmark exige una fuente fija y un solo modo por proceso: ejecutar ambos juntos conservaría búferes de las dos rutas y mezclaría su RAM. |
| [369–378](../Principal.cpp#L369-L378) | Abre video y prepara OpenCL si corresponde. Sin GPU válida, `both` queda en CPU; `gpu` explícito da error. El código interno usa 0 para CPU, 1 para OpenCL y 2 para ambos. |
| [379–389](../Principal.cpp#L379-L389) | Elige `benchmark` o prepara `EstadoInterfaz` y llama a `visualizar`. Ante una excepción imprime la causa y termina con estado 1. |
| [117–135](../Principal.cpp#L117-L135) | Una cadena no vacía de dígitos es índice de cámara: se intenta DirectShow y luego el selector normal de OpenCV. Si es una ruta, abre el archivo. Comprueba que la fuente haya abierto. |
| [137–149](../Principal.cpp#L137-L149) | Lee una imagen. Si falla y `repetir` es falso, informa fin; si es verdadero, vuelve al fotograma 0 e intenta una vez más. Reescala con `INTER_AREA` si la imagen no tiene el tamaño solicitado. |

### CPU, OpenCL y salida del filtro

| Líneas | Qué hace el código |
|---|---|
| [21–23](../Principal.cpp#L21-L23) | Convierte la diferencia de un reloj monotónico a milisegundos. El reloj sirve para intervalos, no para mostrar fecha u hora. |
| [25–47](../Principal.cpp#L25-L47) | Comprueba que exista OpenCL; crea el contexto del selector, enlaza el primer dispositivo de ese contexto y activa OpenCL. Solo acepta un dispositivo de tipo GPU y reporta qué GPU y controlador se usaron. |
| [49–55](../Principal.cpp#L49-L55) | Los tiempos CPU y GPU empiezan en −1 para indicar «no se ejecutó». `procesoGPU()` suma subida, filtro, bajada y dibujo. |
| [57–69](../Principal.cpp#L57-L69) | `escogerEtapa` devuelve una matriz intermedia que ya produjo el filtro: gris con CLAHE, Gaussiano, kernel propio o máscara. El vitral (etapa 0) se dibuja aparte. |
| [72–75](../Principal.cpp#L72-L75) | `procesarFrame` recibe el original sin alterarlo, las dos instancias del filtro, un `UMat` reutilizable y una referencia donde dejar el resultado. |
| [76–80](../Principal.cpp#L76-L80) | Si no es modo GPU exclusivo, desactiva OpenCL, inicia el reloj, ejecuta el filtro `Mat` y guarda la duración de CPU. |
| [81–82](../Principal.cpp#L81-L82) | Si solo se ejecutó CPU y se pidió una etapa, usa la etapa CPU. Si se pidieron ambos, se elegirá la etapa OpenCL más adelante. |
| [84–90](../Principal.cpp#L84-L90) | Si no es CPU exclusivo, vuelve a activar OpenCL. Da error si no está activo y espera la cola anterior antes de iniciar el cronómetro. |
| [92–94](../Principal.cpp#L92-L94) | Copia el fotograma a `frameGPU` y espera la operación con `finish()`. Guarda la fase de subida por separado. |
| [95–98](../Principal.cpp#L95-L98) | Ejecuta el mismo núcleo en `UMat`. Espera la cola otra vez para que el tiempo mida trabajo completado y no solo el envío. |
| [99–105](../Principal.cpp#L99-L105) | Para vitral recupera únicamente los dos mapas reducidos. Para una etapa obtiene su imagen completa. Guarda el tiempo de bajada después de sincronizar. |
| [107–112](../Principal.cpp#L107-L112) | Si se pidió el efecto, obtiene los mapas CPU cuando corresponde y llama a `dibujarVitral` en CPU. El temporizador de dibujo termina en la 112. |
| [114](../Principal.cpp#L114) | Devuelve los tiempos. La imagen quedó en `resultado`; no es lo que esta función retorna. |

En modo ambos, las rutas se ejecutan **una después de la otra sobre el mismo fotograma**. Se muestran dos tiempos, pero el cuadro de la derecha solo muestra el resultado elegido de OpenCL. El original permanece a la izquierda, no es una segunda salida del filtro.

### Vista en vivo: qué ocurre en cada vuelta

| Líneas | Qué hace el código |
|---|---|
| [191–210](../Principal.cpp#L191-L210) | `dibujarCabecera` reúne etiquetas de modo y etapa, duración, transferencias, FPS y recursos. Usa rayas para rutas no ejecutadas y «sin muestra» para contadores no disponibles. |
| [212–222](../Principal.cpp#L212-L222) | `putText` pinta la cabecera de dos columnas y los atajos sobre el lienzo. No procesa los píxeles del video. |
| [225–232](../Principal.cpp#L225-L232) | Lee la primera imagen para validar la fuente. Prepara filtros, matrices y contador de recursos una vez, antes del bucle. |
| [233–245](../Principal.cpp#L233-L245) | Crea `Lienzo` y sus barras. Reserva 80 píxeles para texto y una región de 800×300 píxeles del lienzo debajo de la cabecera para el panel VTK. |
| [246–257](../Principal.cpp#L246-L257) | Crea el estado del bucle. En la primera vuelta reutiliza el fotograma ya leído; luego pide los siguientes. Fin de archivo termina normalmente; pérdida de cámara informa un error. |
| [258–262](../Principal.cpp#L258-L262) | Lee las barras, guarda los parámetros y la etapa de ese fotograma, y llama a `procesarFrame`. |
| [263–269](../Principal.cpp#L263-L269) | Convierte una etapa gris a BGR si es necesario, limpia el texto anterior y dibuja la cabecera con la muestra actual y los FPS calculados al final de la vuelta anterior. |
| [270–279](../Principal.cpp#L270-L279) | Actualiza HighGUI, atiende su teclado y, si hace falta, recoge la tecla registrada por VTK. `R` restaura la pose del visor; las otras teclas pasan a `eventoTeclado`. |
| [280–288](../Principal.cpp#L280-L288) | Comprueba si la ventana sigue visible. Si HighGUI avisa que la ventana ya no existe, termina el bucle sin volver a actualizarla. |
| [289–295](../Principal.cpp#L289-L295) | Crea el visor solo la primera vez. Le pasa original y resultado para cambiar la textura, y ajusta el panel al rectángulo reservado. |
| [296–299](../Principal.cpp#L296-L299) | Calcula `1000 / msDelCiclo`. Desde la segunda vuelta muestra un promedio suavizado: 90 % del anterior y 10 % del actual. No son los FPS aislados del filtro. |
| [301–304](../Principal.cpp#L301-L304) | Suelta primero el visor VTK; después cierra la ventana padre si sigue abierta. |

### Benchmark: por qué no es lo mismo que la vista en vivo

| Líneas | Qué hace el código |
|---|---|
| [151–156](../Principal.cpp#L151-L156) | Crea filtros, matrices y contador para esta ejecución, sin crear ventana. |
| [157–166](../Principal.cpp#L157-L166) | Calienta al menos 15 fotogramas y 1,1 segundos. Reinicia el video para que ambos ensayos midan desde el mismo fotograma. |
| [167–177](../Principal.cpp#L167-L177) | Guarda tiempos de filtro y de proceso completo para cada muestra, y conserva el máximo de RAM observado. No incluye lectura de video ni renderizado VTK en esos tiempos. |
| [178–183](../Principal.cpp#L178-L183) | Calcula media y mediana de filtrado. Para mediana, toma el punto central de la lista ordenada o promedia los dos centrales si es par. Calcula aparte la media del proceso total. |
| [184–188](../Principal.cpp#L184-L188) | Imprime resultados en consola. Un total medio de 4 ms da 250 FPS de proceso por `1000/4`; eso no equivale a una cámara de 250 FPS. No guarda CSV. |

## 2. Principal.hpp

[`Principal.hpp`](../Principal.hpp) se incluye desde C++ y contiene el núcleo que también usa Android. No es lo mismo que `Principal.cpp`: el encabezado transforma matrices y no abre cámaras ni ventanas. Al ser una plantilla, la implementación está en el encabezado para poder instanciarla con `Mat` y con `UMat`.

### Estado y kernel diseñado para el proyecto

| Líneas | Qué hace el código |
|---|---|
| [1–7](../Principal.hpp#L1-L7) | Evita incluir dos veces el encabezado en una compilación y trae matrices, operaciones de imagen, algoritmos y excepciones. |
| [10–17](../Principal.hpp#L10-L17) | Declara los siete parámetros: umbral (40), Gaussiano (5), sigma (1,73), peso propio (0,30), diagonal (1), escala (9) y CLAHE (1,5). |
| [19–23](../Principal.hpp#L19-L23) | Limita umbral a 0–255 y tamaño a un impar de 3 a 51. Un impar deja un píxel central en la ventana del Gaussiano. |
| [24–33](../Principal.hpp#L24-L33) | Rechaza sigma, mezcla, diagonal, escala o contraste fuera de rango. Protege también llamadas que no pasan por la interfaz. |
| [38–42](../Principal.hpp#L38-L42) | Comprueba la diagonal, pasa su valor a `float` y forma `K(a)`. El numerador suma `(4+2a)−4−2a=0`. Dividir por `4+2a` deja el peso central en 1 y mantiene suma cero. Una zona constante da respuesta cero. |
| [45–48](../Principal.hpp#L45-L48) | `CuadriculaVitral` empaqueta el mapa BGR, el mapa de detalle y la escala. Todavía no contiene las posiciones de los vértices. |

### De un fotograma BGR a dos mapas

| Líneas | Qué hace el código |
|---|---|
| [128–135](../Principal.hpp#L128-L135) | Especializa `Efecto` para el tipo de matriz conveniente y guarda etapas. `procesar` valida parámetros y solo acepta una entrada no vacía BGR de tres canales de 8 bits. |
| [136–139](../Principal.hpp#L136-L139) | Construye un kernel nuevo únicamente si cambió la diagonal. |
| [140–143](../Principal.hpp#L140-L143) | Cambia el límite de CLAHE únicamente si cambió su control. |
| [145](../Principal.hpp#L145) | `cvtColor` convierte BGR a luminancia aproximada y escribe `gris`. No toca el original, que luego da los colores. |
| [146](../Principal.hpp#L146) | `medianBlur` ordena valores de cada vecindad 3×3 y toma el central. Reduce ruido impulsivo y deja el resultado en `mediana`. |
| [147](../Principal.hpp#L147) | CLAHE ajusta el contraste local de la mediana. La variable `gris` ahora contiene esa versión mejorada, no la conversión inicial. |
| [148](../Principal.hpp#L148) | Llama a `detectarContornos` para obtener las etapas del borde y la máscara. |
| [150–151](../Principal.hpp#L150-L151) | Fija la escala y calcula el tamaño reducido por división entera, con un mínimo de 1 por eje. En 640×480 con escala 9 son 71×53 celdas de datos. |
| [152](../Principal.hpp#L152) | Reduce el BGR original con `INTER_AREA`. Este mapa aporta los colores. |
| [153](../Principal.hpp#L153) | Reduce la máscara también por área. Al promediar bordes blancos y negros, una zona reducida puede tener valores entre 0 y 255. Este mapa aporta el detalle. |
| [156–163](../Principal.hpp#L156-L163) | Devuelve ambos mapas como `Mat` y la escala. En la ruta OpenCL del vitral se descargan esos mapas, no el fotograma completo filtrado. |
| [165–170](../Principal.hpp#L165-L170) | Mantiene CLAHE, kernel, imágenes de trabajo y últimos valores como estado reutilizable de cada filtro. |

### El cálculo de bordes, instrucción por instrucción

| Línea | Qué hace el código |
|---|---|
| [174](../Principal.hpp#L174) | `GaussianBlur` suaviza `gris` con una ventana cuadrada de lado `tamGauss` y los sigmas elegidos antes de calcular derivadas. |
| [175](../Principal.hpp#L175) | Sobel con `dx=1, dy=0` aproxima el cambio en x. `CV_16S` conserva valores negativos. |
| [176](../Principal.hpp#L176) | Sobel con `dx=0, dy=1` aproxima el cambio en y. |
| [177–178](../Principal.hpp#L177-L178) | Convierte ambas derivadas a magnitudes de 8 bits, con saturación a 255. |
| [179](../Principal.hpp#L179) | `S = 0,5·absX + 0,5·absY` promedia las magnitudes. No calcula la raíz de la suma de cuadrados. |
| [181](../Principal.hpp#L181) | `filter2D` aplica nuestro kernel al gris suavizado y deja una respuesta con signo en `CV_32F`. |
| [183](../Principal.hpp#L183) | `convertScaleAbs` calcula la magnitud del kernel multiplicada por 12 y la limita a 255. La ganancia acerca la escala de esa respuesta `H` a la de Sobel. |
| [184](../Principal.hpp#L184) | `gradiente = (1−w)·S + w·H`, donde `w` es `pesoPropio`. Con 0 queda Sobel, con 1 queda el kernel; desde aquí `gradiente` representa la mezcla. |
| [185](../Principal.hpp#L185) | Da 255 a valores estrictamente mayores que el umbral y 0 al resto. |
| [186](../Principal.hpp#L186) | Una mediana 3×3 elimina puntos aislados de la máscara. |

### De los mapas a los triángulos

| Líneas | Qué hace el código |
|---|---|
| [50–55](../Principal.hpp#L50-L55) | Recibe los mapas y el tamaño de salida. Si algún eje tiene menos de 3 píxeles, devuelve el color reescalado en vez de construir triángulos degenerados. |
| [56–59](../Principal.hpp#L56-L59) | Usa un lado objetivo de `3 × escala` y redondea el número de bloques por eje. Limita el desplazamiento de vértices a un cuarto del menor espaciado. |
| [60–63](../Principal.hpp#L60-L63) | Crea el lienzo BGR oscuro y un vector que guarda los vértices de la grilla. `punto(x,y)` devuelve la posición de la fila `y` y columna `x`. |
| [65–71](../Principal.hpp#L65-L71) | Calcula cada vértice regular más un desplazamiento pequeño. Los enteros de las líneas 67–68 mezclan coordenadas, no contenido de la cámara. En el extremo de un eje, no lo desplaza en ese eje. |
| [74–79](../Principal.hpp#L74-L79) | Proyecta una posición de la imagen al tamaño del mapa reducido y lee allí el azul, verde y rojo. |
| [81–86](../Principal.hpp#L81-L86) | Recibe tres vértices. Promedia sus coordenadas para hallar el centro, consulta el color base y rellena el triángulo con `fillConvexPoly`. |
| [87–93](../Principal.hpp#L87-L93) | Busca detalle en el centro y los centros de las tres subdivisiones. Se queda con el máximo para no ignorar un borde que cruza una parte de la pieza. |
| [94–95](../Principal.hpp#L94-L95) | Deja el color base sin subdivisiones visibles si el detalle es cero o el centro cae sobre un vértice. |
| [97–101](../Principal.hpp#L97-L101) | Calcula `min(detalle/56, 1)` y mezcla color base con el color local de cada subdivisión. Con detalle 28, la mezcla es mitad y mitad. |
| [102–103](../Principal.hpp#L102-L103) | Traza las uniones internas con un color entre el relleno base y el plomo oscuro. El detalle cambia el contraste, no las ubicaciones. |
| [105–115](../Principal.hpp#L105-L115) | Para cada bloque toma cuatro esquinas y forma dos triángulos. La paridad de `x+y` elige la diagonal alternada. |
| [117–124](../Principal.hpp#L117-L124) | Repasa los lados y la diagonal de cada bloque con líneas oscuras y devuelve el `Mat` BGR final. |

**Cómo explicarlo:** empieza por las líneas 145–153. Señala dónde sale el color y dónde sale el detalle. Después salta a la 83 para ver cómo se usa un centro y a la 97 para explicar por qué las subdivisiones se ven más cerca de los bordes.

## 3. Interfaz.hpp

[`Interfaz.hpp`](../Interfaz.hpp) controla el panel de OpenCV, no el visor 3D. Recibe posiciones del usuario y las transforma en parámetros; no calcula el vitral.

| Líneas | Qué hace el código |
|---|---|
| [12–16](../Interfaz.hpp#L12-L16) | `numero` convierte un valor a texto con dos decimales de forma predeterminada. No redondea la variable que usa el filtro. |
| [18–22](../Interfaz.hpp#L18-L22) | `EstadoInterfaz` reúne parámetros, modo, etapa, disponibilidad de GPU y la bandera para salir. El valor inicial de la etapa es 0, el vitral. |
| [24–25](../Interfaz.hpp#L24-L25) | El callback de las barras no procesa nada. Es el bucle de `Principal.cpp:258` el que consulta cada barra una vez por fotograma. |
| [28–35](../Interfaz.hpp#L28-L35) | Crea siete barras HighGUI, cada una con un tope entero. El parámetro `nullptr` significa que la barra no actualiza un entero externo: el programa lee su posición con `getTrackbarPos`. |
| [37–40](../Interfaz.hpp#L37-L40) | Fija los mínimos 3 para kernel, 10 para sigma ×100, 6 para escala y 10 para CLAHE ×100. Las otras barras pueden empezar en 0. |
| [42–48](../Interfaz.hpp#L42-L48) | Usa `setTrackbarPos` para poner los valores iniciales de `p`. Multiplica los decimales por 100 y los redondea a enteros. Sigma 1,73 se muestra como posición 173. |
| [52–59](../Interfaz.hpp#L52-L59) | Lee el kernel. Si llega a un valor par, salta al siguiente impar en la dirección del movimiento y corrige la barra. De 5 a 6 salta a 7; de 9 a 8 baja a 7. |
| [60–67](../Interfaz.hpp#L60-L67) | Conserva tamaño, umbral y escala como enteros. Divide sigma, peso propio, diagonal y CLAHE por 100,0 para recuperar sus valores reales. Devuelve los siete parámetros. |
| [70–76](../Interfaz.hpp#L70-L76) | `Esc` o `Q` marcan salida. Espacio cicla CPU, GPU y ambos si hay GPU; si no, deja CPU. `T` recorre las cinco etapas. `R` se atiende en [`Principal.cpp:276–277`](../Principal.cpp#L276-L277) y no restablece deslizadores. |

## 4. Vista3D.hpp

[`Vista3D.hpp`](../Vista3D.hpp) recibe el original y el resultado ya calculados. Los coloca en una textura 2D con un marco de cuatro barras en una escena 3D. No vuelve a aplicar filtros.

### Crear el plano y las cuatro barras

| Líneas | Qué hace el código |
|---|---|
| [3–9](../Vista3D.hpp#L3-L9) | Incluye Win32 para integrar ventanas y OpenCV para HighGUI y Viz. `NOMINMAX` evita que los macros de Windows interfieran con `std::min` y `std::max`. |
| [15–21](../Vista3D.hpp#L15-L21) | Guarda el lienzo, el visor `Viz3d`, el plano `WImage3D`, los identificadores nativos de ventana, la última tecla y la pose que recuperará `R`. |
| [23–26](../Vista3D.hpp#L23-L26) | Construye el visor y un plano texturizado en el origen, con normal en +Z y arriba hacia +Y. Llama a `componer` para crear la textura y a `tamano` para conservar su proporción. |
| [127–131](../Vista3D.hpp#L127-L131) | `componer` valida que no falte el original, que sea BGR de 8 bits y que el resultado tenga el mismo tipo y tamaño. |
| [132](../Vista3D.hpp#L132) | Reserva un lienzo de las mismas filas y el doble de columnas. `Mat::create` reutiliza el búfer si el formato ya coincide. |
| [133](../Vista3D.hpp#L133) | Selecciona con `Rect` la región izquierda, desde x=0, y copia allí el fotograma original. |
| [134–135](../Vista3D.hpp#L134-L135) | Copia el resultado a partir de x=`frame.cols`, en la mitad derecha, y retorna el lienzo compuesto. |
| [138–142](../Vista3D.hpp#L138-L142) | Convierte la proporción en tamaño del mundo 3D. El ancho es 3,7 unidades y el alto es `3,7 × filas / (2 × columnas)`; el 2 corresponde a las dos imágenes. |
| [27–34](../Vista3D.hpp#L27-L34) | Si se solicita vista oculta, configura render fuera de pantalla. En uso normal coloca temporalmente la ventana lejos del escritorio mientras la prepara. Da un tamaño inicial al visor y posiciona la cámara a z=−5,6 de manera provisional. |
| [35–37](../Vista3D.hpp#L35-L37) | Muestra el plano, crea el marco y registra un callback de teclado que recibirá este objeto. |
| [107–118](../Vista3D.hpp#L107-L118) | Define ancho, borde y espesor. `mostrarMarco` usa los semiejes de la textura y crea barras sólidas `WCube` en color grafito. Cada barra llega desde z=−0,12 hasta z=0,12. |
| [120–123](../Vista3D.hpp#L120-L123) | Coloca barra superior, inferior, izquierda y derecha alrededor de la textura. Las horizontales cubren las esquinas; las verticales quedan entre ellas. |

**Ejemplo:** dos imágenes de 640×480 producen un `Mat` de 1280×480. El original ocupa x=0–639 y el efecto x=640–1279. En la escena, el plano mide 3,7×1,3875 y está en z=0. Al girar el visor se ve el espesor del marco, pero todos los píxeles del video siguen sobre un plano: no hay reconstrucción de profundidad.

### Integrar el visor en la misma ventana

| Líneas | Qué hace el código |
|---|---|
| [59–65](../Vista3D.hpp#L59-L65) | En la primera llamada a `integrar` busca la ventana principal `Lienzo` y la ventana del visor VTK. Si falta una, informa el error en vez de seguir con una referencia inválida. |
| [149–168](../Vista3D.hpp#L149-L168) | `buscarVentana` recorre ventanas de primer nivel con `EnumWindows`. Acepta únicamente un título exacto del PID actual y detiene la búsqueda al encontrarlo. |
| [66–70](../Vista3D.hpp#L66-L70) | Quita los estilos de ventana independiente y añade `WS_CHILD`: el visor pasará a ser un panel hijo. |
| [71–73](../Vista3D.hpp#L71-L73) | `SetParent` mete el panel VTK en `Lienzo`. Un padre anterior nulo no siempre es un error; por eso limpia el último error antes de llamar y lo consulta si la llamada retorna nulo. |
| [74–78](../Vista3D.hpp#L74-L78) | Evita que el dibujo de `Lienzo` tape el área hija y registra el callback que cerrará VTK cuando Windows cierre el padre. |
| [80–86](../Vista3D.hpp#L80-L86) | Mide el rectángulo real de HighGUI y calcula escalas `sx` y `sy` respecto al lienzo lógico. Convierte la posición de pantalla a coordenadas de la ventana padre y limita ancho y alto a por lo menos un píxel. |
| [87–93](../Vista3D.hpp#L87-L93) | Recoloca y redimensiona el panel solo si la posición cambió. `SWP_NOACTIVATE` evita quitar el foco al control activo. |
| [94–103](../Vista3D.hpp#L94-L103) | Al integrar por primera vez, incluye el grosor visible del borde en el encuadre. Calcula cuánto alto virtual hace falta para que entren ancho y alto exteriores. Usa el campo visual vertical, un margen de 8 % y medio espesor para fijar la distancia. Guarda esa pose como la inicial. |

Si te preguntan por la fórmula de las líneas 97–99: en una cámara de perspectiva, `tan(FOV/2) = (alto/2) / distancia`. Despejar da `distancia = alto / (2 × tan(FOV/2))`. Ajustarla con margen evita que el marco toque el borde.

### Actualizar, girar y cerrar

| Líneas | Qué hace el código |
|---|---|
| [50–52](../Vista3D.hpp#L50-L52) | Si VTK ya se detuvo, retorna falso y el bucle deja de enviar fotogramas. |
| [53](../Vista3D.hpp#L53) | Compone el nuevo par original/efecto y actualiza con `setImage` la textura del plano existente. No crea otro plano ni otras barras. |
| [54–55](../Vista3D.hpp#L54-L55) | `spinOnce` vuelve a dibujar y atiende eventos; a diferencia de un bucle propio de VTK, devuelve el control para leer la siguiente imagen. |
| [144–147](../Vista3D.hpp#L144-L147) | Registra el código de una tecla al presionarla. `Principal.cpp` la consume. Arrastrar cambia la cámara virtual; `R` restaura su pose, no el filtro. |
| [40–47](../Vista3D.hpp#L40-L47) | No permite copiar dos veces un objeto que administra las mismas ventanas. El destructor retira su callback del padre si sigue vivo y cierra Viz. |
| [172–185](../Vista3D.hpp#L172-L185) | Si Windows cierra o destruye el padre, primero cierra el visor para liberar VTK/OpenGL. En `WM_NCDESTROY` limpia el identificador y retira el callback. `DefSubclassProc` permite que continúe el cierre normal. |

## 5. Mediciones.hpp y Mediciones.cpp

[`Mediciones.hpp`](../Mediciones.hpp) declara lo que puede consultar el programa. [`Mediciones.cpp`](../Mediciones.cpp) contiene las llamadas a Windows. Esta parte es distinta de los cronómetros del filtro en `Principal.cpp`.

| Archivo y líneas | Qué hace el código |
|---|---|
| [`Mediciones.hpp:5–13`](../Mediciones.hpp#L5-L13) | Una muestra tiene RAM del proceso, porcentaje CPU y ocupación del motor GPU, más dos banderas de disponibilidad. El porcentaje inicial cero no significa que se midió cero. |
| [`Mediciones.hpp:15–26`](../Mediciones.hpp#L15-L26) | Expone `leer` y mantiene el estado interno tras un `unique_ptr`. Evita copias para que la consulta nativa tenga un solo propietario. |
| [`Mediciones.cpp:19–28`](../Mediciones.cpp#L19-L28) | `GetProcessTimes` devuelve partes alta y baja del tiempo del proceso. Las combina y suma tiempo de usuario y de sistema en unidades de 100 ns. |
| [`Mediciones.cpp:31–40`](../Mediciones.cpp#L31-L40) | Guarda el tiempo anterior, el reloj, el número de procesadores lógicos, la muestra y un prefijo con el PID para distinguir los motores del proceso. |
| [`Mediciones.cpp:42–49`](../Mediciones.cpp#L42-L49) | Lee el `WorkingSetSize` en bytes y divide por 1024² para mostrar MiB. Es la memoria residente del proceso, no VRAM. Si Windows no entrega el dato retorna 0. |
| [`Mediciones.cpp:51–70`](../Mediciones.cpp#L51-L70) | Toma valores iniciales, intenta abrir una consulta PDH de los motores GPU y empieza el muestreo. Si falla, deja el contador no disponible. El destructor cierra la consulta. |
| [`Mediciones.cpp:72–80`](../Mediciones.cpp#L72-L80) | Actualiza la RAM al llamar. Para no consultar demasiado CPU/GPU, si aún no pasó un segundo devuelve los porcentajes previos. |
| [`Mediciones.cpp:81–91`](../Mediciones.cpp#L81-L91) | Calcula el incremento de tiempo CPU respecto al anterior y lo divide por tiempo de pared y número de procesadores lógicos. Acota a 0–100 %. Medio segundo de CPU en un segundo de pared y 8 lógicos da 6,25 %. |
| [`Mediciones.cpp:92–105`](../Mediciones.cpp#L92-L105) | Empieza sin muestra GPU válida. Pide primero cuánto búfer requiere Windows y luego obtiene la lista de motores; una consulta fallida no se presenta como medición válida. |
| [`Mediciones.cpp:106–120`](../Mediciones.cpp#L106-L120) | Conserva motores del PID actual con valores válidos. Reporta el mayor, no la suma de todos. Puede incluir renderizado VTK; no es una métrica exclusiva de OpenCL ni mide memoria de video. |

## 6. CMakeLists.txt

[`CMakeLists.txt`](../CMakeLists.txt) describe cómo se construye el ejecutable y cómo se registran las pruebas. CMake no procesa fotogramas.

| Líneas | Qué hace el código |
|---|---|
| [1–9](../CMakeLists.txt#L1-L9) | Requiere CMake 3.16 y C++17. Busca los módulos OpenCV core (matrices), imgproc (filtros), highgui (panel), videoio (fuente) y viz (escena 3D). |
| [11–19](../CMakeLists.txt#L11-L19) | Compila `Principal.cpp` y `Mediciones.cpp`, enlaza OpenCV y, en Windows, PSAPI y PDH para métricas y Comctl32 para el manejo de cierre del visor integrado. Los encabezados se incluyen desde C++, no se ejecutan como programas aparte. |
| [21–32](../CMakeLists.txt#L21-L32) | `BUILD_TESTING` habilita la prueba de VTK. Distingue el caso que toma captura del caso que verifica estructura y pose sin captura. |
| [33–44](../CMakeLists.txt#L33-L44) | Registra ayuda y benchmark sin archivos, y el filtro de vitral. En cada caso el ejecutable de prueba es independiente de la app normal. |

**Orden sugerido para la defensa:** abre `Principal.cpp:261`, salta a `Principal.hpp:145` para explicar el procesamiento, regresa a `Principal.cpp:291` para mostrar la llamada al visor y termina en `Vista3D.hpp:133–134` para señalar dónde se copian las dos imágenes. Luego diferencia FPS del ciclo, FPS de proceso y RAM.
