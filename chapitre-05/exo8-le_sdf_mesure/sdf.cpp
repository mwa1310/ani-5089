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

// smoothstep, comme dans le nuanceur kNkFontFragSDF du moteur
static float smoothstep(float a, float b, float x) {
    float t = (x - a) / (b - a);
    if (t < 0.f) t = 0.f;
    if (t > 1.f) t = 1.f;
    return t * t * (3.f - 2.f * t);
}

static bool Construire(NkFontAtlas& atlas, float taille, bool sdf, nkft_uint8*& px, int& w, int& h, NkFont*& police) {
    atlas.sdfMode   = sdf;
    atlas.sdfSpread = 6;
    NkFontEmbedded::AddToAtlas(atlas, NkEmbeddedFontId::Inter, taille);
    police = atlas.fonts[0];
    if (!police || !atlas.Build()) return false;
    nkft_int32 aw = 0, ah = 0;
    atlas.GetTexDataAsAlpha8(&px, &aw, &ah);
    w = (int)aw;
    h = (int)ah;
    return px != nullptr && w > 0 && h > 0;
}

// Lecture de l'atlas par interpolation bilineaire entre les quatre pixels voisins (fx, fy en pixels d'atlas)
static float Lire(const nkft_uint8* px, int w, int h, float fx, float fy) {
    fx -= 0.5f;
    fy -= 0.5f;
    const int x0 = (int)std::floor(fx), y0 = (int)std::floor(fy);
    const float tx = fx - (float)x0, ty = fy - (float)y0;
    auto at = [&](int x, int y) {
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= w) x = w - 1;
        if (y >= h) y = h - 1;
        return (float)px[y * w + x];
    };
    const float a = at(x0, y0) * (1.f - tx) + at(x0 + 1, y0) * tx;
    const float b = at(x0, y0 + 1) * (1.f - tx) + at(x0 + 1, y0 + 1) * tx;
    return a * (1.f - ty) + b * ty;
}

static const char* MOT = "Sortie";
static const int K = 8;   // l'image fait huit fois la taille des lettres de l'atlas

// Meme taille d'image pour les deux methodes : seule la methode change
static NkImage Fabriquer(NkFont* police, const nkft_uint8* px, int aw, int ah, bool sdf, float s) {
    const float marge = 2.f;
    float largeurTexte = 0.f;
    for (const char* c = MOT; *c; ++c) {
        const NkFontGlyph* g = police->FindGlyph((NkFontCodepoint)(unsigned char)*c);
        if (g) largeurTexte += g->advanceX;
    }
    const int W = (int)std::ceil((largeurTexte + 2.f * marge) * (float)K);
    const int H = (int)std::ceil((police->ascent - police->descent + 2.f * marge) * (float)K);
    NkImage img = NkImage::Create(W, H, 4, 0x0B6B3AFFu);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            // position du pixel dans l'espace du texte (unites d'atlas), ligne de base a ascent
            const float tx = ((float)x + 0.5f) / (float)K - marge;
            const float ty = ((float)y + 0.5f) / (float)K - marge;
            float curseur = 0.f, valeur = 0.f;
            bool dedans = false;
            for (const char* c = MOT; *c; ++c) {
                const NkFontGlyph* g = police->FindGlyph((NkFontCodepoint)(unsigned char)*c);
                if (!g) continue;
                if (g->visible && g->x1 > g->x0 && g->y1 > g->y0) {
                    const float fx = (tx - (curseur + g->x0)) / (g->x1 - g->x0);
                    const float fy = (ty - (police->ascent + g->y0)) / (g->y1 - g->y0);
                    if (fx >= 0.f && fx <= 1.f && fy >= 0.f && fy <= 1.f) {
                        const float u = g->u0 + fx * (g->u1 - g->u0);
                        const float v = g->v0 + fy * (g->v1 - g->v0);
                        valeur = Lire(px, aw, ah, u * (float)aw, v * (float)ah);
                        dedans = true;
                    }
                }
                curseur += g->advanceX;
            }
            float alpha = 0.f;
            if (dedans) {
                if (sdf) alpha = smoothstep(0.5f - s, 0.5f + s, valeur / 255.f);   // regle du nuanceur : le bord est a 0,5
                else     alpha = valeur / 255.f;                                   // couverture
            }
            const math::NkColor fond = img.GetPixel(x, y);
            img.SetPixel(x, y, math::NkColor((uint8)((float)fond.r * (1.f - alpha) + 255.f * alpha),
                                             (uint8)((float)fond.g * (1.f - alpha) + 255.f * alpha),
                                             (uint8)((float)fond.b * (1.f - alpha) + 255.f * alpha), 255));
        }
    }
    return img;
}

