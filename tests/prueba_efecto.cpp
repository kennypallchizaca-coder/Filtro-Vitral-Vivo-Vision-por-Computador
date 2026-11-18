#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Principal.hpp"
#include <opencv2/core/ocl.hpp>
#include <cassert>
#include <iostream>

using namespace cv;
using namespace std;

// Asistencia de IA: OpenAI Codex añadió casos para símbolos, kernel y tamaños de salida.
int main() {
    assert(caracterAscii(0, 0) == ' ');
    assert(caracterAscii(255, 255) == '@');
    const string rampa(tonosAscii);
    for (int luz = 0; luz <= 255; ++luz) {
        size_t normal = rampa.find(caracterAscii(luz, 24));
        size_t borde = rampa.find(caracterAscii(luz, 25));
        assert(normal != string::npos && borde != string::npos);
        assert(borde == min(normal + 1, rampa.size() - 1));
    }

    Parametros p;
    Efecto<Mat> efecto;
    Mat original(48, 65, CV_8UC3);
    randu(original, 0, 256);
    Mat intacta = original.clone();
    efecto.procesar(original, p);
    Mat final = dibujarAscii(efecto.cuadricula(), original.size());
    assert(norm(original, intacta, NORM_INF) == 0);
    assert(final.size() == original.size() && final.type() == CV_8UC3);

    ocl::setUseOpenCL(false);
    UMat entrada;
    original.copyTo(entrada);
    Efecto<UMat> otraRuta;
    otraRuta.procesar(entrada, p);
    assert(norm(final, dibujarAscii(otraRuta.cuadricula(), entrada.size()), NORM_INF) == 0);

    Mat luz(Size(96, 72), CV_8UC3, Scalar::all(240)), brillo;
    efecto.procesar(luz, p);
    cvtColor(dibujarAscii(efecto.cuadricula(), luz.size()), brillo, COLOR_BGR2GRAY);
    assert(countNonZero(brillo < 35) > 0 && countNonZero(brillo > 100) > 0);
    assert(countNonZero(efecto.binaria) == 0);

    Mat escalon(72, 96, CV_8UC3, Scalar::all(0));
    escalon.colRange(48, 96).setTo(255);
    p.tamGauss = 3;
    p.sigma = 0.1;
    p.pesoPropio = 1;
    p.umbral = 20;
    efecto.procesar(escalon, p);
    CuadriculaAscii c = efecto.cuadricula();
    int celdasRealzadas = 0;
    for (int x = 0; x < c.luz.cols; ++x) {
        int y = c.luz.rows / 2;
        uchar brilloCelda = c.luz.at<uchar>(y, x);
        celdasRealzadas += caracterAscii(brilloCelda, c.bordes.at<uchar>(y, x)) !=
                          caracterAscii(brilloCelda, 0);
    }
    assert(celdasRealzadas > 0);
    p.umbral = 255;
    efecto.procesar(escalon, p);
    assert(countNonZero(efecto.cuadricula().bordes) == 0);

    Mat pequena(5, 7, CV_8UC3, Scalar::all(220));
    p.celda = 24;
    efecto.procesar(pequena, p);
    assert(efecto.cuadricula().luz.size() == Size(1, 1));
    assert(dibujarAscii(efecto.cuadricula(), pequena.size()).size() == pequena.size());
    cout << "ASCII: brillo, densidad de borde, kernel y dimensiones comprobados\n";
}
