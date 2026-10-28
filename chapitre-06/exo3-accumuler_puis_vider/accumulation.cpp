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

static void OuvrirConsole() {
    AllocConsole();
    std::freopen("CONOUT$", "w", stdout);
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

int nkmain(const NkEntryState&) {
    OuvrirConsole();

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Accumulation";
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsOpen()) return 1;

    NkDeviceInitInfo init;
    init.surface = fenetre.GetSurfaceDesc();
    init.width = init.surface.width;
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
    NkRender3D* rendu3d = renderer->GetRender3D();
    NkMeshSystem* meshes  = renderer->GetMeshSystem();
    rendu3d->SetIBLStrength(0.5f);
    const NkMeshHandle cube = meshes->GetCube();

    // Symptome : la tete tourne toute seule quand on ne touche plus a la souris.
    // Cause : NkInput.MouseRawDeltaX() garde le delta du dernier evenement brut et personne ne le remet a zero.
    // Preuve : constate en relisant le code du module d'entrees (NkEventState) : OnRaw ecrase la valeur, aucun appelant ne la consomme.
    // Correction : s'abonner aux evenements bruts, accumuler soi-meme, lire une fois par image, puis vider l'accumulateur.
    const float SENSIBILITE = 0.002f;
    float lacetLu = 0.f, lacetVide = 0.f, lacetSans = 0.f;
    int accumVide = 0, accumSans = 0;    
    const NkVec3f position = { 0.f, 1.70f, 1.5f };
    EcrireTexte("trois_log.txt", "");

    NkSceneContext scene;
    scene.camera.SetPosition(position);
    scene.camera.SetUp({ 0.f, 1.f, 0.f });
    scene.camera.SetFOV(70.f);
    scene.camera.SetNearFar(0.05f, 200.f);
    scene.camera.SetAspect(cfg.width, cfg.height);

    NkLightDesc soleil;
    soleil.type = NkLightType::NK_DIRECTIONAL;
    soleil.direction = { -0.4f, -1.f, -0.3f };
    soleil.color = { 1.f, 0.95f, 0.85f };
    soleil.intensity = 3.f;
    scene.lights.PushBack(soleil);

    const float H = 2.5f, E = 0.1f, C = 2.05f;   // hauteur, epaisseur, centre des murs
    const NkVec3f mur = { 0.72f, 0.70f, 0.66f };
    const NkVec3f clair = { 0.85f, 0.85f, 0.85f };
    std::vector<NkDrawCall3D> objets;
    objets.push_back(Sol(meshes));
    objets.push_back(Pave(cube, { 0.f, H * 0.5f, -C }, { 4.2f, H, E }, mur));   // fond
    objets.push_back(Pave(cube, { 0.f, H * 0.5f,  C }, { 4.2f, H, E }, mur));   // entree
    objets.push_back(Pave(cube, { -C, H * 0.5f, 0.f }, { E, H, 4.0f }, mur));   // gauche
    objets.push_back(Pave(cube, {  C, H * 0.5f, 0.f }, { E, H, 4.0f }, mur));   // droite
    objets.push_back(Pave(cube, { 0.f, H + E * 0.5f, 0.f }, { 4.2f, E, 4.2f }, clair));   // plafond

    fenetre.ShowMouse(false);
    fenetre.CaptureMouse(true);
    fenetre.ClipMouseToClient(true);

    NkChrono chrono;
    int prochaineSeconde = 1;
    bool tourne = true;
    while (tourne && fenetre.IsOpen()) {
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) tourne = false;
            if (auto* k = ev->As<NkKeyPressEvent>()) {
                if (k->GetKey() == NkKey::NK_ESCAPE) tourne = false;
            }
            // A chaque evenement brut : on ajoute. On ne vide pas ici.
            if (auto* r = ev->As<NkMouseRawEvent>()) {
                accumVide += r->GetDeltaX();
                accumSans += r->GetDeltaX();
            }
        }
        if (!tourne) break;

        // Version 1, fautive 
        lacetLu += (float)NkInput.MouseRawDeltaX() * SENSIBILITE;

        // Version 2, juste 
        lacetVide += (float)accumVide * SENSIBILITE;
        accumVide = 0;

        // Version 3 : meme lecture
        lacetSans += (float)accumSans * SENSIBILITE;

        const NkVec3f d = { -std::sin(lacetVide), 0.f, -std::cos(lacetVide) };
        scene.camera.SetTarget({ position.x + d.x, position.y + d.y, position.z + d.z });

        if (!renderer->BeginFrame()) continue;
        rendu3d->BeginScene(scene);
        for (const NkDrawCall3D& o : objets) rendu3d->Submit(o);
        renderer->Present();
        renderer->EndFrame();

        const double t = chrono.Elapsed().ToMilliseconds() / 1000.0;
        if (t >= (double)prochaineSeconde) {
            const double k = 180.0 / 3.14159265;
            const std::string ligne = Fmt("t = %.1f s, lu = %.1f, vide = %.1f, sans vidage = %.1f\n", t, lacetLu * k, lacetVide * k, lacetSans * k);
            std::printf("%s", ligne.c_str());
            if (FILE* f = std::fopen("trois_log.txt", "ab")) { std::fputs(ligne.c_str(), f); std::fclose(f); }
            ++prochaineSeconde;
        }
    }

    EcrireTexte("exo3_details.txt", Fmt("sensibilite %.5f\nbrut %d\naccum %d\n", SENSIBILITE, (int)NkInput.MouseRawDeltaX(), accumSans));
    fenetre.ShowMouse(true);
    fenetre.CaptureMouse(false);
    fenetre.ClipMouseToClient(false);
    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(peripherique);
    return 0;
}
