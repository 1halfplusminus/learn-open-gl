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

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  glViewport(0, 0, width, height);
}

int main()
{
  // GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Get Started", NULL, NULL);

  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  // INIT GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }
  // projection
  glm::mat4 projection;
  projection = glm::perspective(
      glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
  auto camera = Camera(projection);
  OrbitCamera orbit(camera);
  GLFWInputHandler inputHandler(*window, orbit);
  // INIT OPEN GL
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
  // configure global opengl state
  // -----------------------------
  glEnable(GL_DEPTH_TEST);
  /*   glEnable(GL_BLEND); */

  auto renderer = std::make_unique<Renderer>();
  auto resourceManager = std::make_unique<ResourceManager>(*renderer);
  ModelLoader modelLoader = ModelLoader(*resourceManager);
  SpriteRenderer spriteRenderer = SpriteRenderer();

  std::shared_ptr<Shader> meshShader(
      new Shader("./shader/vLight.glsl", "./shader/fModel.glsl"));

  std::shared_ptr<Shader> shader(
      new Shader("./shader/vSprite.glsl", "./shader/fSprite.glsl"));

  std::shared_ptr<Shader> screenShader(
      new Shader("./shader/vframe_buffer.glsl", "./shader/fframe_buffer.glsl"));

  std::shared_ptr<Shader> skyBoxShader(
      new Shader("./shader/vSkybox.glsl", "./shader/fSkybox.glsl"));

  std::vector<Image> images = {Image("./texture/grass.png", true),
                               Image("./texture/container.jpg", true),
                               Image("./texture/wall.jpg", true),
                               Image("./texture/container2.png", false),
                               Image("./texture/container2_specular.png", false)};

  std::vector<Image> skyBox = {Image("./texture/skybox/right.jpg"),
                               Image("./texture/skybox/left.jpg"),
                               Image("./texture/skybox/top.jpg"),
                               Image("./texture/skybox/bottom.jpg"),
                               Image("./texture/skybox/front.jpg"),
                               Image("./texture/skybox/back.jpg")};
  Material mat = Material(shader);
  Material screenMat = Material(screenShader);
  Material meshMat = Material(meshShader);
  Material wallMath = Material(meshShader);
  Texture skyBoxTexture = resourceManager->loadTextureCube(skyBox);
  for (auto image : images)
  {
    // load image
    Texture texture = resourceManager->loadTexture2D(image);
    texture.type = TextureType::Diffuse;
  }

  mat.textures.push_back(resourceManager->getTexture("./texture/container2.png"));
  auto diffuse = resourceManager->getTexture("./texture/container2_specular.png");
  diffuse.type = TextureType::Diffuse;
  mat.textures.push_back(diffuse);
  meshMat.textures.push_back(
      resourceManager->getTexture("./texture/container.jpg"));
  wallMath.textures.push_back(
      resourceManager->getTexture("./texture/wall.jpg"));

  resourceManager->addMaterial(mat);
  resourceManager->addMaterial(meshMat);
  resourceManager->addMaterial(wallMath);

  MeshRenderer render;

  Light light = Light();
  light.Position = glm::vec3(0.0f, 0.2f, 1.0f);
  light.Ambiant = glm::vec3(1.0f, 1.0f, 1.0f);
  light.Diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  light.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
  light.Direction = glm::vec3(0.0f, 0.0f, 1.0f);
  /* 
  auto floor = modelLoader.loadModel("./texture/untitled.glb");
  for (auto &mesh : floor.meshes)
  {
    renderer->createBuffer(mesh);
  } 
  auto floorTransform = glm::mat4(1.0f);
  floorTransform = glm::translate(floorTransform, glm::vec3(0.0f, 0.0f, 0.0f));
  */
  auto container = createCube();
  renderer->createBuffer(container);

  auto skyBoxMesh = createSkybox();

  renderer->createBuffer(skyBoxMesh);

  camera.Position = glm::vec3(0.0f, 1.0f, 5.0f);
  while (!glfwWindowShouldClose(window))
  {
    double currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    inputHandler.update(deltaTime);
    // input
    // -----
    processInput(window);
    // render
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render.useLight(*meshShader, light, camera);

    render.render(camera, container, *meshShader, mat, glm::mat4(1.0));
    render.renderSkyBox(camera, skyBoxMesh, *skyBoxShader, skyBoxTexture);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}