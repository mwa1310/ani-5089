#include <cstdio>
#include <cstdlib>
#include <string>

#include "actions.h"

namespace {

constexpr int kImages = 6;

const bool kToucheSaisir[kImages] = {false, true, true, true, false, false};
const bool kToucheTirer[kImages]  = {false, false, false, false, true, false};
const bool kPressionFantome[kImages] = {false, false, true, false, false, false};

std::string cellule(const std::string& texte, std::size_t largeur) {
    std::string r = texte;
    if (r.size() < largeur) {
        r.append(largeur - r.size(), ' ');
    }
    return r;
}

void ecrire(std::string ligne) {
    while (!ligne.empty() && ligne.back() == ' ') {
        ligne.pop_back();
    }
    std::printf("%s\n", ligne.c_str());
}

constexpr std::size_t kLargeurImage = 7;
constexpr std::size_t kLargeurChamp = 9;

int verifier(const char* quoi, bool ok) {
    if (!ok) {
        std::fprintf(stderr, "ECHEC : %s\n", quoi);
    }
    return ok ? 0 : 1;
}

}  

int main() {
    Action saisir("saisir");
    Action tirer("tirer");
    Action avancer("avancer");  

    const Action* toutes[3] = {&saisir, &tirer, &avancer};

    std::string entete = cellule("image", kLargeurImage);
    std::string champs = cellule("", kLargeurImage);
    for (const Action* a : toutes) {
        entete += cellule(a->nom(), 3 * kLargeurChamp);
        champs += cellule("courant", kLargeurChamp) + cellule("change", kLargeurChamp) +
                  cellule("actif", kLargeurChamp);
    }
    ecrire(entete);
    ecrire(champs);

    int fautes = 0;
    int fronts = 0;
    int niveaux = 0;
    int lachers = 0;

    for (int i = 0; i < kImages; ++i) {
        saisir.synchroniser(true, kToucheSaisir[i]);
        tirer.synchroniser(true, kToucheTirer[i]);
        avancer.synchroniser(false, kPressionFantome[i]);

        std::string ligne = cellule(std::to_string(i + 1), kLargeurImage);
        for (const Action* a : toutes) {
            const EtatAction& e = a->etat();
            ligne += cellule(std::to_string(e.courant ? 1 : 0), kLargeurChamp) +
                     cellule(std::to_string(e.change ? 1 : 0), kLargeurChamp) +
                     cellule(std::to_string(e.actif ? 1 : 0), kLargeurChamp);
        }
        ecrire(ligne);

        const EtatAction& s = saisir.etat();
        if (s.saisi()) {
            ++fronts;
        }
        if (s.courant) {
            ++niveaux;
        }
        if (s.lache()) {
            ++lachers;
        }

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
