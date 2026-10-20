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

struct Resultat {
    const char* nom;
    long octets;
    bool lisible;
    int l, h;
    double ecart;   // ecart moyen par canal (sur 255) avec la photo chargee
};

static bool Ecrire(const NkImage& im, int type, const char* chemin) {
    switch (type) {
        case 0:  return im.SavePNG(chemin);
        case 1:  return im.SaveJPEG(chemin, 90);
        case 2:  return im.SaveBMP(chemin);
        case 3:  return im.SaveTGA(chemin);
        default: return im.SaveQOI(chemin);
    }
}

// Compare un pixel sur quatre dans chaque direction, sur les trois canaux de couleur.
static double EcartMoyen(const NkImage& a, const NkImage& b) {
    if (a.Width() != b.Width() || a.Height() != b.Height()) return -1.0;
    double somme = 0.0;
    long n = 0;
    for (int y = 0; y < a.Height(); y += 4) {
        for (int x = 0; x < a.Width(); x += 4) {
            const math::NkColor p = a.GetPixel(x, y), q = b.GetPixel(x, y);
            somme += std::abs((int)p.r - (int)q.r) + std::abs((int)p.g - (int)q.g) + std::abs((int)p.b - (int)q.b);
            n += 3;
        }
    }
    return n ? somme / (double)n : 0.0;
}

int nkmain(const NkEntryState&) {
    NkImage photo;
    if (!photo.Load("murs/beton.jpg") || !photo.IsValid()) {
        EcrireTexte("erreur.txt", "photo murs/beton.jpg introuvable ou illisible");
        return 1;
    }
    const int L = photo.Width(), H = photo.Height();

    const char* noms[5]     = { "png", "jpeg", "bmp", "tga", "qoi" };
    const char* fichiers[5] = { "photo_out.png", "photo_out.jpg", "photo_out.bmp", "photo_out.tga", "photo_out.qoi" };
    Resultat res[5];
    std::string formatsTxt;

    for (int i = 0; i < 5; ++i) {
        Ecrire(photo, i, fichiers[i]);
        res[i].nom = noms[i];
        res[i].octets = TailleFichier(fichiers[i]);
        NkImage relue;
        res[i].lisible = relue.Load(fichiers[i]) && relue.IsValid();
        res[i].l = res[i].lisible ? relue.Width() : 0;
        res[i].h = res[i].lisible ? relue.Height() : 0;
        res[i].ecart = res[i].lisible ? EcartMoyen(photo, relue) : -1.0;
        if (res[i].octets < 0) res[i].octets = 0;
        if (res[i].lisible) formatsTxt += Fmt("%s : %ld octets, %d x %d\n", noms[i], res[i].octets, res[i].l, res[i].h);
        else                formatsTxt += Fmt("%s : %ld octets, illisible\n", noms[i], res[i].octets);
    }
    EcrireTexte(Depot("exo1-treize_formats_une_image", "formats.txt"), formatsTxt);

    // Details bruts, lus ensuite par un script a part
    std::string details = Fmt("photo %d %d\n", L, H);
    for (int i = 0; i < 5; ++i) details += Fmt("%s %ld %d %d %d %.4f\n", noms[i], res[i].octets, res[i].lisible ? 1 : 0, res[i].l, res[i].h, res[i].ecart);
    EcrireTexte("exo1_details.txt", details);
    return 0;
}
