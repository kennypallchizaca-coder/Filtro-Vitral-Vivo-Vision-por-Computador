#pragma once

#include "Principal.hpp"
#include <opencv2/highgui.hpp>
#include <iomanip>
#include <sstream>
#include <string>

using namespace std;
using namespace cv;

inline string numero(double valor, int decimales = 2) {
    ostringstream texto;
    texto << fixed << setprecision(decimales) << valor;
    return texto.str();
}

struct EstadoInterfaz {
    Parametros parametros;
    int modo = 2, etapa = 0;
    bool gpuDisponible = false, salir = false;
};

inline void eventoTrack(int, void*) {
}

// Crea las barras y sus valores iniciales, siguiendo los Ejemplos 3 y 7.
inline void crearControles(const string& ventana, const Parametros& p) {
    createTrackbar("Kernel", ventana, nullptr, 51, eventoTrack, nullptr);
    createTrackbar("Sigma x100", ventana, nullptr, 1000, eventoTrack, nullptr);
    createTrackbar("Umbral", ventana, nullptr, 255, eventoTrack, nullptr);
    createTrackbar("Peso propio %", ventana, nullptr, 100, eventoTrack, nullptr);
    createTrackbar("Diagonal x100", ventana, nullptr, 300, eventoTrack, nullptr);
    createTrackbar("Radio puntos", ventana, nullptr, 3, eventoTrack, nullptr);
    createTrackbar("Niveles", ventana, nullptr, 16, eventoTrack, nullptr);
    createTrackbar("Saturacion x100", ventana, nullptr, 300, eventoTrack, nullptr);
    createTrackbar("Brillo x100", ventana, nullptr, 300, eventoTrack, nullptr);
    createTrackbar("Neon x100", ventana, nullptr, 300, eventoTrack, nullptr);
    createTrackbar("Glitch px", ventana, nullptr, 3, eventoTrack, nullptr);
    createTrackbar("CLAHE x100", ventana, nullptr, 800, eventoTrack, nullptr);

    setTrackbarMin("Kernel", ventana, 3);
    setTrackbarMin("Sigma x100", ventana, 10);
    setTrackbarMin("Niveles", ventana, 2);
    setTrackbarMin("CLAHE x100", ventana, 10);

    setTrackbarPos("Kernel", ventana, p.tamGauss);
    setTrackbarPos("Sigma x100", ventana, cvRound(p.sigma * 100));
    setTrackbarPos("Umbral", ventana, p.umbral);
    setTrackbarPos("Peso propio %", ventana, cvRound(p.pesoPropio * 100));
    setTrackbarPos("Diagonal x100", ventana, cvRound(p.diagonal * 100));
    setTrackbarPos("Radio puntos", ventana, p.radioPuntos);
    setTrackbarPos("Niveles", ventana, p.niveles);
    setTrackbarPos("Saturacion x100", ventana, cvRound(p.saturacion * 100));
    setTrackbarPos("Brillo x100", ventana, cvRound(p.brillo * 100));
    setTrackbarPos("Neon x100", ventana, cvRound(p.neon * 100));
    setTrackbarPos("Glitch px", ventana, p.desplazamiento);
    setTrackbarPos("CLAHE x100", ventana, cvRound(p.contraste * 100));
}

// Convierte las posiciones enteras de las barras a parámetros del filtro.
inline Parametros leerControles(const string& ventana, int kernelAnterior) {
    Parametros p;
    int tam = getTrackbarPos("Kernel", ventana);
    if (tam % 2 == 0) {
        // El Gaussiano exige impares. Saltamos el par en la dirección del ajuste.
        tam += tam < kernelAnterior ? -1 : 1;
        setTrackbarPos("Kernel", ventana, tam);
    }
    p.tamGauss = tam;
    p.sigma = getTrackbarPos("Sigma x100", ventana) / 100.0;
    p.umbral = getTrackbarPos("Umbral", ventana);
    p.pesoPropio = getTrackbarPos("Peso propio %", ventana) / 100.0;
    p.diagonal = getTrackbarPos("Diagonal x100", ventana) / 100.0;
    p.radioPuntos = getTrackbarPos("Radio puntos", ventana);
    p.niveles = getTrackbarPos("Niveles", ventana);
    p.saturacion = getTrackbarPos("Saturacion x100", ventana) / 100.0;
    p.brillo = getTrackbarPos("Brillo x100", ventana) / 100.0;
    p.neon = getTrackbarPos("Neon x100", ventana) / 100.0;
    p.desplazamiento = getTrackbarPos("Glitch px", ventana);
    p.contraste = getTrackbarPos("CLAHE x100", ventana) / 100.0;
    return p;
}

inline void eventoTeclado(int tecla, EstadoInterfaz& estado) {
    if (tecla == 27 || tecla == 'q' || tecla == 'Q')
        estado.salir = true;
    if (tecla == ' ')
        estado.modo = estado.gpuDisponible ? (estado.modo + 1) % 3 : 0;
    if (tecla == 't' || tecla == 'T')
        estado.etapa = (estado.etapa + 1) % 5;
}
