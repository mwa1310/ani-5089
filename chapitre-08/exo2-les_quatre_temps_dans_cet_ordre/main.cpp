#include <iostream>
#include <string>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n = 0;
    if (!(std::cin >> n)) return 0;

    bool acquise = false;
    bool enAttente = false;
    int fautes = 0;

    for (int tour = 1; tour <= n; ++tour) {
        std::string appel;
        if (!(std::cin >> appel)) break;

        const char* refus = nullptr;
        if (appel == "ACQUIERT") {
            if (acquise) refus = "DEJA ACQUISE";
            else acquise = true;
        } else if (appel == "DESSINE") {
            if (!acquise) refus = "PAS ACQUISE";
        } else if (appel == "REND") {
            if (!acquise) {
                refus = "RIEN ACQUIS";
            } else {
                acquise = false;
                enAttente = true;
            }
        } else if (appel == "SOUMET") {
            if (!enAttente) refus = "PAS RENDUE";
            else enAttente = false;
        }

        std::cout << tour << ' ' << appel << ' ' << (refus ? refus : "OK") << '\n';
        if (refus) ++fautes;
    }

    std::cout << "FAUTES " << fautes << '\n';
    return 0;
}
