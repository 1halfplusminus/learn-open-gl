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

float deltaTime = 0.0f; // Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

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
  glEnable(GL_DEPTH_TEST);
  // init texture
  // texture wrapping
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  // texture filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // texture mipmap
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto renderer = std::make_unique<Renderer>();
  auto resourceManager = std::make_unique<ResourceManager>(*renderer);
  ModelLoader modelLoader = ModelLoader(*resourceManager);

  Model backpackModel =
      modelLoader.loadModel("./texture/backpack/backpack.obj");
  for (auto &mesh : backpackModel.meshes) {
    renderer->createBuffer(mesh);
  }
  std::shared_ptr<Shader> lightShader(
      new Shader("./shader/vLight.glsl", "./shader/fModel.glsl"));
  std::shared_ptr<Shader> dLightShader(
      new Shader("./shader/vLight.glsl", "./shader/fWhite.glsl"));

  std::vector<Image> images = {
      Image("./texture/container2.png", false),
      Image("./texture/container2_specular.png", false),
  };

  Material mat = Material(lightShader);
  Material lMat = Material(dLightShader);
  resourceManager->addMaterial(mat);
  for (auto image : images) {
    // load image
    Texture texture = resourceManager->loadTexture(image);
    texture.type = TextureType::Diffuse;
    mat.textures.push_back(texture);
  }

  Mesh mesh = createCube();
  renderer->createBuffer(mesh);

  MeshRenderer render;
  // projection
  glm::mat4 projection;
  projection = glm::perspective(
      glm::radians(80.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
  auto camera = Camera(projection);

  Light light = Light();
  light.Position = glm::vec3(0.0f, 0.2f, 1.0f);
  light.Ambiant = glm::vec3(0.2f, 0.2f, 0.2f);
  light.Diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  light.Specular = glm::vec3(1.0f, 1.0f, 1.0f);

  glm::vec3 cubePositions[] = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};
  glm::vec3 pointLightPositions[] = {
      glm::vec3(0.7f, 0.2f, 2.0f),
      glm::vec3(2.3f, -3.3f, -4.0f),
      glm::vec3(-4.0f, 2.0f, -12.0f),
      glm::vec3(0.0f, 0.0f, 5.0f),
  };
  std::vector<Light> pointLights(pointLightPositions->length());
  for (auto pointLightPosition : pointLightPositions) {
    Light pLight;
    pLight.Position = glm::vec3(0.0f, 0.2f, 1.0f);
    pLight.Ambiant = glm::vec3(0.1f, 0.1f, 0.1f);
    pLight.Diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    pLight.Specular = glm::vec3(1.0f, 1.0f, 1.0f);
    pLight.Position = pointLightPosition;
    pLight.Direction = glm::vec3(0.0f, 0.0f, -1.0f);
    pointLights.push_back(pLight);
  }

  while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    // input
    // -----
    processInput(window);
    // render
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    camera.Position = glm::vec3(0.2f, 0.0f, 3.0f);
    camera.Target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraFront = glm::normalize(camera.Target - camera.Position);
    light.Direction = glm::vec3(cameraFront);
    light.Position = camera.Position;
    /*         render.useLight(*lightShader, light, camera); */
    render.addSpotLights(*lightShader, pointLights, camera);
    for (unsigned int i = 0; i < 2; i++) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      float angle = 20.0f * i;
      model =
          glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      model = glm::scale(model, glm::vec3(0.8f));
      render.render(camera, mesh, mat, model);
    }
    for (auto pointLightPosition : pointLightPositions) {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, pointLightPosition);
      model = glm::scale(model, glm::vec3(0.2f));
      render.render(camera, mesh, lMat, model);
    }
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f));
    render.render(camera, backpackModel, *lightShader, *resourceManager, model);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}