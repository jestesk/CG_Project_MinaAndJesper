// Model viewer code for the assignments in Computer Graphics 1TD388/1MD150.
//
// Modify this and other source files according to the tasks in the instructions.
//

#include "gltf_io.h"
#include "gltf_scene.h"
#include "gltf_render.h"
#include "cg_utils.h"
#include "cg_trackball.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <cstdlib>
#include <iostream>
#include <cstring>

// Struct for our application context
struct Context {
    int width = 800;
    int height = 800;
    GLFWwindow *window;
    gltf::GLTFAsset asset;
    gltf::DrawableList drawables;
    cg::Trackball trackball;
    GLuint program;
    GLuint emptyVAO;
    float elapsedTime;
    std::string gltfFilename = "lpshead.gltf";
    glm::vec3 u_diffuseColor;
    glm::vec3 u_lightPosition;
    float u_enableTexture = 0.0f;
    bool hasPerspectiveProjection = true;
    gltf::TextureList textures;
    GLuint framebufferProgram;
    // Add more variables here...
};

GLuint FBO, framebufferTexture, RBO, rectVAO, rectVBO;

float rectangleVertices[] =
{
    // Coords    // texCoords
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
    -1.0f,  1.0f,  0.0f, 1.0f,

     1.0f,  1.0f,  1.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f,  0.0f, 1.0f
};


// Returns the absolute path to the src/shader directory
std::string shader_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/src/shaders/";
}

// Returns the absolute path to the assets/gltf directory
std::string gltf_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/gltf/";
}

// Returns the absolute path to the assets/gltf directory
std::string cubemap_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/cubemaps/";
}


void do_initialization(Context &ctx)
{
    ctx.program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");
    ctx.framebufferProgram = cg::load_shader_program(shader_dir() + "framebuffer.vert", shader_dir() + "framebuffer.frag");
    gltf::load_gltf_asset(ctx.gltfFilename, gltf_dir(), ctx.asset);
    gltf::create_drawables_from_gltf_asset(ctx.drawables, ctx.asset);
    gltf::create_textures_from_gltf_asset(ctx.textures, ctx.asset);
    
    
}

void draw_scene(Context &ctx)
{
    
    // Activate shader program
    glUseProgram(ctx.program);
    

    // Set render state
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering

    // Define per-scene uniforms
    glUniform1f(glGetUniformLocation(ctx.program, "u_time"), ctx.elapsedTime);
    glUniform1f(glGetUniformLocation(ctx.program, "u_enableTexture"), ctx.u_enableTexture);
  
    // ...

    // Draw scene
    for (unsigned i = 0; i < ctx.asset.nodes.size(); ++i) {
        const gltf::Node &node = ctx.asset.nodes[i];
        const gltf::Drawable &drawable = ctx.drawables[node.mesh];

        // Define per-object uniforms
        // Model matrix
        glm::mat4 rotation = glm::mat4(node.rotation);
        glm::mat4 translation = glm::translate(glm::mat4(), node.translation);
        glm::mat4 scale = glm::scale(node.scale);
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_model"), 1, GL_FALSE, &model[0][0]);
        
        // View matrix
        glm::vec3 eye = glm::vec3(0.0f, 0.0f, 2.0f);
        glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 lookAt = glm::lookAt(eye, center, up);
        glm::mat4 trackball = glm::mat4(ctx.trackball.orient);
        glm::mat4 view = lookAt * trackball;
        
        glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_view"), 1, GL_FALSE, &view[0][0]);
        
        // Projection matrix
        //
        //glm::mat4 perspective = glm::perspective(45.0f, 800.0f / 600.0f, 1.0f, 10.0f);
        //glm::mat4 orthographic = glm::ortho(-2.0f,2.0f,-2.0f,2.0f,1.0f,2.0f);
        glm::mat4 perspective = glm::mat4(1.0f);
        glm::mat4 orthographic = glm::mat4(1.0f);
        if(ctx.hasPerspectiveProjection){
            glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_projection"), 1, GL_FALSE, &perspective[0][0]);
        } else {
            glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_projection"), 1, GL_FALSE, &orthographic[0][0]);

        }
     
        const gltf::Mesh &mesh = ctx.asset.meshes[node.mesh];
        if (mesh.primitives[0].hasMaterial) {
            const gltf::Primitive &primitive = mesh.primitives[0];
            const gltf::Material &material = ctx.asset.materials[primitive.material];
            const gltf::PBRMetallicRoughness &pbr = material.pbrMetallicRoughness;

            // Define material textures and uniforms
            // ...
            
            if (pbr.hasBaseColorTexture) {
                GLuint texture_id = ctx.textures[pbr.baseColorTexture.index];
                glBindVertexArray(drawable.vao);
                glActiveTexture(GL_TEXTURE0);
                glUniform1i(glGetUniformLocation(ctx.program, "u_texture"), 0);
                glBindTexture(GL_TEXTURE_2D, texture_id);
                // Bind texture and define uniforms...
            } else {
                // Need to handle this case as well, by telling
                // the shader that no texture is available
            }
        }

        // Draw object
        
        // Bind the custom framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        // Specify the color of the background
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        // Clean the back buffer and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Enable depth testing since it's disabled when drawing the framebuffer rectangle
        glEnable(GL_DEPTH_TEST);
    
        glDrawElements(GL_TRIANGLES, drawable.indexCount, drawable.indexType,
                       (GLvoid *)(intptr_t)drawable.indexByteOffset);
        glBindVertexArray(0);
        
        
        // Bind the default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Draw the framebuffer rectangle
        glUseProgram(ctx.framebufferProgram);
        glBindVertexArray(rectVAO);
        glDisable(GL_DEPTH_TEST);
        // prevents framebuffer rectangle from being discarded
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

    }

    // Clean up
    cg::reset_gl_render_state();
    glUseProgram(0);
}


