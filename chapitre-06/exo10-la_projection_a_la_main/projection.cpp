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

static const float kPi = 3.14159265f;

// Matrice de projection decentree
static NkMat4f Projection(float gauche, float droite, float haut, float bas, float nearPlane, float farPlane) {
    const float tanLeft  = std::tan(gauche);
    const float tanRight = std::tan(droite);
    const float tanUp = std::tan(haut);
    const float tanDown  = std::tan(bas);
    const float tanWidth  = tanRight - tanLeft;
    const float tanHeight = tanUp - tanDown;

    NkMat4f result;   
    result.mat[0][0] = 2.f / tanWidth;
    result.mat[1][1] = 2.f / tanHeight;
    result.mat[2][0] = (tanRight + tanLeft) / tanWidth; 
    result.mat[2][1] = (tanUp + tanDown) / tanHeight; 
    result.mat[2][3] = -1.f;
    result.mat[2][2] = (farPlane + nearPlane) / (nearPlane - farPlane);
    result.mat[3][2] = 2.f * farPlane * nearPlane / (nearPlane - farPlane);
    return result;
}

// Ecarts
static std::string Comparer(const char* nom, const NkMat4f& a, const NkMat4f& b, double& plusGrand) {
    std::string s;
    plusGrand = 0.0;
    for (int ligne = 0; ligne < 4; ++ligne) {
        double e[4];
        for (int col = 0; col < 4; ++col) {
            e[col] = std::fabs((double)a.mat[col][ligne] - (double)b.mat[col][ligne]);
            if (e[col] > plusGrand) plusGrand = e[col];
        }
        s += Fmt("%s, ligne %d : %.3e %.3e %.3e %.3e\n", nom, ligne, e[0], e[1], e[2], e[3]);
    }
    return s;
}

int nkmain(const NkEntryState&) {
    const float proche = 0.05f, loin = 100.f;

    // Epreuve symetrique 
    const float demiV = 35.f * kPi / 180.f;
    const float tanH = std::tan(demiV) * (1280.f / 720.f);
    const NkMat4f mienneSym = Projection(-std::atan(tanH), std::atan(tanH), demiV, -demiV, proche, loin);
    const NkMat4f moteurSym = NkMat4f::Perspective(NkAngle(70.f), 1280.f / 720.f, proche, loin);

    // Epreuve quest 
    const float gauche = -52.f * kPi / 180.f, droite = 45.f * kPi / 180.f, haut = 48.f * kPi / 180.f, bas = -50.f * kPi / 180.f;
    const NkMat4f mienneQuest = Projection(gauche, droite, haut, bas, proche, loin);
    NkCamera3D cam;
    cam.SetNearFar(proche, loin);
    cam.SetFovAsym(gauche, droite, haut, bas);
    const NkMat4f moteurQuest = cam.GetProj();

    double maxSym = 0.0, maxQuest = 0.0;
    std::string ecarts = Comparer("symetrique", mienneSym, moteurSym, maxSym);
    ecarts += Comparer("quest", mienneQuest, moteurQuest, maxQuest);
    ecarts += Fmt("symetrique : plus grand ecart %.3e\n", maxSym);
    ecarts += Fmt("quest : plus grand ecart %.3e\n", maxQuest);
    std::printf("%s", ecarts.c_str());
    EcrireTexte(Depot("exo10-la_projection_a_la_main", "ecarts.txt"), ecarts);
    return 0;
}
