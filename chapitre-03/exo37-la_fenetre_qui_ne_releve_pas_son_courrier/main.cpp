#include <iostream>
#include <string>

int main() {
    int p = 0, f = 0;
    std::cin >> p >> f;

    int compteur = 0;
    int premier = 0;  // la fenêtre n'est jamais morte

    for (int tour = 1; tour <= f; ++tour) {
        std::string action;
        std::cin >> action;

        if (action == "releve") {
            compteur = 0;
        } else if (action == "travaille") {
            ++compteur;
        }

        bool morte = compteur >= p;
        if (morte && premier == 0) {
            premier = tour;
        }

        std::cout << compteur << " " << (morte ? "MORTE" : "VIVANTE") << "\n";
    }

    std::cout << "PREMIER " << premier << "\n";
    return 0;
}