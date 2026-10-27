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

int nkmain(const NkEntryState&) {
    const float lacetDeg = 45.f, tangageDeg = 30.f;
    const NkQuatf qLacet = NkQuatf::RotateY(NkAngle(lacetDeg));
    const NkQuatf qTangage = NkQuatf::RotateX(NkAngle(tangageDeg));
    const NkVec3f avant(0.f, 0.f, -1.f);

    const NkVec3f a = (qLacet * qTangage) * avant;
    const NkVec3f b = (qTangage * qLacet) * avant;

    const NkMat4f rotation = NkMat4f::RotationY(NkAngle(lacetDeg)) * NkMat4f::RotationX(NkAngle(tangageDeg));
    const NkQuatf q = NkQuatf(rotation).Normalized();
    const NkVec3f c = q * avant;

    const float yMain = std::sin(tangageDeg * 3.14159265f / 180.f);

    std::string regards;
    regards += Fmt("lacet*tangage : (%.4f, %.4f, %.4f)\n", a.x, a.y, a.z);
    regards += Fmt("tangage*lacet : (%.4f, %.4f, %.4f)\n", b.x, b.y, b.z);
    regards += Fmt("matrices : (%.4f, %.4f, %.4f)\n", c.x, c.y, c.z);
    std::printf("%s", regards.c_str());
    EcrireTexte(Depot("exo5-les_deux_ordres_et_la_matrice", "regards.txt"), regards);
    EcrireTexte("exo5_details.txt", Fmt("main %.4f\n", yMain));
    return 0;
}
