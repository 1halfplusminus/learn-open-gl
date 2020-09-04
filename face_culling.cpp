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

  // INIT OPEN GL
  glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
  glEnable(GL_BLEND);
  glEnable(GL_CULL_FACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glCullFace(GL_BACK);
  /*  glEnable(GL_DEPTH_TEST); */
  /*   glDepthFunc(GL_ALWAYS); */
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
  SpriteRenderer spriteRenderer = SpriteRenderer();

  std::shared_ptr<Shader> shader(
      new Shader("./shader/vSprite.glsl", "./shader/fSprite.glsl"));

  std::vector<Image> images = {
      Image("./texture/blending_transparent_window.png", true)};

  Material mat = Material(shader);

  for (auto image : images)
  {
    // load image
    Texture texture = resourceManager->loadTexture2D(image);
    texture.type = TextureType::Diffuse;
    mat.textures.push_back(texture);
  }

  resourceManager->addMaterial(mat);

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

  std::vector<glm::mat4> vegetation;
  vegetation.push_back(
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)));
  vegetation.push_back(
      glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, -0.48f)));
  vegetation.push_back(
      glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, 0.51f)));
  vegetation.push_back(
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.7f)));
  vegetation.push_back(
      glm::translate(glm::mat4(1.0f), glm::vec3(-0.3f, 0.0f, -2.3f)));
  vegetation.push_back(
      glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 0.0f, -0.6f)));

  std::vector<Sprite> sprites((int)vegetation.size());

  while (!glfwWindowShouldClose(window))
  {
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

    camera.Position = glm::vec3(0.2f, 0.0f, 3.0f);
    camera.Target = glm::vec3(0.0f, 0.0f, 0.0f);
    /*
        render.useLight(*shader, light, camera); */
    spriteRenderer.render(camera, sprites, 0, resourceManager->getMaterials(),
                          vegetation);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}