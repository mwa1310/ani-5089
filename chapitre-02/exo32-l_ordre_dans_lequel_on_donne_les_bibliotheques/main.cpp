#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

int main() {
    std::string ligne;

    if (!std::getline(std::cin, ligne)) return 0;
    int n = std::stoi(ligne);

    std::map<std::string, std::set<std::string>> besoins;

    for (int i = 0; i < n; ++i) {
        if (!std::getline(std::cin, ligne)) break;
        std::istringstream flux(ligne);
        std::string nom;
        if (!(flux >> nom)) continue;
        besoins[nom];
        std::string dep;
        while (flux >> dep) {
            besoins[nom].insert(dep);
        }
    }

    int m = 0;
    if (!(std::cin >> m)) m = 0;

    // 1. Calculez d'abord la liste complète, comme à l'exercice précédent.
    std::set<std::string> liste;
    std::vector<std::string> pile;
    std::string mot;

    for (int i = 0; i < m; ++i) {
        if (!(std::cin >> mot)) break;
        if (liste.insert(mot).second) pile.push_back(mot);
    }

    while (!pile.empty()) {
        std::string courant = pile.back();
        pile.pop_back();

        auto it = besoins.find(courant);
        if (it == besoins.end()) continue;

        for (const std::string& dep : it->second) {
            if (liste.insert(dep).second) pile.push_back(dep);
        }
    }

    // 2. Comptez, pour chaque module, combien de modules de la liste ont besoin de lui.
    std::map<std::string, int> compte;
    for (const std::string& nom : liste) compte[nom] = 0;

    for (const std::string& nom : liste) {
        auto it = besoins.find(nom);
        if (it == besoins.end()) continue;
        for (const std::string& dep : it->second) {
            ++compte[dep];
        }
    }

    // 3. Sortez d'abord ceux dont ce compte vaut zéro. S'il y en a plusieurs, prenez le premier par ordre alphabétique.
    std::set<std::string> prets;
    for (const auto& p : compte) {
        if (p.second == 0) prets.insert(p.first);
    }

    // 4. Chaque fois que vous sortez un module, retirez-le des besoins des autres, ce qui peut faire tomber d'autres comptes à zéro.
    std::vector<std::string> ordre;
    while (!prets.empty()) {
        std::string courant = *prets.begin();
        prets.erase(prets.begin());
        ordre.push_back(courant);

        auto it = besoins.find(courant);
        if (it == besoins.end()) continue;
        for (const std::string& dep : it->second) {
            if (--compte[dep] == 0) prets.insert(dep);
        }
    }

    // 5. Si à un moment aucun compte ne vaut zéro alors qu'il reste des modules, il y a un cycle : affichez CYCLE et arrêtez-vous.
    if (ordre.size() != liste.size()) {
        std::cout << "CYCLE\n";
        return 0;
    }

    for (const std::string& nom : ordre) {
        std::cout << nom << "\n";
    }

    return 0;
}