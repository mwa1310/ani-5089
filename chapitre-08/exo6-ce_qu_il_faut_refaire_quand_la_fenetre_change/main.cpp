#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string ligne;
    int n = 0;
    if (!std::getline(std::cin, ligne)) return 0;
    std::istringstream(ligne) >> n;

    int survivants = 0;
    int traites = 0;

    while (traites < n && std::getline(std::cin, ligne)) {
        std::istringstream flux(ligne);
        std::string nom;
        if (!(flux >> nom)) continue;
        ++traites;

        int posDetruit = -1, posRecree = -1;
        bool reenregistre = false;
        int rang = 0;
        std::string mot;
        while (flux >> mot) {
            if (mot == "DETRUIT" && posDetruit < 0) posDetruit = rang;
            else if (mot == "RECREE" && posRecree < 0) posRecree = rang;
            else if (mot == "REENREGISTRE") reenregistre = true;
            ++rang;
        }

        const char* verdict;
        if (posDetruit >= 0 && posRecree >= 0 && posRecree < posDetruit) verdict = "MAUVAIS ORDRE";
        else if (posDetruit < 0 || posRecree < 0) verdict = "CHAINE PERIMEE";
        else if (!reenregistre) verdict = "POIGNEES PERDUES";
        else {
            verdict = "SURVIT";
            ++survivants;
        }

        std::cout << nom << ' ' << verdict << '\n';
    }

    std::cout << "SURVIVANTS " << survivants << '\n';
    return 0;
}
