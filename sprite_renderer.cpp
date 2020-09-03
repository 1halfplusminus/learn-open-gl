// clang-format off
#include <glad/glad.h>
// clang-format on
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <unordered_map>
#include <ostream>
#include <shader.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "engine.h"
// settings
const unsigned int SCR_WIDTH = 1440;
const unsigned int SCR_HEIGHT = 900;

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
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

  Renderer manager;
  std::shared_ptr<Shader> lightShader(
      new Shader("./shader/vSprite.glsl", "./shader/fSprite.glsl"));
  std::vector<Image> images = {
      Image("./texture/atlas.png", true),
  };
  std::vector<Material> materials = {Material(lightShader)};

  for (auto image : images) {
    // load image
    stbi_set_flip_vertically_on_load(image.flipVertically);
    image.data = stbi_load(image.path.c_str(), &image.width, &image.height,
                           &image.nrChannels, 0);
    Texture texture;
    texture.id = manager.createTexture2D(image);
    texture.type = TextureType::Diffuse;
    materials[0].textures.push_back(texture);
    stbi_image_free(image.data);
  }
  MeshRenderer render;
  // projection
  glm::mat4 projection;
  projection = glm::perspective(
      glm::radians(80.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
  auto camera = Camera(projection);

  int frameX = 0;
  int frameY = 0;
  int nbFrameX = 12;
  int nbFrameY = 12;

  SpriteSheet spritesheet = SpriteSheet::fixed_size(12, 8);
  for (Sprite &sprite : spritesheet.getSprites()) {
    manager.createBuffer(sprite.mesh);
  }
  int nbFrames = 0;
  const int nbTestSprite = 10000;
  SpriteRenderer spriterender;
  std::vector<glm::mat4> transforms;
  AnimatedSprite animated;
  animated.Sprites.push_back({0, 0});
  animated.Sprites.push_back({0, 1});
  animated.Sprites.push_back({0, 2});
  std::vector<Sprite> currentSprite(nbTestSprite);
  for (int i = 0; i < nbTestSprite; ++i) {
    int gridSize = 20;
    int x = rand() % gridSize - rand() % gridSize;
    int y = rand() % gridSize - rand() % gridSize;
    currentSprite[i] = spritesheet[animated.Sprites[animated.CurrentFrame]];
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    transforms.push_back(model);
  }
  double lastTime = glfwGetTime();
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

    camera.Position = glm::vec3(0.0f, 0.0f, 3.0f);
    camera.Target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 cameraFront = glm::normalize(camera.Target - camera.Position);

    for (int i = 0; i < nbTestSprite; i++) {
      animated.CurrentTime += deltaTime;
      if (animated.CurrentTime >= 2000.0f) {
        animated.CurrentTime = 0;
        ++animated.CurrentFrame;
        if (animated.CurrentFrame >= animated.Sprites.size()) {
          animated.CurrentFrame = 0;
        }
      }
      currentSprite[i] = spritesheet[animated.Sprites[animated.CurrentFrame]];
    }

    spriterender.render(camera, currentSprite, 0, materials, transforms);
    glfwSwapBuffers(window);
    glfwPollEvents();
    if (currentFrame - lastTime >=
        1.0) { // If last prinf() was more than 1 sec ago
      // printf and reset timer
      printf("%f ms/frame\n", 1000.0 / double(nbFrames));
      nbFrames = 0;
      lastTime += 1.0f;
    }
    nbFrames++;
  }
  glfwTerminate();
  return 0;
}
