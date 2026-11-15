#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

int main() {
    int a = 0;
    if (!(std::cin >> a)) {
        a = 0;
    }

    std::vector<std::pair<std::string, std::string>> actions;
    for (int i = 0; i < a; ++i) {
        std::string nom, usage;
        if (!(std::cin >> nom >> usage)) {
            break;
        }
        actions.emplace_back(nom, usage);
    }

    int l = 0;
    if (!(std::cin >> l)) {
        l = 0;
    }

    std::set<std::string> fournis;
    for (int i = 0; i < l; ++i) {
        std::string usage;
        if (!(std::cin >> usage)) {
            break;
        }
        fournis.insert(usage);
    }

    int mortes = 0;
    for (const auto& act : actions) {
        const bool vivante = fournis.count(act.second) != 0;
        if (!vivante) {
            ++mortes;
        }
        std::cout << act.first << (vivante ? " ACTIVE" : " MORTE") << "\n";
    }
    std::cout << "MORTES " << mortes << "\n";
    return 0;
}
