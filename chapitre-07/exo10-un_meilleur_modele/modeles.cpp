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
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"
#include "NKEvent/NkMouseEvent.h"
#include "NKEvent/NkKeyboardEvent.h"
#include "NKRenderer/Core/NkRenderGraph.h"
#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Materials/NkMaterialCollection.h"
#include "NKRenderer/Tools/Offscreen/NkOffscreenTarget.h"
#include "NKRenderer/Tools/Render2D/NkRender2D.h"
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
    const std::string p = std::string("../chapitre-07/") + dossier + "/" + nom;
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

static NkDrawCall3D Pave(NkMeshHandle mesh, NkVec3f c, NkVec3f t, NkVec3f teinte, bool ombre = true) {
    NkDrawCall3D dc;
    dc.mesh = mesh;
    dc.transform  = NkMat4f::Translate(c) * NkMat4f::Scale(t);
    dc.tint = teinte;
    dc.castShadow = ombre;
    dc.aabb.min   = { c.x - t.x * 0.5f, c.y - t.y * 0.5f, c.z - t.z * 0.5f };
    dc.aabb.max   = { c.x + t.x * 0.5f, c.y + t.y * 0.5f, c.z + t.z * 0.5f };
    return dc;
}

static const bool SOL_EN_PLAN = true;

static NkDrawCall3D Sol(NkMeshSystem* meshes) {
    NkDrawCall3D dc;
    dc.tint = { 0.45f, 0.40f, 0.35f };
    dc.castShadow = false;
    if (SOL_EN_PLAN) {
        dc.mesh = meshes->GetPlane();
        dc.transform = NkMat4f::Translate({ 0.f, 0.f, 0.f }) * NkMat4f::Scale({ 4.f, 1.f, 4.f });
        dc.aabb.min  = { -2.f, -0.01f, -2.f };
        dc.aabb.max  = {  2.f,  0.01f,  2.f };
    } else {
        dc.mesh = meshes->GetCube();
        dc.transform = NkMat4f::Translate({ 0.f, -0.005f, 0.f }) * NkMat4f::Scale({ 4.f, 0.01f, 4.f });
        dc.aabb.min  = { -2.f, -0.01f, -2.f };
        dc.aabb.max  = {  2.f,  0.00f,  2.f };
    }
    return dc;
}

static const double PI10 = 3.14159265358979323846;

static unsigned long long EnvU64(const char* nom, unsigned long long defaut) {
    const char* v = std::getenv(nom);
    return v ? (unsigned long long)std::atoll(v) : defaut;
}

static double Nanos() {
    return (double)NkChrono::Now().nanoseconds;
}

static double FrequenceEcran() {
    DEVMODEA dm;
    std::memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &dm)) return (double)dm.dmDisplayFrequency;
    return 0.0;
}

static double CadenceChoisie() {
    const char* v = std::getenv("MASALLE_HZ");
    if (v) return std::atof(v);
    const double f = FrequenceEcran();
    return f > 1.0 ? f : 72.0;
}

static const char* ModeAttente() {
    return std::getenv("NK_VK_MAILBOX") ? "sans_attente" : "ecran";
}

static double LacetDoux(double t, double amplitude, double duree) {
    if (t <= 0.0) return 0.0;
    if (t >= duree) return amplitude;
    return amplitude * 0.5 * (1.0 - std::cos(PI10 * t / duree));
}

static double LacetArret(double t, double vitesse, double duree) {
    if (t <= 0.0) return 0.0;
    if (t >= duree) return vitesse * duree;
    return vitesse * t;
}

struct Enregistrement {
    const char* nom;
    int type;          
    double amplitude;  
    double duree;      
};

static double Y(const Enregistrement& e, double t) {
    return e.type == 0 ? LacetDoux(t, e.amplitude, e.duree) : LacetArret(t, e.amplitude, e.duree);
}

int nkmain(const NkEntryState&) {
    const double T = 1.0 / 72.0; 
    const Enregistrement enregistrements[4] = {
        { "lent", 0, 45.0,  1.000 }, 
        { "rapide", 0, 90.0,  0.400 }, 
        { "demi-tour", 0, 180.0, 0.300 },    
        { "arret", 1, 150.0, 0.500 }, 
    };

    std::string sortie;
    for (const Enregistrement& e : enregistrements) {
        const int images = (int)((e.duree + 0.200) / T);
        double sV = 0.0, sA = 0.0, mV = 0.0, mA = 0.0;
        int n = 0;
        for (int k = 2; k <= images; ++k) {
            const double y0 = Y(e, (k - 2) * T), y1 = Y(e, (k - 1) * T), y2 = Y(e, k * T);  
            const double vitesse = (y2 - y1) / T;   
            const double acceleration = (y2 - 2.0 * y1 + y0) / (T * T);   
            const double predV = y2 + vitesse * T;      
            const double predA = y2 + vitesse * T + acceleration * T * T / 2.0;  
            const double vraie = Y(e, (k + 1) * T);           
            const double eV = std::fabs(vraie - predV), eA = std::fabs(vraie - predA);
            sV += eV; sA += eA;
            if (eV > mV) mV = eV;
            if (eA > mA) mA = eA;
            ++n;
        }
        if (e.type == 0) sortie += Fmt("%s (%.0f deg en %.0f ms) : vitesse %.4f deg (max %.4f), acceleration %.4f deg (max %.4f)\n", e.nom, e.amplitude, e.duree * 1000.0, sV / n, mV, sA / n, mA);
        else             sortie += Fmt("%s (%.0f deg/s pendant %.0f ms, puis arret) : vitesse %.4f deg (max %.4f), acceleration %.4f deg (max %.4f)\n", e.nom, e.amplitude, e.duree * 1000.0, sV / n, mV, sA / n, mA);
    }
    std::printf("%s", sortie.c_str());
    EcrireTexte(Depot("exo10-un_meilleur_modele", "modeles.txt"), sortie);
    return 0;
}
