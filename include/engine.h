// clang-format off
#include <glad/glad.h>
// clang-format on
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <numeric>
#include <ostream>
#include <shader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <functional>

struct Light
{
  glm::vec3 Direction;
  glm::vec3 Position;
  glm::vec3 Ambiant;
  glm::vec3 Diffuse;
  glm::vec3 Specular;

  float OuterCutOff = 17.5;
  float CutOff = 12.5f;
  float Constant = 1.0f;
  float Linear = 0.09f;
  float Quadratic = 0.032f;
};
struct Vertex
{
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
  Vertex() {}
  Vertex(glm::vec3 position, glm::vec2 texCoords, glm::vec3 normal)
  {
    Position = position;
    TexCoords = texCoords;
    Normal = normal;
  }
};
enum TextureType
{
  Diffuse,
  Specular
};
struct Texture
{
  unsigned int id;
  TextureType type = TextureType::Diffuse;
};
struct Mesh
{
public:
  // mesh data
  std::vector<Vertex> Vertices;
  std::vector<unsigned int> Indices;
  unsigned int Id;
  int MaterialID;
  Mesh() {}
  Mesh(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices)
  {
    Vertices = vertices;
    Indices = indices;
  }
};
struct Model
{
  std::vector<Mesh> meshes;
};
struct OpenGLVAO
{
public:
  unsigned int VBO, EBO, VAO;
};

struct Material
{
public:
  float shininess = 32.0f;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  glm::vec3 color;
  std::shared_ptr<Shader> shader;
  std::vector<Texture> textures;
  Material(const std::shared_ptr<Shader> shader) { this->shader = shader; }
  Material(){};
};
struct Image
{
public:
  int width, height, nrChannels;
  unsigned char *data = nullptr;
  bool flipVertically;
  std::string path;
  Image() {}
  Image(std::string path, bool flipVertically = false)
  {
    this->path = path;
    this->flipVertically = flipVertically;
  }
};
struct Camera
{
public:
  static constexpr float YAW = -90.0f;
  static constexpr float PITCH = 0.0f;

  glm::vec3 Target;
  glm::vec3 Front;
  glm::vec3 Position;
  glm::vec3 Up;
  glm::vec3 Right;
  glm::vec3 WorldUp;

  glm::mat4 Projection;
  // euler Angles
  double Yaw;
  double Pitch;

  Camera(glm::mat4 projection, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f))
  {
    this->Projection = projection;
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
  }
  glm::mat4 calculateViewMatrix()
  {
    return glm::lookAt(Position, Position + Front, Up);
  }
  // calculates the front vector from the Camera's (updated) Euler Angles
  void updateCameraVectors()
  {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp)); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up = glm::normalize(glm::cross(Right, Front));
  }
};
enum Camera_Movement
{
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT
};

class OrbitCamera
{
public:
  static constexpr float SPEED = 2.5f;
  static constexpr float SENSITIVITY = 0.1f;
  static constexpr float ZOOM = 45.0f;
  Camera &ControlledCamera;
  // camera options
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;
  OrbitCamera(Camera &camera) : ControlledCamera(camera), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
  {
  }
  // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
  void ProcessKeyboard(Camera_Movement direction, double deltaTime)
  {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
      ControlledCamera.Position += ControlledCamera.Front * velocity;
    if (direction == BACKWARD)
      ControlledCamera.Position -= ControlledCamera.Front * velocity;
    if (direction == LEFT)
      ControlledCamera.Position -= ControlledCamera.Right * velocity;
    if (direction == RIGHT)
      ControlledCamera.Position += ControlledCamera.Right * velocity;
  }
  // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
  void ProcessMouseMovement(double xoffset, double yoffset, GLboolean constrainPitch = true)
  {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    ControlledCamera.Yaw += xoffset;
    ControlledCamera.Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
      if (ControlledCamera.Pitch > 89.0f)
        ControlledCamera.Pitch = 89.0f;
      if (ControlledCamera.Pitch < -89.0f)
        ControlledCamera.Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    ControlledCamera.updateCameraVectors();
  }

  // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
  void ProcessMouseScroll(double yoffset)
  {
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
      Zoom = 1.0f;
    if (Zoom > 45.0f)
      Zoom = 45.0f;
  }
};
class GLFWInputHandler
{
public:
  GLFWInputHandler(GLFWwindow &pWindow, OrbitCamera &pcamera) : window(pWindow), orbitCamera(pcamera)
  {
    int wWidght;
    int wHeight;
    glfwGetWindowSize(&pWindow, &wWidght, &wHeight);
    lastX = wWidght / 2;
    lastY = wHeight / 2;
  }
  void update(double deltaTime)
  {
    double xpos, ypos;
    glfwGetCursorPos(&window, &xpos, &ypos);
    mouse_callback(&window, xpos, ypos);
    processInput(&window, deltaTime);
  }
  void mouse_callback(GLFWwindow *window, double xpos, double ypos)
  {
    if (firstMouse)
    {
      lastX = xpos;
      lastY = ypos;
      firstMouse = false;
    }

    double xoffset = xpos - lastX;
    double yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    orbitCamera.ProcessMouseMovement(xoffset, yoffset, true);
  }
  // glfw: whenever the mouse scroll wheel scrolls, this callback is called
  // ----------------------------------------------------------------------
  void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
  {
    orbitCamera.ProcessMouseScroll(yoffset);
  }
  // process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
  // ---------------------------------------------------------------------------------------------------------
  void processInput(GLFWwindow *window, double deltaTime)
  {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      orbitCamera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      orbitCamera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      orbitCamera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      orbitCamera.ProcessKeyboard(RIGHT, deltaTime);
  }

private:
  OrbitCamera &orbitCamera;
  GLFWwindow &window;
  double lastX = 0;
  double lastY = 0;
  bool firstMouse = true;
};
class Renderer
{
public:
  unsigned int createBuffer(Mesh &mesh)
  {
    OpenGLVAO openGlMesh;
    glGenVertexArrays(1, &openGlMesh.VAO);
    glGenBuffers(1, &openGlMesh.VBO);
    glGenBuffers(1, &openGlMesh.EBO);

    glBindVertexArray(openGlMesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, openGlMesh.VBO);

    glBufferData(GL_ARRAY_BUFFER, mesh.Vertices.size() * sizeof(Vertex),
                 mesh.Vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, openGlMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mesh.Indices.size() * sizeof(unsigned int),
                 mesh.Indices.data(), GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Position));
    // vertex normals
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    // vertex texture coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);

    mesh.Id = openGlMesh.VAO;

    return openGlMesh.VAO;
  }
  unsigned int createTexture2D(const aiTexture *aiTexture)
  {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture); // Binding of texture name
    //
    // redefine standard texture values
    //
    // We will use linear interpolation for magnification filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // tiling mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    (aiTexture->achFormatHint[0] & 0x01) ? GL_REPEAT
                                                         : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    (aiTexture->achFormatHint[0] & 0x01) ? GL_REPEAT
                                                         : GL_CLAMP);
    // Texture specification
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 500, 500, 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, (void *)aiTexture->pcData);
    return texture;
  }
  unsigned int createTextureCube(const std::vector<Image> &images)
  {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    for (unsigned int i = 0; i < images.size(); i++)
    {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, images[i].width, images[i].height, 0, GL_RGB, GL_UNSIGNED_BYTE, images[i].data);
    }

    return textureID;
  }
  unsigned int createTexture2D(const Image &image)
  {
    unsigned int texture;
    glGenTextures(1, &texture);
    if (image.data)
    {
      GLenum format;
      if (image.nrChannels == 1)
        format = GL_RED;
      else if (image.nrChannels == 3)
        format = GL_RGB;
      else if (image.nrChannels == 4)
        format = GL_RGBA;
      else
        format = GL_RGBA8;
      std::cout << "Image format: " << image.nrChannels << "\n";
      // bind texture
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0,
                   format, GL_UNSIGNED_BYTE, image.data);

      // texture wrapping
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
      // texture filtering
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // texture mipmap
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glGenerateMipmap(GL_TEXTURE_2D);
    }

    return texture;
  }
};

