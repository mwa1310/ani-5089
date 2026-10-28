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
    const NkVec3f mur = { 0.72f, 0.70f, 0.66f };
    const NkVec3f clair = { 0.85f, 0.85f, 0.85f };
    std::vector<NkDrawCall3D> objets;
    objets.push_back(Sol(meshes));
    objets.push_back(Pave(cube, { 0.f, H * 0.5f, -C }, { 4.2f, H, E }, mur));   // fond
    objets.push_back(Pave(cube, { 0.f, H * 0.5f,  C }, { 4.2f, H, E }, mur));   // entree
    objets.push_back(Pave(cube, { -C, H * 0.5f, 0.f }, { E, H, 4.0f }, mur));   // gauche
    objets.push_back(Pave(cube, {  C, H * 0.5f, 0.f }, { E, H, 4.0f }, mur));   // droite
    objets.push_back(Pave(cube, { 0.f, H + E * 0.5f, 0.f }, { 4.2f, E, 4.2f }, clair));   // plafond

    // Porte et fenetres eur
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

static const int L = 640, H = 720;

struct Resultat {
    long pixels = 0;
    int ecartMax = 0;
    long tiers[3] = { 0, 0, 0 };  
    int minX = L, maxX = -1, minY = H, maxY = -1;
    long nonNoirs = 0;    
};

static Resultat Comparer(const char* gauche, const char* droite, const char* sortie) {
    Resultat r;
    NkImage a, b;
    if (!a.Load(gauche) || !b.Load(droite) || a.Width() != L || a.Height() != H || b.Width() != L || b.Height() != H) return r;
    NkImage diff = NkImage::Create(L, H, 4, 0x000000FFu);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < L; ++x) {
            const math::NkColor p = a.GetPixel(x, y), q = b.GetPixel(x, y);
            if (p.r || p.g || p.b) ++r.nonNoirs;
            int d = std::abs((int)p.r - (int)q.r);
            const int dg = std::abs((int)p.g - (int)q.g), db = std::abs((int)p.b - (int)q.b);
            if (dg > d) d = dg;
            if (db > d) d = db;
            if (d > r.ecartMax) r.ecartMax = d;
            if (d > 1) {
                ++r.pixels;
                ++r.tiers[x * 3 / L];
                if (x < r.minX) r.minX = x;
                if (x > r.maxX) r.maxX = x;
                if (y < r.minY) r.minY = y;
                if (y > r.maxY) r.maxY = y;
            }
            const int v = d * 4 > 255 ? 255 : d * 4;  
            diff.SetPixel(x, y, math::NkColor((uint8)v, (uint8)v, (uint8)v, 255));
        }
    }
    diff.SavePNG(sortie);
    return r;
}

