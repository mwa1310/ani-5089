#include <cstdio>
#include <cstdlib>

#include "actions.h"

namespace {

constexpr int kImages = 6;

const bool kToucheSaisir[kImages] = {false, true, true, true, false, false};
const bool kToucheTirer[kImages]  = {false, false, false, false, true, false};
const bool kPressionFantome[kImages] = {false, false, true, false, false, false};

int verifier(const char* quoi, bool ok) {
    if (!ok) std::fprintf(stderr, "ECHEC : %s\n", quoi);
    return ok ? 0 : 1;
}

} 

int main() {
    Action saisir("saisir");
    Action tirer("tirer");
    Action avancer("avancer");

    std::printf("%-6s %-14s %-14s %-14s\n", "image", "saisir", "tirer", "avancer");
    std::printf("%-6s %-14s %-14s %-14s\n", "", "c ch a", "c ch a", "c ch a");

    int fautes = 0;
    int fronts = 0;
    int niveaux = 0;
    int lachers = 0;

    for (int i = 0; i < kImages; ++i) {
        saisir.synchroniser(true, kToucheSaisir[i]);
        tirer.synchroniser(true, kToucheTirer[i]);
        avancer.synchroniser(false, kPressionFantome[i]);

        const Action* toutes[3] = {&saisir, &tirer, &avancer};
        std::printf("%-6d", i + 1);
        for (const Action* a : toutes) {
            const EtatAction& e = a->etat();
            std::printf(" %d %-2d %-9d", e.courant, e.change, e.actif);
        }
        std::printf("\n");

        const EtatAction& s = saisir.etat();
        if (s.saisi()) ++fronts;
        if (s.courant) ++niveaux;
        if (s.lache()) ++lachers;

        fautes += verifier("avancer reste inactive", !avancer.etat().actif);
        fautes += verifier("avancer reste a faux", !avancer.etat().courant);
        fautes += verifier("avancer ne change jamais", !avancer.etat().change);
    }

    fautes += verifier("un seul front de saisie", fronts == 1);
    fautes += verifier("trois images de niveau", niveaux == 3);
    fautes += verifier("un seul relachement", lachers == 1);

    if (fautes != 0) {
        std::fprintf(stderr, "%d controle(s) en echec\n", fautes);
        return EXIT_FAILURE;
    }
    std::fprintf(stderr, "controles : ok\n");
    return EXIT_SUCCESS;
}