Mesh createPlane()
{
  const int rowSize = 5;
  Mesh plane;
  float normals[] = {0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
                     0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f};
  std::vector<float> vertices = {
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
      0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f};
  int nbVertice = (int)vertices.size() / rowSize;
  plane.Vertices = std::vector<Vertex>(nbVertice);
  int j = 0;
  for (int i = 0; i < vertices.size(); i = i + rowSize)
  {
    int indice = i / rowSize;
    plane.Vertices[indice] =
        Vertex(glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]),
               glm::vec2(vertices[i + 3], vertices[i + 4]),
               glm::vec3(normals[j], normals[j + 1], normals[j + 2]));
    j = j + 3;
  }
  std::vector<unsigned int> indices(nbVertice);
  std::iota(indices.begin(), indices.end(), 0);
  plane.Indices = indices;

  return plane;
}
Mesh createSkybox()
{
  auto skyBoxMesh = Mesh();
  std::vector<float> skyboxVertices = {
      // positions
      -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,

      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f, -1.0f,
      1.0f, 1.0f, -1.0f,
      1.0f, 1.0f, 1.0f,
      1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f,

      -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, 1.0f};
  skyBoxMesh.Vertices = std::vector<Vertex>((int)skyboxVertices.size() / 3);
  for (int i = 0; i < skyboxVertices.size(); i += 3)
  {
    Vertex vertex;
    vertex.Position = glm::vec3(skyboxVertices[i], skyboxVertices[i + 1], skyboxVertices[i + 2]);
    skyBoxMesh.Vertices.push_back(vertex);
  }
  return skyBoxMesh;
}
Mesh createCube()
{
  const int rowSize = 5;
  Mesh cube;
  float normals[] = {
      0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
      0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,

      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

      -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
      -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,

      1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

      0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,

      0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
  std::vector<float> vertices = {
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
      0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
      0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,

      -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

      0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
      0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
      0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
      -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

      -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
      0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
      -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f};

  int nbVertice = (int)vertices.size() / rowSize;
  cube.Vertices = std::vector<Vertex>(nbVertice);
  int j = 0;
  for (int i = 0; i < vertices.size(); i = i + rowSize)
  {
    int indice = i / rowSize;
    cube.Vertices[indice] =
        Vertex(glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]),
               glm::vec2(vertices[i + 3], vertices[i + 4]),
               glm::vec3(normals[j], normals[j + 1], normals[j + 2]));
    j = j + 3;
  }
  std::vector<unsigned int> indices(nbVertice);
  std::iota(indices.begin(), indices.end(), 0);
  cube.Indices = indices;

  return cube;
}

struct Sprite
{
public:
  glm::vec<3, int> Position;
  Mesh mesh;
  Sprite() { mesh = createPlane(); }
};
struct AnimatedSprite
{
  std::vector<std::pair<int, int>> Sprites;
  int NumFrame;
  int CurrentFrame = 0;
  double CurrentTime = 0;
  int SpriteSheetId;
};
struct SpriteSheet
{
public:
  SpriteSheet(int row, int col) { nbFrame = glm::vec<3, int>(row, col, 0); }
  std::vector<Sprite> &getSprites() { return sprites; }
  static SpriteSheet fixed_size(int row, int col)
  {
    SpriteSheet sheet(row, col);
    int frameX = 0;
    int frameY = 0;
    for (int frameY = 0; frameY < col; ++frameY)
    {
      for (int frameX = 0; frameX < row; ++frameX)
      {
        Sprite sprite;
        for (auto &vertice : sprite.mesh.Vertices)
        {
          float xPos = static_cast<float>(frameX) / row;
          float yPos = static_cast<float>(frameY) / col;
          vertice.TexCoords.x = (vertice.TexCoords.x / row) + xPos;
          vertice.TexCoords.y = (vertice.TexCoords.y / col) + yPos;
        }
        sheet.sprites.push_back(sprite);
      }
    }

    return sheet;
  }
  const Sprite &operator[](const std::pair<int, int> &Index)
  {
    int index = nbFrame.x * ((nbFrame.y - 1) - Index.second) + Index.first;
    return sprites[index];
  }
  friend SpriteSheet &operator+=(SpriteSheet &sheet, const Sprite &sprite)
  {
    sheet.sprites.push_back(sprite);
    return sheet;
  }

private:
  glm::vec<3, int> nbFrame;
  std::vector<Sprite> sprites;
};

