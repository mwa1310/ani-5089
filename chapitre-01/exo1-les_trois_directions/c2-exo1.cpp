#include <cstdio>

struct Vec3 {
    float x, y, z;
};

float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Avant() { return { 0.0f,  0.0f, -1.0f }; }
Vec3 Haut() { return { 0.0f,  1.0f,  0.0f }; }
Vec3 Droite() { return { 1.0f,  0.0f,  0.0f }; }

int main() {
    Vec3 p;
    if (std::scanf("%f %f %f", &p.x, &p.y, &p.z) != 3) {
        std::fprintf(stderr, "Entree invalide : attendu trois reels (x y z).\n");
        return 1;
    }

    std::printf("Avant : %.4f\n", dot(p, Avant()));
    std::printf("Haut : %.4f\n", dot(p, Haut()));
    std::printf("Droite : %.4f\n", dot(p, Droite()));

    return 0;
}