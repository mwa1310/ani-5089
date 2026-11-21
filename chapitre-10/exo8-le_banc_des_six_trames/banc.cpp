#include <cstdint>
#include <cstdio>

#include "couches.h"

namespace {

constexpr int kImagesAnneau = 3;
constexpr int kTrames = 6;
constexpr float kDegresParTrame = 1.0f;

class Anneau {
public:
    explicit Anneau(int taille) : taille_(taille) {}

    int emprunter() {
        if (prete_ == false) {
            return -1;
        }
        prete_ = false;
        return suivant_;
    }

    bool rendre() {
        if (prete_) {
            return false;
        }
        prete_ = true;
        suivant_ = (suivant_ + 1) % taille_;
        return true;
    }

    int suivant() const { return suivant_; }

private:
    int taille_;
    int suivant_ = 0;
    bool prete_ = true;
};

class Session {
public:
    bool commencerTrame() {
        if (ouverte_) {
            return false;
        }
        ouverte_ = true;
        ++debuts_;
        return true;
    }

    bool finirTrame(const CoucheProjection* couche) {
        if (ouverte_ == false) {
            return false;
        }
        ouverte_ = false;
        ++fins_;
        derniereCouches_ = (couche != nullptr) ? 1 : 0;
        return true;
    }

    int debuts() const { return debuts_; }
    int fins() const { return fins_; }
    int dernieresCouches() const { return derniereCouches_; }

private:
    bool ouverte_ = false;
    int debuts_ = 0;
    int fins_ = 0;
    int derniereCouches_ = 0;
};

int echecs = 0;

void verifier(bool condition, const char* message) {
    if (condition == false) {
        std::fprintf(stderr, "CONTROLE RATE : %s\n", message);
        ++echecs;
    }
}

}  

int main() {
    Anneau anneau(kImagesAnneau);
    Session session;
    float lacet = 0.0f;

    std::printf("image on_rend couches_soumises index_image\n");

    for (int image = 1; image <= kTrames; ++image) {
        bool onRend = (image != 3 && image != 4);

        verifier(session.commencerTrame(), "debut de trame refuse");

        CoucheProjection couche{};
        const CoucheProjection* soumise = nullptr;
        int index = -1;

        if (onRend) {
            lacet += kDegresParTrame;
            index = anneau.emprunter();
            verifier(index >= 0, "emprunt refuse alors que l'anneau est libre");

            couche.espace = EspaceCouche::Plancher;
            couche.gauche = VueOeil{1u, index, lacet};
            couche.droite = VueOeil{2u, index, lacet};
            soumise = &couche;

            verifier(anneau.rendre(), "rendu refuse");
        }

        int avant = anneau.suivant();
        bool fin = session.finirTrame(soumise);
        verifier(fin, "fin de trame refusee");
        verifier(anneau.suivant() == avant, "la fin de trame a deplace l'index");

        if (onRend) {
            std::printf("%d %d %d %d\n", image, 1, session.dernieresCouches(), index);
        } else {
            std::printf("%d %d %d -\n", image, 0, session.dernieresCouches());
        }
    }

    int finsAvant = session.fins();
    int suivantAvant = anneau.suivant();

    verifier(session.finirTrame(nullptr) == false, "fin sans debut acceptee");
    verifier(session.fins() == finsAvant, "un refus de fin a change le compteur de fins");

    verifier(anneau.rendre() == false, "rendu sans emprunt accepte");
    verifier(anneau.suivant() == suivantAvant, "un rendu refuse a avance l'index");

    int e1 = anneau.emprunter();
    int e2 = anneau.emprunter();
    verifier(e1 == suivantAvant, "premier emprunt sur un mauvais index");
    verifier(e2 == -1, "double emprunt accepte");
    verifier(anneau.suivant() == suivantAvant, "un emprunt refuse a avance l'index");
    anneau.rendre();

    verifier(session.debuts() == kTrames, "nombre de debuts different du nombre de trames");
    verifier(session.fins() == kTrames, "nombre de fins different du nombre de trames");

    std::fprintf(stderr, "controles internes : %s\n", echecs == 0 ? "tous bons" : "echecs");
    return echecs == 0 ? 0 : 1;
}
