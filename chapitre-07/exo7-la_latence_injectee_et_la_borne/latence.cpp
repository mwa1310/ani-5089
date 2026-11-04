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
        dc.mesh      = meshes->GetPlane();
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

static const double PI7 = 3.14159265358979323846;

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
    return amplitude * 0.5 * (1.0 - std::cos(PI7 * t / duree));
}

struct Echantillon {
    double temps = 0.0;
    NkQuatf pose;
    NkVec3f vitesse;
};

struct Historique {
    static const unsigned MAX = 64;
    Echantillon ech[MAX];
    unsigned tete = 0, compte = 0;

    void Ajouter(const Echantillon& e) {
        tete = (tete + 1u) % MAX;
        ech[tete] = e;
        if (compte < MAX) ++compte;
    }

    const Echantillon& SampleAt(double maintenant, double latence) const {
        if (latence <= 0.0 || compte <= 1u) return ech[tete];
        const double cible = maintenant - latence;
        unsigned index = tete;
        for (unsigned i = 0; i < compte; ++i) {
            if (ech[index].temps <= cible) return ech[index];
            index = (index + MAX - 1u) % MAX;
        }
        return ech[(tete + MAX - (compte - 1u)) % MAX];
    }
};

static float Borner(float v, float bas, float haut) {
    return v < bas ? bas : (v > haut ? haut : v);
}

static NkQuatf PredictAt(const Echantillon& e, double quand, bool borne, float& horizonSortie) {
    float dt = (float)(quand - e.temps);
    if (borne) dt = Borner(dt, -0.1f, 0.1f);
    horizonSortie = dt;
    return Extrapoler(e.pose, e.vitesse, dt);
}

static double LacetDe(const NkQuatf& q) {   
    return 2.0 * std::atan2((double)q.y, (double)q.w) * 180.0 / PI7;
}

int nkmain(const NkEntryState&) {
    const double periode = 1.0 / 72.0, amplitude = 90.0, duree = 0.400, debutGeste = 1.0;
    auto vrai = [&](double t) { return NkQuatf::RotateY(NkAngle((float)Lacet(t - debutGeste, amplitude, duree))); };

    const int latencesMs[5] = { 0, 10, 20, 40, 80 };
    std::string sortie;
    for (int latMs : latencesMs) {
        const double latence = latMs / 1000.0;
        Historique h;
        double sommeAvec = 0.0, sommeSans = 0.0, maxAvec = 0.0, maxSans = 0.0;
        int n = 0;
        NkQuatf precedente = vrai(0.0);
        // Une pose par periode, trois secondes
        for (int k = 0; k * periode <= 3.0; ++k) {
            const double t = k * periode;
            const NkQuatf pose = vrai(t);
            Echantillon e;
            e.temps = t;
            e.pose = pose;
            e.vitesse = k > 0 ? VitesseAngulaire(precedente, pose, (float)periode) : NkVec3f(0.f, 0.f, 0.f);
            h.Ajouter(e);
            precedente = pose;
            if (t >= 1.0 && t <= 1.9) {
                const Echantillon& s = h.SampleAt(t, latence);
                float horizon = 0.f;
                const NkQuatf predite = PredictAt(s, t + periode, true, horizon);
                const NkQuatf reelle = vrai(t + periode);   // la vraie pose de l'instant d'affichage
                const double eAvec = (double)(reelle * predite.Conjugate()).Normalized().Angle().Deg();
                const double eSans = (double)(reelle * s.pose.Conjugate()).Normalized().Angle().Deg();
                sommeAvec += eAvec; sommeSans += eSans;
                if (eAvec > maxAvec) maxAvec = eAvec;
                if (eSans > maxSans) maxSans = eSans;
                ++n;
            }
        }
        sortie += Fmt("latence %d ms : avec %.4f deg (max %.4f), sans %.4f deg (max %.4f)\n", latMs, sommeAvec / n, maxAvec, sommeSans / n, maxSans);
    }

    std::string borne;
    for (int avecBorne = 0; avecBorne < 2; ++avecBorne) {
        Historique h;
        const double latence = 1.0;
        NkQuatf precedente = vrai(0.0);
        std::string ligne;
        for (int k = 0; k * periode <= 3.0; ++k) {
            const double t = k * periode;
            const NkQuatf pose = vrai(t);
            Echantillon e;
            e.temps = t;
            e.pose = pose;
            e.vitesse = k > 0 ? VitesseAngulaire(precedente, pose, (float)periode) : NkVec3f(0.f, 0.f, 0.f);
            h.Ajouter(e);
            precedente = pose;
            const Echantillon& s = h.SampleAt(t, latence);
            if (t < 1.0 + latence) continue;
            if (std::fabs(s.temps - 1.2) <= periode * 0.5 + 1e-9) {
                float horizon = 0.f;
                const NkQuatf predite = PredictAt(s, t + periode, avecBorne == 1, horizon);
                ligne = Fmt("%s : echantillon a %.4f s, horizon %.1f ms, predit %.1f deg, vrai %.1f deg\n", avecBorne ? "avec borne" : "sans borne",
                            s.temps, (double)horizon * 1000.0, LacetDe(predite), LacetDe(vrai(t + periode)));
                break;
            }
        }
        borne += ligne;
    }
    sortie += borne;
    std::printf("%s", sortie.c_str());
    EcrireTexte(Depot("exo7-la_latence_injectee_et_la_borne", "latences.txt"), sortie);
    return 0;
}
