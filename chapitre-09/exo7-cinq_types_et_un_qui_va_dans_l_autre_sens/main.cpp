#include <iostream>
#include <map>
#include <string>

int main() {
    int a = 0;
    if (!(std::cin >> a)) a = 0;

    std::map<std::string, bool> sortie;
    for (int i = 0; i < a; ++i) {
        std::string nom, type;
        if (!(std::cin >> nom >> type)) break;
        sortie[nom] = (type == "HAPTIC");
    }

    int o = 0;
    if (!(std::cin >> o)) o = 0;

    for (int i = 0; i < o; ++i) {
        std::string nom, operation;
        if (!(std::cin >> nom >> operation)) break;
        const auto it = sortie.find(nom);
        if (it == sortie.end()) continue;
        if (it->second && operation == "LIT") {
            std::cout << nom << " LIT UNE SORTIE\n";
        } else if (!it->second && operation == "ECRIT") {
            std::cout << nom << " ECRIT UNE ENTREE\n";
        }
    }

    int sorties = 0;
    for (const auto& p : sortie) if (p.second) ++sorties;
    std::cout << "ENTREES " << sortie.size() - sorties << "\n";
    std::cout << "SORTIES " << sorties << "\n";
    return 0;
}
