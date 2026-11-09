#include <cstdio>

#include "chaine.h"

static int echecs = 0;

static void verifier(const char* libelle, bool attendu) {
    std::printf("%s : %s\n", libelle, attendu ? "oui" : "NON");
    if (!attendu) ++echecs;
}

int main() {
    ChaineEchange anneau;
    anneau.Creer(3);

    std::puts("six tours sur un anneau de trois images");
    for (unsigned tour = 1; tour <= 6; ++tour) {
        std::uint32_t pris = 0;
        const bool emprunte = anneau.Acquerir(pris);
        const bool rendu = anneau.Rendre();
        std::printf("tour %u : emprunte %u, rendu, a soumettre %u\n",
                    tour, pris, anneau.DerniereRendue());
        verifier("  emprunt et rendu acceptes", emprunte && rendu);
        verifier("  la soumission vise l'image empruntee", anneau.DerniereRendue() == pris);
        verifier("  l'index suit tour modulo trois", pris == (tour - 1) % 3);
    }

    std::puts("les trois refus");
    std::uint32_t index = 0;
    verifier("premier emprunt accepte", anneau.Acquerir(index));
    verifier("double emprunt refuse", !anneau.Acquerir(index));
    anneau.Rendre();
    verifier("rendu sans emprunt refuse", !anneau.Rendre());

    ChaineEchange trop;
    verifier("creation de 5 images refusee (maximum 4)", !trop.Creer(5));
    verifier("creation de 0 image refusee", !trop.Creer(0));
    verifier("chaine non creee, emprunt refuse", !trop.Acquerir(index));
    verifier("creation de 4 images acceptee", trop.Creer(4));

    std::printf("echecs : %d\n", echecs);
    return echecs == 0 ? 0 : 1;
}
