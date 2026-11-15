#include <iostream>

int main() {
    int n = 0;
    if (!(std::cin >> n)) return 0;

    int precedent = 0;
    int fronts = 0;
    int niveaux = 0;

    for (int image = 1; image <= n; ++image) {
        int presse = 0, lie = 0;
        if (!(std::cin >> presse >> lie)) break;

        const int actif = lie != 0 ? 1 : 0;
        const int courant = (actif && presse != 0) ? 1 : 0;
        const int change = courant != precedent ? 1 : 0;

        if (courant && change) ++fronts;
        if (courant) ++niveaux;

        std::cout << image << " " << courant << " " << change << " " << actif << "\n";
        precedent = courant;
    }

    std::cout << "FRONTS " << fronts << "\n";
    std::cout << "NIVEAUX " << niveaux << "\n";
    return 0;
}
