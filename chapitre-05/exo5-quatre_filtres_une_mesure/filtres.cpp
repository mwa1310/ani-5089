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

int nkmain(const NkEntryState&) {
    NkImage photo;
    if (!photo.Load("murs/beton.jpg") || !photo.IsValid()) {
        EcrireTexte("erreur.txt", "photo murs/beton.jpg introuvable ou illisible");
        return 1;
    }
    // Source : 2048 x 2048
    NkImage source = photo.Resize(2048, 2048, NkResizeFilter::NK_BILINEAR);

    const NkResizeFilter filtres[4] = { NkResizeFilter::NK_NEAREST, NkResizeFilter::NK_BILINEAR,
                                        NkResizeFilter::NK_BICUBIC, NkResizeFilter::NK_LANCZOS3 };
    const char* noms[4]     = { "NK_NEAREST", "NK_BILINEAR", "NK_BICUBIC", "NK_LANCZOS3" };
    const char* fichiers[4] = { "nearest.png", "bilinear.png", "bicubic.png", "lanczos3.png" };

    NkImage res[4];
    double us[4];
    const int REPETITIONS = 20;
    for (int i = 0; i < 4; ++i) {
        double total = 0.0;
        for (int r = 0; r < REPETITIONS; ++r) {
            NkChrono chrono;
            NkImage x = source.Resize(256, 256, filtres[i]);
            total += chrono.Elapsed().ToMicroseconds();
            if (r == REPETITIONS - 1) res[i] = std::move(x);
        }
        us[i] = total / REPETITIONS;
        res[i].SavePNG(Depot("exo5-quatre_filtres_une_mesure", fichiers[i]).c_str());
    }

    // Comparaison pixel par pixel avec le bilineaire : un pixel differe s'il change sur au moins un canal
    int diff[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < 4; ++i) {
        for (int y = 0; y < 256; ++y) {
            for (int x = 0; x < 256; ++x) {
                const math::NkColor p = res[i].GetPixel(x, y), q = res[1].GetPixel(x, y);
                if (p.r != q.r || p.g != q.g || p.b != q.b || p.a != q.a) ++diff[i];
            }
        }
    }

    std::string mesures;
    for (int i = 0; i < 4; ++i) mesures += Fmt("%s : %.0f us, differences : %d\n", noms[i], us[i], diff[i]);
    EcrireTexte(Depot("exo5-quatre_filtres_une_mesure", "mesures.txt"), mesures);

    return 0;
}
