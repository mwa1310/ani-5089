#include <iostream>
#include <sstream>
#include <string>

int main() {
    int n = 0;
    std::cin >> n;
    std::string ligne;
    std::getline(std::cin, ligne);  

    for (int i = 0; i < n; ++i) {
        std::getline(std::cin, ligne);
        std::istringstream flux(ligne);
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();

        bool avance = false, recule = false, gauche = false, droite = false;
        std::string touche;
        while (flux >> touche) {
            if (touche == "W" || touche == "Z") {
                avance = true;
            } else if (touche == "S") {
                recule = true;
            } else if (touche == "A" || touche == "Q") {
                gauche = true;
            } else if (touche == "D") {
                droite = true;
            }
        }

        int pasAvance = (avance ? 1 : 0) - (recule ? 1 : 0);
        int pasCote = (droite ? 1 : 0) - (gauche ? 1 : 0);
        std::cout << pasAvance << " " << pasCote << "\n";
    }

    return 0;
}