class SpriteRenderer
{
private:
  std::unordered_map<int, unsigned int> vaos;
  std::unordered_map<int, std::pair<unsigned, unsigned>> vbos;

public:
  void render(Camera &camera, const std::vector<Sprite> &sprites, int matID,
              const std::vector<Material> &materials,
              const std::vector<glm::mat4> &transforms)
  {
    Mesh mesh;
    std::vector<glm::mat4> verticeTransforms(transforms.size() * 6);
    mesh.Vertices = std::vector<Vertex>(sprites.size() * 6);
    int nbSprite = 0;
    int nbVertice = 0;
    for (auto sprite : sprites)
    {
      for (auto vertice : sprite.mesh.Vertices)
      {
        verticeTransforms[nbVertice] = transforms[nbSprite];
        mesh.Vertices[nbVertice] = vertice;
        ++nbVertice;
      }
      ++nbSprite;
    }
    if (vaos.find(matID) == vaos.end())
    {
      glGenVertexArrays(1, &vaos[matID]);
      glBindVertexArray(vaos[matID]);

      vbos[matID] = std::make_pair<int, int>(0, 0);

      glGenBuffers(1, &vbos[matID].first);

      glBindBuffer(GL_ARRAY_BUFFER, vbos[matID].first);

      glBufferData(GL_ARRAY_BUFFER, mesh.Vertices.size() * sizeof(Vertex),
                   mesh.Vertices.data(), GL_DYNAMIC_DRAW);

      // vertex positions
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                            (void *)offsetof(Vertex, Position));
      glEnableVertexAttribArray(0);
      // vertex normals
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                            (void *)offsetof(Vertex, Normal));
      // vertex texture coords
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                            (void *)offsetof(Vertex, TexCoords));
      glEnableVertexAttribArray(1);

      glGenBuffers(1, &vbos[matID].second);

      glBindBuffer(GL_ARRAY_BUFFER, vbos[matID].second);
      glBufferData(GL_ARRAY_BUFFER,
                   sizeof(glm::mat4) * verticeTransforms.size(),
                   verticeTransforms.data(), GL_STREAM_DRAW);

      for (unsigned int i = 0; i < 4; i++)
      {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                              (const GLvoid *)(sizeof(GLfloat) * i * 4));
      }
    }
    else
    {
      glBindBuffer(GL_ARRAY_BUFFER, vbos[matID].first);
      glBufferData(GL_ARRAY_BUFFER, mesh.Vertices.size() * sizeof(Vertex),
                   mesh.Vertices.data(), GL_DYNAMIC_DRAW);

      glBindBuffer(GL_ARRAY_BUFFER, vbos[matID].second);
      glBufferData(GL_ARRAY_BUFFER,
                   sizeof(glm::mat4) * verticeTransforms.size(),
                   verticeTransforms.data(), GL_STREAM_DRAW);
    }
    Material mat = materials[matID];
    mat.shader->use();
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int i = 0;
    for (auto text : mat.textures)
    {
      glActiveTexture(GL_TEXTURE0 + i);
      int uniformLocation;
      unsigned int number;
      switch (text.type)
      {
      case TextureType::Diffuse:
        number = diffuseNr++;
        uniformLocation = glGetUniformLocation(
            mat.shader->ID, "material.texture_diffuse" + number);
        break;
      case TextureType::Specular:
        number = specularNr++;
        uniformLocation = glGetUniformLocation(
            mat.shader->ID, "material.texture_specular" + number);
      default:
        break;
      }
      glUniform1i(uniformLocation, i);
      glBindTexture(GL_TEXTURE_2D, text.id);
      ++i;
    }

    int shininess = glGetUniformLocation(mat.shader->ID, "material.shininess");
    glUniform1f(shininess, mat.shininess);

    // coordinate system
    int viewPos = glGetUniformLocation(mat.shader->ID, "viewPos");
    glUniform3fv(viewPos, 1, glm::value_ptr(camera.Position));

    int viewLoc = glGetUniformLocation(mat.shader->ID, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE,
                       glm::value_ptr(camera.calculateViewMatrix()));

    int projectionLoc = glGetUniformLocation(mat.shader->ID, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE,
                       glm::value_ptr(camera.Projection));

    glBindVertexArray(vaos[matID]);
    glDrawArrays(GL_TRIANGLES, 0, (int)mesh.Vertices.size());
    glBindVertexArray(0);
  }
};

