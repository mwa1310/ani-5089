#include <cstdlib>
#include <iostream>

int main() {
    int n = 0;
    if (!(std::cin >> n)) {
        return 0;
    }

    int justes = 0;
    int pires = 0;

    for (int i = 0; i < n; ++i) {
        long long rendu = 0;
        long long reel = 0;
        long long soumis = 0;
        if (!(std::cin >> rendu >> reel >> soumis)) {
            break;
        }

        long long sans = rendu - reel;
        long long avec = rendu - soumis;
        long long absSans = std::llabs(sans);
        long long absAvec = std::llabs(avec);

        const char* verdict = "PIRE";
        if (avec == 0) {
            verdict = "JUSTE";
            ++justes;
        } else if (absAvec < absSans) {
            verdict = "PARTIEL";
        } else if (absAvec == absSans) {
            verdict = "SANS EFFET";
        } else {
            ++pires;
        }

        std::cout << "SANS " << sans << " AVEC " << avec << " " << verdict << "\n";
    }

    std::cout << "JUSTES " << justes << "\n";
    std::cout << "PIRES " << pires << "\n";
    return 0;
}
