#include "NKWindow/Core/NkEntry.h"
#include "NKWindow/EntryPoints/NkWindowsDesktop.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkDeviceInitInfo.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
#include "NKImage/Core/NkImage.h"
#include "NKRenderer/Materials/NkMaterial.h"
#include "NKEvent/NkEventSystem.h"
#include "NKEvent/NkWindowEvent.h"
#include "NKRenderer/Core/NkRenderGraph.h"
#include "NKRenderer/Core/NkTextureLibrary.h"
#include "NKRenderer/Materials/NkMaterialCollection.h"
#include "NKRenderer/Tools/Offscreen/NkOffscreenTarget.h"
#include "NKRenderer/Tools/Render2D/NkRender2D.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string>

using namespace nkentseu;
using namespace nkentseu::renderer;

namespace {

constexpr unsigned kLargeur = 1280;
constexpr unsigned kHauteur = 720;
constexpr float kPi = 3.14159265f;

NkVec3f Position(const NkMat4f& m) {
    return { m.m30, m.m31, m.m32 };
}

NkDrawCall3D Cube(NkMeshHandle mesh, const NkMat4f& pose, NkVec3f taille, NkVec3f teinte) {
    const NkVec3f c = Position(pose);
    NkDrawCall3D dc;
    dc.mesh = mesh;
    dc.transform = pose * NkMat4f::Scale(taille);
    dc.tint = teinte;
    dc.castShadow = false;
    dc.aabb.min = { c.x - taille.x * 0.5f, c.y - taille.y * 0.5f, c.z - taille.z * 0.5f };
    dc.aabb.max = { c.x + taille.x * 0.5f, c.y + taille.y * 0.5f, c.z + taille.z * 0.5f };
    return dc;
}

NkDrawCall3D Sol(NkMeshSystem* meshes) {
    NkDrawCall3D dc;
    dc.mesh = meshes->GetPlane();
    dc.transform = NkMat4f::Translate({ 0.f, 0.f, 0.f }) * NkMat4f::Scale({ 4.f, 1.f, 4.f });
    dc.tint = { 0.45f, 0.40f, 0.35f };
    dc.castShadow = false;
    dc.aabb.min = { -2.f, -0.01f, -2.f };
    dc.aabb.max = { 2.f, 0.01f, 2.f };
    return dc;
}

NkLightDesc Soleil() {
    NkLightDesc s;
    s.type = NkLightType::NK_DIRECTIONAL;
    s.direction = { -0.5f, -1.f, -0.35f };
    s.color = { 1.f, 0.96f, 0.88f };
    s.intensity = 3.f;
    s.castShadow = false;
    return s;
}

std::string Depot(const char* dossier, const char* nom) {
    const std::string p = std::string("../chapitre-10/") + dossier + "/" + nom;
    if (FILE* f = std::fopen(p.c_str(), "ab")) {
        std::fclose(f);
        return p;
    }
    return nom;
}

void Ecrire(const std::string& chemin, const std::string& texte) {
    if (FILE* f = std::fopen(chemin.c_str(), "wb")) {
        std::fputs(texte.c_str(), f);
        std::fclose(f);
    }
}

constexpr float kPas = 1.0f / 72.0f;      
constexpr float kDureeRendu = 0.020f;       
constexpr float kAmplitude = 20.0f;        
constexpr float kPulsation = 5.4f;      
constexpr int kImageCapture = 40;
constexpr float kSeuilJuste = 0.001f;

enum class EspaceCouche {
    Plancher,
    Yeux
};

struct VueOeil {
    std::uint64_t chaine;
    float lacet;
};

// Couche de projection
struct CoucheProjection {
    EspaceCouche espace;
    VueOeil gauche;
    VueOeil droite;
};

// Lacet de la tete au temps t
float LacetA(float t) {
    return kAmplitude * std::sin(kPulsation * t);
}

// "Localiser les vues" pour un instant donne
float LocaliserVues(float instant) {
    return LacetA(instant);
}

// Compositeur simulé
float Compositeur(float lacetRendu, float lacetReel, const CoucheProjection& couche) {
    const float correction = lacetReel - couche.gauche.lacet;
    return lacetRendu + correction;
}

NkVec3f Direction(float lacetDegres) {
    const float a = lacetDegres * kPi / 180.0f;
    return { -std::sin(a), 0.0f, -std::cos(a) };
}

}  

