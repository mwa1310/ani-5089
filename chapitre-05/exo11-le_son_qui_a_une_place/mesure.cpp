#include "NKWindow/Core/NkEntry.h"
#include "NKWindow/EntryPoints/NkWindowsDesktop.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkDeviceInitInfo.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
#include "NKImage/Core/NkImage.h"
#include "NKRenderer/Materials/NkMaterial.h"
#include "NKFont/NkFont.h"
#include "NKFont/Embedded/NkFontEmbedded.h"
#include "NKFileSystem/NkPath.h"
#include "NKTime/NkChrono.h"
#include <cstdarg>
#include <utility>
#include "NKAudio/NkAudio.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;
using namespace nkentseu::audio;

static std::string Fmt(const char* f, ...) {
    char tampon[8192];
    va_list a;
    va_start(a, f);
    std::vsnprintf(tampon, sizeof(tampon), f, a);
    va_end(a);
    return tampon;
}

// Chemin dans le dossier a deposer (si on lance depuis Mes_builds), sinon dans le dossier courant.
static std::string Depot(const char* dossier, const char* nom) {
    const std::string p = std::string("../chapitre-05/") + dossier + "/" + nom;
    if (FILE* f = std::fopen(p.c_str(), "ab")) {
        std::fclose(f);
        return p;
    }
    return nom;
}

static void EcrireTexte(const std::string& chemin, const std::string& texte) {
    if (FILE* f = std::fopen(chemin.c_str(), "wb")) {
        std::fputs(texte.c_str(), f);
        std::fclose(f);
    }
}

static long TailleFichier(const std::string& chemin) {
    FILE* f = std::fopen(chemin.c_str(), "rb");
    if (!f) return -1;
    std::fseek(f, 0, SEEK_END);
    const long n = std::ftell(f);
    std::fclose(f);
    return n;
}

struct Niveaux {
    double gauche, droite;
};

// Rend 48000 trames par blocs de 256 et calcule la racine de la moyenne des carres, oreille par oreille.
// Les echantillons sont ranges gauche, droite, gauche, droite.
static Niveaux Mesurer(AudioEngine& moteur) {
    const int TOTAL = 48000, BLOC = 256;
    std::vector<float> tampon(BLOC * 2);
    for (int i = 0; i < 4; ++i) moteur.RenderToBuffer(tampon.data(), BLOC, 2);   // chauffe, non comptee
    double sg = 0.0, sd = 0.0;
    int rendus = 0;
    while (rendus < TOTAL) {
        const int n = (TOTAL - rendus < BLOC) ? (TOTAL - rendus) : BLOC;
        moteur.RenderToBuffer(tampon.data(), n, 2);
        for (int i = 0; i < n; ++i) {
            sg += (double)tampon[2 * i] * tampon[2 * i];
            sd += (double)tampon[2 * i + 1] * tampon[2 * i + 1];
        }
        rendus += n;
    }
    return { std::sqrt(sg / TOTAL), std::sqrt(sd / TOTAL) };
}

int nkmain(const NkEntryState&) {
    // Ni fenetre ni sortie sonore
    AudioEngineConfig cfg;
    cfg.backend    = AudioBackendType::NULL_OUTPUT;
    cfg.sampleRate = 48000;
    if (!AudioEngine::Instance().Initialize(cfg)) return 1;
    AudioEngine& son = AudioEngine::Instance();

    // L'auditeur est a l'origine et regarde vers -Z
    son.SetListenerPosition(0.f, 0.f, 0.f);
    son.SetListenerOrientation(0.f, 0.f, -1.f, 0.f, 1.f, 0.f);

    AudioSample pluie = AudioGenerator::GenerateNoise(4.f, WaveformType::NOISE_PINK);
    const char* noms[3] = { "gauche", "devant", "droite" };
    const float pos[3][3] = { { -2.f, 0.f, 0.f }, { 0.f, 0.f, -2.f }, { 2.f, 0.f, 0.f } };

    std::string mesures;
    for (int i = 0; i < 3; ++i) {
        for (int positional = 1; positional >= 0; --positional) {
            VoiceParams p;
            p.looping = true;
            p.source3d.position[0] = pos[i][0];
            p.source3d.position[1] = pos[i][1];
            p.source3d.position[2] = pos[i][2];
            p.source3d.positional  = positional == 1;
            const AudioHandle h = son.Play(pluie, p);
            const Niveaux n = Mesurer(son);
            son.Stop(h);   // la source ne bouge pas pendant une mesure, on l'arrete avant la suivante
            mesures += Fmt("%s %d : G %.5f D %.5f\n", noms[i], positional, n.gauche, n.droite);
        }
    }
    EcrireTexte(Depot("exo11-le_son_qui_a_une_place", "mesures.txt"), mesures);

    son.Shutdown();
    AudioLoader::Free(pluie);
    return 0;
}