class ResourceManager
{
public:
  ResourceManager(Renderer &rendeder) : rendeder(rendeder)
  {
    textures = std::make_unique<std::unordered_map<std::string, Texture>>();
    materials = std::make_unique<std::vector<Material>>();
  }
  int addMaterial(Material material)
  {
    materials->push_back(material);
    size_t pos = materials->size() - 1;
    return (int)pos;
  }
  Material getMaterial(int id)
  {
    if (materials->size() < id)
    {
      std::cout << "No material with id " << id << "\n";
      return Material();
    }
    return materials->at(id);
  }

  const std::vector<Material> getMaterials() { return *materials; }

  const Texture loadTextureCube(std::vector<Image> &images)
  {
    for (auto &image : images)
    {
      loadImage(image);
    }

    Texture texture;
    texture.id = rendeder.createTextureCube(images);
    texture.type = TextureType::Diffuse;
    std::cout << "Cube map texture " << texture.id << " loaded \n";
    return texture;
  }
  const Texture loadTexture2D(Image &image)
  {
    if (textures->find(image.path) == textures->end())
    {
      loadImage(image);
      Texture texture;
      texture.id = rendeder.createTexture2D(image);
      texture.type = TextureType::Diffuse;
      textures->insert({image.path, texture});
      std::cout << "Texture " << texture.id << " loaded \n";
      stbi_image_free(image.data);
      return texture;
    }
    return getTexture(image.path);
  }
  const Texture loadTexture2D(const aiTexture *aiTexture)
  {
    if (textures->find(aiTexture->mFilename.C_Str()) == textures->end())
    {
      Texture texture;
      texture.id = rendeder.createTexture2D(aiTexture);
      textures->insert({aiTexture->mFilename.C_Str(), texture});
      std::cout << "Texture " << texture.id << " loaded \n";
      return texture;
    }
    return getTexture(aiTexture->mFilename.C_Str());
  }
  const Texture getTexture(const std::string &path)
  {
    if (textures->find(path) == textures->end())
    {
      std::cout << "No texture for " << path << "\n";
      return Texture();
    }
    return textures->at(path);
  }

private:
  void loadImage(Image &image)
  {
    if (image.data == nullptr)
    {
      stbi_set_flip_vertically_on_load(image.flipVertically);
      image.data = stbi_load(image.path.c_str(), &image.width, &image.height,
                             &image.nrChannels, 0);
      if (!image.data)
      {
        std::cout << "Cubemap texture failed to load at path: " << image.path.c_str() << std::endl;
        stbi_image_free(image.data);
      }
      /*       stbi_image_free(image.data); */
    }
  }
  std::unique_ptr<std::unordered_map<std::string, Texture>> textures;
  std::unique_ptr<std::vector<Material>> materials;
  Renderer &rendeder;
};
class ModelLoader
{
public:
  ModelLoader(ResourceManager &rManager) : resourceManager(rManager) {}
  Model loadModel(std::string path)
  {
    Model model;
    Assimp::Importer import;

    const aiScene *scene =
        import.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals);
    if (!scene || scene->mFlags && AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode)
    {
      std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
      return model;
    }

