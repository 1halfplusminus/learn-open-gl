// clang-format off
#include <glad/glad.h>
// clang-format on
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <ostream>
#include <shader.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "engine.h"

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

double deltaTime = 0.0f; // Time between current frame and last frame
double lastFrame = 0.0f; // Time of last frame

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}

int main() {
  // GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Get Started", NULL, NULL);

  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // INIT GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // INIT OPEN GL
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
  /*  glEnable(GL_BLEND); */
  /*   glEnable(GL_CULL_FACE); */
  /*   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glCullFace(GL_BACK); */

  auto renderer = std::make_unique<Renderer>();
  auto resourceManager = std::make_unique<ResourceManager>(*renderer);
  ModelLoader modelLoader = ModelLoader(*resourceManager);
  SpriteRenderer spriteRenderer = SpriteRenderer();

  std::shared_ptr<Shader> meshShader(
      new Shader("./shader/vLight.glsl", "./shader/fKernel.glsl"));

  std::shared_ptr<Shader> shader(
      new Shader("./shader/vSprite.glsl", "./shader/fSprite.glsl"));

  std::shared_ptr<Shader> screenShader(
      new Shader("./shader/vframe_buffer.glsl", "./shader/fframe_buffer.glsl"));

  std::vector<Image> images = {Image("./texture/grass.png", true),
                               Image("./texture/container.jpg", true),
                               Image("./texture/wall.jpg", true)};

  Material mat = Material(shader);
  Material screenMat = Material(screenShader);
  Material meshMat = Material(meshShader);
  Material wallMath = Material(meshShader);

  for (auto image : images) {
    // load image
    Texture texture = resourceManager->loadTexture(image);
    texture.type = TextureType::Diffuse;
  }
  mat.textures.push_back(resourceManager->getTexture("./texture/grass.png"));
  meshMat.textures.push_back(
      resourceManager->getTexture("./texture/container.jpg"));
  wallMath.textures.push_back(
      resourceManager->getTexture("./texture/wall.jpg"));

  resourceManager->addMaterial(mat);
  resourceManager->addMaterial(meshMat);
  resourceManager->addMaterial(wallMath);

  MeshRenderer render;
  // projection
  glm::mat4 projection;
  projection = glm::perspective(
      glm::radians(30.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
  auto camera = Camera(projection);

  Light light = Light();
  light.Position = glm::vec3(0.0f, 0.2f, 1.0f);
  light.Ambiant = glm::vec3(1.0f, 1.0f, 1.0f);
  light.Diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  light.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
  light.Direction = glm::vec3(0.0f, 0.0f, 1.0f);

  // Create Framebuffer
  unsigned int fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // Create a texture
  Texture texture;
  glGenTextures(1, &texture.id);
  glBindTexture(GL_TEXTURE_2D, texture.id);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB,
               GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture.id, 0);
  // create a renderbuffer object for depth and stencil attachment (we won't be
  // sampling these)
  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH,
                        SCR_HEIGHT); // use a single renderbuffer object for
                                     // both a depth AND stencil buffer.
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo); // now actually attach it
  // now that we actually created the framebuffer and added all attachments we
  // want to check if it is actually complete now
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  screenMat.textures.push_back(texture);
  resourceManager->addMaterial(screenMat);

  auto screen = createPlane();
  renderer->createBuffer(screen);

  auto floor = modelLoader.loadModel("./texture/untitled.glb");
  for (auto &mesh : floor.meshes) {
    renderer->createBuffer(mesh);
  }
  auto floorTransform = glm::mat4(1.0f);
  floorTransform = glm::translate(floorTransform, glm::vec3(0.0f, 0.0f, 0.0f));
  /*   floorTransform = glm::scale(floorTransform,
   * glm::vec3(20.0f, 1.0f, 20.0f)); */
  /*  floorTransform = glm::translate(floorTransform, glm::vec3(0.0f, 0.0f,
   * 0.0f)); */
  /*   float angle = -180.0f;
    floorTransform = glm::rotate(floorTransform, glm::radians(angle),
                                 glm::vec3(1.0f, 0.0f, 0.0f));
    floorTransform = glm::scale(floorTransform, glm::vec3(10.0f)); */

  auto container = createCube();
  renderer->createBuffer(container);

  while (!glfwWindowShouldClose(window)) {
    double currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    // input
    // -----
    processInput(window);
    // render
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera.Position = glm::vec3(-10.0f, 10.0f, 15.0f);
    camera.Target = glm::vec3(5.0f, 0.0f, 0.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    render.useLight(*meshShader, light, camera);

    render.render(camera, floor, *meshShader, *resourceManager, floorTransform);

    /*  render.render(camera, container, meshMat, glm::mat4(1.0f));

     render.render(
         camera, container, meshMat,
         glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, -0.5f)));
  */
    /* spriteRenderer.render(camera, sprites, 0,
       resourceManager->getMaterials(), vegetation); */

    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    render.render(camera, screen, screenMat,
                  glm::scale(glm::mat4(1.0f), glm::vec3(2.0f)));

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}