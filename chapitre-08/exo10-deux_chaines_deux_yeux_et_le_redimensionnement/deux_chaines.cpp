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
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

using namespace nkentseu;
using namespace nkentseu::renderer;

class ChaineEchange {
public:
    static constexpr std::uint32_t MAX_IMAGES = 4;

    bool Creer(std::uint32_t nombre) noexcept {
        if (nombre == 0 || nombre > MAX_IMAGES) return false;
        mNombre = nombre;
        mSuivante = 0;
        mEmpruntee = false;
        for (std::uint64_t& p : mPoignees) p = 0;
        return true;
    }
    void Detruire() noexcept {
        mNombre = 0;
        mSuivante = 0;
        mEmpruntee = false;
        for (std::uint64_t& p : mPoignees) p = 0;
    }
    bool EnregistrerImage(std::uint32_t index, std::uint64_t poignee) noexcept {
        if (index >= mNombre) return false;
        mPoignees[index] = poignee;
        return true;
    }
    bool Acquerir(std::uint32_t& index) noexcept {
        if (mNombre == 0 || mEmpruntee) return false;
        index = mSuivante;
        mEmpruntee = true;
        return true;
    }
    bool Rendre() noexcept {
        if (mNombre == 0 || !mEmpruntee) return false;
        mEmpruntee = false;
        mSuivante = (mSuivante + 1u) % mNombre;
        return true;
    }
    std::uint32_t DerniereRendue() const noexcept {
        if (mNombre == 0) return 0;
        return (mSuivante + mNombre - 1u) % mNombre;
    }
    std::uint64_t Poignee(std::uint32_t index) const noexcept { return index < mNombre ? mPoignees[index] : 0; }

private:
    std::uint64_t mPoignees[MAX_IMAGES] = {};
    std::uint32_t mNombre = 0;
    std::uint32_t mSuivante = 0;
    bool mEmpruntee = false;
};

static const std::uint32_t IMAGES_PAR_OEIL = 3;
static const int IMAGES_TRACEES = 30;

struct Oeil {
    const char* nom = "";
    ChaineEchange chaine;
    NkOffscreenTarget* images[IMAGES_PAR_OEIL] = { nullptr, nullptr };
    NkRenderer* rendu = nullptr;
    std::vector<NkDrawCall3D> objets;
    int valides = 0, perimees = 0, nulles = 0;
};

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

static std::string Depot(const char* nom) {
    const std::string p = std::string("../chapitre-08/exo10-deux_chaines_deux_yeux_et_le_redimensionnement/") + nom;
    if (FILE* f = std::fopen(p.c_str(), "ab")) {
        std::fclose(f);
        return p;
    }
    return nom;
}

static void Ecrire(FILE* f, const char* texte) {
    if (!f) return;
    std::fputs(texte, f);
    std::fputc('\n', f);
    std::fflush(f);
}

