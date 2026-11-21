#include <iostream>
#include <string>

int main() {
    long long hauteur = 0;
    int n = 0;
    if (!(std::cin >> hauteur >> n)) {
        return 0;
    }

    int decalees = 0;

    for (int i = 0; i < n; ++i) {
        std::string nom;
        std::string pose;
        std::string couche;
        if (!(std::cin >> nom >> pose >> couche)) {
            break;
        }

        long long decalage = 0;
        const char* verdict = "OK";

        if (pose != couche) {
            if (pose == "STAGE") {
                decalage = hauteur;
                verdict = "LEVITATION";
            } else {
                decalage = -hauteur;
                verdict = "ENTERRE";
            }
            ++decalees;
        }

        std::cout << nom << " " << decalage << " " << verdict << "\n";
    }

    std::cout << "DECALEES " << decalees << "\n";
    return 0;
}
