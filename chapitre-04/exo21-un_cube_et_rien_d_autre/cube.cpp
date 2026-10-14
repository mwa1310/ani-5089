#include "NKWindow/Core/NkEntry.h"
#include "NKWindow/EntryPoints/NkWindowsDesktop.h"
#include "NKWindow/Core/NkWindow.h"
#include "NKRHI/Core/NkDeviceFactory.h"
#include "NKRHI/Core/NkDeviceInitInfo.h"
#include "NKRenderer/NkRenderer.h"
#include "NKRenderer/Tools/Render3D/NkRender3D.h"
#include <cstdio>

using namespace nkentseu;
using namespace nkentseu::renderer;

int nkmain(const NkEntryState& etat) {
    (void)etat;

    NkWindowConfig cfgFenetre;
    cfgFenetre.title = "Cube";
    NkWindow fenetre(cfgFenetre);
    if (!fenetre.IsOpen()) {
        std::fprintf(stderr, "Fenetre non creee\n");
        return 1;
    }

    NkDeviceInitInfo init;
    init.surface = fenetre.GetSurfaceDesc();
    init.width = init.surface.width;
    init.height = init.surface.height;

    NkIDevice* peripherique = NkDeviceFactory::CreateAutoDetect(init);
    if (!peripherique || !peripherique->IsValid()) {
        std::fprintf(stderr, "Peripherique invalide\n");
        return 1;
    }
    std::printf("Interface graphique : %s\n", NkGraphicsApiName(init.api));

    NkRendererConfig cfg;
    cfg.api = init.api;
    cfg.width = init.width;
    cfg.height = init.height;
    cfg.subsystems = NK_SS_RENDER3D;

    NkRenderer* renderer = NkRenderer::Create(peripherique, cfg);
    if (!renderer || !renderer->Initialize()) {
        std::fprintf(stderr, "Renderer non initialise\n");
        return 1;
    }
    NkRender3D* rendu3d = renderer->GetRender3D();

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

    // Cube devant la camera 
    const NkVec3f centre = { 0.f, 1.5f, -0.5f };
    const float32 arete = 0.5f;
    const float32 demi = arete * 0.5f;

    NkDrawCall3D cube;
    cube.mesh = renderer->GetMeshSystem()->GetCube();
    cube.transform = NkMat4f::Translate(centre) * NkMat4f::Scale({ arete, arete, arete });
    cube.tint = { 1.f, 1.f, 1.f };
    cube.aabb.min = { centre.x - demi, centre.y - demi, centre.z - demi };
    cube.aabb.max = { centre.x + demi, centre.y + demi, centre.z + demi };

    unsigned image = 0;
    while (fenetre.IsOpen()) {
        NkEvents().PollEvents();
        if (!renderer->BeginFrame()) continue;
        rendu3d->BeginScene(scene);
        rendu3d->Submit(cube);
        renderer->Present();
        renderer->EndFrame();

        if (++image == 60) {
            std::printf("aabb non renseignees : %u\n", rendu3d->GetCullStats().aabbNonRenseignee);
        }
    }

    NkRenderer::Destroy(renderer);
    NkDeviceFactory::Destroy(peripherique);
    return 0;
}