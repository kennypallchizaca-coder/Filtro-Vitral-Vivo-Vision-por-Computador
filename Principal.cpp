#include "Vista3D.hpp"
#include "Principal.hpp"
#include "Mediciones.hpp"
#include "Interfaz.hpp"
#include <opencv2/core/ocl.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/viz.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>

using namespace std;
using namespace cv;
using Reloj = chrono::steady_clock;

double milisegundos(Reloj::time_point inicio) {
    return chrono::duration<double, milli>(Reloj::now() - inicio).count();
}

bool prepararOpenCL(const string& seleccion, string& dispositivo) {
    dispositivo = "OpenCL no disponible";
    try {
        if (!ocl::haveOpenCL())
            return false;
        // El selector explícito evita medir otra GPU distinta de la solicitada.
        ocl::Context contexto = ocl::Context::create(seleccion);
        if (contexto.empty() || contexto.ndevices() == 0)
            return false;
        ocl::OpenCLExecutionContext::create(contexto, contexto.device(0)).bind();
        ocl::setUseOpenCL(true);
        const ocl::Device& gpu = ocl::Device::getDefault();
        if (!ocl::useOpenCL() || !(gpu.type() & ocl::Device::TYPE_GPU))
            return false;
        dispositivo = gpu.name();
        cout << "OpenCL GPU: " << dispositivo << "\nFabricante: " << gpu.vendorName()
             << "\nControlador: " << gpu.driverVersion() << '\n';
        return true;
    } catch (const cv::Exception& error) {
        cerr << "No se pudo seleccionar OpenCL: " << error.what() << '\n';
        return false;
    }
}

struct Tiempos {
    // -1 identifica una ruta no ejecutada, en vez de presentarla como cero milisegundos.
    double cpu = -1, gpu = -1, subida = 0, bajada = 0, dibujo = 0;
    double procesoGPU() const {
        return subida + gpu + bajada + dibujo;
    }
};

template <class Imagen>
const Imagen& escogerEtapa(const Efecto<Imagen>& efecto, int etapa) {
    switch (etapa) {
        case 1:
            return efecto.gris;
        case 2:
            return efecto.imgGauss;
        case 3:
            return efecto.filtroPropioAbs;
        default:
            return efecto.binaria;
    }
}

// Asistencia de IA: OpenAI Codex separó filtrado, transferencia y dibujo del vitral para medirlos.
Tiempos procesarFrame(const Mat& frame, const Parametros& p, int modo, int etapa, Efecto<Mat>& cpu,
                      Efecto<UMat>& gpu, UMat& frameGPU, Mat& resultado) {
    Tiempos tiempos;
    CuadriculaVitral cuadricula;
    if (modo != 1) {
        ocl::setUseOpenCL(false);
        auto inicio = Reloj::now();
        cpu.procesar(frame, p);
        tiempos.cpu = milisegundos(inicio);
        if (modo == 0 && etapa != 0)
            resultado = escogerEtapa(cpu, etapa);
    }
    if (modo != 0) {
        ocl::setUseOpenCL(true);
        if (!ocl::useOpenCL())
            throw runtime_error("OpenCL dejo de estar disponible.");
        // OpenCL es asíncrono: finish espera el trabajo, no solo su envío a la cola.
        ocl::finish();
        auto inicio = Reloj::now();
        // Medimos las transferencias aparte para distinguir filtrado y costo total.
        frame.copyTo(frameGPU);
        ocl::finish();
        tiempos.subida = milisegundos(inicio);
        inicio = Reloj::now();
        gpu.procesar(frameGPU, p);
        ocl::finish();
        tiempos.gpu = milisegundos(inicio);
        inicio = Reloj::now();
        if (etapa == 0)
            cuadricula = gpu.cuadricula();
        else
            escogerEtapa(gpu, etapa).copyTo(resultado);
        ocl::finish();
        tiempos.bajada = milisegundos(inicio);
    }
    if (etapa == 0) {
        const auto inicio = Reloj::now();
        if (modo == 0)
            cuadricula = cpu.cuadricula();
        resultado = dibujarVitral(cuadricula, frame.size());
        tiempos.dibujo = milisegundos(inicio);
    }
    return tiempos;
}

bool esCamara(const string& fuente) {
    return !fuente.empty() && all_of(fuente.begin(), fuente.end(),
                                     [](unsigned char c) { return c >= '0' && c <= '9'; });
}

