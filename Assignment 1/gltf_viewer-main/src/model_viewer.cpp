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

#include <cstdlib>
#include <iostream>

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
    GLuint programPostProcessing;
    float elapsedTime;
    std::string gltfFilename = "armadillo.gltf";
    float myColor[3];
    float translate[3];
    float scale[3] = {1, 1, 1};
    float rotation[3];
    float eyePosition[3];
    float lookAtPosition[3];
    float diffuseColor[3];
    float lightPosition[3];

    float ambientColor[3];
    float specularColor[3];
    float specularPower;

    bool isNormalsRGB;

    bool useOrthographicProjection;

    bool useGammaCompensation;
    bool useCubemapColor;

    GLuint cubemaps[8];
    int activeCubemapIndex;

    gltf::TextureList textures;

    bool showTextureCoords;
    bool showTexture;
    // Add more variables here...
};

float zoom;
GLuint framebuffer, framebufferTexture, framebufferNormalsTexture, framebufferDepthTexture,
    vboFramebufferVertices, rectVAO, depthrenderbuffer;

float rectangleVertices[] = {
    1.0f, -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,

    1.0f, 1.0f,  1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f};

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

// Returns the absolute path to the \assets\cubemaps directory
std::string cubemap_dir(void)
{
    std::string rootDir = cg::get_env_var("MODEL_VIEWER_ROOT");
    if (rootDir.empty()) {
        std::cout << "Error: MODEL_VIEWER_ROOT is not set." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return rootDir + "/assets/cubemaps/";
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

void do_initialization(Context &ctx)
{
    ctx.program = cg::load_shader_program(shader_dir() + "mesh.vert", shader_dir() + "mesh.frag");
    ctx.programPostProcessing = cg::load_shader_program(shader_dir() + "postprocessing.vert", shader_dir() + "postprocessing.frag");

    gltf::load_gltf_asset(ctx.gltfFilename, gltf_dir(), ctx.asset);
    gltf::create_drawables_from_gltf_asset(ctx.drawables, ctx.asset);

    const char *folderNames[8] = {"0.5", "0.125", "2", "8", "32", "128", "512", "2048"};

    for (int i = 0; i < 8; i++) 
    { 
        ctx.cubemaps[i] = cg::load_cubemap(cubemap_dir() + "/Forrest/prefiltered/" + folderNames[i]);
    }

    gltf::create_textures_from_gltf_asset(ctx.textures, ctx.asset);

    // Create post processing target quad
    glGenVertexArrays(1, &rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glGenBuffers(1, &vboFramebufferVertices);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, vboFramebufferVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), rectangleVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));   

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_scene(Context &ctx)
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(ctx.translate[0], ctx.translate[1], ctx.translate[2]));
    model = glm::rotate(model, ctx.rotation[0], glm::vec3(1, 0, 0));
    model = glm::rotate(model, ctx.rotation[1], glm::vec3(0, 1, 0));
    model = glm::rotate(model, ctx.rotation[2], glm::vec3(0, 0, 1));
    model = glm::scale(model, glm::vec3(ctx.scale[0], ctx.scale[1], ctx.scale[2]));    

    // Activate shader program
    glUseProgram(ctx.program);

    // Set active texture
    if (ctx.useCubemapColor) {
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ctx.cubemaps[ctx.activeCubemapIndex]);

        glUniform1i(glGetUniformLocation(ctx.program, "u_cubemap"), GL_TEXTURE0);
    }

    // Define per-scene uniforms
    glUniform1f(glGetUniformLocation(ctx.program, "u_time"), ctx.elapsedTime);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_diffuseColor"), 1, ctx.diffuseColor);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_lightPosition"), 1, ctx.lightPosition);

    glUniform3fv(glGetUniformLocation(ctx.program, "u_ambientColor"), 1, ctx.ambientColor);
    glUniform3fv(glGetUniformLocation(ctx.program, "u_specularColor"), 1, ctx.specularColor);
    glUniform1f(glGetUniformLocation(ctx.program, "u_specularPower"), ctx.specularPower);

    glUniform1i(glGetUniformLocation(ctx.program, "u_isNormalsRGB"), ctx.isNormalsRGB);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useGammaCompensation"), ctx.useGammaCompensation);
    glUniform1i(glGetUniformLocation(ctx.program, "u_useCubemapColor"), ctx.useCubemapColor);

    glUniform1i(glGetUniformLocation(ctx.program, "u_showTextureCoords"), ctx.showTextureCoords);
    glUniform1i(glGetUniformLocation(ctx.program, "u_showTexture"), ctx.showTexture);
    
    
    // ...

    // Draw scene
    // Set post processing frame buffer as target
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    const GLenum buffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, buffers);

    glClearColor(ctx.myColor[0], ctx.myColor[1], ctx.myColor[2], 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Set render state
    glEnable(GL_DEPTH_TEST);  // Enable Z-buffering
    
    for (unsigned i = 0; i < ctx.asset.nodes.size(); ++i) {
        const gltf::Node &node = ctx.asset.nodes[i];
        const gltf::Drawable &drawable = ctx.drawables[node.mesh];

        // Define per-object uniforms
        // ...
        glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_model"), 1, GL_FALSE, &model[0][0]);

        glm::mat4 view =
            glm::lookAt(glm::vec3(ctx.eyePosition[0], ctx.eyePosition[1], ctx.eyePosition[2]),
            glm::vec3(ctx.lookAtPosition[0], ctx.lookAtPosition[1], ctx.lookAtPosition[2]),
            glm::vec3(0, -1, 0)) * glm::mat4(ctx.trackball.orient);
            
            //glm::mat4(ctx.trackball.orient); /*glm::lookAt(glm::vec3(0, 1, -2),
                                     //glm::vec3(ctx.trackball.center.x, ctx.trackball.center.y, 0),
                                     //glm::vec3(0, 1, 0));*/
        if (ctx.useOrthographicProjection) {
            projection = glm::ortho(-0.5f, 0.5f, 0.5f, -0.5f, -100.0f, 100.0f);
        } 
        else {
            projection = glm::perspective(10.0f +  0.1f * zoom, 16.0f / 9.0f, 0.2f, 150.0f);
        }
        
        glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_projection"), 1, GL_FALSE,
                           &projection[0][0]);

        glUniformMatrix4fv(glGetUniformLocation(ctx.program, "u_view"), 1, GL_FALSE, &view[0][0]);

        //Texture stuff
        const gltf::Mesh &mesh = ctx.asset.meshes[node.mesh];
        if (mesh.primitives[0].hasMaterial && ctx.showTexture) {
            const gltf::Primitive &primitive = mesh.primitives[0];
            const gltf::Material &material = ctx.asset.materials[primitive.material];
            const gltf::PBRMetallicRoughness &pbr = material.pbrMetallicRoughness;

            if (pbr.hasBaseColorTexture) {
                GLuint texture_id = ctx.textures[pbr.baseColorTexture.index];
                // Bind texture and define uniforms...
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture_id);


                glUniform1i(glGetUniformLocation(ctx.program, "u_hasTexture"), true);
                glUniform1i(glGetUniformLocation(ctx.program, "u_texture"), GL_TEXTURE0);

            } else {
                // Need to handle this case as well, by telling
                // the shader that no texture is available
                glUniform1i(glGetUniformLocation(ctx.program, "u_hasTexture"),
                            false);
            }
        }
        
        // Draw object
        glBindVertexArray(drawable.vao);
        glDrawElements(GL_TRIANGLES, drawable.indexCount, drawable.indexType,
                       (GLvoid *)(intptr_t)drawable.indexByteOffset);
        glBindVertexArray(0);
    }

    // Scene has been drawn, now do post processing
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind default frame buffer

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Activate postprocessing program and set uniforms
    glUseProgram(ctx.programPostProcessing);

    // Draw target quad
    glBindVertexArray(rectVAO);
    glDisable(GL_DEPTH_TEST);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);

    glActiveTexture(GL_TEXTURE0 +1);
    glBindTexture(GL_TEXTURE_2D, framebufferNormalsTexture);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, framebufferDepthTexture);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
    
    glBindVertexArray(0);
    // Clean up
    cg::reset_gl_render_state();
    glUseProgram(0);
}

