#include <iostream>

int main() {
    int images = 0;
    int trames = 0;
    if (!(std::cin >> images >> trames)) {
        return 0;
    }

    int index = 0;
    int rendues = 0;

    for (int t = 1; t <= trames; ++t) {
        int dessine = 0;
        if (!(std::cin >> dessine)) {
            break;
        }

        if (dessine != 0) {
            std::cout << t << " OUI 1 " << index << "\n";
            ++rendues;
            if (images > 0) {
                index = (index + 1) % images;
            }
        } else {
            std::cout << t << " NON 0 -\n";
        }
    }

    std::cout << "RENDUES " << rendues << "\n";
    std::cout << "FINS " << trames << "\n";
    return 0;
}
