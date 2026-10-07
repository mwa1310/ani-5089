#include <iostream>
#include <string>

int main() {
    int n = 0;
    std::cin >> n;

    int sansGarde = 0;

    for (int i = 0; i < n; ++i) {
        std::string evenement;
        std::cin >> evenement;

        if (evenement == "enfonce") {
            std::cout << "SAISIR\n";
            ++sansGarde;
        } else if (evenement == "repete") {
            std::cout << "RIEN\n";
            ++sansGarde; 
        } else if (evenement == "relache") {
            std::cout << "LACHER\n";
        } else {
            std::cout << "RIEN\n";
        }
    }

    std::cout << "SANS_GARDE " << sansGarde << "\n";
    return 0;
}