bool enableTexture = true;
void do_rendering(Context &ctx)
{
    // Clear render states at the start of each frame
    cg::reset_gl_render_state();

    ImGui::Begin("Setting");

    ImGui::Checkbox("Texture Mapping", &enableTexture);
    
    ImGui::End();

    if(enableTexture){
        ctx.u_enableTexture = 1.0f;
    } else {
        ctx.u_enableTexture = 0.0f;
    }
    // Clear color and depth buffers
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_scene(ctx);
}

void reload_shaders(Context *ctx)
{
    glDeleteProgram(ctx->program);
    ctx->program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");
}

void error_callback(int /*error*/, const char *description)
{
    std::cerr << description << std::endl;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_R && action == GLFW_PRESS) { reload_shaders(ctx); }
}

void char_callback(GLFWwindow *window, unsigned int codepoint)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_CharCallback(window, codepoint);
    if (ImGui::GetIO().WantTextInput) return;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;

    double x, y;
    glfwGetCursorPos(window, &x, &y);

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        ctx->trackball.center = glm::vec2(x, y);
        ctx->trackball.tracking = (action == GLFW_PRESS);
    }
}

void cursor_pos_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    if (ImGui::GetIO().WantCaptureMouse) return;

    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    cg::trackball_move(ctx->trackball, float(x), float(y));
}

void scroll_callback(GLFWwindow *window, double x, double y)
{
    // Forward event to ImGui
    ImGui_ImplGlfw_ScrollCallback(window, x, y);
    if (ImGui::GetIO().WantCaptureMouse) return;
}

void resize_callback(GLFWwindow *window, int width, int height)
{
    // Update window size and viewport rectangle
    Context *ctx = static_cast<Context *>(glfwGetWindowUserPointer(window));
    ctx->width = width;
    ctx->height = height;
    glViewport(0, 0, width, height);
}

void prepare_framebuffer_rectangle(){
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), &rectangleVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void initialize_framebuffer(Context &ctx){
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx.width, ctx.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    // Create Render Buffer Object
    glGenRenderbuffers(1, &RBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ctx.width, ctx.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);


    // Error checking framebuffer
    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer error: " << fboStatus << std::endl;
    
}

int main(int argc, char *argv[])
{
    Context ctx = Context();
    if (argc > 1) { ctx.gltfFilename = std::string(argv[1]); }

    // Create a GLFW window
    glfwSetErrorCallback(error_callback);
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    ctx.window = glfwCreateWindow(ctx.width, ctx.height, "Model viewer", nullptr, nullptr);
    glfwMakeContextCurrent(ctx.window);
    glfwSetWindowUserPointer(ctx.window, &ctx);
    glfwSetKeyCallback(ctx.window, key_callback);
    glfwSetCharCallback(ctx.window, char_callback);
    glfwSetMouseButtonCallback(ctx.window, mouse_button_callback);
    glfwSetCursorPosCallback(ctx.window, cursor_pos_callback);
    glfwSetScrollCallback(ctx.window, scroll_callback);
    glfwSetFramebufferSizeCallback(ctx.window, resize_callback);

    // Load OpenGL functions
    if (gl3wInit() || !gl3wIsSupported(3, 3) /*check OpenGL version*/) {
        std::cerr << "Error: failed to initialize OpenGL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Initialize ImGui
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(ctx.window, false /*do not install callbacks*/);
    ImGui_ImplOpenGL3_Init("#version 330" /*GLSL version*/);

    // Initialize rendering
    glGenVertexArrays(1, &ctx.emptyVAO);
    glBindVertexArray(ctx.emptyVAO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    do_initialization(ctx);
    glUseProgram(ctx.framebufferProgram);
    glUniform1i(glGetUniformLocation(ctx.framebufferProgram, "screenTexture"), 0);
    
    prepare_framebuffer_rectangle();
    initialize_framebuffer(ctx);

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        glfwPollEvents();
        ctx.elapsedTime = glfwGetTime();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //ImGui::ShowDemoWindow();
        
        do_rendering(ctx);
        
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