    std::cout << "Loaded " << scene->mRootNode->mName.C_Str() << "\n";

    directory = path.substr(0, path.find_last_of("/"));

    processNode(model, scene->mRootNode, scene);

    return model;
  }

private:
  void processNode(Model &model, aiNode *node, const aiScene *scene)
  {
    // process all the node's meshes
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      std::cout << "Processing mesh: " << mesh->mName.C_Str() << "\n";
      model.meshes.push_back(processMesh(mesh, scene));
    }
    // process each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
      processNode(model, node->mChildren[i], scene);
    }
  }
  Mesh processMesh(aiMesh *mesh, const aiScene *scene)
  {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    Material mat;
    int matId = -1;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
      Vertex vertex;
      glm::vec3 vector;
      vector.x = mesh->mVertices[i].x;
      vector.y = mesh->mVertices[i].y;
      vector.z = mesh->mVertices[i].z;
      vertex.Position = vector;

      vector.x = mesh->mNormals[i].x;
      vector.y = mesh->mNormals[i].y;
      vector.z = mesh->mNormals[i].z;
      vertex.Normal = vector;
      if (mesh->mTextureCoords[0])
      {
        glm::vec2 vec;
        vec.x = mesh->mTextureCoords[0][i].x;
        vec.y = mesh->mTextureCoords[0][i].y;
        /*  std::cout << "u: " << vec.x << " v: " << vec.y << "\n"; */
        vertex.TexCoords = vec;
      }
      else
      {
        /* std::cout << "no texture coords \n"; */
        vertex.TexCoords = glm::vec2(0.0f, 0.0f);
      }
      /*     std::cout << "x: " << vertex.Position.x << " y: " << vertex.Position.y
                << " z: " << vertex.Position.z << "\n"; */
      vertices.push_back(vertex);
    }
    std::cout << mesh->mNumFaces << " face \n";
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
      aiFace face = mesh->mFaces[i];
      /*    std::cout << face.mNumIndices << " indices \n"; */
      for (unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }
    if (mesh->mMaterialIndex >= 0)
    {

      aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
      // load diffuse texture
      std::vector<Texture> diffuseMaps = loadMaterialTextures(
          scene, material, aiTextureType_DIFFUSE, TextureType::Diffuse);
      textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
      // load spec
      std::vector<Texture> specularMaps = loadMaterialTextures(
          scene, material, aiTextureType_SPECULAR, TextureType::Specular);
      textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
      mat.textures = textures;
      matId = resourceManager.addMaterial(mat);
    }
    Mesh mMesh = Mesh(vertices, indices);
    if (matId != -1)
    {
      mMesh.MaterialID = matId;
    }
    return mMesh;
  }
  std::vector<Texture> loadMaterialTextures(const aiScene *scene,
                                            aiMaterial *mat, aiTextureType type,
                                            TextureType textureType)
  {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
      aiString str;
      mat->GetTexture(type, i, &str);
      if (auto embeddedTexture = scene->GetEmbeddedTexture(str.C_Str()))
      {
        // returned pointer is not null, read texture from memory
        /*    std::cout << "embedded texture " << embeddedTexture->mFilename.C_Str()
                  << " width : " << embeddedTexture->mWidth << "\n"
                  << " height : " << embeddedTexture->mHeight << "\n"
                  << " hint : " << embeddedTexture->achFormatHint << "\n"; */
        unsigned char *img;
        int width, height, type;
        unsigned char *res_data = (unsigned char *)embeddedTexture->pcData;
        img = stbi_load_from_memory(res_data, (int)embeddedTexture->mWidth,
                                    &width, &height, &type, 0);
        Image image;
        image.data = img;
        image.height = height;
        image.width = width;
        image.path = embeddedTexture->mFilename.C_Str();
        image.nrChannels = type;
        Texture texture = resourceManager.loadTexture2D(image);
        texture.type = textureType;
        textures.push_back(texture);
      }
      else
      {
        // regular file, check if it exists and read it
        std::cout << "reading file " << str.C_Str() << "\n";
        Image image(directory + "/" + str.C_Str(), true);
        Texture texture = resourceManager.loadTexture2D(image);
        texture.type = textureType;
        textures.push_back(texture);
      }
    }
    return textures;
  }
  std::string directory;
  ResourceManager &resourceManager;
};
class MeshRenderer
{
public:
  MeshRenderer() {}
  void addSpotLights(Shader &shader, const std::vector<Light> &lights,
                     Camera &camera)
  {
    shader.use();
    int nbSpotLight = 0;
    std::string uniformName("spotLights");
    for (auto light : lights)
    {
      addLight(shader, light, camera, uniformName, nbSpotLight);
      shader.setFloat(getUniformName(uniformName, "quadratic", nbSpotLight),
                      light.Quadratic);
      shader.setFloat(getUniformName(uniformName, "cutOff", nbSpotLight),
                      glm::cos(glm::radians(light.CutOff)));
      shader.setFloat(getUniformName(uniformName, "outerCutOff", nbSpotLight),
                      glm::cos(glm::radians(light.OuterCutOff)));
      ++nbSpotLight;
    }

    shader.setInt("nbSpotLight", nbSpotLight);
  }
  void addPointLights(Shader &shader, const std::vector<Light> &lights,
                      Camera &camera)
  {
    shader.use();
    int nbPointLight = 0;
    std::string uniformName("pointLights");
    for (auto light : lights)
    {
      addLight(shader, light, camera, uniformName, nbPointLight);
      shader.setFloat(getUniformName(uniformName, "constant", nbPointLight),
                      light.Constant);
      shader.setFloat(getUniformName(uniformName, "linear", nbPointLight),
                      light.Linear);
      shader.setFloat(getUniformName(uniformName, "quadratic", nbPointLight),
                      light.Quadratic);
      ++nbPointLight;
    }

    shader.setInt("nbPointLight", nbPointLight);
  }
  void useLight(Shader &shader, const Light &light, Camera &camera)
  {
    shader.use();
    glm::vec3 position =
        camera.calculateViewMatrix() * glm::vec4(light.Position, 1.0f);

    int ligthPosition = glGetUniformLocation(shader.ID, "light.position");
    glUniform3fv(ligthPosition, 1, glm::value_ptr(position));

    int lightAmbient = glGetUniformLocation(shader.ID, "light.ambient");
    glUniform3fv(lightAmbient, 1, glm::value_ptr(light.Ambiant));

    int lightDiffuse = glGetUniformLocation(shader.ID, "light.diffuse");
    glUniform3fv(lightDiffuse, 1, glm::value_ptr(light.Diffuse));

    int lightSpecular = glGetUniformLocation(shader.ID, "light.specular");
    glUniform3fv(lightSpecular, 1, glm::value_ptr(light.Specular));

    int lightDirection = glGetUniformLocation(shader.ID, "light.direction");
    glUniform3fv(lightDirection, 1, glm::value_ptr(light.Direction));
  }
  void renderSkyBox(Camera &camera, Mesh &mesh, Shader &shader, const Texture &texture)
  {
    glDepthFunc(GL_LEQUAL);
    shader.use();
    glm::mat4 view = glm::mat4(glm::mat3(camera.calculateViewMatrix()));

    int viewLoc = glGetUniformLocation(shader.ID, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE,
                       glm::value_ptr(view));
    int projectionLoc = glGetUniformLocation(shader.ID, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE,
                       glm::value_ptr(camera.Projection));

    auto uniformLocation = glGetUniformLocation(
        shader.ID, "skybox");
    glUniform1i(uniformLocation, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture.id);

    glBindVertexArray(mesh.Id);
    glDrawArrays(GL_TRIANGLES, 0, mesh.Vertices.size());
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
  }
  void render(Camera &camera, Model &model, Shader &shader,
              ResourceManager &resourceManager, glm::mat4 transform)
  {
    for (auto &mesh : model.meshes)
    {
      Material mat = resourceManager.getMaterial(mesh.MaterialID);
      render(camera, mesh, shader, mat, transform);
    }
  }
  void render(Camera &camera, const Mesh &mesh, Shader &shader, Material &mat,
              glm::mat4 transform)
  {
    shader.use();
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int i = 0;
    for (auto text : mat.textures)
    {
      glActiveTexture(GL_TEXTURE0 + i);
      int uniformLocation;
      unsigned int number;
      switch (text.type)
      {
      case TextureType::Diffuse:
        number = diffuseNr++;
        uniformLocation = glGetUniformLocation(
            shader.ID, "material.texture_diffuse" + number);
        break;
      case TextureType::Specular:
        number = specularNr++;
        uniformLocation = glGetUniformLocation(
            shader.ID, "material.texture_specular" + number);
      default:
        break;
      }
      glUniform1i(uniformLocation, i);
      glBindTexture(GL_TEXTURE_2D, text.id);
      ++i;
    }
    int shininess = glGetUniformLocation(shader.ID, "material.shininess");
    glUniform1f(shininess, mat.shininess);

    // coordinate system
    int viewPos = glGetUniformLocation(shader.ID, "viewPos");
    glUniform3fv(viewPos, 1, glm::value_ptr(camera.Position));

    int viewLoc = glGetUniformLocation(shader.ID, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE,
                       glm::value_ptr(camera.calculateViewMatrix()));

    int projectionLoc = glGetUniformLocation(shader.ID, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE,
                       glm::value_ptr(camera.Projection));

    int modelLoc = glGetUniformLocation(shader.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));

