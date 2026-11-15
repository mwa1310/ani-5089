#include <iostream>
#include <set>
#include <sstream>
#include <string>

namespace {

bool endsWith(const std::string& s, const std::string& suffixe) {
    return s.size() >= suffixe.size() &&
           s.compare(s.size() - suffixe.size(), suffixe.size(), suffixe) == 0;
}

// 0 : inconnu, 1 : droite, 2 : gauche
int mainDe(const std::string& usage) {
    static const std::set<std::string> droite = {
        "SELECT", "GRAB", "AIM_POSE", "GRIP_POSE", "HAPTIC", "MOVE_RIGHT"};
    if (usage == "MENU" || usage == "MOVE") return 2;
    if (endsWith(usage, "_LEFT")) {
        static const std::set<std::string> gauche = {
            "SELECT_LEFT", "GRAB_LEFT", "AIM_POSE_LEFT", "GRIP_POSE_LEFT", "HAPTIC_LEFT"};
        return gauche.count(usage) ? 2 : 0;
    }
    return droite.count(usage) ? 1 : 0;
}

} 

int main() {
    std::string ligne;
    if (!std::getline(std::cin, ligne)) return 0;
    int n = 0;
    try { n = std::stoi(ligne); } catch (...) { return 0; }

    int deuxMains = 0;
    for (int lues = 0; lues < n && std::getline(std::cin, ligne);) {
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
        std::istringstream flux(ligne);
        std::string nom;
        if (!(flux >> nom)) continue;
        ++lues;

        int droit = 0, gauche = 0;
        std::string usage;
        while (flux >> usage) {
            const int m = mainDe(usage);
            if (m == 1) ++droit;
            else if (m == 2) ++gauche;
        }

        const bool deux = droit > 0 && gauche > 0;
        if (deux) ++deuxMains;
        std::cout << nom << " " << droit << " " << gauche
                  << (deux ? " DEUX MAINS" : " UNE MAIN") << "\n";
    }

    std::cout << "DEUX MAINS " << deuxMains << "\n";
    return 0;
}
