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

constexpr int kImagesJournal = 20;
constexpr int kPauseDebut = 6;
constexpr int kPauseFin = 10;
constexpr int kImageSaisie = 3;
constexpr float kPas = 1.0f / 72.0f;

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

struct CoucheProjection {
    std::uint64_t chaineGauche;
    std::uint64_t chaineDroite;
};

// Session simulée
class Session {
public:
    bool Attendre() {
        return !ouverte_;
    }

    bool Commencer() {
        if (ouverte_) {
            return false;
        }
        ouverte_ = true;
        return true;
    }

    // couche nulle
    bool Finir(const CoucheProjection*) {
        if (!ouverte_) {
            return false;
        }
        ouverte_ = false;
        return true;
    }

private:
    bool ouverte_ = false;
};

} 

int nkmain(const NkEntryState&) {
    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Boucle";
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
    od.name = "Scene_Boucle";
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

    const bool avant = std::getenv("AVANT") != nullptr;   

    Session session;
    EtatBool gachette;
    std::string journal;
    bool tourne = true;

    for (int n = 1; tourne && fenetre.IsValid(); ++n) {
        // 1. Pompe d'évènements
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) {
                tourne = false;
            }
        }
        if (!tourne) {
            break;
        }

        const bool shouldRender = !(n >= kPauseDebut && n <= kPauseFin);
        bool rend = false;
        bool debut = false;
        bool fin = false;

        // 2. Attente de trame 
        if (session.Attendre()) {
            // 3. Debut de trame
            debut = session.Commencer();

            // 4. Synchronisation des actions
            Synchroniser(gachette, true, n >= kImageSaisie);

            // 5. Rendu conditionnel
            CoucheProjection couche{ 1u, 2u };
            if (shouldRender) {
                rend = true;

    if (rEcran->BeginFrame()) {
                NkICommandBuffer* cmd = rEcran->GetCmd();
                NkRender3D* r3d = rendu->GetRender3D();
                rendu->FlushGraphRebuilds();
                r3d->ResetFrame();
                if (auto* mc = rendu->GetMaterialCollection()) {
                    mc->Upload();
                }

                NkCamera3DData cd;
                cd.position = { 0.f, 1.6f, 1.5f };
                cd.target = { 0.f, 1.4f, -3.0f };
                cd.up = { 0.f, 1.f, 0.f };
                cd.fovY = 60.f;
                cd.aspect = (float)kLargeur / (float)kHauteur;
                cd.nearPlane = 0.05f;
                cd.farPlane = 100.f;
                NkSceneContext sctx;
                sctx.camera = NkCamera3D(cd);
                sctx.time = (float)n * kPas;
                r3d->SetIBLStrength(0.35f);
                sctx.lights.PushBack(Soleil());
                r3d->BeginScene(sctx);

                r3d->Submit(Sol(meshes));
                r3d->Submit(Cube(cube, NkMat4f::Translate({ 0.f, 1.5f, -3.0f }), { 7.0f, 3.0f, 0.05f }, { 0.72f, 0.70f, 0.66f }));
                const float longueur = 0.04f * (float)n;
                r3d->Submit(Cube(cube, NkMat4f::Translate({ -1.5f + longueur * 0.5f, 0.8f, -2.9f }), { longueur, 0.2f, 0.1f }, { 0.90f, 0.75f, 0.20f }));
                r3d->Submit(Cube(cube, NkMat4f::Translate({ -1.5f + 0.0f, 1.6f, -2.9f }), { 0.1f, 0.4f, 0.1f }, { 0.15f, 0.35f, 0.90f }));

                if (auto* graph = rendu->GetRenderGraph()) {
                    graph->Execute(cmd);
                }
                if (auto* r2d = rEcran->GetRender2D()) {
                    r2d->DrawImage(cible->GetColorResult(), { 0.f, 0.f, (float)kLargeur, (float)kHauteur });
                }
                rEcran->Present();
                rEcran->EndFrame();
            }

                // Version fautive
                if (avant) {
                    fin = session.Finir(&couche);
                }
            }

            // 6. Fin de trame 
            if (!avant) {
                fin = session.Finir(rend ? &couche : nullptr);
            }
        }

        if (n <= kImagesJournal) {
            char ligne[128];
            std::snprintf(ligne, sizeof(ligne), "trame : %d, rend : %d, debut : %d, fin : %d\n",
                          n, rend ? 1 : 0, debut ? 1 : 0, fin ? 1 : 0);
            journal += ligne;
        }

        if (n == kImagesJournal) {
            const char* dossier = "exo10-la_boucle_qui_ne_reprend_jamais";
            if (avant) {
                Ecrire(Depot(dossier, "journal.txt"), journal);
            } else {
                Ecrire("exo10_journal_apres.txt", journal);
            }
            const std::string chemin = Depot(dossier, avant ? "avant.png" : "apres.png");
            const bool ok = cible->Capture(chemin.c_str());
            std::fprintf(stderr, "capture %s : %s\n", chemin.c_str(), ok ? "ok" : "echec");
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