void abrirFuente(VideoCapture& video, const string& fuente, Size tamano) {
    if (esCamara(fuente)) {
        int indice = stoi(fuente);
        video.open(indice, CAP_DSHOW);
        if (!video.isOpened())
            video.open(indice);
        video.set(CAP_PROP_FRAME_WIDTH, tamano.width);
        video.set(CAP_PROP_FRAME_HEIGHT, tamano.height);
    } else {
        video.open(fuente);
    }
    if (!video.isOpened())
        throw runtime_error("No se pudo abrir la fuente: " + fuente);
}

bool leerFrame(VideoCapture& video, Mat& frame, Size tamano, bool repetir) {
    if (!video.read(frame) || frame.empty()) {
        if (!repetir)
            return false;
        if (!video.set(CAP_PROP_POS_FRAMES, 0))
            return false;
        if (!video.read(frame) || frame.empty())
            return false;
    }
    if (frame.size() != tamano)
        resize(frame, frame, tamano, 0, 0, INTER_AREA);
    return true;
}

int benchmark(VideoCapture& video, int cantidad, Size tamano, int modo, const Parametros& p) {
    Efecto<Mat> cpu;
    Efecto<UMat> gpu;
    UMat frameGPU;
    Mat frame, resultado;
    RecursosProceso recursos;
    // Calentar excluye la compilación inicial de kernels y prepara los contadores de recursos.
    auto inicioCalentamiento = Reloj::now();
    for (int i = 0; i < 15 || milisegundos(inicioCalentamiento) < 1100; ++i) {
        if (!leerFrame(video, frame, tamano, true))
            throw runtime_error("No hay fotogramas para calentar.");
        procesarFrame(frame, p, modo, 0, cpu, gpu, frameGPU, resultado);
    }
    // Ambos procesos deben empezar a medir desde el mismo fotograma del video fijo.
    if (!video.set(CAP_PROP_POS_FRAMES, 0))
        throw runtime_error("El video no permite volver al inicio para comparar.");
    vector<double> filtros, totales;
    double ramMaxima = 0;
    for (int i = 0; i < cantidad; ++i) {
        if (!leerFrame(video, frame, tamano, true))
            throw runtime_error("No hay fotogramas para medir.");
        Tiempos t = procesarFrame(frame, p, modo, 0, cpu, gpu, frameGPU, resultado);
        MuestraRecursos muestra = recursos.leer();
        filtros.push_back(modo == 0 ? t.cpu : t.gpu);
        totales.push_back(modo == 0 ? t.cpu + t.dibujo : t.procesoGPU());
        ramMaxima = max(ramMaxima, muestra.ramMiB);
    }
    double media = accumulate(filtros.begin(), filtros.end(), 0.0) / filtros.size();
    sort(filtros.begin(), filtros.end());
    double mediana = filtros.size() % 2
                         ? filtros[filtros.size() / 2]
                         : (filtros[filtros.size() / 2 - 1] + filtros[filtros.size() / 2]) / 2;
    double total = accumulate(totales.begin(), totales.end(), 0.0) / totales.size();
    cout << "Muestras: " << cantidad << "; filtro medio: " << numero(media)
         << " ms; mediana: " << numero(mediana) << " ms; proceso medio: " << numero(total)
         << " ms; FPS de proceso: " << numero(1000 / total)
         << "; RAM maxima proceso: " << numero(ramMaxima) << " MiB\n";
    return 0;
}

static void dibujarCabecera(Mat& lienzo, const EstadoInterfaz& estado, const Tiempos& t,
                           const MuestraRecursos& muestra, double fps) {
    static const array<string, 5> etapas = {
        "Vitral Vivo", "Gris CLAHE + mediana", "Gaussiano", "Kernel propio",
        "Mascara de contornos"};
    static const array<string, 3> modos = {"CPU Mat", "OpenCL UMat", "Comparar CPU / OpenCL"};
    const Scalar texto(25, 25, 25);
    string tiempos = "CPU filtro: " + (t.cpu < 0 ? string("--") : numero(t.cpu)) +
                     " ms | OpenCL: " + (t.gpu < 0 ? string("--") : numero(t.gpu)) +
                     " ms | FPS: " + numero(fps, 1);
    string transferencias = "Subida: " + (t.gpu < 0 ? string("--") : numero(t.subida)) +
                            " ms | Bajada: " + (t.gpu < 0 ? string("--") : numero(t.bajada)) +
                            " ms";
    string recursosTexto =
        "RAM: " + numero(muestra.ramMiB, 1) + " MiB | CPU proceso: " +
        (muestra.cpuDisponible ? numero(muestra.cpuPorcentaje, 1) + "%" : "sin muestra");
    string usoGPU =
        "GPU proceso (motor max): " +
        (muestra.gpuDisponible ? numero(muestra.gpuMotorPorcentaje, 1) + "%" : "sin muestra");
    string modo = modos[estado.modo] + " | Sigma: " + numero(estado.parametros.sigma);

    putText(lienzo, "Original | Arrastrar: girar 3D", Point(8, 18), FONT_HERSHEY_SIMPLEX, 0.55,
            texto, 1, LINE_AA);
    putText(lienzo, "Efecto: " + etapas[estado.etapa], Point(408, 18), FONT_HERSHEY_SIMPLEX, 0.55,
            texto, 1, LINE_AA);
    putText(lienzo, tiempos, Point(8, 37), FONT_HERSHEY_SIMPLEX, 0.45, texto, 1, LINE_AA);
    putText(lienzo, transferencias, Point(408, 37), FONT_HERSHEY_SIMPLEX, 0.45, texto, 1, LINE_AA);
    putText(lienzo, recursosTexto, Point(8, 55), FONT_HERSHEY_SIMPLEX, 0.45, texto, 1, LINE_AA);
    putText(lienzo, usoGPU, Point(408, 55), FONT_HERSHEY_SIMPLEX, 0.45, texto, 1, LINE_AA);
    putText(lienzo, modo, Point(8, 73), FONT_HERSHEY_SIMPLEX, 0.45, texto, 1, LINE_AA);
    putText(lienzo, "Espacio modo | T etapa | R vista | Esc salir", Point(408, 73),
            FONT_HERSHEY_SIMPLEX, 0.45, texto, 1, LINE_AA);
}