    glBindVertexArray(mesh.Id);
    if (mesh.Indices.size() > 0)
    {
      glDrawElements(GL_TRIANGLES, (int)mesh.Indices.size(), GL_UNSIGNED_INT,
                     0);
    }
    else
    {
      glDrawArrays(GL_TRIANGLES, 0, (int)mesh.Vertices.size());
    }
  }
  void render(Camera &camera, const Mesh &mesh, Material &mat,
              glm::mat4 transform)
  {
    render(camera, mesh, *mat.shader, mat, transform);
  }

private:
  void addLight(const Shader &shader, const Light &light, Camera &camera,
                const std::string &name, int index)
  {
    glm::vec3 position =
        camera.calculateViewMatrix() * glm::vec4(light.Position, 1.0f);
    int ligthPosition = glGetUniformLocation(
        shader.ID, getUniformName(name, "position", index).c_str());
    glUniform3fv(ligthPosition, 1, glm::value_ptr(position));

    int lightAmbient = glGetUniformLocation(
        shader.ID, getUniformName(name, "ambient", index).c_str());
    glUniform3fv(lightAmbient, 1, glm::value_ptr(light.Ambiant));

    int lightDiffuse = glGetUniformLocation(
        shader.ID, getUniformName(name, "diffuse", index).c_str());
    glUniform3fv(lightDiffuse, 1, glm::value_ptr(light.Diffuse));

    int lightSpecular = glGetUniformLocation(
        shader.ID, getUniformName(name, "specular", index).c_str());
    glUniform3fv(lightSpecular, 1, glm::value_ptr(light.Specular));

    int lightDirection = glGetUniformLocation(
        shader.ID, getUniformName(name, "direction", index).c_str());
    glUniform3fv(lightDirection, 1, glm::value_ptr(light.Direction));
  }
  const std::string getUniformName(const std::string &name,
                                   const std::string &attribute, int index)
  {
    return name + "[" + std::to_string(index) + "]." + attribute;
  }
};