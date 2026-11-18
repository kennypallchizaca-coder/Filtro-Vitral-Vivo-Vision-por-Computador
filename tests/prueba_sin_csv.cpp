#ifdef NDEBUG
#undef NDEBUG
#endif
#define main mainAplicacion
#include "../Principal.cpp"
#undef main
#include <cassert>
#include <filesystem>

int ejecutar(vector<string> argumentos, string& texto) {
    vector<char*> argv;
    for (string& argumento : argumentos)
        argv.push_back(argumento.data());
    ostringstream captura;
    auto* anterior = cout.rdbuf(captura.rdbuf());
    int codigo = mainAplicacion(static_cast<int>(argv.size()), argv.data());
    cout.rdbuf(anterior);
    texto = captura.str();
    return codigo;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    string texto;
    if (string(argv[1]) == "ayuda") {
        assert(ejecutar({"vision", "--help"}, texto) == 0);
        assert(texto.find("CSV") == string::npos);
        assert(texto.find("Portal") == string::npos);
        assert(texto.find("textura VTK") != string::npos);
        cout << "Ayuda sin opciones de guardado CSV\n";
        return 0;
    }
    auto carpeta = filesystem::temp_directory_path() /
        ("vision-sin-csv-" + to_string(Reloj::now().time_since_epoch().count()));
    filesystem::create_directories(carpeta / "salidas");
    auto video = carpeta / "entrada.avi";
    {
        VideoWriter grabacion(video.string(), VideoWriter::fourcc('M', 'J', 'P', 'G'), 30,
                              Size(64, 48));
        assert(grabacion.isOpened());
        for (int i = 0; i < 20; ++i)
            grabacion.write(Mat(48, 64, CV_8UC3, Scalar(i * 10, 60, 140)));
    }
    auto anterior = filesystem::current_path();
    filesystem::current_path(carpeta / "salidas");
    int codigo = ejecutar({"vision", "--benchmark", "--source=" + video.string(), "--mode=cpu",
                            "--frames=3", "--width=64", "--height=48"}, texto);
    filesystem::current_path(anterior);
    assert(codigo == 0);
    assert(texto.find("Muestras: 3") != string::npos);
    assert(filesystem::is_empty(carpeta / "salidas"));
    auto salida = carpeta / "salidas" / "salida.avi";
    for (const string& opcion : {"--capture=" + salida.string(), string("--smoke"),
                                 "--output=" + salida.string()}) {
        assert(ejecutar({"vision", "--benchmark", "--source=" + video.string(),
                         "--mode=cpu", "--frames=3", opcion}, texto) == 1);
        assert(filesystem::is_empty(carpeta / "salidas"));
    }
    cout << "Mediciones en consola, sin archivos; entrada temporal: " << carpeta << '\n';
}