int visualizar(VideoCapture& video, const string& fuente, Size tamano, EstadoInterfaz& estado) {
    Mat frame, resultado, color;
    if (!leerFrame(video, frame, tamano, false))
        throw runtime_error("El primer fotograma esta vacio.");
    Efecto<Mat> cpu;
    Efecto<UMat> gpu;
    UMat frameGPU;
    RecursosProceso recursos;
    const string ventana = "Lienzo";
#ifdef _WIN32
    // Qt lee esta opción del proceso al crear la primera ventana.
    if (!getenv("QT_QPA_PLATFORM"))
        _putenv_s("QT_QPA_PLATFORM", "windows:darkmode=0");
#endif
    namedWindow(ventana, WINDOW_AUTOSIZE);
    crearControles(ventana, estado.parametros);
    const int anchoVista = 400, altoVista = 300, altoTexto = 80;
    const Rect regionVideo(0, altoTexto, 2 * anchoVista, altoVista);
    // El visor VTK ocupa esta región del lienzo, sin abrir otra ventana de escritorio.
    const Scalar fondo(240, 240, 240);
    Mat lienzo(Size(2 * anchoVista, altoTexto + altoVista), CV_8UC3, fondo);
    bool primerFrame = true;
    double fps = 0;
    bool ventanaAbierta = true;
    unique_ptr<Vista3D> vista3D;
    while (!estado.salir && ventanaAbierta) {
        auto inicio = Reloj::now();
        if (!primerFrame && !leerFrame(video, frame, tamano, false)) {
            if (esCamara(fuente))
                throw runtime_error("La camara dejo de entregar fotogramas.");
            cout << "Fin de la fuente de video.\n";
            break;
        }
        estado.parametros = leerControles(ventana, estado.parametros.tamGauss);
        const Parametros parametrosFrame = estado.parametros;
        const int etapaFrame = estado.etapa;
        Tiempos t = procesarFrame(frame, parametrosFrame, estado.modo, etapaFrame, cpu, gpu,
                                  frameGPU, resultado);
        if (resultado.channels() == 1)
            cvtColor(resultado, color, COLOR_GRAY2BGR);
        else
            color = resultado;
        lienzo(Rect(0, 0, lienzo.cols, altoTexto)).setTo(fondo);

        dibujarCabecera(lienzo, estado, t, recursos.leer(), fps);
        imshow(ventana, lienzo);
        int tecla = waitKey(1);
        if (vista3D) {
            if (tecla < 0)
                tecla = vista3D->tecla;
            vista3D->tecla = -1;
            if (tecla == 'r' || tecla == 'R')
                vista3D->visor.setViewerPose(vista3D->poseInicial);
        }
        eventoTeclado(tecla, estado);
        try {
            ventanaAbierta = getWindowProperty(ventana, WND_PROP_VISIBLE) >= 1;
        } catch (const cv::Exception& error) {
            if (error.code != Error::StsNullPtr)
                throw;
            ventanaAbierta = false;
        }
        if (!ventanaAbierta || estado.salir)
            break;
        if (!vista3D)
            vista3D = make_unique<Vista3D>(frame, color);
        if (!vista3D->actualizar(frame, color)) {
            estado.salir = true;
            break;
        }
        vista3D->integrar(ventana, regionVideo, lienzo.size());
        double ciclo = milisegundos(inicio);
        double actual = 1000 / max(ciclo, 0.001);
        fps = primerFrame ? actual : fps * 0.9 + actual * 0.1;
        primerFrame = false;
    }
    vista3D.reset();
    if (ventanaAbierta)
        destroyWindow(ventana);
    return 0;
}

