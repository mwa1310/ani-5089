#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    long long images = 0;
    int n = 0;
    if (!(std::cin >> images >> n)) return 0;
    if (images < 1) images = 1;

    long long suivant = 0;
    bool emprunte = false;
    long long prets = 0;

    for (int tour = 1; tour <= n; ++tour) {
        std::string commande;
        if (!(std::cin >> commande)) break;

        std::cout << tour << ' ' << commande << ' ';
        if (commande == "EMPRUNTE" && !emprunte) {
            emprunte = true;
            ++prets;
            std::cout << suivant << '\n';
        } else if (commande == "REND" && emprunte) {
            std::cout << suivant << '\n';
            emprunte = false;
            suivant = (suivant + 1) % images;
        } else {
            std::cout << "REFUSE\n";
        }
    }

    std::cout << "PRETS " << prets << '\n';
    return 0;
}
