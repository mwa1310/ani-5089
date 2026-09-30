#include <iostream>
#include <string>

int main() {
    long long budget = 0;
    int s = 0;

    if (!(std::cin >> budget)) return 0;
    if (!(std::cin >> s)) s = 0;

    int trompe = 0;

    for (int i = 0; i < s; ++i) {
        std::string nom;
        long long debug = 0, release = 0;
        if (!(std::cin >> nom >> debug >> release)) break;

        long long facteur = 0;
        if (release != 0) {
            facteur = (debug + release / 2) / release;
        }

        bool tient = (release <= budget);

        std::cout << nom << " " << facteur << " "
                  << (tient ? "TIENT" : "DEPASSE") << "\n";

        if (debug > budget && release <= budget) {
            ++trompe;
        }
    }

    std::cout << "TROMPE " << trompe << "\n";

    return 0;
}