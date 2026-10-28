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
    dc.aabb.min = { c.x - t.x * 0.5f, c.y - t.y * 0.5f, c.z - t.z * 0.5f };
    dc.aabb.max = { c.x + t.x * 0.5f, c.y + t.y * 0.5f, c.z + t.z * 0.5f };
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
    // Repère
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

static const float hPI = 3.14159265f;

static float Hauteur(float lacetDeg, bool lacetFirst) {
    const NkQuatf qy = NkQuatf::RotateY(NkAngle(lacetDeg));
    const NkQuatf qx = NkQuatf::RotateX(NkAngle(30.f));  
    const NkQuatf q = lacetFirst ? qy * qx : qx * qy;
    const NkVec3f f = q * NkVec3f(0.f, 0.f, -1.f);
    return std::asin(f.y) * 180.f / hPI;
}

int nkmain(const NkEntryState&) {
    OuvrirConsole();


    const char* envLacet = std::getenv("MASALLE_LACET");
    const bool fige = envLacet != nullptr;
    const float lacetFige = fige ? (float)std::atof(envLacet) * hPI / 180.f : 0.f;
    const char* envOrdre = std::getenv("ORDRE");
    bool mauvais = envOrdre && std::strcmp(envOrdre, "mauvais") == 0;
    const char* envShot = std::getenv("MASALLE_SHOT");
    const unsigned imageCapture = envShot ? (unsigned)std::atoi(envShot) : 0;
    const char* cheminShot = std::getenv("MASALLE_SHOT_PATH");

    const bool bonLacetFirst = std::fabs(Hauteur(0.f, true) - Hauteur(90.f, true)) < 0.05f;

    std::string hauteurs;
    const int lacets[5] = { 0, 45, 90, 135, 180 };
    for (int p = 0; p < 2; ++p) {
        for (int i = 0; i < 5; ++i) {
            hauteurs += Fmt("%s, lacet %d : hauteur %.1f deg\n", p == 0 ? "lacet*tangage" : "tangage*lacet", lacets[i], Hauteur((float)lacets[i], p == 0));
        }
    }
    std::printf("%s", hauteurs.c_str());
    EcrireTexte(Depot("exo6-le_cone_du_mauvais_ordre", "hauteurs.txt"), hauteurs);

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Cone";
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
    od.width = devInit.width;
    od.height = devInit.height;
    od.hasDepth = true;
    od.hdr = false;
    od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
    od.readable = true;
    od.readback = true;
    od.name = "Cone_Cible";
    NkOffscreenTarget* cible = rEcran->CreateOffscreen(od);
    if (!cible) return 1;

    NkRendererConfig cfgSalle = NkRendererConfig::ForGame(devInit.api, devInit.width, devInit.height);
    cfgSalle.Disable(NK_SS_RENDER2D);
    cfgSalle.Disable(NK_SS_TEXT);
    cfgSalle.Disable(NK_SS_OVERLAY);
    cfgSalle.Enable(NK_SS_OFFSCREEN);
    NkRenderer* renderer = NkRenderer::Create(device, cfgSalle);
    if (!renderer || !renderer->Initialize()) return 1;
    renderer->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cible->GetColorResult()));

    const std::vector<NkDrawCall3D> salle = ConstruireSalle(renderer->GetMeshSystem());
    const NkVec3f position = { 0.30f, 1.70f, 1.20f };

    if (!fige) {
        fenetre.ShowMouse(false);
        fenetre.CaptureMouse(true);
        fenetre.ClipMouseToClient(true);
    }

    float lacet = lacetFige; 
    int accum = 0;
    const float SENSIBILITE = 0.002f;
    unsigned image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsValid()) {
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) tourne = false;
            if (auto* k = ev->As<NkKeyPressEvent>()) {
                if (k->GetKey() == NkKey::NK_ESCAPE) tourne = false;
                if (k->GetKey() == NkKey::NK_SPACE) mauvais = !mauvais; 
            }
            if (auto* r = ev->As<NkMouseRawEvent>()) accum += r->GetDeltaX();
        }
        if (!tourne) break;

        if (!fige) lacet += (float)accum * SENSIBILITE;
        accum = 0;

        const float lacetDeg = lacet * 180.f / PI;
        const bool lacetFirst = mauvais ? !bonLacetFirst : bonLacetFirst;
        const NkQuatf qy = NkQuatf::RotateY(NkAngle(lacetDeg));
        const NkQuatf qx = NkQuatf::RotateX(NkAngle(30.f));
        const NkQuatf q = lacetFirst ? qy * qx : qx * qy;
        const NkVec3f f = q * NkVec3f(0.f, 0.f, -1.f);
        const NkVec3f u = q * NkVec3f(0.f, 1.f, 0.f);

        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            NkRender3D* r3d = renderer->GetRender3D();
            renderer->FlushGraphRebuilds();
            r3d->ResetFrame();
            if (auto* mc = renderer->GetMaterialCollection()) mc->Upload();

            NkCamera3DData cd;
            cd.position = position;
            cd.target = { position.x + f.x, position.y + f.y, position.z + f.z };   
            cd.up = u;                                                              
            cd.fovY = 70.f;
            cd.aspect = (float)devInit.width / (float)devInit.height;
            cd.nearPlane = 0.05f;
            cd.farPlane = 100.f;
            NkSceneContext sctx;
            sctx.camera = NkCamera3D(cd);
            sctx.time = (float)image / 60.f;
            r3d->SetIBLStrength(0.35f);
            sctx.lights.PushBack(Soleil());
            r3d->BeginScene(sctx);
            for (const NkDrawCall3D& o : salle) r3d->Submit(o);
            if (auto* graph = renderer->GetRenderGraph()) graph->Execute(cmd);
            if (auto* r2d = rEcran->GetRender2D()) r2d->DrawImage(cible->GetColorResult(), { 0.f, 0.f, (float)devInit.width, (float)devInit.height });
            rEcran->Present();
            rEcran->EndFrame();
        }
        ++image;

        if (imageCapture != 0 && image == imageCapture) {
            cible->Capture(cheminShot && *cheminShot ? cheminShot : "cone.png");
            tourne = false;   
        }
    }

    if (!fige) {
        fenetre.ShowMouse(true);
        fenetre.CaptureMouse(false);
        fenetre.ClipMouseToClient(false);
    }
    NkRenderer::Destroy(renderer);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
