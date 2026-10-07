#include <iostream>
#include <string>
#include <vector>

struct Commande {
    long long echelle;
    long long seuil;
};

int main() {
    int c = 0;
    std::cin >> c;

    std::vector<Commande> commandes(c);
    for (int i = 0; i < c; ++i) {
        std::string nom; 
        std::cin >> nom >> commandes[i].echelle >> commandes[i].seuil;
    }

    int t = 0;
    std::cin >> t;

    for (int tour = 0; tour < t; ++tour) {
        long long axe = 0;
        for (int i = 0; i < c; ++i) {
            long long brut = 0;
            std::cin >> brut;

            // L'échelle
            long long contribution = brut * commandes[i].echelle / 1000;

            // La zone morte
            long long absolue = contribution < 0 ? -contribution : contribution;
            if (absolue < commandes[i].seuil) {
                continue;
            }
            axe += contribution;
        }
        std::cout << axe << "\n";
    }

    return 0;
}