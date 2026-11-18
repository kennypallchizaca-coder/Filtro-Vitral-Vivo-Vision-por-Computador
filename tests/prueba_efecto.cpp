#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Principal.hpp"
#include <opencv2/core/ocl.hpp>
#include <cassert>
#include <iostream>

using namespace cv;
using namespace std;

// Asistencia de IA: OpenAI Codex añadió casos para color, malla estable y bordes.
int main() {
    Parametros p;
    Efecto<Mat> efecto;
    Mat original(48, 65, CV_8UC3);
    randu(original, 0, 256);
    Mat intacta = original.clone();
    efecto.procesar(original, p);
    Mat final = dibujarVitral(efecto.cuadricula(), original.size());
    assert(norm(original, intacta, NORM_INF) == 0);
    assert(final.size() == original.size() && final.type() == CV_8UC3);

    ocl::setUseOpenCL(false);
    UMat entrada;
    original.copyTo(entrada);
    Efecto<UMat> otraRuta;
    otraRuta.procesar(entrada, p);
    assert(norm(final, dibujarVitral(otraRuta.cuadricula(), entrada.size()), NORM_INF) == 0);

    Mat rojo(Size(240, 160), CV_8UC3, Scalar(40, 70, 240));
    efecto.procesar(rojo, p);
    Mat mosaicoRojo = dibujarVitral(efecto.cuadricula(), rojo.size());
    Mat rojoInterior, plomoRojo;
    inRange(mosaicoRojo, Scalar(40, 70, 240), Scalar(40, 70, 240), rojoInterior);
    inRange(mosaicoRojo, Scalar::all(0), Scalar::all(32), plomoRojo);
    assert(countNonZero(rojoInterior) > rojo.total() / 2);
    assert(countNonZero(plomoRojo) > 0);
    assert(countNonZero(efecto.binaria) == 0);
    Mat verde(rojo.size(), CV_8UC3, Scalar(40, 240, 70)), plomoVerde;
    efecto.procesar(verde, p);
    inRange(dibujarVitral(efecto.cuadricula(), verde.size()), Scalar::all(0), Scalar::all(32), plomoVerde);
    assert(norm(plomoRojo, plomoVerde, NORM_INF) == 0);

    Mat escalon(480, 640, CV_8UC3, Scalar::all(0));
    escalon.colRange(317, escalon.cols).setTo(255);
    p.pesoPropio = 1;
    efecto.procesar(escalon, p);
    assert(countNonZero(efecto.binaria) > 0);
    Mat detallado = dibujarVitral(efecto.cuadricula(), escalon.size());
    p.umbral = 255;
    efecto.procesar(escalon, p);
    assert(countNonZero(efecto.cuadricula().bordes) == 0);
    assert(norm(detallado, dibujarVitral(efecto.cuadricula(), escalon.size()), NORM_INF) > 0);

    Mat pequena(5, 7, CV_8UC3, Scalar::all(220));
    p.celda = 24;
    efecto.procesar(pequena, p);
    assert(efecto.cuadricula().color.size() == Size(1, 1));
    assert(dibujarVitral(efecto.cuadricula(), pequena.size()).size() == pequena.size());
    cout << "Vitral Vivo: color, malla estable, detalle y dimensiones comprobados\n";
}
