#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::string ligne;

    if (!std::getline(std::cin, ligne)) return 0;
    int p = std::stoi(ligne);

    std::vector<std::pair<std::string, std::string>> prefixes;
    for (int i = 0; i < p; ++i) {
        if (!std::getline(std::cin, ligne)) break;
        std::istringstream flux(ligne);
        std::string prefixe, module;
        if (flux >> prefixe >> module) {
            prefixes.push_back({prefixe, module});
        }
    }

    int l = 0;
    if (std::getline(std::cin, ligne)) {
        try { l = std::stoi(ligne); } catch (...) { l = 0; }
    }

    const std::string marque = "undefined reference to '";
    std::set<std::string> modules;
    int inconnus = 0;

    for (int i = 0; i < l; ++i) {
        if (!std::getline(std::cin, ligne)) break;
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();

        std::size_t pos = ligne.find(marque);
        if (pos == std::string::npos) continue;

        std::size_t debut = pos + marque.size();
        std::size_t fin = ligne.find('\'', debut);
        if (fin == std::string::npos) continue;

        std::string symbole = ligne.substr(debut, fin - debut);

        const std::string* meilleurModule = nullptr;
        std::size_t meilleureLongueur = 0;
        for (const auto& pr : prefixes) {
            if (symbole.compare(0, pr.first.size(), pr.first) == 0 &&
                (meilleurModule == nullptr || pr.first.size() > meilleureLongueur)) {
                meilleurModule = &pr.second;
                meilleureLongueur = pr.first.size();
            }
        }

        if (meilleurModule) modules.insert(*meilleurModule);
        else ++inconnus;
    }

    for (const std::string& m : modules) {
        std::cout << m << "\n";
    }
    if (inconnus > 0) {
        std::cout << "INCONNU " << inconnus << "\n";
    }

    return 0;
}