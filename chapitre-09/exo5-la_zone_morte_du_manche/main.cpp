#include <iostream>

int main() {
    long long seuil = 0, maximum = 0;
    if (!(std::cin >> seuil >> maximum)) {
        return 0;
    }
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }

    const long long course = 1000 - seuil;
    int morts = 0;

    for (int i = 0; i < n; ++i) {
        long long brut = 0;
        if (!(std::cin >> brut)) {
            break;
        }
        const long long absolu = brut < 0 ? -brut : brut;

        long long vitesse = 0;
        if (absolu < seuil) {
            ++morts;
        } else {
            const long long reste = absolu - seuil;
            vitesse = course > 0 ? maximum * reste * reste / (course * course) : maximum;
            if (brut < 0) {
                vitesse = -vitesse;
            }
        }
        std::cout << vitesse << "\n";
    }

    std::cout << "MORTS " << morts << "\n";
    return 0;
}
