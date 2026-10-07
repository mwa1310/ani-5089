#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Rappel {
    long long id;
    std::string type;
};

static void retirerId(std::vector<Rappel>& registre, long long id) {
    registre.erase(
        std::remove_if(registre.begin(), registre.end(),
                       [id](const Rappel& r) { return r.id == id; }),
        registre.end());
}

int main() {
    int n = 0;
    std::cin >> n;

    std::vector<Rappel> registre;

    for (int i = 0; i < n; ++i) {
        std::string commande;
        std::cin >> commande;

        if (commande == "poser") {
            long long id = 0;
            std::string type;
            std::cin >> id >> type;
            retirerId(registre, id);          
            registre.push_back({id, type}); 
        } else if (commande == "retirer") {
            long long id = 0;
            std::cin >> id;
            retirerId(registre, id);         
        } else if (commande == "envoyer") {
            std::string type;
            std::cin >> type;

            bool premier = true;
            for (const Rappel& r : registre) {
                if (r.type != type) {
                    continue;
                }
                if (!premier) {
                    std::cout << " ";
                }
                std::cout << r.id;
                premier = false;
            }
            if (premier) {
                std::cout << "AUCUN";
            }
            std::cout << "\n";
        }
    }

    return 0;
}