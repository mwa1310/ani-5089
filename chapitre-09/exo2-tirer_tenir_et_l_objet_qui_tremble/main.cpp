#include <iostream>

int main() {
    int n = 0;
    if (!(std::cin >> n)) n = 0;

    int precedent = 0;
    int fronts = 0;
    int niveaux = 0;

    for (int i = 0; i < n; ++i) {
        int presse = 0;
        if (!(std::cin >> presse)) break;
        const int courant = presse != 0 ? 1 : 0;

        if (courant == 1 && precedent == 0) ++fronts;
        if (courant == 1) ++niveaux;
        precedent = courant;
    }

    std::cout << "BALLES FRONT " << fronts << "\n";
    std::cout << "BALLES NIVEAU " << niveaux << "\n";
    std::cout << "TENU NIVEAU " << niveaux << "\n";
    std::cout << "TENU FRONT " << fronts << "\n";
    return 0;
}
