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

static NkQuatf ParMatrices(float lacetDeg, float tangageDeg) {
    const NkMat4f rotation = NkMat4f::RotationY(NkAngle(lacetDeg)) * NkMat4f::RotationX(NkAngle(tangageDeg));
    return NkQuatf(rotation).Normalized();
}

static NkQuatf Direct(float lacetDeg, float tangageDeg, bool lacetFirst) {
    const NkQuatf qy = NkQuatf::RotateY(NkAngle(lacetDeg));
    const NkQuatf qx = NkQuatf::RotateX(NkAngle(tangageDeg));
    return (lacetFirst ? qy * qx : qx * qy).Normalized();
}

static float Proche(const NkVec3f& u, const NkVec3f& v) {
    return std::fabs(u.x - v.x) + std::fabs(u.y - v.y) + std::fabs(u.z - v.z);
}

int nkmain(const NkEntryState&) {
    const NkVec3f avant(0.f, 0.f, -1.f), droite(1.f, 0.f, 0.f), haut(0.f, 1.f, 0.f);

    const NkQuatf qm = ParMatrices(45.f, 30.f);
    const bool lacetFirst = Proche(qm * avant, Direct(45.f, 30.f, true) * avant) + Proche(qm * droite, Direct(45.f, 30.f, true) * droite) < 1e-4f;
    const NkQuatf qd = Direct(45.f, 30.f, lacetFirst);
    const float ecartVerif = Proche(qm * avant, qd * avant) + Proche(qm * droite, qd * droite) + Proche(qm * haut, qd * haut);

    const int N = 1000000;
    double mesures[2][3];
    volatile float puits = 0.f;
    for (int essai = 0; essai < 3; ++essai) {
        for (int version = 0; version < 2; ++version) {
            float somme = 0.f;
            NkChrono chrono;
            for (int i = 0; i < N; ++i) {
                const float lacetDeg = (float)(i % 3600) * 0.1f;
                const float tangageDeg = 30.f + (float)(i % 7);
                const NkQuatf q = version == 0 ? ParMatrices(lacetDeg, tangageDeg) : Direct(lacetDeg, tangageDeg, lacetFirst);
                const NkVec3f f = q * avant, r = q * droite, u = q * haut;
                somme += f.x + f.y + f.z + r.x + r.y + r.z + u.x + u.y + u.z;
            }
            mesures[version][essai] = chrono.Elapsed().ToMilliseconds();
            puits = puits + somme;
        }
    }

    const char* envConfig = std::getenv("CONFIG_NOM");
#ifdef NDEBUG
    const char* config = envConfig ? envConfig : "Release";
#else
    const char* config = envConfig ? envConfig : "Debug";
#endif
    std::string lignes;
    for (int essai = 0; essai < 3; ++essai) lignes += Fmt("%s, matrices : %.2f ms\n", config, mesures[0][essai]);
    for (int essai = 0; essai < 3; ++essai) lignes += Fmt("%s, quaternions : %.2f ms\n", config, mesures[1][essai]);
    EcrireTexte(Fmt("mesures_%s.txt", config), lignes);
    EcrireTexte(Fmt("exo7_details_%s.txt", config), Fmt("ordre %s\nverif %.6f\npuits %.3f\n", lacetFirst ? "lacet*tangage" : "tangage*lacet", ecartVerif, (double)puits));
    std::printf("%s", lignes.c_str());
    return 0;
}
