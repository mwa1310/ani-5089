#include <iostream>
#include <map>
#include <string>

int main() {
    const std::map<std::string, std::string> causes = {
        {"BLOQUE_AU_WAIT", "FIN DE TRAME"},
        {"PAS_DE_REPRISE", "FIN SANS RENDU"},
        {"OEIL_NOIR", "CHAINE MANQUANTE"},
        {"TREMBLEMENT", "MAUVAIS INSTANT"},
        {"CORRECTION_DE_TRAVERS", "POSE FRAICHE"},
        {"SCENE_DECALEE", "MAUVAIS ESPACE"}};

    int n = 0;
    if (!(std::cin >> n)) {
        return 0;
    }

    int connus = 0;
    int inconnus = 0;

    for (int i = 0; i < n; ++i) {
        std::string symptome;
        if (!(std::cin >> symptome)) {
            break;
        }

        auto it = causes.find(symptome);
        if (it == causes.end()) {
            std::cout << symptome << " INCONNU\n";
            ++inconnus;
        } else {
            std::cout << symptome << " " << it->second << "\n";
            ++connus;
        }
    }

    std::cout << "DIAGNOSTIQUES " << connus << "\n";
    std::cout << "INCONNUS " << inconnus << "\n";
    return 0;
}
