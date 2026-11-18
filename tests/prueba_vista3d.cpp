#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Vista3D.hpp"
#include "Interfaz.hpp"
#include <cassert>
#include <iostream>
#include <opencv2/highgui.hpp>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

using namespace cv;
using namespace std;

BOOL CALLBACK contarVentanas(HWND ventana, LPARAM datos) {
    DWORD proceso = 0;
    GetWindowThreadProcessId(ventana, &proceso);
    if (proceso == GetCurrentProcessId() && IsWindowVisible(ventana))
        ++*reinterpret_cast<int*>(datos);
    return TRUE;
}

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
        // La misma escena debe aceptar una textura nueva sin reiniciar su cámara.
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
    {
        namedWindow("Panel prueba", WINDOW_AUTOSIZE);
        crearControles("Panel prueba", Parametros());
        assert(getTrackbarPos("Transformacion %", "Panel prueba") < 0);
        imshow("Panel prueba", Mat(380, 800, CV_8UC3, Scalar::all(240)));
        waitKey(1);
        Vista3D vista(rojo, verde);
        assert(vista.actualizar(rojo, verde));
        vista.integrar("Panel prueba", Rect(0, 80, 800, 300), Size(800, 380));
        assert(vista.actualizar(rojo, verde));
        assert(GetParent(vista.panel) == vista.padre);
        assert(GetWindowLongPtr(vista.panel, GWL_STYLE) & WS_CHILD);
        Rect marco = getWindowImageRect("Panel prueba");
        RECT panel{};
        assert(GetWindowRect(vista.panel, &panel));
        assert(abs(panel.left - marco.x) <= 1);
        assert(abs(panel.top - (marco.y + cvRound(80.0 * marco.height / 380))) <= 1);
        assert(abs((panel.right - panel.left) - marco.width) <= 1);
        assert(abs((panel.bottom - panel.top) - cvRound(300.0 * marco.height / 380)) <= 1);
        int visibles = 0;
        EnumWindows(contarVentanas, reinterpret_cast<LPARAM>(&visibles));
        assert(visibles == 1);
        assert(colorCentro(vista, -0.98)[2] > 200);
        assert(colorCentro(vista, 0.98)[1] > 200);
        setTrackbarPos("Kernel", "Panel prueba", 7);
        assert(getTrackbarPos("Kernel", "Panel prueba") == 7);
        EstadoInterfaz estado;
        for (int etapa = 1; etapa <= 4; ++etapa) {
            eventoTeclado('T', estado);
            assert(estado.etapa == etapa);
        }
        eventoTeclado('T', estado);
        assert(estado.etapa == 0);
        // Cerrar la ventana principal debe liberar OpenGL antes de destruir su hija.
        SendMessage(vista.padre, WM_CLOSE, 0, 0);
        assert(vista.visor.wasStopped());
    }
    cout << "VTK: textura dinamica, pose conservada y reapertura correcta\n";
}
