#include <iostream>
#include <string>

enum Etat { REPOS, ATTENDUE, COMMENCEE };

int main() {
    int n = 0;
    if (!(std::cin >> n)) {
        return 0;
    }

    Etat etat = REPOS;
    int fautes = 0;
    int bloque = 0;

    for (int i = 1; i <= n; ++i) {
        std::string geste;
        if (!(std::cin >> geste)) {
            break;
        }

        const char* faute = nullptr;

        if (geste == "ATTEND") {
            if (etat == REPOS) {
                etat = ATTENDUE;
            } else if (etat == ATTENDUE) {
                faute = "DEJA ATTENDUE";
            } else {
                faute = "TRAME EN COURS";
                if (bloque == 0) {
                    bloque = i;
                }
            }
        } else if (geste == "COMMENCE") {
            if (etat == ATTENDUE) {
                etat = COMMENCEE;
            } else if (etat == REPOS) {
                faute = "PAS ATTENDUE";
            } else {
                faute = "DEJA COMMENCEE";
            }
        } else if (geste == "FINIT") {
            if (etat == COMMENCEE) {
                etat = REPOS;
            } else {
                faute = "PAS COMMENCEE";
            }
        }

        std::cout << i << " " << geste << " ";
        if (faute == nullptr) {
            std::cout << "OK\n";
        } else {
            std::cout << faute << "\n";
            ++fautes;
        }
    }

    std::cout << "FAUTES " << fautes << "\n";
    std::cout << "BLOQUE " << bloque << "\n";
    return 0;
}
