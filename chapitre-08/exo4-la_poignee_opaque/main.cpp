#include <iostream>
#include <string>
#include <unordered_map>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    long long images = 0;
    int n = 0;
    if (!(std::cin >> images >> n)) return 0;

    std::unordered_map<long long, long long> poignees;

    for (int i = 0; i < n; ++i) {
        std::string commande;
        long long index = 0;
        if (!(std::cin >> commande >> index)) break;

        const bool dansAnneau = index >= 0 && index < images;

        if (commande == "POSE") {
            long long valeur = 0;
            if (!(std::cin >> valeur)) break;
            if (dansAnneau) poignees[index] = valeur;
            else std::cout << "HORS ANNEAU\n";
        } else if (commande == "LIT") {
            const auto it = dansAnneau ? poignees.find(index) : poignees.end();
            if (it == poignees.end()) std::cout << "VIDE\n";
            else std::cout << it->second << '\n';
        }
    }

    std::cout << "POSEES " << poignees.size() << '\n';
    return 0;
}
