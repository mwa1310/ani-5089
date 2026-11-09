#include "NKWindow/Core/NkEntry.h"
#include "NKWindow/EntryPoints/NkWindowsDesktop.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkDeviceInitInfo.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
#include "NKRenderer/Materials/NkMaterial.h"
#include "NKRenderer/Core/NkRenderGraph.h"
#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Materials/NkMaterialCollection.h"
#include "NKRenderer/Tools/Offscreen/NkOffscreenTarget.h"
#include "NKRenderer/Tools/Render2D/NkRender2D.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

static const int LC = 640, HC = 720;

static const int IMAGES_APRES = 12;

static NkDrawCall3D Pave(NkMeshHandle mesh, NkVec3f c, NkVec3f t, NkVec3f teinte) {
    NkDrawCall3D dc;
    dc.mesh = mesh;
    dc.transform = NkMat4f::Translate(c) * NkMat4f::Scale(t);
    dc.tint = teinte;
    dc.castShadow = true;
    dc.aabb.min = { c.x - t.x * 0.5f, c.y - t.y * 0.5f, c.z - t.z * 0.5f };
    dc.aabb.max = { c.x + t.x * 0.5f, c.y + t.y * 0.5f, c.z + t.z * 0.5f };
    return dc;
}

static std::vector<NkDrawCall3D> Scene(NkMeshSystem* meshes) {
    const NkMeshHandle cube = meshes->GetCube();
    std::vector<NkDrawCall3D> objets;

    NkDrawCall3D sol;
    sol.mesh = meshes->GetPlane();
    sol.transform = NkMat4f::Translate({ 0.f, 0.f, 0.f }) * NkMat4f::Scale({ 4.f, 1.f, 4.f });
    sol.tint = { 0.45f, 0.40f, 0.35f };
    sol.castShadow = false;
    sol.aabb.min = { -2.f, -0.01f, -2.f };
    sol.aabb.max = { 2.f, 0.01f, 2.f };
    objets.push_back(sol);

    objets.push_back(Pave(cube, { 0.f, 1.25f, -2.05f }, { 4.2f, 2.5f, 0.1f }, { 0.72f, 0.70f, 0.66f }));
    objets.push_back(Pave(cube, { -1.1f, 0.5f, 0.2f }, { 1.f, 1.f, 1.f }, { 0.85f, 0.80f, 0.30f }));
    objets.push_back(Pave(cube, { 0.8f, 0.35f, -0.4f }, { 0.7f, 0.7f, 0.7f }, { 0.55f, 0.38f, 0.20f }));
    return objets;
}

static void Ligne(FILE* f, int image) {
    if (!f) return;
    std::fprintf(f, "image : %d, etat : dessinee\n", image);
    std::fflush(f);
}

static std::string Depot(const char* nom) {
    const std::string p = std::string("../chapitre-08/exo9-le_figement_provoque_puis_leve/") + nom;
    if (FILE* f = std::fopen(p.c_str(), "ab")) {
        std::fclose(f);
        return p;
    }
    return nom;
}

int nkmain(const NkEntryState&) {
    const bool corrige = std::getenv("CORRIGE") != nullptr;

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Cible hors ecran";
    cfgFenetre.width = 1280;
    cfgFenetre.height = 720;
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsValid()) return 1;

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    NkIDevice* device = NkDeviceFactory::CreateAutoDetect(devInit);
    if (!device) return 1;

    NkRenderer* rEcran = NkRenderer::Create(device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
    if (!rEcran || !rEcran->Initialize()) return 1;

    NkOffscreenDesc od;
    od.width = LC;
    od.height = HC;
    od.hasDepth = true;
    od.hdr = false;
    od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
    od.readable = true;
    od.readback = false;
    od.name = "Cible_Exo9";
    NkOffscreenTarget* cible = rEcran->CreateOffscreen(od);
    if (!cible) return 1;

    NkRendererConfig cfgSalle = NkRendererConfig::ForGame(devInit.api, devInit.width, devInit.height);
    cfgSalle.Disable(NK_SS_RENDER2D);
    cfgSalle.Disable(NK_SS_TEXT);
    cfgSalle.Disable(NK_SS_OVERLAY);
    cfgSalle.Enable(NK_SS_OFFSCREEN);
    NkRenderer* rendu = NkRenderer::Create(device, cfgSalle);
    if (!rendu || !rendu->Initialize()) return 1;

    rendu->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));
    if (corrige) rendu->SetRenderSizeOverride(LC, HC);

    const std::vector<NkDrawCall3D> objets = Scene(rendu->GetMeshSystem());

    FILE* sortie = std::fopen(Depot(corrige ? "apres.txt" : "avant.txt").c_str(), "wb");
    FILE* temps = std::fopen(corrige ? "exo9_temps_apres.txt" : "exo9_temps_avant.txt", "wb");

    int image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsValid()) {
        const auto debutImage = std::chrono::steady_clock::now();
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) tourne = false;
        }
        if (!tourne) break;

        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            NkRender3D* r3d = rendu->GetRender3D();
            rendu->FlushGraphRebuilds();
            r3d->ResetFrame();
            if (auto* mc = rendu->GetMaterialCollection()) mc->Upload();

            NkCamera3DData cd;
            cd.position = { 0.3f, 1.7f, 1.2f };
            cd.target = { 0.3f, 1.7f, 0.2f };
            cd.up = { 0.f, 1.f, 0.f };
            cd.fovY = 70.f;
            cd.aspect = (float)LC / (float)HC;
            cd.nearPlane = 0.05f;
            cd.farPlane = 100.f;
            NkSceneContext sctx;
            sctx.camera = NkCamera3D(cd);
            sctx.time = (float)image / 60.f;
            NkLightDesc soleil;
            soleil.type = NkLightType::NK_DIRECTIONAL;
            soleil.direction = { -0.5f, -1.f, -0.35f };
            soleil.color = { 1.f, 0.96f, 0.88f };
            soleil.intensity = 3.f;
            soleil.castShadow = false;
            sctx.lights.PushBack(soleil);
            r3d->BeginScene(sctx);
            for (const NkDrawCall3D& o : objets) r3d->Submit(o);
            if (auto* graph = rendu->GetRenderGraph()) graph->Execute(cmd);

            if (auto* r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cible->GetColorResult(), { 0.f, 0.f, (float)LC, (float)HC });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }
        ++image;
        Ligne(sortie, image);
        if (temps) {
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - debutImage).count();
            std::fprintf(temps, "image : %d, duree : %.1f ms\n", image, ms);
            std::fflush(temps);
        }

        if (corrige && image >= IMAGES_APRES) tourne = false;
    }

    if (sortie) std::fclose(sortie);
    if (temps) std::fclose(temps);
    NkRenderer::Destroy(rendu);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}