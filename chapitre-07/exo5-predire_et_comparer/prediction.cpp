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

static std::vector<NkDrawCall3D> ConstruireSalle(NkMeshSystem* meshes) {
    const NkMeshHandle cube = meshes->GetCube();

    const float H = 2.5f, E = 0.1f, C = 2.05f;   // hauteur, epaisseur, centre des murs
    const NkVec3f mur   = { 0.72f, 0.70f, 0.66f };
    const NkVec3f clair = { 0.85f, 0.85f, 0.85f };
    std::vector<NkDrawCall3D> objets;
    objets.push_back(Sol(meshes));
    objets.push_back(Pave(cube, { 0.f, H * 0.5f, -C }, { 4.2f, H, E }, mur));   // fond
    objets.push_back(Pave(cube, { 0.f, H * 0.5f,  C }, { 4.2f, H, E }, mur));   // entree
    objets.push_back(Pave(cube, { -C, H * 0.5f, 0.f }, { E, H, 4.0f }, mur));   // gauche
    objets.push_back(Pave(cube, {  C, H * 0.5f, 0.f }, { E, H, 4.0f }, mur));   // droite
    objets.push_back(Pave(cube, { 0.f, H + E * 0.5f, 0.f }, { 4.2f, E, 4.2f }, clair));   // plafond

    // Porte et fenetres 
    const float EP = 0.02f, AVANCE = 0.002f;
    const NkVec3f porte = { 0.40f, 0.25f, 0.12f };
    const NkVec3f vitre = { 0.55f, 0.75f, 0.90f };
    objets.push_back(Pave(cube, { -0.70f, 1.00f, -2.0f - EP * 0.5f + AVANCE }, { 0.90f, 2.00f, EP }, porte));
    objets.push_back(Pave(cube, { -2.0f - EP * 0.5f + AVANCE, 1.40f, -0.90f }, { EP, 1.00f, 1.20f }, vitre));
    objets.push_back(Pave(cube, { -2.0f - EP * 0.5f + AVANCE, 1.40f,  0.90f }, { EP, 1.00f, 1.20f }, vitre));

    const NkVec3f bois = { 0.55f, 0.38f, 0.20f };
    objets.push_back(Pave(cube, { 0.60f, 0.725f, 0.70f }, { 1.20f, 0.05f, 0.80f }, bois));
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            objets.push_back(Pave(cube, { 0.60f + sx * 0.57f, 0.35f, 0.70f + sz * 0.37f }, { 0.06f, 0.70f, 0.06f }, bois));
    objets.push_back(Pave(cube, { -1.10f, 0.50f, 1.20f }, { 1.f, 1.f, 1.f }, { 0.85f, 0.80f, 0.30f }));
    return objets;
}

static NkLightDesc Soleil() {
    NkLightDesc soleil;
    soleil.type = NkLightType::NK_DIRECTIONAL;
    soleil.direction  = { -0.5f, -1.f, -0.35f };
    soleil.color = { 1.f, 0.96f, 0.88f };
    soleil.intensity  = 3.f;
    soleil.castShadow = false;
    return soleil;
}

static const double PI = 3.14159265358979323846;

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

