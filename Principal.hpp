#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

// Asistencia de IA: OpenAI Codex adaptó los filtros de clase al efecto ASCII y a Mat/UMat.
struct Parametros {
    int umbral = 40;
    int tamGauss = 5;
    double sigma = 1.73;
    double pesoPropio = 0.3;
    double diagonal = 1.0;
    int celda = 9;
    double contraste = 1.5;

    void validar() const {
        if (umbral < 0 || umbral > 255)
            throw std::invalid_argument("Umbral: 0..255");
        if (tamGauss < 3 || tamGauss > 51 || tamGauss % 2 == 0)
            throw std::invalid_argument("Tamano Gaussiano: impar entre 3 y 51");
        if (!(sigma >= 0.1 && sigma <= 10))
            throw std::invalid_argument("Sigma: 0.1..10");
        if (!(pesoPropio >= 0 && pesoPropio <= 1))
            throw std::invalid_argument("Peso del filtro propio: 0..1");
        if (!(diagonal >= 0 && diagonal <= 3))
            throw std::invalid_argument("Diagonal: 0..3");
        if (celda < 6 || celda > 24)
            throw std::invalid_argument("Celda ASCII: 6..24");
        if (!(contraste >= 0.1 && contraste <= 8))
            throw std::invalid_argument("Contraste CLAHE: 0.1..8");
    }
};

// La suma cero elimina regiones uniformes y deja las transiciones de intensidad.
inline cv::Mat crearKernel(double diagonal) {
    if (!(diagonal >= 0 && diagonal <= 3))
        throw std::invalid_argument("Diagonal: 0..3");
    float a = static_cast<float>(diagonal);
    return cv::Mat_<float>({3, 3}, {-a, -1, 0, -1, 4 + 2 * a, -1, 0, -1, -a}) / (4 + 2 * a);
}

struct CuadriculaAscii {
    cv::Mat luz, bordes, dx, dy;
    int celda;
};

inline constexpr char tonosAscii[] = " .:-=+*#%@";

inline char caracterAscii(uchar luz, uchar borde, int dx, int dy) {
    // El carácter apunta a lo largo del borde, perpendicular al gradiente.
    if (borde >= 25 && (dx != 0 || dy != 0)) {
        if (std::abs(dx) > 2 * std::abs(dy)) return '|';
        if (std::abs(dy) > 2 * std::abs(dx)) return '-';
        return dx * dy > 0 ? '/' : '\\';
    }
    return tonosAscii[(luz * (sizeof(tonosAscii) - 2) + 127) / 255];
}

inline cv::Mat dibujarAscii(const CuadriculaAscii& c, cv::Size tamano) {
    int ancho = c.celda, alto = c.celda * 3 / 2;
    const cv::Scalar fondo(16, 23, 16), tinta(164, 249, 177);
    const std::string caracteres = std::string(tonosAscii) + "|/\\";
    const int fuente = cv::FONT_HERSHEY_PLAIN;
    double escala = (ancho - 1) / 13.0;
    std::vector<cv::Mat> letras;
    // Una imagen por símbolo, no una llamada a putText por cada casilla.
    for (char letra : caracteres) {
        cv::Mat ficha(alto, ancho, CV_8UC3, fondo);
        std::string texto(1, letra);
        int base;
        cv::Size medida = cv::getTextSize(texto, fuente, escala, 1, &base);
        cv::putText(ficha, texto, cv::Point((ancho - medida.width) / 2,
                    (alto + medida.height - base) / 2), fuente, escala, tinta, 1, cv::LINE_AA);
        letras.push_back(ficha);
    }

    cv::Mat mosaico(c.luz.rows * alto, c.luz.cols * ancho, CV_8UC3);
    for (int y = 0; y < c.luz.rows; ++y) {
        for (int x = 0; x < c.luz.cols; ++x) {
            char letra = caracterAscii(c.luz.at<uchar>(y, x), c.bordes.at<uchar>(y, x),
                                       c.dx.at<short>(y, x), c.dy.at<short>(y, x));
            letras[caracteres.find(letra)].copyTo(mosaico(cv::Rect(x * ancho, y * alto, ancho, alto)));
        }
    }
    if (mosaico.size() != tamano)
        cv::resize(mosaico, mosaico, tamano, 0, 0, cv::INTER_NEAREST);
    return mosaico;
}

