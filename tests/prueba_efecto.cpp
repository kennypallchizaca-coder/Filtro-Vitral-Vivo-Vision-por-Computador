#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Principal.hpp"
#include <cassert>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    Mat original(48, 65, CV_8UC3);
    randu(original, 0, 256);
    Mat intacta = original.clone();
    Efecto<Mat> efecto;
    Mat final = efecto.procesar(original, Parametros()).clone();
    assert(norm(original, intacta, NORM_INF) == 0);
    assert(final.size() == original.size());

    // Los puntos no deben oscurecer toda la piel ni cubrir las zonas claras.
    Mat gris(24, 24, CV_8UC1, Scalar(100));
    Mat trama(24, 24, CV_8UC3, Scalar::all(100));
    efecto.aplicarTrama(gris, trama, 3);
    double minimo;
    minMaxLoc(trama.reshape(1), &minimo);
    assert(minimo >= 80 && minimo < 100);
    gris.setTo(160);
    trama.setTo(100);
    efecto.aplicarTrama(gris, trama, 3);
    assert(countNonZero(trama.reshape(1) != 100) == 0);

    // Fuera de los contornos, desplazar RGB no debe cambiar la imagen.
    Mat sinBordes = Mat::zeros(original.size(), CV_8UC1);
    Mat glitch = original.clone();
    efecto.aplicarGlitch(glitch, 2, sinBordes);
    assert(norm(glitch, original, NORM_INF) == 0);
    Mat borde = sinBordes.clone();
    borde.col(32).setTo(255);
    efecto.aplicarGlitch(glitch, 2, borde);
    assert(norm(glitch.colRange(0, 30), original.colRange(0, 30), NORM_INF) == 0);
    assert(norm(glitch.colRange(35, 65), original.colRange(35, 65), NORM_INF) == 0);
    assert(norm(glitch.colRange(30, 35), original.colRange(30, 35), NORM_INF) > 0);

    Mat mascara;
    threshold(efecto.gradiente, mascara, 40, 255, THRESH_BINARY);
    medianBlur(mascara, mascara, 3);
    assert(norm(mascara, efecto.binaria, NORM_INF) == 0);
    cout << "Trama suave, RGB selectivo y contornos comprobados\n";
}
