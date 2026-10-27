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
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

static std::string Fmt(const char* f, ...) {
    char tampon[8192];
    va_list a;
    va_start(a, f);
    std::vsnprintf(tampon, sizeof(tampon), f, a);
    va_end(a);
    return tampon;
}

static std::string Depot(const char* dossier, const char* nom) {
    const std::string p = std::string("../chapitre-06/") + dossier + "/" + nom;
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

static NkVec3f Moins(NkVec3f a, NkVec3f b)  { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static NkVec3f Plus(NkVec3f a, NkVec3f b)   { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static NkVec3f Fois(NkVec3f a, float s) { return { a.x * s, a.y * s, a.z * s }; }

static std::string Ligne(const char* nom, const char* methode, NkVec3f g, NkVec3f d) {
    return Fmt("%s, %s : G (%.4f, %.4f, %.4f) D (%.4f, %.4f, %.4f)\n", nom, methode, g.x, g.y, g.z, d.x, d.y, d.z);
}

int nkmain(const NkEntryState&) {
    const NkVec3f position(0.30f, 1.70f, 1.20f);
    const float ipd = 0.063f;
    const float demiEcart = ipd * 0.5f;

    const char* noms[3] = { "neutre", "roulis 30", "lacet 90" };
    const NkQuatf orientations[3] = { NkQuatf::Identity(), NkQuatf::RotateZ(NkAngle(30.f)), NkQuatf::RotateY(NkAngle(90.f)) };

    std::string positions, ecarts;
    for (int i = 0; i < 3; ++i) {
        const NkVec3f droiteTete = orientations[i] * NkVec3f(1.f, 0.f, 0.f);
        const NkVec3f droiteMonde(1.f, 0.f, 0.f);

        const NkVec3f gTete = Moins(position, Fois(droiteTete, demiEcart));
        const NkVec3f dTete = Plus(position, Fois(droiteTete, demiEcart));
        const NkVec3f gMonde = Moins(position, Fois(droiteMonde, demiEcart));
        const NkVec3f dMonde = Plus(position, Fois(droiteMonde, demiEcart));

        positions += Ligne(noms[i], "tete", gTete, dTete);
        positions += Ligne(noms[i], "monde", gMonde, dMonde);

        const NkVec3f ecart = Moins(gTete, gMonde);
        const float mm = std::sqrt(ecart.x * ecart.x + ecart.y * ecart.y + ecart.z * ecart.z) * 1000.f;   // en millimètres
        ecarts += Fmt("%s : ecart %.2f mm\n", noms[i], mm);
    }
    const std::string tout = positions + ecarts;
    std::printf("%s", tout.c_str());
    EcrireTexte(Depot("exo8-les_deux_poses_et_l_axe_du_monde", "poses.txt"), tout);
    return 0;
}
