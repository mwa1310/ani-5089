#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    long long largeur = 0, hauteur = 0;
    int n = 0;
    if (!(std::cin >> largeur >> hauteur >> n)) return 0;

    int bloquantes = 0;

    for (int i = 0; i < n; ++i) {
        std::string nom;
        int cible = 0, taille = 0;
        long long lc = 0, hc = 0;
        if (!(std::cin >> nom >> cible >> taille >> lc >> hc)) break;

        long long l = largeur, h = hauteur;
        const char* verdict;

        if (cible == 0) {
            verdict = "PAS RELIEE";
        } else if (taille != 0) {
            l = lc;
            h = hc;
            verdict = "OK";
        } else if (lc != largeur || hc != hauteur) {
            verdict = "BLOQUE";
            ++bloquantes;
        } else {
            verdict = "SANS EFFET";
        }

        std::cout << nom << ' ' << l << ' ' << h << ' ' << verdict << '\n';
    }

    std::cout << "BLOQUANTES " << bloquantes << '\n';
    return 0;
}
