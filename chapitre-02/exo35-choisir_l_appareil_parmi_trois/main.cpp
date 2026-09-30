#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Appareil {
    std::string serie;
    std::string etat;
};

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
    int d = 0;
    try { d = std::stoi(ligne); } catch (...) { return 0; }

    std::vector<Appareil> appareils;
    for (int i = 0; i < d; ++i) {
        if (!std::getline(std::cin, ligne)) break;
        std::istringstream flux(ligne);
        Appareil a;
        if (flux >> a.serie >> a.etat) {
            appareils.push_back(a);
        }
    }

    std::string cible;
    if (std::getline(std::cin, ligne)) cible = nettoyer(ligne);
    if (cible.empty()) cible = "-";

    if (cible != "-") {
        // Cible demandée
        const Appareil* trouve = nullptr;
        for (const Appareil& a : appareils) {
            if (a.serie == cible) {
                trouve = &a;
                break;
            }
        }

        if (trouve == nullptr) {
            std::cout << "ERREUR cible introuvable\n";
        } else if (trouve->etat != "device") {
            std::cout << "ERREUR " << trouve->serie << " est " << trouve->etat << "\n";
        } else {
            std::cout << trouve->serie << "\n";
        }
        return 0;
    }

    // Ne garder que les appareils à l'état "device"
    std::vector<std::string> prets;
    for (const Appareil& a : appareils) {
        if (a.etat == "device") prets.push_back(a.serie);
    }

    if (prets.empty()) {
        std::cout << "ERREUR aucun appareil\n";
    } else if (prets.size() == 1) {
        std::cout << prets[0] << "\n";
    } else {
        std::sort(prets.begin(), prets.end());
        std::cout << "ERREUR plusieurs appareils\n";
        for (const std::string& s : prets) {
            std::cout << s << "\n";
        }
    }

    return 0;
}