void do_rendering(Context &ctx)
{
    // Clear render states at the start of each frame
    cg::reset_gl_render_state();  
    
    // Clear color and depth buffers
    
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
    zoom += y;

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

    // change scale of post processing buffers
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, framebufferNormalsTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_2D, framebufferDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
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

    // Create frame buffer and bind texture and depth buffer
    
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Create buffer texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx.width, ctx.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create buffers normals texture
    glActiveTexture(GL_TEXTURE0 + 1);
    glGenTextures(1, &framebufferNormalsTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferNormalsTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx.width, ctx.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create buffers depth texture
    glActiveTexture(GL_TEXTURE0 + 2);
    glGenTextures(1, &framebufferDepthTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ctx.width, ctx.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    //GLuint depthrenderbuffer;
    glGenRenderbuffers(1, &depthrenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ctx.width, ctx.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              depthrenderbuffer);
    
    //bind
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, framebufferNormalsTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, framebufferDepthTexture, 0);

    // Set post processing uniforms
    glUseProgram(ctx.programPostProcessing);
    
    glUniform1i(glGetUniformLocation(ctx.programPostProcessing, "u_screenTexture"), 0);
    glUniform1i(glGetUniformLocation(ctx.programPostProcessing, "u_normalTexture"), 1);
    glUniform1i(glGetUniformLocation(ctx.programPostProcessing, "u_depthTexture"), 2);

    glUniform1f(glGetUniformLocation(ctx.programPostProcessing, "u_width"), ctx.width);
    glUniform1f(glGetUniformLocation(ctx.programPostProcessing, "u_height"), ctx.height);

    GLenum status;
    if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Framebuffer status: error %p", status);
        return 0;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

    // Start rendering loop
    while (!glfwWindowShouldClose(ctx.window)) {
        

        glfwPollEvents();
        ctx.elapsedTime = glfwGetTime();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        //GUI?
        ImGui::ColorEdit3("My color", &ctx.myColor[0]);
        ImGui::SliderFloat3("Translate", &ctx.translate[0], -3, 3);
        ImGui::SliderFloat3("Rotate", &ctx.rotation[0], -3, 3);
        ImGui::SliderFloat3("Scale", &ctx.scale[0], 1, 2);
        ImGui::SliderFloat3("LookAt", &ctx.lookAtPosition[0], -10, 10);
        ImGui::SliderFloat3("EyePosition", &ctx.eyePosition[0], -10, 10);
        ImGui::SliderFloat3("LightPosition", &ctx.lightPosition[0], -15, 15);
        ImGui::ColorEdit3("DiffuseColor", &ctx.diffuseColor[0]);

        ImGui::ColorEdit3("AmbientColor", &ctx.ambientColor[0]);
        ImGui::ColorEdit3("SpecularColor", &ctx.specularColor[0]);
        ImGui::SliderFloat("SpecularPower", &ctx.specularPower, 0.1f, 2048.0f);

        ImGui::Checkbox("Normals As RGB", &ctx.isNormalsRGB);
        ImGui::Checkbox("Use Gamma Compensation", &ctx.useGammaCompensation);
        ImGui::Checkbox("Use Cubemap Color", &ctx.useCubemapColor);
        ImGui::SliderInt("Cubemap resolution", &ctx.activeCubemapIndex, 0, 7);

        ImGui::Checkbox("Show Texture Coords", &ctx.showTextureCoords);
        ImGui::Checkbox("Show Texture", &ctx.showTexture);

        ImGui::Checkbox("Orthographic projection", &ctx.useOrthographicProjection);
        // ImGui::ShowDemoWindow();
        do_rendering(ctx);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ctx.window);
    }

    // Shutdown
    glDeleteBuffers(1, &vboFramebufferVertices);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    std::exit(EXIT_SUCCESS);
}
