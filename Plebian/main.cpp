#include "renderer/window.h"

#include <entityx/Entity.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"
#include "input/input.h"
#include "input/fp_camera_controller.h"
#include "components.h"
#include "io/meshloader.h"
#include "io/textureloader.h"
#include "renderer/mesh_renderer.h"
#include "renderer/shader.h"
#include "renderer/camera.h"
#include "entity_gui.h"

int main(void) {
    Window window;
    window.Init(1280, 720);

    window.SetInput(new Input);
    window.GetInput()->Init(window.GetWindow());

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    MeshRenderer mesh_renderer;
    Shader shader;
    shader.Init("basic.glsl");

    MeshLoader mesh_loader;
    TextureLoader texture_loader;

    entityx::EventManager events;
    entityx::EntityManager entities(events);
    entityx::Entity ent = entities.create();
    Transform* transform = ent.assign<Transform>().get();
    transform->pos.x += 0;
    ent.assign<MeshComponent>(mesh_loader.GetMesh("suzanne.obj"), &shader, texture_loader.GetTexture("suzanne.png"));
    mesh_renderer.RegisterEntity(ent);

    ent = entities.create();
    ent.assign<Transform>();
    ent.assign<MeshComponent>(mesh_loader.GetMesh("torus.obj"), &shader, texture_loader.GetTexture("mona_lisa.png"));
    mesh_renderer.RegisterEntity(ent);

    Camera camera(1280, 720, 60);
    camera.position = glm::vec3(0.f, 0.f, 5.f);
    window.resizeListeners.push_back(&camera);
    glfwSetInputMode(window.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    ImGui_ImplGlfwGL3_Init(window.GetWindow(), true);

    FPCameraController camController(&camera);
    window.GetInput()->AddListener(&camController);
    ImGuiListener guiListener;
    window.GetInput()->AddListener(&guiListener);

    double time = glfwGetTime();
    float delta;

    bool show_entity_editor = true;
    while (!window.ShouldClose()) {
        delta = (float) (glfwGetTime() - time);
        time = glfwGetTime();

        window.UpdateInput();
        ImGui_ImplGlfwGL3_NewFrame();

        ShowEntityEditor(&show_entity_editor, &camera, &entities);

        camController.Update(delta);
        camera.UpdateMatrix();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mesh_renderer.Render(delta, camera);
        ImGui::Render();
        window.SwapBuffers();
    }

    window.GetInput()->ClearListeners();
}