static NkQuatf Orientation(float lacetRad, float tangageRad) {
    const NkMat4f rotation = NkMat4f::RotationY(NkAngle::FromRad(lacetRad)) * NkMat4f::RotationX(NkAngle::FromRad(tangageRad));
    return NkQuatf(rotation).Normalized();
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

static NkQuatf Extrapoler(const NkQuatf &orientation, const NkVec3f &vitesse, float32 dtSeconds) {
    const float32 speedRad = vitesse.Len();
    if (speedRad > 1e-6f) {
        const NkVec3f axis = vitesse * (1.f / speedRad);
        const NkQuatf delta(NkAngle::FromRad(speedRad * dtSeconds), axis);
        return (delta * orientation).Normalized();
    }
    return orientation;
}

int nkmain(const NkEntryState&) {
    const double hz = CadenceChoisie();          
    const float periode = (float)(1.0 / hz);      
    const float SENSIBILITE = 0.0022f;         

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Prediction";
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsOpen()) return 1;

    NkDeviceInitInfo init;
    init.surface = fenetre.GetSurfaceDesc();
    init.width   = init.surface.width;
    init.height  = init.surface.height;

    NkIDevice* peripherique = NkDeviceFactory::CreateAutoDetect(init);
    if (!peripherique || !peripherique->IsValid()) return 1;

    NkRendererConfig cfg;
    cfg.api = init.api;
    cfg.width = init.width;
    cfg.height = init.height;
    cfg.subsystems = NK_SS_RENDER3D;

    NkRenderer* renderer = NkRenderer::Create(peripherique, cfg);
    if (!renderer || !renderer->Initialize()) return 1;
    NkRender3D*   rendu3d = renderer->GetRender3D();
    NkMeshSystem* meshes  = renderer->GetMeshSystem();
    rendu3d->SetIBLStrength(0.5f);
    const NkMeshHandle cube = meshes->GetCube();

    const NkVec3f position = { 0.30f, 1.70f, 1.20f };
    NkSceneContext scene;
    scene.camera.SetPosition(position);
    scene.camera.SetUp({ 0.f, 1.f, 0.f });
    scene.camera.SetFOV(70.f);
    scene.camera.SetNearFar(0.05f, 200.f);
    scene.camera.SetAspect(cfg.width, cfg.height);
    scene.lights.PushBack(Soleil());
    std::vector<NkDrawCall3D> objets = ConstruireSalle(meshes);

    fenetre.ShowMouse(false);
    fenetre.CaptureMouse(true);
    fenetre.ClipMouseToClient(true);

    float lacet = 0.f, tangage = 0.f;
    int accumX = 0, accumY = 0;
    NkQuatf precedente = Orientation(0.f, 0.f), predite = precedente;
    double precedentT = 0.0;
    double sommeAvec = 0.0, sommeSans = 0.0, maxAvec = 0.0, maxSans = 0.0;
    unsigned long long n = 0, immobiles = 0, image = 0;
    const unsigned long long ECARTEES = 30, MESURES = 1000;
    bool tourne = true;
    while (tourne && fenetre.IsOpen() && n < MESURES) {
        const double debut = Nanos();
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) tourne = false;
            if (auto* k = ev->As<NkKeyPressEvent>()) {
                if (k->GetKey() == NkKey::NK_ESCAPE) tourne = false;
            }
            if (auto* r = ev->As<NkMouseRawEvent>()) {
                accumX += r->GetDeltaX();
                accumY += r->GetDeltaY();
            }
        }
        if (!tourne) break;
        lacet   -= (float)accumX * SENSIBILITE;
        tangage -= (float)accumY * SENSIBILITE;
        if (tangage >  1.5533f) tangage =  1.5533f;
        if (tangage < -1.5533f) tangage = -1.5533f;
        accumX = 0;
        accumY = 0;

        const NkQuatf mesuree = Orientation(lacet, tangage);
        const float dt = (float)((debut - precedentT) / 1e9);
        if (image > ECARTEES) {
            // Prediction faite a l'image d'avant
            const double eAvec = (double)(mesuree * predite.Conjugate()).Normalized().Angle().Deg();
            // Sans prediction
            const double eSans = (double)(mesuree * precedente.Conjugate()).Normalized().Angle().Deg();
            sommeAvec += eAvec; sommeSans += eSans;
            if (eAvec > maxAvec) maxAvec = eAvec;
            if (eSans > maxSans) maxSans = eSans;
            if (eSans < 0.001) ++immobiles;
            ++n;
        }
        const NkVec3f v = VitesseAngulaire(precedente, mesuree, dt);
        predite = Extrapoler(mesuree, v, periode);
        precedente = mesuree;
        precedentT = debut;

        const NkVec3f f = mesuree * NkVec3f(0.f, 0.f, -1.f);
        scene.camera.SetTarget({ position.x + f.x, position.y + f.y, position.z + f.z });
        if (!renderer->BeginFrame()) continue;
        rendu3d->BeginScene(scene);
        for (const NkDrawCall3D& o : objets) rendu3d->Submit(o);
        renderer->Present();
        renderer->EndFrame();
        ++image;
    }

    if (n > 0) {
        std::string s;
        s += Fmt("avec prediction : moyenne %.4f deg, max %.4f deg, sur %llu images\n", sommeAvec / (double)n, maxAvec, n);
        s += Fmt("sans prediction : moyenne %.4f deg, max %.4f deg, sur %llu images\n", sommeSans / (double)n, maxSans, n);
        EcrireTexte(Depot("exo5-predire_et_comparer", "erreurs.txt"), s);
        EcrireTexte("exo5_details.txt", Fmt("hz %.1f\nimmobiles %llu\nimages %llu\n", hz, immobiles, n));
    }
    fenetre.ShowMouse(true);
    fenetre.CaptureMouse(false);
    fenetre.ClipMouseToClient(false);

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(peripherique);
    return 0;
}
