#pragma once

#include <opencv2/viz.hpp>
#include <stdexcept>

struct Vista3D {
    cv::viz::Viz3d visor;
    cv::viz::WImage3D original, efecto;

    Vista3D(const cv::Mat& frame, const cv::Mat& resultado, bool oculta = false)
        : visor("Spiderman | Video 3D | Q o Esc para cerrar esta vista"),
          original(frame, tamano(frame), cv::Vec3d(-0.98, 0, 0), cv::Vec3d(0, 0, 1), cv::Vec3d(0, 1, 0)),
          efecto(resultado, tamano(frame), cv::Vec3d(0.98, 0, 0), cv::Vec3d(0, 0, 1), cv::Vec3d(0, 1, 0)) {
        if (oculta)
            visor.setOffScreenRendering();
        visor.setWindowSize(cv::Size(1280, 720));
        visor.setBackgroundColor(cv::viz::Color(22, 22, 22));
        visor.setViewerPose(cv::Affine3d(cv::Matx33d::eye(), cv::Vec3d(0, 0, -5.6)));
        visor.showWidget("Original", original);
        visor.showWidget("Efecto", efecto);
        visor.registerKeyboardCallback(teclado, &visor);
    }

    ~Vista3D() { visor.close(); }

    bool actualizar(const cv::Mat& frame, const cv::Mat& resultado) {
        if (visor.wasStopped())
            return false;
        original.setImage(frame);
        efecto.setImage(resultado);
        // Process VTK events without stopping the camera/trackbar loop.
        visor.spinOnce(1, true);
        return !visor.wasStopped();
    }

private:
    static cv::Size2d tamano(const cv::Mat& imagen) {
        if (imagen.empty())
            throw std::invalid_argument("La textura 3D requiere una imagen no vacia");
        return cv::Size2d(1.85, 1.85 * imagen.rows / imagen.cols);
    }

    static void teclado(const cv::viz::KeyboardEvent& evento, void* datos) {
        if (evento.action == cv::viz::KeyboardEvent::KEY_DOWN &&
            (evento.code == 27 || evento.code == 'q' || evento.code == 'Q'))
            static_cast<cv::viz::Viz3d*>(datos)->close();
    }
};