int main(int argc, char** argv) {
    try {
        const string claves =
            "{help h||Ayuda}{benchmark||Medir sin ventanas}"
            "{source|0|Camara numerica o archivo}{mode|both|cpu, gpu o both}"
            "{frames|120|Numero de muestras del benchmark}{width|640|Ancho}{height|480|Alto}"
            "{device|NVIDIA:GPU:RTX 4070|Seleccion OpenCL explicita}"
            "{kernel|5|Tamano impar Gaussiano, 3 a 51}{sigma|1.73|Sigma Gaussiano}"
            "{own-weight|0.3|Peso filtro propio, 0 a 1}{diagonal|1|Peso diagonal, 0 a 3}";
        CommandLineParser argumentos(argc, argv, claves);
        // Asistencia de IA: OpenAI Codex añadió esta validación para no ignorar errores de escritura.
        for (int i = 1; i < argc; ++i) {
            string opcion = argv[i];
            if (opcion.empty() || opcion.front() != '-')
                continue;
            opcion = opcion.substr(opcion.compare(0, 2, "--") == 0 ? 2 : 1);
            opcion = opcion.substr(0, opcion.find('='));
            if (opcion.empty())
                continue;
            if (opcion == "capture" || opcion == "smoke" || opcion == "output")
                throw runtime_error("Opcion retirada: " + opcion + ". Consulta --help.");
            // has consulta las claves declaradas, incluso las que tienen un valor por defecto.
            try {
                argumentos.has(opcion);
            } catch (const cv::Exception& error) {
                if (error.code != Error::StsBadArg)
                    throw;
                throw runtime_error("Opcion no reconocida: " + opcion + ". Consulta --help.");
            }
        }
        if (argumentos.has("help")) {
            argumentos.printMessage();
            cout << "Controles: sliders=parametros, Espacio=modo, T=etapa, R=restaurar vista 3D, "
                    "Esc=salir.\nUna sola ventana Lienzo: original y efecto como textura VTK "
                    "interactiva; arrastrar sobre las imagenes=rotar.\n";
            return 0;
        }
        string fuente = argumentos.get<string>("source");
        string modoTexto = argumentos.get<string>("mode");
        Size tamano(argumentos.get<int>("width"), argumentos.get<int>("height"));
        int cantidad = argumentos.get<int>("frames");
        Parametros p;
        p.tamGauss = argumentos.get<int>("kernel");
        p.sigma = argumentos.get<double>("sigma");
        p.pesoPropio = argumentos.get<double>("own-weight");
        p.diagonal = argumentos.get<double>("diagonal");
        if (!argumentos.check()) {
            argumentos.printErrors();
            return 1;
        }
        if (cantidad < 1 || cantidad > 10000 || tamano.width < 8 || tamano.height < 8 ||
            tamano.width > 3840 || tamano.height > 2160)
            throw runtime_error("Dimensiones o cantidad invalidas.");
        p.validar();
        if (modoTexto != "cpu" && modoTexto != "gpu" && modoTexto != "both")
            throw runtime_error("mode debe ser cpu, gpu o both.");
        bool medir = argumentos.has("benchmark");
        if (medir && modoTexto == "both")
            throw runtime_error(
                "Para comparar RAM use procesos separados: --mode=cpu y --mode=gpu.");
        if (medir && esCamara(fuente))
            throw runtime_error("Benchmark requiere un video fijo: --source=entrada.avi.");
        VideoCapture video;
        abrirFuente(video, fuente, tamano);
        string dispositivo = "CPU Mat";
        bool gpuDisponible =
            modoTexto != "cpu" && prepararOpenCL(argumentos.get<string>("device"), dispositivo);
        if (modoTexto == "gpu" && !gpuDisponible)
            throw runtime_error(
                "No hay una GPU OpenCL que coincida con device; no se usara CPU como si fuera "
                "GPU.");
        int modo = modoTexto == "cpu" || !gpuDisponible ? 0 : (modoTexto == "gpu" ? 1 : 2);
        if (medir)
            return benchmark(video, cantidad, tamano, modo, p);
        EstadoInterfaz estado;
        estado.parametros = p;
        estado.modo = modo;
        estado.gpuDisponible = gpuDisponible;
        return visualizar(video, fuente, tamano, estado);
    } catch (const exception& error) {
        cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
