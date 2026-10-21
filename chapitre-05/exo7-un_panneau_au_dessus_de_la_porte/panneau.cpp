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

static const char* MOT = "Sortie";

int nkmain(const NkEntryState&) {
    // --- Atlas : police embarquee dans le moteur, aucun fichier a fournir
    NkFontAtlas atlas;
    NkFontEmbedded::AddToAtlas(atlas, NkEmbeddedFontId::Inter, 32.f);
    NkFont* police = atlas.fonts[0];
    if (!police || !atlas.Build()) {
        EcrireTexte("erreur.txt", "atlas non construit");
        return 1;
    }
    nkft_uint8* px = nullptr;
    nkft_int32 aw = 0, ah = 0;
    atlas.GetTexDataAsAlpha8(&px, &aw, &ah);
    if (!px || aw <= 0 || ah <= 0) return 1;

    // --- Image du panneau : largeur = somme des advanceX, hauteur = ascent - descent
    const int MARGE = 2;
    float largeurTexte = 0.f;
    for (const char* c = MOT; *c; ++c) {
        const NkFontGlyph* g = police->FindGlyph((NkFontCodepoint)(unsigned char)*c);
        if (g) largeurTexte += g->advanceX;
    }
    const int W = (int)std::ceil(largeurTexte) + 2 * MARGE;
    const int H = (int)std::ceil(police->ascent - police->descent) + 2 * MARGE;
    NkImage img = NkImage::Create(W, H, 4, 0x0B6B3AFFu);   // fond vert
    const float base = police->ascent + (float)MARGE;       // ligne de base, a ascent pixels du haut
    float curseur = (float)MARGE;

    for (const char* c = MOT; *c; ++c) {
        const NkFontGlyph* g = police->FindGlyph((NkFontCodepoint)(unsigned char)*c);
        if (!g) continue;
        if (g->visible && g->x1 > g->x0 && g->y1 > g->y0) {
            const int xa = (int)std::floor(curseur + g->x0), xb = (int)std::ceil(curseur + g->x1);
            const int ya = (int)std::floor(base + g->y0),    yb = (int)std::ceil(base + g->y1);
            for (int y = ya; y < yb; ++y) {
                for (int x = xa; x < xb; ++x) {
                    if (x < 0 || y < 0 || x >= W || y >= H) continue;
                    // fraction du pixel dans le rectangle de la lettre, puis coordonnees dans l'atlas
                    const float fx = ((float)x + 0.5f - (curseur + g->x0)) / (g->x1 - g->x0);
                    const float fy = ((float)y + 0.5f - (base + g->y0)) / (g->y1 - g->y0);
                    if (fx < 0.f || fx > 1.f || fy < 0.f || fy > 1.f) continue;
                    const float u = g->u0 + fx * (g->u1 - g->u0);
                    const float v = g->v0 + fy * (g->v1 - g->v0);
                    int sx = (int)(u * (float)aw), sy = (int)(v * (float)ah);
                    if (sx < 0) sx = 0;
                    if (sy < 0) sy = 0;
                    if (sx >= aw) sx = aw - 1;
                    if (sy >= ah) sy = ah - 1;
                    const float a = (float)px[sy * aw + sx] / 255.f;      // couverture
                    const math::NkColor fond = img.GetPixel(x, y);
                    img.SetPixel(x, y, math::NkColor((uint8)((float)fond.r * (1.f - a) + 255.f * a),
                                                     (uint8)((float)fond.g * (1.f - a) + 255.f * a),
                                                     (uint8)((float)fond.b * (1.f - a) + 255.f * a), 255));
                }
            }
        }
        curseur += g->advanceX;
    }
    img.SavePNG(Depot("exo7-un_panneau_au_dessus_de_la_porte", "panneau.png").c_str());

    // Copie utilisee comme texture, retournee si la face du pave l'exige : FLIP=v, h ou vh
    NkImage pourMur = img.Copy();
    const char* envFlip = std::getenv("FLIP");
    const char* flip = envFlip ? envFlip : "h";
    if (std::strchr(flip, 'v')) pourMur.FlipVertical();
    if (std::strchr(flip, 'h')) pourMur.FlipHorizontal();    
    pourMur.SavePNG("panneau_mur.png");

    // --- Salle
    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Panneau";
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

    // VUE=3m (defaut) ou VUE=50cm
    const char* envVue = std::getenv("VUE");
    const bool pres = envVue && std::strcmp(envVue, "50cm") == 0;
    NkVec3f camPos = { -0.70f, 1.70f, 1.00f };
    if (pres) camPos = { -0.70f, 2.20f, -1.50f };
    const NkVec3f centrePanneau = { -0.70f, 2.15f, -1.985f };   // au-dessus de la porte, decolle de 5 mm du mur

    NkSceneContext scene;
    scene.camera.SetPosition(camPos);
    scene.camera.SetTarget(centrePanneau);
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

    const float HAUTEUR_MURS = 2.5f, E = 0.1f, C = 2.05f;   // hauteur, epaisseur, centre des murs
    const NkVec3f mur   = { 0.72f, 0.70f, 0.66f };
    const NkVec3f clair = { 0.85f, 0.85f, 0.85f };
    std::vector<NkDrawCall3D> objets;
    objets.push_back(Sol(meshes));
    objets.push_back(Pave(cube, { 0.f, HAUTEUR_MURS * 0.5f, -C }, { 4.2f, HAUTEUR_MURS, E }, mur));   // fond
    objets.push_back(Pave(cube, { 0.f, HAUTEUR_MURS * 0.5f,  C }, { 4.2f, HAUTEUR_MURS, E }, mur));   // entree
    objets.push_back(Pave(cube, { -C, HAUTEUR_MURS * 0.5f, 0.f }, { E, HAUTEUR_MURS, 4.0f }, mur));   // gauche
    objets.push_back(Pave(cube, {  C, HAUTEUR_MURS * 0.5f, 0.f }, { E, HAUTEUR_MURS, 4.0f }, mur));   // droite
    objets.push_back(Pave(cube, { 0.f, HAUTEUR_MURS + E * 0.5f, 0.f }, { 4.2f, E, 4.2f }, clair));   // plafond

    // Porte et fenetres : panneaux de 2 cm, avances de 2 mm vers l'interieur
    const float EP = 0.02f, AVANCE = 0.002f;
    const NkVec3f porte = { 0.40f, 0.25f, 0.12f };
    const NkVec3f vitre = { 0.55f, 0.75f, 0.90f };
    objets.push_back(Pave(cube, { -0.70f, 1.00f, -2.0f - EP * 0.5f + AVANCE }, { 0.90f, 2.00f, EP }, porte));
    objets.push_back(Pave(cube, { -2.0f - EP * 0.5f + AVANCE, 1.40f, -0.90f }, { EP, 1.00f, 1.20f }, vitre));
    objets.push_back(Pave(cube, { -2.0f - EP * 0.5f + AVANCE, 1.40f,  0.90f }, { EP, 1.00f, 1.20f }, vitre));

    NkLoadOptions opts;
    opts.useClampEdge = true;
    NkTexHandle tex = renderer->GetTextures()->Load(NkString("panneau_mur.png"), opts);
    NkMaterial* mat = NkMaterial::Create(renderer->GetMaterials(), NkMaterialType::NK_PBR_METALLIC);
    mat->SetAlbedoMap(tex);

    const float hauteurPanneau = 0.20f;
    const float largeurPanneau = hauteurPanneau * (float)W / (float)H;   // garde les proportions de l'image
    NkDrawCall3D pan = Pave(cube, centrePanneau, { largeurPanneau, hauteurPanneau, 0.02f }, { 1.f, 1.f, 1.f });
    pan.material = mat->GetInstHandle();
    objets.push_back(pan);

    // --- panneau.txt : les deux mesures seulement ; le reste est redige a part
    EcrireTexte(Depot("exo7-un_panneau_au_dessus_de_la_porte", "panneau.txt"), Fmt("atlas : %d x %d\nimage : %d x %d\n", (int)aw, (int)ah, W, H));
    EcrireTexte("exo7_details.txt", Fmt("atlas %d %d\nimage %d %d\nfenetre %u %u\n", (int)aw, (int)ah, W, H, (unsigned)cfg.width, (unsigned)cfg.height));

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
