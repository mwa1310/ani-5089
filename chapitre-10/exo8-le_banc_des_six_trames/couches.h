#pragma once

#include <cstdint>

// Espace de reference
enum class EspaceCouche {
    Plancher,
    Yeux
};

// Ce que la couche retient d'un oeil
struct VueOeil {
    std::uint64_t chaine;
    int index;
    float lacetRendu;
};

// Couche de projection 
struct CoucheProjection {
    EspaceCouche espace;
    VueOeil gauche;
    VueOeil droite;
};
