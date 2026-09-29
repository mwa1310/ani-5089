#include <iostream>
#include <map>
#include <string>
#include <vector>

static std::string nettoyer(const std::string& s) {
    std::size_t debut = 0;
    std::size_t fin = s.size();
    while (debut < fin && (s[debut] == ' ' || s[debut] == '\t' || s[debut] == '\r')) ++debut;
    while (fin > debut && (s[fin - 1] == ' ' || s[fin - 1] == '\t' || s[fin - 1] == '\r')) --fin;
    return s.substr(debut, fin - debut);
}

int main() {
    std::string ligne;

    if (!std::getline(std::cin, ligne)) return 0;
    int v = std::stoi(ligne);

    std::map<std::string, std::string> machine;
    for (int i = 0; i < v; ++i) {
        if (!std::getline(std::cin, ligne)) break;
        ligne = nettoyer(ligne);
        std::size_t egal = ligne.find('=');
        if (egal == std::string::npos) continue;
        machine[nettoyer(ligne.substr(0, egal))] = nettoyer(ligne.substr(egal + 1));
    }

    int f = 0;
    if (std::getline(std::cin, ligne)) {
        try { f = std::stoi(ligne); } catch (...) { f = 0; }
    }

    for (int i = 0; i < f; ++i) {
        if (!std::getline(std::cin, ligne)) break;

        std::vector<std::string> termes;
        std::size_t debut = 0;
        while (true) {
            std::size_t pos = ligne.find("&&", debut);
            if (pos == std::string::npos) {
                termes.push_back(ligne.substr(debut));
                break;
            }
            termes.push_back(ligne.substr(debut, pos - debut));
            debut = pos + 2;
        }

        bool applique = true;
        for (std::string terme : termes) {
            terme = nettoyer(terme);

            bool inverse = false;
            if (!terme.empty() && terme[0] == '!') {
                inverse = true;
                terme = nettoyer(terme.substr(1));
            }

            bool vrai = false;
            std::size_t egal = terme.find('=');
            if (egal != std::string::npos) {
                std::string cle = nettoyer(terme.substr(0, egal));
                std::string valeur = nettoyer(terme.substr(egal + 1));
                auto it = machine.find(cle);
                vrai = (it != machine.end() && it->second == valeur);
            }

            if (inverse) vrai = !vrai;
            if (!vrai) {
                applique = false;
                break;
            }
        }

        std::cout << (applique ? "OUI" : "NON") << "\n";
    }

    return 0;
}