#include <iostream>
#include <string>

int main() {
    int n = 0;
    if (!(std::cin >> n)) {
        return 0;
    }

    int refusees = 0;
    int averties = 0;

    for (int i = 0; i < n; ++i) {
        std::string nom;
        int commence = 0;
        int rend = 0;
        int gauche = 0;
        int droite = 0;
        if (!(std::cin >> nom >> commence >> rend >> gauche >> droite)) {
            break;
        }

        const char* verdict = "OK";
        if (commence == 0) {
            verdict = "REFUSEE";
            ++refusees;
        } else if (rend == 0) {
            if (gauche != 0 || droite != 0) {
                verdict = "COUCHE SANS RENDU";
                ++averties;
            }
        } else if (gauche == 0 && droite == 0) {
            verdict = "SOUMISSION VIDE";
            ++averties;
        } else if (gauche == 0) {
            verdict = "OEIL NOIR G";
            ++averties;
        } else if (droite == 0) {
            verdict = "OEIL NOIR D";
            ++averties;
        }

        std::cout << nom << " " << verdict << "\n";
    }

    std::cout << "REFUSEES " << refusees << "\n";
    std::cout << "AVERTIES " << averties << "\n";
    return 0;
}
