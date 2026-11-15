#include <iostream>
#include <string>

int main() {
    int n = 0;
    if (!(std::cin >> n)) {
        n = 0;
    }

    int fautes = 0;
    for (int i = 0; i < n; ++i) {
        std::string nom, intention, pose;
        if (!(std::cin >> nom >> intention >> pose)) {
            break;
        }

        std::cout << nom << " ";
        if (intention == "TENIR" && pose == "AIM") {
            std::cout << "FLOTTE\n";
            ++fautes;
        } else if (intention == "POINTER" && pose == "GRIP") {
            std::cout << "PART DE TRAVERS\n";
            ++fautes;
        } else {
            std::cout << "OK\n";
        }
    }

    std::cout << "FAUTES " << fautes << "\n";
    return 0;
}