int nkmain(const NkEntryState&) {
    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Soumission";
    cfgFenetre.width = kLargeur;
    cfgFenetre.height = kHauteur;
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsValid()) {
        return 1;
    }

    NkDeviceInitInfo devInit{};
    devInit.surface = fenetre.GetSurfaceDesc();
    devInit.width = fenetre.GetSize().x;
    devInit.height = fenetre.GetSize().y;
    NkIDevice* device = NkDeviceFactory::CreateAutoDetect(devInit);
    if (!device) {
        return 1;
    }

    NkRenderer* rEcran = NkRenderer::Create(device, NkRendererConfig::For2D(devInit.api, devInit.width, devInit.height));
    if (!rEcran || !rEcran->Initialize()) {
        return 1;
    }

    NkOffscreenDesc od;
    od.width = kLargeur;
    od.height = kHauteur;
    od.hasDepth = true;
    od.hdr = false;
    od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
    od.readable = true;
    od.readback = true;
    od.name = "Scene_Soumission";
    NkOffscreenTarget* cible = rEcran->CreateOffscreen(od);
    if (!cible) {
        return 1;
    }

    NkRendererConfig cfgScene = NkRendererConfig::ForGame(devInit.api, kLargeur, kHauteur);
    cfgScene.Disable(NK_SS_RENDER2D);
    cfgScene.Disable(NK_SS_TEXT);
    cfgScene.Disable(NK_SS_OVERLAY);
    cfgScene.Enable(NK_SS_OFFSCREEN);
    NkRenderer* rendu = NkRenderer::Create(device, cfgScene);
    if (!rendu || !rendu->Initialize()) {
        return 1;
    }
    rendu->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));
    rendu->SetRenderSizeOverride(kLargeur, kHauteur);

    NkMeshSystem* meshes = rendu->GetMeshSystem();
    const NkMeshHandle cube = meshes->GetCube();

    std::string mesures;
    bool tourne = true;
    for (int n = 1; tourne && fenetre.IsValid(); ++n) {
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) {
                tourne = false;
            }
        }
        if (!tourne) {
            break;
        }

        // 1. Instant prédit de la trame, et vues localises pour cet instant
        const float instantPredit = (float)n * kPas;
        const float lacetRendu = LocaliserVues(instantPredit);

        // 2. Pendant le rendu
        const float instantAffichage = instantPredit + kDureeRendu;
        const float lacetReel = LacetA(instantAffichage);

        // 3. Remplir la couche
        CoucheProjection couche{};
        couche.espace = EspaceCouche::Plancher;

        // VERSION JUSTE
        couche.gauche = VueOeil{ 1u, lacetRendu };
        couche.droite = VueOeil{ 2u, lacetRendu };

        // VERSION FAUTIVE 
        // const float lacetFrais = LocaliserVues(instantAffichage);   
        // couche.gauche = VueOeil{ 1u, lacetFrais };
        // couche.droite = VueOeil{ 2u, lacetFrais };

        // 4. Ce que le compositeur simulé en fait
        const float lacetVue = Compositeur(lacetRendu, lacetReel, couche);
        const float sans = lacetRendu - lacetReel;
        const float avec = lacetRendu - couche.gauche.lacet;

        // 5. Dessiner la scène depuis la pose corrigée

        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            NkRender3D* r3d = rendu->GetRender3D();
            rendu->FlushGraphRebuilds();
            r3d->ResetFrame();
            if (auto* mc = rendu->GetMaterialCollection()) {
                mc->Upload();
            }

            NkCamera3DData cd;
            cd.position = { 0.f, 1.6f, 0.f };
            const NkVec3f dir = Direction(lacetVue);
            cd.target = { cd.position.x + dir.x * 3.f, cd.position.y, cd.position.z + dir.z * 3.f };
            cd.up = { 0.f, 1.f, 0.f };
            cd.fovY = 60.f;
            cd.aspect = (float)kLargeur / (float)kHauteur;
            cd.nearPlane = 0.05f;
            cd.farPlane = 100.f;
            NkSceneContext sctx;
            sctx.camera = NkCamera3D(cd);
            sctx.time = instantPredit;
            r3d->SetIBLStrength(0.35f);
            sctx.lights.PushBack(Soleil());
            r3d->BeginScene(sctx);

            r3d->Submit(Sol(meshes));
            r3d->Submit(Cube(cube, NkMat4f::Translate({ 0.f, 1.5f, -3.0f }), { 7.0f, 3.0f, 0.05f }, { 0.72f, 0.70f, 0.66f }));
            r3d->Submit(Cube(cube, NkMat4f::Translate({ -1.0f, 1.6f, -2.9f }), { 0.4f, 0.4f, 0.4f }, { 0.85f, 0.15f, 0.15f }));
            r3d->Submit(Cube(cube, NkMat4f::Translate({ 0.0f, 1.6f, -2.9f }), { 0.4f, 0.4f, 0.4f }, { 0.90f, 0.75f, 0.20f }));
            r3d->Submit(Cube(cube, NkMat4f::Translate({ 1.2f, 1.6f, -2.9f }), { 0.4f, 0.4f, 0.4f }, { 0.15f, 0.35f, 0.90f }));

            if (auto* graph = rendu->GetRenderGraph()) {
                graph->Execute(cmd);
            }
            if (auto* r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cible->GetColorResult(), { 0.f, 0.f, (float)kLargeur, (float)kHauteur });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }

        if (n >= kImageCapture - 2 && n <= kImageCapture) {
            char ligne[256];
            std::snprintf(ligne, sizeof(ligne),
                          "image %d : instant %.4f s, lacet rendu %.3f, lacet reel %.3f, lacet soumis %.3f, SANS %.3f, AVEC %.3f\n",
                          n, instantPredit, lacetRendu, lacetReel, couche.gauche.lacet, sans, avec);
            mesures += ligne;
        }

        if (n == kImageCapture) {
            const bool juste = std::fabs(lacetVue - lacetReel) < kSeuilJuste;
            const char* nom = juste ? "rendu.png" : "fraiche.png";
            const std::string chemin = Depot("exo9-la_pose_du_rendu_contre_la_pose_fraiche", nom);
            const bool ok = cible->Capture(chemin.c_str());
            std::fprintf(stderr, "image %d : %s, capture %s : %s\n", n,
                         juste ? "pose du rendu" : "pose fraiche", chemin.c_str(), ok ? "ok" : "echec");
            Ecrire(juste ? "exo9_mesures_rendu.txt" : "exo9_mesures_fraiche.txt", mesures);
            if (!std::getenv("GARDER")) {
                tourne = false;
            }
        }
    }

    NkRenderer::Destroy(rendu);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
