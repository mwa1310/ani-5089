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
    dc.mesh       = mesh;
    dc.transform  = NkMat4f::Translate(c) * NkMat4f::Scale(t);
    dc.tint       = teinte;
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

static const double PI_exo6 = 3.14159265358979323846;

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

static NkVec3f VitesseAngulaire(const NkQuatf &from, const NkQuatf &to, float32 dtSeconds) {
    if (dtSeconds <= 0.f) {
        return NkVec3f(0.f, 0.f, 0.f);
    }
    NkQuatf delta = (to * from.Conjugate()).Normalized();
    if (delta.w < 0.f) {
        delta = NkQuatf(-delta.x, -delta.y, -delta.z, -delta.w);
    }
    const float32 angleRad = delta.Angle().Rad();
    if (angleRad <= 1e-6f) {
        return NkVec3f(0.f, 0.f, 0.f);
    }
    return delta.Axis() * (angleRad / dtSeconds);
}

// Prediction de l'orientation
static NkQuatf Extrapoler(const NkQuatf &orientation, const NkVec3f &vitesse, float32 dtSeconds) {
    const float32 speedRad = vitesse.Len();
    if (speedRad > 1e-6f) {
        const NkVec3f axis = vitesse * (1.f / speedRad);
        const NkQuatf delta(NkAngle::FromRad(speedRad * dtSeconds), axis);
        return (delta * orientation).Normalized();
    }
    return orientation;
}

static double Lacet(double t, double amplitude, double duree) {
    if (t <= 0.0) return 0.0;
    if (t >= duree) return amplitude;
    return amplitude * 0.5 * (1.0 - std::cos(PI_exo6 * t / duree));
}

struct Geste {
    const char* nom;
    double amplitude;   // degres
    double duree;       // secondes
};

int nkmain(const NkEntryState&) {
    const double hz = 72.0, periode = 1.0 / hz;
    const Geste gestes[3] = {
        { "lent", 45.0, 1.000 },       
        { "rapide", 90.0, 0.400 },    
        { "demi-tour", 180.0, 0.300 }, 
    };

    std::string sortie;
    for (const Geste& g : gestes) {
        double sommeAvec = 0.0, sommeSans = 0.0;
        int n = 0;
        NkQuatf precedente = NkQuatf::RotateY(NkAngle((float)Lacet(0.0, g.amplitude, g.duree)));
        const int images = (int)(g.duree / periode);
        for (int k = 1; k <= images; ++k) {
            const double t = k * periode;
            const NkQuatf pose = NkQuatf::RotateY(NkAngle((float)Lacet(t, g.amplitude, g.duree)));
            const NkVec3f v = VitesseAngulaire(precedente, pose, (float)periode);     
            const NkQuatf predite = Extrapoler(pose, v, (float)periode);              
            const NkQuatf vraie = NkQuatf::RotateY(NkAngle((float)Lacet(t + periode, g.amplitude, g.duree)));  
            sommeAvec += (double)(vraie * predite.Conjugate()).Normalized().Angle().Deg();
            sommeSans += (double)(vraie * pose.Conjugate()).Normalized().Angle().Deg();
            ++n;
            precedente = pose;
        }
        const double avec = n ? sommeAvec / n : 0.0, sans = n ? sommeSans / n : 0.0;
        sortie += Fmt("%s (%.0f deg en %.0f ms) : avec %.4f deg, sans %.4f deg, rapport %.2f\n", g.nom, g.amplitude, g.duree * 1000.0, avec, sans, avec > 0.0 ? sans / avec : 0.0);
    }
    std::printf("%s", sortie.c_str());
    EcrireTexte(Depot("exo6-ce_que_la_prediction_vous_apporte", "gestes.txt"), sortie);
    return 0;
}
