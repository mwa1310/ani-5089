#include <cstdlib>
#include <iostream>
#include <string>

namespace {

struct Anneau {
    long long images = 1;
    long long suivant = 0;
    bool emprunte = false;
    long long rendus = 0;
};

}  

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    long long images = 0;
    int n = 0;
    if (!(std::cin >> images >> n)) return 0;
    if (images < 1) images = 1;

    Anneau anneaux[2];
    anneaux[0].images = anneaux[1].images = images;

    int premierDesync = 0;

    for (int tour = 1; tour <= n; ++tour) {
        std::string oeil, commande;
        if (!(std::cin >> oeil >> commande)) break;

        Anneau& a = anneaux[oeil == "D" ? 1 : 0];

        std::cout << tour << ' ' << oeil << ' ' << commande << ' ';
        if (commande == "EMPRUNTE" && !a.emprunte) {
            a.emprunte = true;
            std::cout << a.suivant << '\n';
        } else if (commande == "REND" && a.emprunte) {
            std::cout << a.suivant << '\n';
            a.emprunte = false;
            a.suivant = (a.suivant + 1) % a.images;
            ++a.rendus;
        } else {
            std::cout << "REFUSE\n";
        }

        if (premierDesync == 0 && std::llabs(anneaux[0].rendus - anneaux[1].rendus) > 1) {
            premierDesync = tour;
        }
    }

    std::cout << "DECALAGE " << std::llabs(anneaux[0].rendus - anneaux[1].rendus) << '\n';
    std::cout << "DESYNC " << premierDesync << '\n';
    return 0;
}
