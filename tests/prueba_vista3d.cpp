#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Vista3D.hpp"
#include <cassert>
#include <iostream>

using namespace cv;
using namespace std;

Vec3b colorCentro(Vista3D& vista, double xMundo) {
    Point3d punto;
    vista.visor.convertToWindowCoordinates(Point3d(xMundo, 0, 0), punto);
    Mat captura = vista.visor.getScreenshot();
    assert(!captura.empty());
    int x = cvRound(punto.x), y = cvRound(punto.y);
    assert(x >= 0 && x < captura.cols && y >= 0 && y < captura.rows);
    return captura.at<Vec3b>(y, x);
}

int main() {
    Mat rojo(48, 64, CV_8UC3, Scalar(0, 0, 255));
    Mat verde(48, 64, CV_8UC3, Scalar(0, 255, 0));
    {
        Vista3D vista(rojo, rojo, true);
        assert(vista.actualizar(rojo, rojo));
        Vec3b antes = colorCentro(vista, -0.98);
        assert(antes[2] > 200 && antes[1] < 40);
        assert(colorCentro(vista, 0.98)[2] > 200);
        assert(vista.actualizar(verde, verde));
        Vec3b despues = colorCentro(vista, -0.98);
        assert(despues[1] > 200 && despues[2] < 40);
        assert(colorCentro(vista, 0.98)[1] > 200);
        // The same scene must accept a new texture without resetting its camera.
        Affine3d pose = vista.visor.getViewerPose();
        assert(vista.actualizar(rojo, verde));
        assert(colorCentro(vista, -0.98)[2] > 200);
        assert(colorCentro(vista, 0.98)[1] > 200);
        assert(norm(Mat(pose.matrix), Mat(vista.visor.getViewerPose().matrix)) == 0);
        Affine3d movida(Matx33d::eye(), Vec3d(0.1, 0, -5.6));
        vista.visor.setViewerPose(movida);
        assert(vista.actualizar(rojo, verde));
        assert(norm(Mat(movida.matrix), Mat(vista.visor.getViewerPose().matrix)) < 1e-9);
        vista.visor.close();
        assert(!vista.actualizar(rojo, rojo));
    }
    {
        Vista3D reabierta(verde, rojo, true);
        assert(reabierta.actualizar(verde, rojo));
        assert(colorCentro(reabierta, -0.98)[1] > 200);
        assert(colorCentro(reabierta, 0.98)[2] > 200);
    }
    cout << "VTK: textura roja/verde actualizada, pose conservada y reapertura correcta\n";
}