int nkmain(const NkEntryState&) {
    // VUE_OUBLIEE=1 
    const bool oubliee = std::getenv("VUE_OUBLIEE") != nullptr;
    const bool garder = std::getenv("GARDER") != nullptr;  

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Deux yeux";
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

    NkOffscreenTarget* cibles[2] = { nullptr, nullptr };
    NkRenderer* rendus[2] = { nullptr, nullptr };
    std::vector<NkDrawCall3D> salles[2];
    for (int i = 0; i < 2; ++i) {
        NkOffscreenDesc od;
        od.width = L;
        od.height = H;
        od.hasDepth = true;
        od.hdr = false;
        od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
        od.readable = true;
        od.readback = true;
        od.name = i == 0 ? "Oeil_Gauche" : "Oeil_Droit";
        cibles[i] = rEcran->CreateOffscreen(od);
        if (!cibles[i]) return 1;

        NkRendererConfig cfgSalle = NkRendererConfig::ForGame(devInit.api, L, H);
        cfgSalle.Disable(NK_SS_RENDER2D);
        cfgSalle.Disable(NK_SS_TEXT);
        cfgSalle.Disable(NK_SS_OVERLAY);
        cfgSalle.Enable(NK_SS_OFFSCREEN);
        rendus[i] = NkRenderer::Create(device, cfgSalle);
        if (!rendus[i] || !rendus[i]->Initialize()) return 1;
        rendus[i]->SetFinalColorTarget(rEcran->GetTextures()->GetRHIHandle(cibles[i]->GetColorResult()));
        rendus[i]->SetRenderSizeOverride(L, H);   
        salles[i] = ConstruireSalle(rendus[i]->GetMeshSystem());
    }

    const NkVec3f tete = { 0.30f, 1.70f, 1.20f };
    const NkVec3f droiteTete = { 1.f, 0.f, 0.f };
    const float demiEcart = 0.063f * 0.5f;
    NkVec3f yeux[2] = { { tete.x - droiteTete.x * demiEcart, tete.y, tete.z }, { tete.x + droiteTete.x * demiEcart, tete.y, tete.z } };
    if (oubliee) { yeux[0] = tete; yeux[1] = tete; }

    const unsigned IMAGE_CAPTURE = 30;
    const char* suffixe = oubliee ? "-oubliee" : "";
    unsigned image = 0;
    bool tourne = true;
    while (tourne && fenetre.IsValid()) {
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) tourne = false;
        }
        if (!tourne) break;

        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            for (int i = 0; i < 2; ++i) {
                NkRender3D* r3d = rendus[i]->GetRender3D();
                rendus[i]->FlushGraphRebuilds();
                r3d->ResetFrame();
                if (auto* mc = rendus[i]->GetMaterialCollection()) mc->Upload();

                NkCamera3DData cd;
                cd.position = yeux[i];
                cd.target = { yeux[i].x, yeux[i].y, yeux[i].z - 1.f };  
                cd.up = { 0.f, 1.f, 0.f };
                cd.fovY = 70.f;
                cd.aspect = (float)L / (float)H;
                cd.nearPlane = 0.05f;
                cd.farPlane = 100.f;
                NkSceneContext sctx;
                sctx.camera = NkCamera3D(cd);
                sctx.time = (float)image / 60.f;
                r3d->SetIBLStrength(0.35f);
                sctx.lights.PushBack(Soleil());
                r3d->BeginScene(sctx);
                for (const NkDrawCall3D& o : salles[i]) r3d->Submit(o); 
                if (auto* graph = rendus[i]->GetRenderGraph()) graph->Execute(cmd);
            }
            if (auto* r2d = rEcran->GetRender2D()) {
                r2d->DrawImage(cibles[0]->GetColorResult(), { 0.f, 0.f, (float)L, (float)H });
                r2d->DrawImage(cibles[1]->GetColorResult(), { (float)L, 0.f, (float)L, (float)H });
            }
            rEcran->Present();
            rEcran->EndFrame();
        }
        ++image;

        if (image == IMAGE_CAPTURE) {
            const std::string g = oubliee ? std::string("oeil-gauche-oubliee.png") : Depot("exo9-les_deux_images_et_l_image_noire", "oeil-gauche.png");
            const std::string d = oubliee ? std::string("oeil-droit-oubliee.png") : Depot("exo9-les_deux_images_et_l_image_noire", "oeil-droit.png");
            cibles[0]->Capture(g.c_str());
            cibles[1]->Capture(d.c_str());
            const std::string sortie = oubliee ? Depot("exo9-les_deux_images_et_l_image_noire", "difference-vue-oubliee.png")
                                               : Depot("exo9-les_deux_images_et_l_image_noire", "difference.png");
            const Resultat r = Comparer(g.c_str(), d.c_str(), sortie.c_str());
            const char* nom = oubliee ? "vue oubliee" : "deux yeux";
            EcrireTexte(Fmt("exo9_ligne%s.txt", suffixe), Fmt("%s : %ld pixels sur %ld, ecart max %d\n", nom, r.pixels, (long)L * H, r.ecartMax));
            EcrireTexte(Fmt("exo9_details%s.txt", suffixe),
                        Fmt("tiers %ld %ld %ld\nboite %d %d %d %d\nnonnoirs %ld\n", r.tiers[0], r.tiers[1], r.tiers[2], r.minX, r.maxX, r.minY, r.maxY, r.nonNoirs));
            if (!garder) tourne = false;
        }
    }

    NkRenderer::Destroy(rendus[0]);
    NkRenderer::Destroy(rendus[1]);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}
