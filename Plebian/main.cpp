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
#include "renderer/shadow_map.h"
#include "entity_gui.h"
#include "renderer/g_buffer.h"

int main(void) {
    Window window;
    window.Init(1280, 720);

    window.SetInput(new Input);
    window.GetInput()->Init(window.GetWindow());

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GBuffer g_buffer;
    g_buffer.Init(1280, 720);
    window.resizeListeners.push_back(&g_buffer);

    MeshRenderer mesh_renderer;
    Shader shader;
    shader.Init("basic.glsl");

    Shader light_shader;
    light_shader.Init("light_pass.glsl");

    glUseProgram(light_shader.m_shader_program);
    glUniform1i(glGetUniformLocation(light_shader.m_shader_program, "position_tex"), GBuffer::tex_position);
    glUniform1i(glGetUniformLocation(light_shader.m_shader_program, "diffuse_tex"), GBuffer::tex_diffuse);
    glUniform1i(glGetUniformLocation(light_shader.m_shader_program, "normal_tex"), GBuffer::tex_normal);

    Shader shadow_pass;
    shadow_pass.Init("shadow_pass.glsl");

    static const GLfloat quad_vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };;

    GLuint quad;
    glGenVertexArrays(1, &quad);
    glBindVertexArray(quad);

    GLuint quad_buf;
    glGenBuffers(1, &quad_buf);
    glBindBuffer(GL_ARRAY_BUFFER, quad_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_buf);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glBindVertexArray(0);

    MeshLoader mesh_loader;
    TextureLoader texture_loader;

    entityx::EventManager events;
    entityx::EntityManager entities(events);
    entityx::Entity ent = entities.create();
    Transform* monkey_transform = ent.assign<Transform>().get();
    ent.assign<MeshComponent>(mesh_loader.GetMesh("suzanne.obj"), &shader, texture_loader.GetTexture("suzanne.png"));
    mesh_renderer.RegisterEntity(ent);

    ent = entities.create();
    Transform* transform = ent.assign<Transform>().get();
    transform->parent = monkey_transform;
    ent.assign<MeshComponent>(mesh_loader.GetMesh("torus.obj"), &shader, texture_loader.GetTexture("mona_lisa.png"));
    mesh_renderer.RegisterEntity(ent);

    ent = entities.create();
    transform = ent.assign<Transform>().get();
    transform->parent = monkey_transform;
    transform->pos.y = -2.5f;
    ent.assign<MeshComponent>(mesh_loader.GetMesh("plane.obj"), &shader, texture_loader.GetTexture("suzanne.png"));
    mesh_renderer.RegisterEntity(ent);


    Camera camera;
    camera.InitPerspective(1280, 720, 60);
    camera.position = glm::vec3(0.f, 0.f, 5.f);
    window.resizeListeners.push_back(&camera);
    glfwSetInputMode(window.GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    Camera shadow_camera;
    shadow_camera.InitOrtho(16, 16, -10, 10);
    shadow_camera.orientation = glm::rotate(shadow_camera.orientation, glm::radians(90.0f), glm::vec3(1, 0, 0));
    shadow_camera.UpdateMatrix();

    glUseProgram(shader.m_shader_program);
    glUniform1i(glGetUniformLocation(shader.m_shader_program, "shadowMap"), 1);
    glUniformMatrix4fv(glGetUniformLocation(shader.m_shader_program, "lightMVP"), 1,
                       GL_FALSE, glm::value_ptr(shadow_camera.combined));
    glUseProgram(0);

    ShadowMap shadow_map;
    shadow_map.Init(1024, 1024);

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

        glViewport(0, 0, window.width, window.height);
        g_buffer.Draw();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mesh_renderer.Render(delta, camera, shadow_camera);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g_buffer.Read();

        glUseProgram(light_shader.m_shader_program);
        glUniform3fv(glGetUniformLocation(light_shader.m_shader_program, "cam_pos"), 1, glm::value_ptr(camera.position));
        glBindVertexArray(quad);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        ImGui::Render();
        window.SwapBuffers();
    }

    window.GetInput()->ClearListeners();
}
