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

static void OuvrirConsole() {
    AllocConsole();
    std::freopen("CONOUT$", "w", stdout);
}

// Pave de dimensions t, centre en c : on compose toujours Translate * Scale.
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

// Le sol est un plan pose a y = 0
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

struct Mesure {
    double fps;
    unsigned drawCalls;
    bool ok;
};

// Cadence moyenne sur 400 images, apres 30 images de chauffe.
static Mesure Mesurer(NkWindow& fenetre, NkRenderer* renderer, NkRender3D* rendu3d,
                      const NkSceneContext& scene, const std::function<void()>& soumettre) {
    const unsigned CHAUFFE = 30, IMAGES = 400;
    using horloge = std::chrono::steady_clock;
    horloge::time_point debut = horloge::now();
    unsigned image = 0, drawCalls = 0;
    while (fenetre.IsOpen() && image < CHAUFFE + IMAGES) {
        NkEvents().PollEvents();
        if (!renderer->BeginFrame()) continue;
        if (image == CHAUFFE) debut = horloge::now();
        rendu3d->BeginScene(scene);
        soumettre();
        renderer->Present();
        renderer->EndFrame();
        drawCalls = renderer->GetStats().drawCalls;
        ++image;
    }
    const double s = std::chrono::duration<double>(horloge::now() - debut).count();
    return { IMAGES / s, drawCalls, image == CHAUFFE + IMAGES };
}

// n cubes identiques rangés en grille, dans le plan z = -1,5, devant la camera.
static std::vector<NkMat4f> Grille(unsigned n) {
    const unsigned colonnes = (unsigned)std::ceil(std::sqrt(n * 1.6));
    const unsigned lignes   = (n + colonnes - 1) / colonnes;
    const float pas   = 3.0f / colonnes;
    const float arete = pas * 0.8f;
    std::vector<NkMat4f> m;
    m.reserve(n);
    for (unsigned i = 0; i < n; ++i) {
        const unsigned c = i % colonnes, l = i / colonnes;
        const float x = ((float)c - (colonnes - 1) * 0.5f) * pas;
        const float y = 1.4f + ((float)l - (lignes - 1) * 0.5f) * pas;
        m.push_back(NkMat4f::Translate({ x, y, -1.5f }) * NkMat4f::Scale({ arete, arete, arete }));
    }
    return m;
}

static void Ecrire(const std::string& texte) {
    FILE* f = std::fopen("../chapitre-04/exo27-cent_cubes_et_le_premier_mur_du_reel/cadence.txt", "w");
    if (!f) f = std::fopen("cadence.txt", "w");
    if (f) {
        std::fputs(texte.c_str(), f);
        std::fclose(f);
    }
}