int nkmain(const NkEntryState&) {
    // Trois atlas : ordinaire 32, sdf 32, ordinaire 128
    NkFontAtlas a1, a2, a3;
    nkft_uint8 *p1 = nullptr, *p2 = nullptr, *p3 = nullptr;
    int w1 = 0, h1 = 0, w2 = 0, h2 = 0, w3 = 0, h3 = 0;
    NkFont *f1 = nullptr, *f2 = nullptr, *f3 = nullptr;
    if (!Construire(a1, 32.f, false, p1, w1, h1, f1) || !Construire(a2, 32.f, true, p2, w2, h2, f2) || !Construire(a3, 128.f, false, p3, w3, h3, f3)) {
        EcrireTexte("erreur.txt", "un atlas n'a pas pu etre construit");
        return 1;
    }
    EcrireTexte(Depot("exo8-le_sdf_mesure", "atlas.txt"),
                Fmt("ordinaire 32 : %d x %d\nsdf 32 : %d x %d\nordinaire 128 : %d x %d\n", w1, h1, w2, h2, w3, h3));

    // s : un pixel de l'image = 1/K pixel d'atlas, et la distance varie de 1/(2 * spread) par pixel d'atlas.
    // Un bord d'environ un pixel et demi d'image demande donc s = 1,5 / K / (2 * spread).
    const float spread = 6.f;
    const float s = 1.5f / (float)K / (2.f * spread);

    NkImage ordinaire = Fabriquer(f1, p1, w1, h1, false, 0.f);
    NkImage champ     = Fabriquer(f2, p2, w2, h2, true, s);
    ordinaire.SavePNG("panneau_ordinaire.png");
    champ.SavePNG("panneau_sdf.png");
    // Autres valeurs de s, pour comparer a l'oeil : trop petit = escalier, trop grand = bord flou
    const float essais[3] = { 0.004f, 0.05f, 0.15f };
    for (int i = 0; i < 3; ++i) {
        NkImage v = Fabriquer(f2, p2, w2, h2, true, essais[i]);
        v.SavePNG(Fmt("panneau_sdf_s%.3f.png", essais[i]).c_str());
    }

    const int W = ordinaire.Width(), H = ordinaire.Height();
    EcrireTexte("exo8_details.txt", Fmt("image %d %d\nK %d\ns %.5f\nspread %.0f\n", W, H, K, s, spread));

    // --- Salle : PANNEAU=ordinaire (defaut) ou sdf, VUE=3m ou 50cm
    const char* envPan = std::getenv("PANNEAU");
    const bool unSdf = envPan && std::strcmp(envPan, "sdf") == 0;
    NkImage choisi = unSdf ? champ.Copy() : ordinaire.Copy();
    const char* envFlip = std::getenv("FLIP");
    const char* flip = envFlip ? envFlip : "h";
    if (std::strchr(flip, 'v')) choisi.FlipVertical();
    if (std::strchr(flip, 'h')) choisi.FlipHorizontal();
    choisi.SavePNG("panneau_mur.png");

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "SDF";
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
    const float largeurPanneau = hauteurPanneau * (float)W / (float)H;
    NkDrawCall3D pan = Pave(cube, centrePanneau, { largeurPanneau, hauteurPanneau, 0.02f }, { 1.f, 1.f, 1.f });
    pan.material = mat->GetInstHandle();
    objets.push_back(pan);

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
