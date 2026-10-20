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

// Chemin dans le dossier a deposer (si on lance depuis Mes_builds), sinon dans le dossier courant.
static std::string Depot(const char* dossier, const char* nom) {
    const std::string p = std::string("../chapitre-05/") + dossier + "/" + nom;
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

static NkDrawCall3D PaveMat(NkMeshHandle mesh, NkVec3f c, NkVec3f t, NkMatInstHandle mat) {
    NkDrawCall3D dc = Pave(mesh, c, t, { 1.f, 1.f, 1.f });
    dc.material = mat;
    return dc;
}

int nkmain(const NkEntryState&) {
    // Partie NkImage : le chemin est faux exprès
    NkImage image;
    const bool charge = image.Load("murs/nexistepas.png");
    const std::string ligne = Fmt("charge : %d, valide : %d, largeur : %d, hauteur : %d", charge, image.IsValid(), image.Width(), image.Height());
    std::printf("%s\n", ligne.c_str());
    EcrireTexte("ligne_programme.txt", ligne + "\n");

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Image absente";
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsOpen()) return 1;

    NkDeviceInitInfo init;
    init.surface = fenetre.GetSurfaceDesc();
    init.width   = init.surface.width;
    init.height  = init.surface.height;

    NkIDevice* peripherique = NkDeviceFactory::CreateAutoDetect(init);
    if (!peripherique || !peripherique->IsValid()) return 1;

    NkRendererConfig cfg;
    cfg.api        = init.api;
    cfg.width      = init.width;
    cfg.height     = init.height;
    cfg.subsystems = NK_SS_RENDER3D;

    NkRenderer* renderer = NkRenderer::Create(peripherique, cfg);
    if (!renderer || !renderer->Initialize()) return 1;
    NkRender3D*   rendu3d = renderer->GetRender3D();
    NkMeshSystem* meshes  = renderer->GetMeshSystem();
    rendu3d->SetIBLStrength(0.5f);
    const NkMeshHandle cube = meshes->GetCube();

    NkSceneContext scene;
    scene.camera.SetPosition({ 0.f, 1.70f, 1.5f });
    scene.camera.SetTarget({ 0.f, 1.40f, -2.f });
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

    // Meme chemin comme texture, pose sur le mur du fond
    NkTexHandle tex = renderer->GetTextures()->Load(NkString("murs/nexistepas.png"));
    NkMaterial* mat = NkMaterial::Create(renderer->GetMaterials(), NkMaterialType::NK_PBR_METALLIC);
    mat->SetAlbedoMap(tex);
    objets[1] = PaveMat(cube, { 0.f, H * 0.5f, -C }, { 4.2f, H, E }, mat->GetInstHandle());

    // AUTO_QUIT_FRAMES=N : le programme se ferme seul apres N images
    const char* envQuit = std::getenv("AUTO_QUIT_FRAMES");
    const unsigned maxImages = envQuit ? (unsigned)std::atoi(envQuit) : 0;
    unsigned compteur = 0;
    while (fenetre.IsOpen()) {
        NkEvents().PollEvents();
        if (!renderer->BeginFrame()) continue;
        rendu3d->BeginScene(scene);
        for (const NkDrawCall3D& o : objets) rendu3d->Submit(o);
        renderer->Present();
        renderer->EndFrame();
        if (maxImages && ++compteur >= maxImages) break;
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(peripherique);
    return 0;
}
