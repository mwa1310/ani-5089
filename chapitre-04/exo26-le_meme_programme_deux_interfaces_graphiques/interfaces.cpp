#include "NKWindow/Core/NkEntry.h"
#include "NKWindow/EntryPoints/NkWindowsDesktop.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkDeviceInitInfo.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
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

static void OuvrirConsole() {
    AllocConsole();
    std::freopen("CONOUT$", "w", stdout);
}

// Pave de dimensions t, centre en c : on compose toujours Translate * Scale.
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

// Le sol est un plan pose a y = 0. Si le plan n'apparait pas chez vous,
// mettre false : le sol devient un cube tres fin.
static const bool SOL_EN_PLAN = true;

static NkDrawCall3D Sol(NkMeshSystem* meshes) {
    NkDrawCall3D dc;
    dc.tint       = { 0.45f, 0.40f, 0.35f };
    dc.castShadow = false;
    if (SOL_EN_PLAN) {
        dc.mesh      = meshes->GetPlane();
        dc.transform = NkMat4f::Translate({ 0.f, 0.f, 0.f }) * NkMat4f::Scale({ 4.f, 1.f, 4.f });
        dc.aabb.min  = { -2.f, -0.01f, -2.f };
        dc.aabb.max  = {  2.f,  0.01f,  2.f };
    } else {
        dc.mesh      = meshes->GetCube();
        dc.transform = NkMat4f::Translate({ 0.f, -0.005f, 0.f }) * NkMat4f::Scale({ 4.f, 0.01f, 4.f });
        dc.aabb.min  = { -2.f, -0.01f, -2.f };
        dc.aabb.max  = {  2.f,  0.00f,  2.f };
    }
    return dc;
}

static void Ecrire(const std::string& fichier, const std::string& texte) {
    if (FILE* f = std::fopen(fichier.c_str(), "w")) {
        std::fprintf(f, "%s\n", texte.c_str());
        std::fclose(f);
    }
}

// L'interface est imposee par la variable EXO_API : VULKAN, SOFTWARE, OPENGL, DX11 ou DX12.
// Le resultat (nom affiche et temps moyen, ou refus) est ecrit dans resultat_<API>.txt.
int nkmain(const NkEntryState&) {
    OuvrirConsole();

    const char* e = std::getenv("EXO_API");
    const std::string voulu = e ? e : "VULKAN";
    const std::string fichier = "resultat_" + voulu + ".txt";

    NkGraphicsApi api = NkGraphicsApi::NK_GFX_API_VULKAN;
    if (voulu == "SOFTWARE")    api = NkGraphicsApi::NK_GFX_API_SOFTWARE;
    else if (voulu == "OPENGL") api = NkGraphicsApi::NK_GFX_API_OPENGL;
    else if (voulu == "DX11")   api = NkGraphicsApi::NK_GFX_API_DX11;
    else if (voulu == "DX12")   api = NkGraphicsApi::NK_GFX_API_DX12;

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Interfaces";
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsOpen()) {
        Ecrire(fichier, "refus fenetre non creee");
        return 1;
    }

    NkDeviceInitInfo init;
    init.surface = fenetre.GetSurfaceDesc();
    init.width   = init.surface.width;
    init.height  = init.surface.height;

    // L'interface est imposee, pas detectee
    NkIDevice* peripherique = NkDeviceFactory::CreateForApi(api, init);
    if (!peripherique || !peripherique->IsValid()) {
        std::printf("Interface %s : peripherique invalide\n", voulu.c_str());
        Ecrire(fichier, "refus peripherique invalide");
        return 1;
    }
    const char* nom = NkGraphicsApiName(api);
    std::printf("Interface graphique : %s\n", nom);

    NkRendererConfig cfg;
    cfg.api        = api;               // CreateForApi ne la recopie pas
    cfg.width      = init.width;
    cfg.height     = init.height;
    cfg.subsystems = NK_SS_RENDER3D;
    cfg.vsync      = false;

    NkRenderer* renderer = NkRenderer::Create(peripherique, cfg);
    if (!renderer || !renderer->Initialize()) {
        std::printf("Interface %s : renderer non initialise\n", voulu.c_str());
        Ecrire(fichier, "refus renderer non initialise");
        return 1;
    }
    renderer->SetFrameRateCap(0.f);
    NkRender3D*   rendu3d = renderer->GetRender3D();
    NkMeshSystem* meshes  = renderer->GetMeshSystem();
    rendu3d->SetIBLStrength(0.5f);
    const NkMeshHandle cube = meshes->GetCube();

    NkSceneContext scene;
    scene.camera.SetPosition({ 1.2f, 1.6f, 1.8f });
    scene.camera.SetTarget({ -0.3f, 1.0f, -0.5f });
    scene.camera.SetUp({ 0.f, 1.f, 0.f });
    scene.camera.SetFOV(70.f);
    scene.camera.SetNearFar(0.05f, 200.f);
    scene.camera.SetAspect(cfg.width, cfg.height);

    NkLightDesc soleil;
    soleil.type      = NkLightType::NK_DIRECTIONAL;
    soleil.direction = { -0.4f, -1.f, -0.3f };
    soleil.color     = { 1.f, 0.95f, 0.85f };
    soleil.intensity = 3.f;
    scene.lights.PushBack(soleil);

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

    // Porte et fenetres : panneaux de 2 cm, avances de 2 mm vers l'interieur
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

    // 400 images mesurees, apres 30 images de chauffe (chargements)
    const unsigned CHAUFFE = 30, IMAGES = 400;
    using horloge = std::chrono::steady_clock;
    horloge::time_point debut = horloge::now();
    unsigned image = 0;
    bool fini = false;

    while (fenetre.IsOpen()) {
        NkEvents().PollEvents();
        if (!renderer->BeginFrame()) continue;
        if (image == CHAUFFE) debut = horloge::now();
        rendu3d->BeginScene(scene);
        for (const NkDrawCall3D& o : objets) rendu3d->Submit(o);
        renderer->Present();
        renderer->EndFrame();
        ++image;

        if (!fini && image == CHAUFFE + IMAGES) {
            const double ms = std::chrono::duration<double, std::milli>(horloge::now() - debut).count() / IMAGES;
            char ligne[128];
            std::snprintf(ligne, sizeof(ligne), "%s %.3f ms", nom, ms);
            Ecrire(fichier, ligne);
            std::printf("Mesure terminee : %s par image. Prenez la capture, puis fermez la fenetre.\n", ligne);
            fini = true;
        }
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(peripherique);
    return 0;
}
