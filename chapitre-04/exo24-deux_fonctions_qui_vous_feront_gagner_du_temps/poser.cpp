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

// Transformation d'un objet de dimensions "taille", pose au sol a y = 0.
NkMat4f PoserAuSol(NkVec3f taille, float x, float z) {
    return NkMat4f::Translate({ x, taille.y * 0.5f, z }) * NkMat4f::Scale(taille);
}

// Meme chose, mais pose sur le dessus de la table, a 0,75 m du sol.
NkMat4f PoserSurTable(NkVec3f taille, float x, float z) {
    return NkMat4f::Translate({ x, 0.75f + taille.y * 0.5f, z }) * NkMat4f::Scale(taille);
}

// La boite englobante se deduit de la matrice : elle suit toujours la vraie place.
static NkDrawCall3D Objet(NkMeshHandle mesh, const NkMat4f& m, NkVec3f teinte) {
    NkDrawCall3D dc;
    dc.mesh = mesh;
    dc.transform = m;
    dc.tint = teinte;
    dc.aabb = NkAABB::Unit().Transformed(m);
    return dc;
}

// Controle sur un pave plat dont les trois dimensions different.
static void Verifier() {
    const NkVec3f t = { 0.4f, 0.1f, 0.2f };
    const NkVec4f origine = { 0.f, 0.f, 0.f, 1.f };
    const NkVec4f sol = PoserAuSol(t, 1.f, 2.f) * origine;
    const NkVec4f table   = PoserSurTable(t, 1.f, 2.f) * origine;
    if (FILE* f = std::fopen("verif.txt", "w")) {
        std::fprintf(f, "PoserAuSol : %.3f %.3f %.3f (attendu 1 0.05 2)\n", sol.x, sol.y, sol.z);
        std::fprintf(f, "PoserSurTable : %.3f %.3f %.3f (attendu 1 0.80 2)\n", table.x, table.y, table.z);
        std::fclose(f);
    }
}

int nkmain(const NkEntryState&) {
    Verifier();

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Poser";
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

    NkSceneContext scene;
    scene.camera.SetPosition({ 1.2f, 1.6f, 1.8f });
    scene.camera.SetTarget({ -0.3f, 1.0f, -0.5f });
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

    // DEBUT SALLE
    const NkVec3f mur = { 0.72f, 0.70f, 0.66f };
    const NkVec3f clair = { 0.85f, 0.85f, 0.85f };
    const NkVec3f bois  = { 0.55f, 0.38f, 0.20f };
    const NkVec3f porte = { 0.40f, 0.25f, 0.12f };
    const NkVec3f vitre = { 0.55f, 0.75f, 0.90f };
    const NkVec3f rouge = { 0.80f, 0.20f, 0.20f };
    std::vector<NkDrawCall3D> objets;
    objets.push_back(Sol(meshes));
    objets.push_back(Objet(cube, PoserAuSol({ 4.2f, 2.5f, 0.1f }, 0.f, -2.05f), mur));
    objets.push_back(Objet(cube, PoserAuSol({ 4.2f, 2.5f, 0.1f }, 0.f,  2.05f), mur));
    objets.push_back(Objet(cube, PoserAuSol({ 0.1f, 2.5f, 4.0f }, -2.05f, 0.f), mur));
    objets.push_back(Objet(cube, PoserAuSol({ 0.1f, 2.5f, 4.0f },  2.05f, 0.f), mur));
    objets.push_back(Objet(cube, NkMat4f::Translate({ 0.f, 2.55f, 0.f }) * NkMat4f::Scale({ 4.2f, 0.1f, 4.2f }), clair));
    objets.push_back(Objet(cube, PoserAuSol({ 0.90f, 2.00f, 0.02f }, -0.70f, -2.008f), porte));
    objets.push_back(Objet(cube, NkMat4f::Translate({ -2.008f, 1.40f, -0.90f }) * NkMat4f::Scale({ 0.02f, 1.00f, 1.20f }), vitre));
    objets.push_back(Objet(cube, NkMat4f::Translate({ -2.008f, 1.40f,  0.90f }) * NkMat4f::Scale({ 0.02f, 1.00f, 1.20f }), vitre));
    objets.push_back(Objet(cube, NkMat4f::Translate({ 0.60f, 0.725f, 0.70f }) * NkMat4f::Scale({ 1.20f, 0.05f, 0.80f }), bois));
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2)
            objets.push_back(Objet(cube, PoserAuSol({ 0.06f, 0.70f, 0.06f }, 0.60f + sx * 0.57f, 0.70f + sz * 0.37f), bois));
    objets.push_back(Objet(cube, PoserSurTable({ 0.20f, 0.20f, 0.20f }, 0.60f, 0.70f), rouge));
    // FIN SALLE

    while (fenetre.IsOpen()) {
        NkEvents().PollEvents();
        if (!renderer->BeginFrame()) continue;
        rendu3d->BeginScene(scene);
        for (const NkDrawCall3D& o : objets) rendu3d->Submit(o);
        renderer->Present();
        renderer->EndFrame();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(peripherique);
    return 0;
}
