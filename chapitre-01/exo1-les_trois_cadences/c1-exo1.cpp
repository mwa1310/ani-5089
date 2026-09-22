#include <iostream>
#include <iomanip>

double duree_image( int frequence) {
    return (1000.0 / frequence) - 8.0;
}

int main() {
    std::cout << std::fixed << std::setprecision(1);
    //  à 70 Hz
    std::cout << "La durée restante pour 70 Hz est : " << duree_image(70) << " ms" << std::endl;

    // à 90 Hz
    std::cout << "La durée restante pour 90 Hz est : " << duree_image(90) << " ms" << std::endl;

    // à 120 Hz
    std::cout << "La durée restante  pour 120 Hz est : " << duree_image(120) << " ms" << std::endl;
    return 0;
}