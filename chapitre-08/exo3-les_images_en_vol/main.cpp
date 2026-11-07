#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    long long vol = 0;
    int n = 0;
    if (!(std::cin >> vol >> n)) return 0;

    long long enVol = 0;
    int premierBlocage = 0;

    for (int tour = 1; tour <= n; ++tour) {
        std::string commande;
        if (!(std::cin >> commande)) break;

        bool attend = false;
        if (commande == "SOUMET") {
            if (enVol >= vol) attend = true;
            else ++enVol;
        } else if (commande == "FINI") {
            if (enVol > 0) --enVol;
        }

        std::cout << tour << ' ' << commande << ' ' << enVol << ' '
                  << (attend ? "ATTEND" : "PASSE") << '\n';
        if (attend && premierBlocage == 0) premierBlocage = tour;
    }

    std::cout << "BLOQUE " << premierBlocage << '\n';
    return 0;
}
