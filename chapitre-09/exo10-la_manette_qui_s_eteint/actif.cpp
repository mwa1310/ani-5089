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
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

namespace {

constexpr unsigned kLargeur = 1280;
constexpr unsigned kHauteur = 720;
constexpr float kPas = 1.0f / 60.0f;      
constexpr int kImageSaisie = 2;         
constexpr int kImageEteinte = 8;        
constexpr int kImageRallumee = 18;       
constexpr int kImagesJournal = 20;
constexpr int kImageCapture = 40;        
constexpr float kRayonProche = 0.10f;     
constexpr float kDemiCote = 0.05f;      

// Etat d'une action booleenne 
struct EtatBool {
    bool current = false;   
    bool changed = false;   
    bool active = false;   
};

void Synchroniser(EtatBool& e, bool lie, bool presse) {
    const bool precedent = e.current;
    e.active = lie;
    e.current = lie && presse;
    e.changed = (e.current != precedent);
}

struct Objet {
    NkMat4f pose;           
    bool tenu = false;
    bool dynamique = false;  
    float vitesseY = 0.0f;
};

NkVec3f Position(const NkMat4f& m) { return { m.m30, m.m31, m.m32 }; }

float Distance(const NkVec3f& a, const NkVec3f& b) {
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::string Depot(const char* nom) {
    const std::string p = std::string("../chapitre-09/exo10-la_manette_qui_s_eteint/") + nom;
    if (FILE* f = std::fopen(p.c_str(), "ab")) {
        std::fclose(f);
        return p;
    }
    return nom;
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

}  

int nkmain(const NkEntryState&) {
    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Manette";
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
    od.name = "Scene_Manette";
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

    NkVec3f posPaume = { -0.30f, 1.00f, -0.50f };
    const NkMat4f decalVisee = NkMat4f::Translate({ 0.f, 0.07f, -0.08f });

    Objet objet;
    objet.pose = NkMat4f::Translate({ -0.28f, 0.98f, -0.50f });

    EtatBool gachette;
    NkMat4f attache;   

    const bool avant = std::getenv("AVANT") != nullptr; 

    std::string journal;
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

        // 1. Synchroniser 
        const bool manetteAllumee = !(n >= kImageEteinte && n < kImageRallumee);
        Synchroniser(gachette, manetteAllumee, n >= kImageSaisie);

        // 2. La main bouge 
        if (gachette.active && n > kImageSaisie) {
            posPaume.x += 0.006f;
            posPaume.y += 0.004f;
        }
        const NkMat4f paume = NkMat4f::Translate(posPaume);
        const NkMat4f visee = paume * decalVisee;

        // 3. Interroger
        const bool saisit = gachette.current && gachette.changed;   
        const bool proche = Distance(posPaume, Position(objet.pose)) < kRayonProche;

        if (saisit && !objet.tenu && proche) {
            attache = paume.Inverse() * objet.pose;   
            objet.tenu = true;
        }
        if (objet.tenu) {
            const bool laisseTomber = avant ? !gachette.current
                                            : (gachette.active && !gachette.current && gachette.changed);
            if (laisseTomber) {
                objet.tenu = false;
                objet.dynamique = true;
            } else {
                objet.pose = paume * attache;
            }
        }

        // 4. Gravité
        if (objet.dynamique && !objet.tenu) {
            NkVec3f p = Position(objet.pose);
            objet.vitesseY -= 9.81f * kPas;
            p.y += objet.vitesseY * kPas;
            if (p.y < kDemiCote) {
                p.y = kDemiCote;
                objet.vitesseY = 0.0f;
            }
            objet.pose = NkMat4f::Translate(p);
        }

        // 5. Dessiner
        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            NkRender3D* r3d = rendu->GetRender3D();
            rendu->FlushGraphRebuilds();
            r3d->ResetFrame();
            if (auto* mc = rendu->GetMaterialCollection()) {
                mc->Upload();
            }

            NkCamera3DData cd;
            cd.position = { 0.35f, 1.35f, 1.30f };
            cd.target = { -0.20f, 0.80f, -0.50f };
            cd.up = { 0.f, 1.f, 0.f };
            cd.fovY = 60.f;
            cd.aspect = (float)kLargeur / (float)kHauteur;
            cd.nearPlane = 0.05f;
            cd.farPlane = 100.f;
            NkSceneContext sctx;
            sctx.camera = NkCamera3D(cd);
            sctx.time = (float)n / 60.f;
            r3d->SetIBLStrength(0.35f);
            sctx.lights.PushBack(Soleil());
            r3d->BeginScene(sctx);

            r3d->Submit(Sol(meshes));
            r3d->Submit(Cube(cube, NkMat4f::Translate({ 0.f, 1.0f, -1.5f }), { 3.0f, 2.0f, 0.05f }, { 0.72f, 0.70f, 0.66f }));
            r3d->Submit(Cube(cube, paume, { 0.08f, 0.08f, 0.08f }, { 0.85f, 0.15f, 0.15f }));    // en rouge
            r3d->Submit(Cube(cube, visee, { 0.04f, 0.04f, 0.04f }, { 0.15f, 0.35f, 0.90f }));    // en bleu
            r3d->Submit(Cube(cube, objet.pose, { 0.10f, 0.10f, 0.10f }, { 0.90f, 0.75f, 0.20f })); // en jaune

            if (auto* graph = rendu->GetRenderGraph()) {
                graph->Execute(cmd);
            }
            if (auto* r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cible->GetColorResult(), { 0.f, 0.f, (float)kLargeur, (float)kHauteur });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }

        if (n <= kImagesJournal) {
            char ligne[128];
            std::snprintf(ligne, sizeof(ligne), "image : %d, courant : %d, change : %d, actif : %d\n",
                          n, gachette.current ? 1 : 0, gachette.changed ? 1 : 0, gachette.active ? 1 : 0);
            journal += ligne;
            if (n == kImagesJournal) {
                if (FILE* f = std::fopen(Depot("journal.txt").c_str(), "wb")) {
                    std::fputs(journal.c_str(), f);
                    std::fclose(f);
                }
            }
        }
        {
            const NkVec3f po = Position(objet.pose);
            char ligne[256];
            std::snprintf(ligne, sizeof(ligne), "image %d : tenu %d, objet y %.3f, paume y %.3f\n",
                          n, objet.tenu ? 1 : 0, po.y, posPaume.y);
            mesures += ligne;
        }

        if (n == kImageCapture) {
            const std::string chemin = Depot(avant ? "avant.png" : "apres.png");
            const bool ok = cible->Capture(chemin.c_str());
            std::fprintf(stderr, "capture %s : %s\n", chemin.c_str(), ok ? "ok" : "echec");
            if (FILE* f = std::fopen(avant ? "exo10_mesures_avant.txt" : "exo10_mesures_apres.txt", "wb")) {
                std::fputs(mesures.c_str(), f);
                std::fclose(f);
            }
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
