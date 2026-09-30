#include <iostream>
#include <string>

static bool commence(const std::string& s, const std::string& prefixe) {
    return s.size() >= prefixe.size() &&
           s.compare(0, prefixe.size(), prefixe) == 0;
}

static bool finit(const std::string& s, const std::string& suffixe) {
    return s.size() >= suffixe.size() &&
           s.compare(s.size() - suffixe.size(), suffixe.size(), suffixe) == 0;
}

int main() {
    std::string arch;
    int f = 0;

    if (!(std::cin >> arch)) return 0;
    if (!(std::cin >> f)) f = 0;

    const std::string prefixeAbi = "lib/" + arch + "/";

    long long total = 0;
    bool signe = false;
    bool abiOui = false;
    int inutile = 0;

    for (int i = 0; i < f; ++i) {
        std::string chemin;
        long long taille = 0;
        if (!(std::cin >> chemin >> taille)) break;

        total += taille;

        if (commence(chemin, "META-INF/") &&
            (finit(chemin, ".RSA") || finit(chemin, ".DSA") || finit(chemin, ".EC"))) {
            signe = true;
        }

        if (commence(chemin, "lib/")) {
            if (commence(chemin, prefixeAbi)) {
                abiOui = true;
            } else {
                ++inutile;
            }
        }
    }

    std::cout << total << "\n";
    std::cout << (signe ? "SIGNE" : "NON SIGNE") << "\n";
    std::cout << (abiOui ? "ABI OUI" : "ABI NON") << "\n";
    std::cout << "INUTILE " << inutile << "\n";

    return 0;
}