#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <stdexcept>
#include <vector>

// Asistencia de IA: OpenAI Codex adaptó los filtros de clase a Vitral Vivo y a Mat/UMat.
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
            throw std::invalid_argument("Escala del vitral: 6..24");
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

struct CuadriculaVitral {
    cv::Mat color, bordes;
    int celda;
};

inline cv::Mat dibujarVitral(const CuadriculaVitral& c, cv::Size tamano) {
    cv::Mat vitral;
    if (tamano.width < 3 || tamano.height < 3) {
        cv::resize(c.color, vitral, tamano, 0, 0, cv::INTER_NEAREST);
        return vitral;
    }
    const int lado = 3 * c.celda;
    const int nx = std::max(1, (tamano.width + lado / 2) / lado);
    const int ny = std::max(1, (tamano.height + lado / 2) / lado);
    const int radio = std::min((tamano.width - 1) / nx, (tamano.height - 1) / ny) / 4;
    const cv::Scalar plomo(17, 14, 12);
    vitral = cv::Mat(tamano, CV_8UC3, plomo);
    std::vector<cv::Point> puntos((nx + 1) * (ny + 1));
    auto punto = [&](int x, int y) -> cv::Point& { return puntos[y * (nx + 1) + x]; };
    // El desplazamiento depende solo de las coordenadas; el video nunca mueve los vértices.
    for (int y = 0; y <= ny; ++y) {
        for (int x = 0; x <= nx; ++x) {
            unsigned h = 0x9e3779b9u * x ^ 0x85ebca6bu * y;
            h = (h ^ (h >> 16)) * 0x7feb352du;
            const int dx = x && x < nx ? int(h % (2 * radio + 1)) - radio : 0;
            const int dy = y && y < ny ? int((h >> 16) % (2 * radio + 1)) - radio : 0;
            punto(x, y) = {x * (tamano.width - 1) / nx + dx, y * (tamano.height - 1) / ny + dy};
        }
    }
    auto muestra = [&](cv::Point p) {
        return cv::Point(p.x * c.color.cols / tamano.width, p.y * c.color.rows / tamano.height);
    };
    auto color = [&](cv::Point p) {
        const cv::Vec3b bgr = c.color.at<cv::Vec3b>(p);
        return cv::Scalar(bgr[0], bgr[1], bgr[2]);
    };
    auto cara = [&](cv::Point a, cv::Point b, cv::Point d) {
        const cv::Point v[] = {a, b, d};
        const cv::Point centro((a.x + b.x + d.x) / 3, (a.y + b.y + d.y) / 3);
        const cv::Point central = muestra(centro);
        const cv::Scalar base = color(central);
        cv::fillConvexPoly(vitral, v, 3, base);
        cv::Point muestras[3];
        int detalle = c.bordes.at<uchar>(central);
        for (int i = 0; i < 3; ++i) {
            const cv::Point suma = v[i] + v[(i + 1) % 3] + centro;
            muestras[i] = muestra({suma.x / 3, suma.y / 3});
            detalle = std::max(detalle, int(c.bordes.at<uchar>(muestras[i])));
        }
        if (detalle == 0 || centro == a || centro == b || centro == d)
            return;
        // Las caras internas ya existen: el borde solo aumenta su contraste, sin moverlas.
        const double intensidad = std::min(detalle / 56.0, 1.0);
        for (int i = 0; i < 3; ++i) {
            const cv::Point interior[] = {v[i], v[(i + 1) % 3], centro};
            cv::fillConvexPoly(vitral, interior, 3, base * (1 - intensidad) + color(muestras[i]) * intensidad);
        }
        for (auto p : v)
            cv::line(vitral, p, centro, base * (1 - intensidad) + plomo * intensidad);
    };
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            auto a = punto(x, y), b = punto(x + 1, y), d = punto(x, y + 1), e = punto(x + 1, y + 1);
            if ((x + y) % 2) {
                cara(a, b, d);
                cara(b, e, d);
            } else {
                cara(a, b, e);
                cara(a, e, d);
            }
        }
    }
    for (int y = 0; y <= ny; ++y)
        for (int x = 0; x <= nx; ++x) {
            if (x < nx) cv::line(vitral, punto(x, y), punto(x + 1, y), plomo);
            if (y < ny) cv::line(vitral, punto(x, y), punto(x, y + 1), plomo);
            if (x < nx && y < ny)
                cv::line(vitral, punto(x + (x + y) % 2, y), punto(x + (1 - (x + y) % 2), y + 1), plomo);
        }
    return vitral;
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
        cv::Size celdas(std::max(1, frame.cols / p.celda), std::max(1, frame.rows / p.celda));
        cv::resize(frame, colorCeldas, celdas, 0, 0, cv::INTER_AREA);
        cv::resize(binaria, bordeCeldas, celdas, 0, 0, cv::INTER_AREA);
    }

    CuadriculaVitral cuadricula() const {
        CuadriculaVitral c;
        c.celda = anchoCelda;
        // Desde OpenCL solo se leen las celdas, no una imagen completa.
        colorCeldas.copyTo(c.color);
        bordeCeldas.copyTo(c.bordes);
        return c;
    }

private:
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    cv::Mat kernelPropio;
    Imagen gx, gy, gxpos, gypos, filtroPropio, colorCeldas, bordeCeldas;
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
        // Ganancia fija: alinea el kernel normalizado con Sobel antes del umbral común.
        cv::convertScaleAbs(filtroPropio, filtroPropioAbs, 12.0);
        cv::addWeighted(gradiente, 1 - p.pesoPropio, filtroPropioAbs, p.pesoPropio, 0, gradiente);
        cv::threshold(gradiente, binaria, p.umbral, 255, cv::THRESH_BINARY);
        cv::medianBlur(binaria, binaria, 3);
    }
};
