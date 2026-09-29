#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

int main() {
    int n = 0;
    std::string ligne;

    if (!std::getline(std::cin, ligne)) return 0;
    n = std::stoi(ligne);

    std::map<std::string, std::vector<std::string>> besoins;

    for (int i = 0; i < n; ++i) {
        if (!std::getline(std::cin, ligne)) break;
        std::istringstream flux(ligne);
        std::string nom;
        if (!(flux >> nom)) continue;
        std::string dep;
        while (flux >> dep) {
            besoins[nom].push_back(dep);
        }
    }

    // Lecture de M puis des M noms 
    int m = 0;
    std::string mot;
    if (!(std::cin >> m)) m = 0;

    std::set<std::string> resultat;
    std::vector<std::string> pile;

    for (int i = 0; i < m; ++i) {
        if (!(std::cin >> mot)) break;
        if (resultat.insert(mot).second) {
            pile.push_back(mot);
        }
    }

    // Parcours
    while (!pile.empty()) {
        std::string courant = pile.back();
        pile.pop_back();

        auto it = besoins.find(courant);
        if (it == besoins.end()) continue; 

        for (const std::string& dep : it->second) {
            if (resultat.insert(dep).second) {
                pile.push_back(dep);
            }
        }
    }

    for (const std::string& nom : resultat) {
        std::cout << nom << "\n";
    }

    return 0;
}