int nkmain(const NkEntryState&) {
    const char* oubli = std::getenv("OUBLI");
    const bool oubliChaine = oubli && std::strcmp(oubli, "chaine") == 0;
    const bool oubliPoignees = oubli && std::strcmp(oubli, "poignees") == 0;
    const bool oubliRecreer = oubli && std::strcmp(oubli, "recreer") == 0;
    const bool simulation = std::getenv("SIMULE") != nullptr;
    bool dejaSimule = false;
    const bool essai = oubliChaine || oubliPoignees || oubliRecreer; 

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Deux chaines";
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

    Oeil yeux[2];
    yeux[0].nom = "gauche";
    yeux[1].nom = "droit";

    long long largeurFenetre = (long long)fenetre.GetSize().x;
    long long hauteurFenetre = (long long)fenetre.GetSize().y;
    int lc = (int)(largeurFenetre / 2), hc = (int)hauteurFenetre;

    auto creerCible = [&](const Oeil& o, std::uint32_t i) -> NkOffscreenTarget* {
        NkOffscreenDesc od;
        od.width = lc;
        od.height = hc;
        od.hasDepth = true;
        od.hdr = false;
        od.colorFmt = NkGPUFormat::NK_RGBA8_UNORM;
        od.readable = true;
        od.readback = false;
        od.name = o.nom;
        (void)i;
        return rEcran->CreateOffscreen(od);
    };

    for (int e = 0; e < 2; ++e) {
        Oeil& o = yeux[e];
        for (std::uint32_t i = 0; i < IMAGES_PAR_OEIL; ++i) {
            o.images[i] = creerCible(o, i);
            if (!o.images[i]) return 1;
        }
        NkRendererConfig cfgSalle = NkRendererConfig::ForGame(devInit.api, lc, hc);
        cfgSalle.Disable(NK_SS_RENDER2D);
        cfgSalle.Disable(NK_SS_TEXT);
        cfgSalle.Disable(NK_SS_OVERLAY);
        cfgSalle.Enable(NK_SS_OFFSCREEN);
        o.rendu = NkRenderer::Create(device, cfgSalle);
        if (!o.rendu || !o.rendu->Initialize()) return 1;
        o.objets = Scene(o.rendu->GetMeshSystem());
    }

    using Natif = std::decay_t<decltype(rEcran->GetTextures()->GetRHIHandle(yeux[0].images[0]->GetColorResult()))>;
    static_assert(std::is_trivially_copyable<Natif>::value && sizeof(Natif) <= sizeof(std::uint64_t),
                  "la poignee du moteur doit tenir dans 64 bits");
    auto natif = [&](const Oeil& o, std::uint32_t i) -> Natif {
        return rEcran->GetTextures()->GetRHIHandle(o.images[i]->GetColorResult());
    };
    auto enEntier = [](const Natif& n) -> std::uint64_t {
        std::uint64_t v = 0;
        std::memcpy(&v, &n, sizeof(Natif));
        return v;
    };

    FILE* trace = essai ? nullptr : std::fopen(Depot("trace.txt").c_str(), "wb");
    FILE* resize = essai ? nullptr : std::fopen(Depot("resize.txt").c_str(), "wb");
    FILE* observations = std::fopen("exo10_observations.txt", "wb");
    char ligne0[160];

    for (Oeil& o : yeux) {
        o.chaine.Creer(IMAGES_PAR_OEIL);
        for (std::uint32_t i = 0; i < IMAGES_PAR_OEIL; ++i) o.chaine.EnregistrerImage(i, enEntier(natif(o, i)));
    }

    std::snprintf(ligne0, sizeof(ligne0), "taille initiale lue : %lld x %lld", largeurFenetre, hauteurFenetre);
    Ecrire(observations, ligne0);

    int image = 0;
    int evenements = 0;
    int imageRedim = 0;  
    bool tourne = true;
    char ligne[256];

    auto bilan = [&](const char* quand) {
        std::snprintf(ligne, sizeof(ligne),
                      "bilan %s, image %d : gauche valides %d perimees %d nulles %d, droit valides %d perimees %d nulles %d",
                      quand, image, yeux[0].valides, yeux[0].perimees, yeux[0].nulles,
                      yeux[1].valides, yeux[1].perimees, yeux[1].nulles);
        Ecrire(observations, ligne);
        for (Oeil& o : yeux) o.valides = o.perimees = o.nulles = 0;
    };

    while (tourne && fenetre.IsValid()) {
        NkEvent* ev = nullptr;
        while (NkEvents().PollEvent(ev)) {
            if (ev->As<NkWindowCloseEvent>() != nullptr) tourne = false;
            if (ev->As<NkWindowResizeEvent>() != nullptr) ++evenements;
        }
        if (!tourne) break;

        long long tx = (long long)fenetre.GetSize().x, ty = (long long)fenetre.GetSize().y;
        const bool tailleChangee = tx > 0 && ty > 0 && (tx != largeurFenetre || ty != hauteurFenetre);
        const bool simule = simulation && !dejaSimule && image >= 60;
        if (tailleChangee || simule) {
            if (imageRedim != 0) bilan("avant le redimensionnement suivant");
            if (simule) {
                dejaSimule = true;
                tx = largeurFenetre * 4 / 5;
                ty = hauteurFenetre * 4 / 5;
            }
            std::snprintf(ligne, sizeof(ligne), "declencheur : %s, taille lue : %lld x %lld, evenements recus depuis le precedent : %d",
                          simule ? "simulation" : "taille changee", tx, ty, evenements);
            Ecrire(observations, ligne);
            evenements = 0;
            largeurFenetre = tx;
            hauteurFenetre = ty;
            lc = (int)(tx / 2);
            hc = (int)ty;
            imageRedim = image + 1;
            FILE* journal = essai ? observations : resize;

            if (!simule) {
                if (auto* r2d = rEcran->GetRender2D()) r2d->OnResize((std::uint32_t)tx, (std::uint32_t)ty);
            }
            int changees = 0;
            for (Oeil& o : yeux) {
                for (std::uint32_t i = 0; i < IMAGES_PAR_OEIL; ++i) {
                    const std::uint64_t avant = enEntier(natif(o, i));
                    o.images[i]->Resize((std::uint32_t)lc, (std::uint32_t)hc);
                    if (enEntier(natif(o, i)) != avant) ++changees;
                }
            }
            std::snprintf(ligne, sizeof(ligne), "poignees de cibles changees par le redimensionnement : %d sur %u",
                          changees, (unsigned)(2 * IMAGES_PAR_OEIL));
            Ecrire(observations, ligne);

            if (oubliChaine) {
                Ecrire(observations, "essai chaine : cibles redimensionnees, chaines laissees telles quelles");
            } else {
                if (!oubliRecreer) {
                    for (Oeil& o : yeux) {
                        o.chaine.Detruire();
                        std::snprintf(ligne, sizeof(ligne), "detruit la chaine %s", o.nom);
                        Ecrire(journal, ligne);
                    }
                    for (Oeil& o : yeux) {
                        o.chaine.Creer(IMAGES_PAR_OEIL);
                        std::snprintf(ligne, sizeof(ligne), "recree la chaine %s (%u images)", o.nom, (unsigned)IMAGES_PAR_OEIL);
                        Ecrire(journal, ligne);
                    }
                } else {
                    Ecrire(observations, "essai recreer : chaines gardees, seules les poignees sont reenregistrees");
                }
                if (!oubliPoignees) {
                    for (Oeil& o : yeux) {
                        for (std::uint32_t i = 0; i < IMAGES_PAR_OEIL; ++i) {
                            o.chaine.EnregistrerImage(i, enEntier(natif(o, i)));
                            std::snprintf(ligne, sizeof(ligne), "reenregistre l'image %u de la chaine %s", (unsigned)i, o.nom);
                            Ecrire(journal, ligne);
                        }
                    }
                } else {
                    Ecrire(observations, "essai poignees : chaines refaites, images non reenregistrees");
                }
            }
        }

        if (rEcran->BeginFrame()) {
            NkICommandBuffer* cmd = rEcran->GetCmd();
            std::uint32_t pris[2] = { 0, 0 };

            for (int e = 0; e < 2; ++e) {
                Oeil& o = yeux[e];

                if (!o.chaine.Acquerir(pris[e])) continue;

                o.rendu->SetFinalColorTarget(natif(o, pris[e]));
                o.rendu->SetRenderSizeOverride(lc, hc);
                NkRender3D* r3d = o.rendu->GetRender3D();
                o.rendu->FlushGraphRebuilds();
                r3d->ResetFrame();
                if (auto* mc = o.rendu->GetMaterialCollection()) mc->Upload();

                const float demiEcart = 0.063f * 0.5f;
                const float cx = 0.3f + (e == 0 ? -demiEcart : demiEcart);
                NkCamera3DData cd;
                cd.position = { cx, 1.7f, 1.2f };
                cd.target = { cx, 1.7f, 0.2f };
                cd.up = { 0.f, 1.f, 0.f };
                cd.fovY = 70.f;
                cd.aspect = (float)lc / (float)hc;
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
                for (const NkDrawCall3D& obj : o.objets) r3d->Submit(obj);
                if (auto* graph = o.rendu->GetRenderGraph()) graph->Execute(cmd);

                o.chaine.Rendre();
            }

            if (auto* r2d = rEcran->GetRender2D()) {
                for (int e = 0; e < 2; ++e) {
                    Oeil& o = yeux[e];
                    const std::uint64_t soumise = o.chaine.Poignee(o.chaine.DerniereRendue());

                    int trouvee = -1;
                    for (std::uint32_t i = 0; i < IMAGES_PAR_OEIL; ++i)
                        if (soumise != 0 && soumise == enEntier(natif(o, i))) trouvee = (int)i;

                    if (imageRedim != 0) {
                        if (soumise == 0) ++o.nulles;
                        else if (trouvee < 0) ++o.perimees;
                        else ++o.valides;
                    }
                    if (trouvee >= 0) {
                        r2d->DrawImage(o.images[trouvee]->GetColorResult(), { e == 0 ? 0.f : (float)lc, 0.f, (float)lc, (float)hc });
                    }
                }
            }
            rEcran->Present();
            rEcran->EndFrame();

            ++image;
            if (imageRedim != 0 && image % 30 == 0) bilan("periodique");
            if (imageRedim != 0 && image == imageRedim) {
                std::snprintf(ligne, sizeof(ligne), "premier index prete apres le redimensionnement : gauche %u, droit %u",
                              (unsigned)pris[0], (unsigned)pris[1]);
                Ecrire(observations, ligne);
            }
            if (image <= IMAGES_TRACEES) {
                std::snprintf(ligne, sizeof(ligne), "image : %d, gauche : %u, droit : %u", image, (unsigned)pris[0], (unsigned)pris[1]);
                Ecrire(trace, ligne);
            }
        }
    }

    if (imageRedim == 0) {
        std::snprintf(ligne, sizeof(ligne), "aucun redimensionnement detecte en %d images, derniere taille lue : %lld x %lld",
                      image, largeurFenetre, hauteurFenetre);
        Ecrire(observations, ligne);
    }
    if (imageRedim != 0) bilan("final");

    if (trace) std::fclose(trace);
    if (resize) std::fclose(resize);
    if (observations) std::fclose(observations);
    NkRenderer::Destroy(yeux[0].rendu);
    NkRenderer::Destroy(yeux[1].rendu);
    NkRenderer::Destroy(rEcran);
    NkDeviceFactory::Destroy(device);
    return 0;
}