int nkmain(const NkEntryState&) {
    OuvrirConsole();
    // Instanciation GPU : un seul ordre de dessin par lot de cubes
    _putenv_s("NK_INSTANCING_GPU", "1");

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Cubes";
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsOpen()) return 1;

    NkDeviceInitInfo init;
    init.surface = fenetre.GetSurfaceDesc();
    init.width   = init.surface.width;
    init.height  = init.surface.height;

    NkIDevice* peripherique = NkDeviceFactory::CreateAutoDetect(init);
    if (!peripherique || !peripherique->IsValid()) return 1;

    NkRendererConfig cfg;
    cfg.api = init.api;
    cfg.width = init.width;
    cfg.height = init.height;
    cfg.subsystems = NK_SS_RENDER3D;
    cfg.vsync = false;   // on mesure ce que la machine peut faire, pas la cadence de l'ecran

    NkRenderer* renderer = NkRenderer::Create(peripherique, cfg);
    if (!renderer || !renderer->Initialize()) return 1;
    NkRender3D* rendu3d = renderer->GetRender3D();
    NkMeshSystem* meshes = renderer->GetMeshSystem();
    rendu3d->SetIBLStrength(0.5f);
    const NkMeshHandle cube = meshes->GetCube();
    renderer->SetFrameRateCap(0.f);

    NkSceneContext scene;
    scene.camera.SetPosition({ 0.f, 1.70f, 1.5f });
    scene.camera.SetTarget({ 0.f, 1.40f, -2.f });
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

    const NkVec3f blanc = { 1.f, 1.f, 1.f };
    const unsigned PLAFOND_UN_PAR_UN = 4000;   
    const unsigned PLAFOND_GROUPE = 20000;
    const unsigned LOT = 128;   
    auto piece = [&]() { for (const NkDrawCall3D& o : objets) rendu3d->Submit(o); };

    // Un par un : une soumission par cube
    auto unParUn = [&](unsigned n) -> Mesure {
        std::vector<NkDrawCall3D> cubes;
        for (const NkMat4f& m : Grille(n)) {
            NkDrawCall3D dc;
            dc.mesh = cube;
            dc.transform = m;
            dc.tint = blanc;
            dc.aabb = NkAABB::Unit().Transformed(m);
            cubes.push_back(dc);
        }
        return Mesurer(fenetre, renderer, rendu3d, scene, [&]() {
            piece();
            for (const NkDrawCall3D& c : cubes) rendu3d->Submit(c);
        });
    };

    // Groupes : une soumission par lot de cubes identiques (NkDrawCallInstanced)
    auto groupes = [&](unsigned n) -> Mesure {
        std::vector<NkDrawCallInstanced> lots;
        const std::vector<NkMat4f> m = Grille(n);
        for (unsigned debut = 0; debut < n; debut += LOT) {
            NkDrawCallInstanced di;
            di.mesh = cube;
            for (unsigned i = debut; i < n && i < debut + LOT; ++i) {
                di.transforms.PushBack(m[i]);
                di.tints.PushBack(blanc);
                di.aabb.Merge(NkAABB::Unit().Transformed(m[i]));   // la boite contient toutes les instances
            }
            lots.push_back(di);
        }
        return Mesurer(fenetre, renderer, rendu3d, scene, [&]() {
            piece();
            for (const NkDrawCallInstanced& l : lots) rendu3d->SubmitInstanced(l);
        });
    };

    // Salle seule : cadence de depart et seuil a viser
    const Mesure base = Mesurer(fenetre, renderer, rendu3d, scene, piece);
    if (!base.ok) return 1;
    const bool sousSoixante = base.fps < 60.0;
    const double cible = sousSoixante ? base.fps / 2.0 : 60.0;
    std::printf("Salle seule : %.1f images/s, drawCalls %u, cible %.1f\n", base.fps, base.drawCalls, cible);

    // Un par un : premier nombre de cubes sous la cible
    unsigned nUn = 25, seuilUn = 0;
    Mesure mUn = base;
    for (unsigned n = 25; n <= PLAFOND_UN_PAR_UN; n = (unsigned)(n * 1.25) + 5) {
        const Mesure m = unParUn(n);
        if (!m.ok) return 1;
        std::printf("un par un : %u cubes, drawCalls %u, %.1f images/s\n", n, m.drawCalls, m.fps);
        nUn = n;
        mUn = m;
        if (m.fps < cible) { seuilUn = n; break; }
    }

    // Groupes, pour le meme nombre de cubes, puis nouveau seuil
    const Mesure mGr = groupes(nUn);
    if (!mGr.ok) return 1;
    std::printf("groupes : %u cubes, drawCalls %u, %.1f images/s\n", nUn, mGr.drawCalls, mGr.fps);

    unsigned seuilGr = 0, dernierGr = nUn;
    if (mGr.fps < cible) {
        seuilGr = nUn;
    } else {
        for (unsigned n = (unsigned)(nUn * 1.5); n <= PLAFOND_GROUPE; n = (unsigned)(n * 1.5)) {
            const Mesure m = groupes(n);
            if (!m.ok) return 1;
            std::printf("groupes : %u cubes, drawCalls %u, %.1f images/s\n", n, m.drawCalls, m.fps);
            dernierGr = n;
            if (m.fps < cible) { seuilGr = n; break; }
        }
    }

    char t[256];
    std::string texte;
    if (seuilUn) std::snprintf(t, sizeof(t), "Un par un : %u cubes, drawCalls %u, %.1f images par seconde\n", seuilUn, mUn.drawCalls, mUn.fps);
    else std::snprintf(t, sizeof(t), "Un par un : %u cubes (seuil non atteint, limite du pool d'objets), drawCalls %u, %.1f images par seconde\n", nUn, mUn.drawCalls, mUn.fps);
    texte += t;
    std::snprintf(t, sizeof(t), "Cadence mesuree sur 400 images, synchronisation verticale coupee. Salle seule : %.1f images par seconde.\n", base.fps);
    texte += t;
    if (sousSoixante) {
        std::snprintf(t, sizeof(t), "La salle seule tourne deja sous 60 images par seconde : le seuil retenu est la moitie de sa cadence, soit %.1f.\n", cible);
        texte += t;
    }
    texte += "\n";
    if (seuilGr) std::snprintf(t, sizeof(t), "Groupés : drawCalls %u, %.1f images par seconde pour les memes %u cubes, nouveau seuil %u cubes\n", mGr.drawCalls, mGr.fps, nUn, seuilGr);
    else         std::snprintf(t, sizeof(t), "Groupés : drawCalls %u, %.1f images par seconde pour les memes %u cubes, nouveau seuil superieur a %u cubes (non atteint, limite fixee a %u)\n", mGr.drawCalls, mGr.fps, nUn, dernierGr, PLAFOND_GROUPE);
    texte += t;
    texte += "\nTrouvé : NkDrawCallInstanced, declare dans R3D/include/NKRenderer/Core/NkRendererTypes.h (champs mesh, material, transforms, tints, aabb), soumis avec NkRender3D::SubmitInstanced.\n";
    Ecrire(texte);
    std::printf("\n%s\ncadence.txt ecrit. Vous pouvez fermer la fenetre.\n", texte.c_str());

    // la fenetre reste ouverte pour lire la console
    while (fenetre.IsOpen()) {
        NkEvents().PollEvents();
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(peripherique);
    return 0;
}
