#include <iostream>
#include <string>

int main() {
    long long rendu = 0;
    long long deformation = 0;
    long long reprojection = 0;
    long long seuil = 0;
    if (!(std::cin >> rendu >> deformation >> reprojection >> seuil)) {
        return 0;
    }

    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }

    int gagnes = 0;
    int perdus = 0;

    for (int i = 0; i < n; ++i) {
        std::string nom;
        long long pixels = 0;
        if (!(std::cin >> nom >> pixels)) {
            break;
        }

        long long scene = pixels * rendu / 1000;
        scene = scene * deformation / 1000;
        scene = scene * reprojection / 1000;

        bool lisibleScene = scene >= seuil;
        bool lisibleQuad = pixels >= seuil;

        if (!lisibleScene && lisibleQuad) {
            ++gagnes;
        }
        if (lisibleScene && !lisibleQuad) {
            ++perdus;
        }

        std::cout << nom << " " << scene << " " << pixels << " "
                  << (lisibleScene ? "LISIBLE" : "FLOU") << " "
                  << (lisibleQuad ? "LISIBLE" : "FLOU") << "\n";
    }

    std::cout << "GAGNES " << gagnes << "\n";
    std::cout << "PERDUS " << perdus << "\n";
    return 0;
}
