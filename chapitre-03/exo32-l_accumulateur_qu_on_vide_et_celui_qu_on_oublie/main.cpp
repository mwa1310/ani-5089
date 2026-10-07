#include <iostream>
#include <string>

int main() {
    int n = 0;
    std::cin >> n;

    // Accumulateur
    long long totalX = 0, totalY = 0;

    // Moteur défectueux
    long long moteurX = 0, moteurY = 0;

    for (int i = 0; i < n; ++i) {
        std::string commande;
        std::cin >> commande;

        if (commande == "bouge") {
            long long dx = 0, dy = 0;
            std::cin >> dx >> dy;
            totalX += dx;   // ajouter, sans écraser
            totalY += dy;
            moteurX = dx;   // le défaut du moteur
            moteurY = dy;
        } else if (commande == "image") {
            std::cout << totalX << " " << totalY << " "
                      << moteurX << " " << moteurY << "\n";
            totalX = 0;
            totalY = 0;
        }
    }

    return 0;
}