// Mat y UMat conservan los mismos filtros y la misma cuadrícula de salida.
template <class Imagen>
struct Efecto {
    Imagen gris, mediana, imgGauss, gradiente, filtroPropioAbs, binaria;

    void procesar(const Imagen& frame, const Parametros& p) {
        p.validar();
        if (frame.empty() || frame.type() != CV_8UC3)
            throw std::invalid_argument("Se requiere una imagen BGR 8UC3 no vacia");
        if (p.diagonal != diagonalAnterior) {
            kernelPropio = crearKernel(p.diagonal);
            diagonalAnterior = p.diagonal;
        }
        if (p.contraste != contrasteAnterior) {
            clahe->setClipLimit(p.contraste);
            contrasteAnterior = p.contraste;
        }

        cv::cvtColor(frame, gris, cv::COLOR_BGR2GRAY);
        cv::medianBlur(gris, mediana, 3);
        clahe->apply(mediana, gris);
        detectarContornos(p);

        anchoCelda = p.celda;
        cv::Size celdas(std::max(1, frame.cols / p.celda),
                       std::max(1, frame.rows / (p.celda * 3 / 2)));
        cv::resize(gris, luzCeldas, celdas, 0, 0, cv::INTER_AREA);
        cv::resize(binaria, bordeCeldas, celdas, 0, 0, cv::INTER_AREA);
        cv::resize(gx, dxCeldas, celdas, 0, 0, cv::INTER_AREA);
        cv::resize(gy, dyCeldas, celdas, 0, 0, cv::INTER_AREA);
    }

    CuadriculaAscii cuadricula() const {
        CuadriculaAscii c;
        c.celda = anchoCelda;
        // Desde OpenCL solo se leen las celdas, no una imagen completa.
        luzCeldas.copyTo(c.luz);
        bordeCeldas.copyTo(c.bordes);
        dxCeldas.copyTo(c.dx);
        dyCeldas.copyTo(c.dy);
        return c;
    }

private:
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    cv::Mat kernelPropio;
    Imagen gx, gy, gxpos, gypos, filtroPropio, luzCeldas, bordeCeldas, dxCeldas, dyCeldas;
    double diagonalAnterior = -1, contrasteAnterior = -1;
    int anchoCelda = 9;

    // Gaussiano y Sobel siguen el Ejemplo 6 de la Unidad 2; el kernel pesa en la máscara.
    void detectarContornos(const Parametros& p) {
        cv::GaussianBlur(gris, imgGauss, cv::Size(p.tamGauss, p.tamGauss), p.sigma, p.sigma);
        cv::Sobel(imgGauss, gx, CV_16S, 1, 0);
        cv::Sobel(imgGauss, gy, CV_16S, 0, 1);
        cv::convertScaleAbs(gx, gxpos);
        cv::convertScaleAbs(gy, gypos);
        cv::addWeighted(gxpos, 0.5, gypos, 0.5, 0, gradiente);

        cv::filter2D(imgGauss, filtroPropio, CV_32F, kernelPropio);
        cv::convertScaleAbs(filtroPropio, filtroPropioAbs);
        cv::addWeighted(gradiente, 1 - p.pesoPropio, filtroPropioAbs, p.pesoPropio, 0, gradiente);
        cv::threshold(gradiente, binaria, p.umbral, 255, cv::THRESH_BINARY);
        cv::medianBlur(binaria, binaria, 3);
    }
};
