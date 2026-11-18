#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <opencv2/highgui.hpp>
#include <opencv2/viz.hpp>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

struct Vista3D {
    cv::Mat lienzo;
    cv::viz::Viz3d visor;
    cv::viz::WImage3D imagen;
    HWND padre = nullptr, panel = nullptr;
    int tecla = -1;
    cv::Affine3d poseInicial;

    Vista3D(const cv::Mat& frame, const cv::Mat& resultado, bool oculta = false)
        : visor("ASCII 3D"),
          imagen(componer(frame, resultado), tamano(frame), cv::Vec3d(0, 0, 0), cv::Vec3d(0, 0, 1),
                 cv::Vec3d(0, 1, 0)) {
        if (oculta)
            visor.setOffScreenRendering();
        else
            visor.setWindowPosition(cv::Point(-32000, -32000));
        visor.setWindowSize(cv::Size(1280, 720));
        visor.setBackgroundColor(cv::viz::Color(240, 240, 240));
        poseInicial = cv::Affine3d(cv::Matx33d::eye(), cv::Vec3d(0, 0, -5.6));
        visor.setViewerPose(poseInicial);
        visor.showWidget("Comparacion", imagen);
        visor.registerKeyboardCallback(teclado, this);
    }

    Vista3D(const Vista3D&) = delete;
    Vista3D& operator=(const Vista3D&) = delete;

    ~Vista3D() {
        if (padre && IsWindow(padre))
            RemoveWindowSubclass(padre, cerrarPadre, reinterpret_cast<UINT_PTR>(this));
        visor.close();
    }

    // Actualiza la textura y atiende VTK sin detener el ciclo de cámara.
    bool actualizar(const cv::Mat& frame, const cv::Mat& resultado) {
        if (visor.wasStopped())
            return false;
        imagen.setImage(componer(frame, resultado));
        visor.spinOnce(1, true);
        return !visor.wasStopped();
    }

    // Coloca el visor dentro de Lienzo y encuadra la textura sin deformarla.
    void integrar(const std::string& nombre, const cv::Rect& region, cv::Size tamanoLienzo) {
        bool nueva = padre == nullptr;
        if (nueva) {
            padre = buscarVentana(nombre);
            panel = buscarVentana(visor.getWindowName());
            if (!padre || !panel)
                throw std::runtime_error("No se encontro la ventana para integrar VTK");
            auto estilo = GetWindowLongPtr(panel, GWL_STYLE);
            SetWindowLongPtr(panel, GWL_STYLE,
                             (estilo & ~(WS_POPUP | WS_OVERLAPPEDWINDOW)) | WS_CHILD);
            SetWindowLongPtr(panel, GWL_EXSTYLE,
                             GetWindowLongPtr(panel, GWL_EXSTYLE) & ~WS_EX_APPWINDOW);
            SetLastError(0);
            if (!SetParent(panel, padre) && GetLastError())
                throw std::runtime_error("No se pudo integrar VTK en Lienzo");
            SetWindowLongPtr(padre, GWL_STYLE,
                             GetWindowLongPtr(padre, GWL_STYLE) | WS_CLIPCHILDREN);
            if (!SetWindowSubclass(padre, cerrarPadre, reinterpret_cast<UINT_PTR>(this),
                                   reinterpret_cast<DWORD_PTR>(this)))
                throw std::runtime_error("No se pudo proteger el cierre del panel VTK");
        }
        cv::Rect marco = cv::getWindowImageRect(nombre);
        double sx = double(marco.width) / tamanoLienzo.width;
        double sy = double(marco.height) / tamanoLienzo.height;
        POINT origen{marco.x + cvRound(region.x * sx), marco.y + cvRound(region.y * sy)};
        ScreenToClient(padre, &origen);
        cv::Rect posicion(origen.x, origen.y, std::max(1, cvRound(region.width * sx)),
                          std::max(1, cvRound(region.height * sy)));
        if (posicion != posicionAnterior) {
            if (!SetWindowPos(panel, HWND_TOP, posicion.x, posicion.y, posicion.width,
                              posicion.height, SWP_NOACTIVATE | SWP_FRAMECHANGED))
                throw std::runtime_error("No se pudo colocar el panel VTK");
            visor.setWindowSize(posicion.size());
            posicionAnterior = posicion;
        }
        if (nueva) {
            double alto =
                std::max(3.7 * lienzo.rows / lienzo.cols, 3.7 * posicion.height / posicion.width);
            double distancia = 1.08 * alto / (2 * std::tan(visor.getCamera().getFov()[1] / 2));
            poseInicial = cv::Affine3d(cv::Matx33d::eye(), cv::Vec3d(0, 0, -distancia));
            visor.setViewerPose(poseInicial);
            visor.spinOnce(1, true);
        }
    }

   private:
    cv::Rect posicionAnterior;

    // Une original y efecto con regiones Rect, como en los Ejemplos 6 y 7.
    cv::Mat& componer(const cv::Mat& frame, const cv::Mat& resultado) {
        if (frame.empty() || frame.type() != CV_8UC3 || resultado.type() != frame.type() ||
            resultado.size() != frame.size())
            throw std::invalid_argument(
                "La comparacion 3D requiere dos imagenes BGR del mismo tamano");
        lienzo.create(frame.rows, frame.cols * 2, CV_8UC3);
        frame.copyTo(lienzo(cv::Rect(0, 0, frame.cols, frame.rows)));
        resultado.copyTo(lienzo(cv::Rect(frame.cols, 0, frame.cols, frame.rows)));
        return lienzo;
    }

    static cv::Size2d tamano(const cv::Mat& imagen) {
        if (imagen.empty())
            throw std::invalid_argument("La textura 3D requiere una imagen no vacia");
        return cv::Size2d(3.7, 1.85 * imagen.rows / imagen.cols);
    }

    static void teclado(const cv::viz::KeyboardEvent& evento, void* datos) {
        if (evento.action == cv::viz::KeyboardEvent::KEY_DOWN)
            static_cast<Vista3D*>(datos)->tecla = evento.code;
    }

    static HWND buscarVentana(const std::string& nombre) {
        struct Busqueda {
            const char* nombre;
            HWND ventana;
        } busqueda{nombre.c_str(), nullptr};
        EnumWindows(
            [](HWND ventana, LPARAM datos) -> BOOL {
                auto& b = *reinterpret_cast<Busqueda*>(datos);
                DWORD proceso = 0;
                char titulo[512]{};
                GetWindowThreadProcessId(ventana, &proceso);
                GetWindowTextA(ventana, titulo, sizeof(titulo));
                if (proceso == GetCurrentProcessId() && b.nombre == std::string(titulo)) {
                    b.ventana = ventana;
                    return FALSE;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&busqueda));
        return busqueda.ventana;
    }

    // Libera OpenGL antes de que Windows destruya la ventana principal y su hija.
    static LRESULT CALLBACK cerrarPadre(HWND ventana, UINT mensaje, WPARAM w, LPARAM l, UINT_PTR id,
                                        DWORD_PTR datos) {
        auto& vista = *reinterpret_cast<Vista3D*>(datos);
        if (mensaje == WM_CLOSE || mensaje == WM_DESTROY) {
            try {
                vista.visor.close();
            } catch (const cv::Exception&) {
            }
        }
        if (mensaje == WM_NCDESTROY) {
            vista.padre = nullptr;
            RemoveWindowSubclass(ventana, cerrarPadre, id);
        }
        return DefSubclassProc(ventana, mensaje, w, l);
    }
};
