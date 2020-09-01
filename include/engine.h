// clang-format off
#include <glad/glad.h>
// clang-format on
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <numeric>
#include <ostream>
#include <shader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

struct Mesh
{
public:
    // mesh data
    std::vector<Vertex> Vertices;
    std::vector<unsigned int> Indices;
    unsigned int Id;
    /*   std::vector<Texture> textures; */
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
    std::vector<unsigned int> textures;
    Material(const std::shared_ptr<Shader> shader) { this->shader = shader; }
};
struct Image
{
public:
    int width, height, nrChannels;
    unsigned char *data;
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
    glm::vec3 Up;
    glm::vec3 Position;
    glm::vec3 Target;
    glm::mat4 Projection;
    glm::mat4 Transform;

    Camera(glm::mat4 projection)
    {
        this->Up = glm::vec3(0.0f, 1.0f, 0.0f);
        this->Projection = projection;
        this->Transform = glm::mat4(1.0f);
    }
    glm::mat4 calculateViewMatrix()
    {
        return glm::lookAt(Position, Target, Up) * Transform;
    }
};
class MeshRenderer
{
public:
    MeshRenderer()
    {
    }
    void addPointLights(Shader &shader, const std::vector<Light> &lights, Camera &camera)
    {
        shader.use();
        int nbPointLight = 0;
        for (auto light : lights)
        {
            glm::vec3 position = camera.calculateViewMatrix() * glm::vec4(light.Position, 1.0f);
            int ligthPosition = glGetUniformLocation(shader.ID, pointLightAttribute("position", nbPointLight).c_str());
            glUniform3fv(ligthPosition, 1, glm::value_ptr(position));

            int lightAmbient = glGetUniformLocation(shader.ID, pointLightAttribute("ambient", nbPointLight).c_str());
            glUniform3fv(lightAmbient, 1, glm::value_ptr(light.Ambiant));

            int lightDiffuse = glGetUniformLocation(shader.ID, pointLightAttribute("diffuse", nbPointLight).c_str());
            glUniform3fv(lightDiffuse, 1, glm::value_ptr(light.Diffuse));

            int lightSpecular = glGetUniformLocation(shader.ID, pointLightAttribute("specular", nbPointLight).c_str());
            glUniform3fv(lightSpecular, 1, glm::value_ptr(light.Specular));

            int lightDirection = glGetUniformLocation(shader.ID, pointLightAttribute("direction", nbPointLight).c_str());
            glUniform3fv(lightDirection, 1, glm::value_ptr(light.Direction));

            shader.setFloat(pointLightAttribute("constant", nbPointLight), light.Constant);
            shader.setFloat(pointLightAttribute("linear", nbPointLight), light.Linear);
            shader.setFloat(pointLightAttribute("quadratic", nbPointLight), light.Quadratic);
            ++nbPointLight;
        }

        shader.setInt("nbPointLight", nbPointLight);
    }
    void useLight(Shader &shader, const Light &light, Camera &camera)
    {
        shader.use();
        glm::vec3 position = camera.calculateViewMatrix() * glm::vec4(light.Position, 1.0f);

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

        shader.setFloat("light.constant", light.Constant);
        shader.setFloat("light.linear", light.Linear);
        shader.setFloat("light.quadratic", light.Quadratic);
        shader.setFloat("light.cutOff", glm::cos(glm::radians(light.CutOff)));
        shader.setFloat("light.outerCutOff", glm::cos(glm::radians(light.OuterCutOff)));
    }
    void render(Camera &camera, const Mesh &mesh, Material &mat, glm::mat4 transform)
    {
        mat.shader->use();
        glUniform1i(glGetUniformLocation(mat.shader->ID, "texture1"), 0);
        glUniform1i(glGetUniformLocation(mat.shader->ID, "texture2"), 1);
        // diffuse
        int materialDiffuse = glGetUniformLocation(mat.shader->ID, "material.diffuse");
        glUniform1i(materialDiffuse, 0);
        int materialSpecular = glGetUniformLocation(mat.shader->ID, "material.specular");
        glUniform1i(materialSpecular, 1);
        int i = 0;
        for (auto text : mat.textures)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, text);
            ++i;
        }
        glBindVertexArray(mesh.Id);

        // material
        /*     int materialAmbient = glGetUniformLocation(mat.shader->ID, "material.ambient");
        glUniform3fv(materialAmbient, 1, glm::value_ptr(mat.ambient)); */

        /*  int materialDiffuse = glGetUniformLocation(mat.shader->ID, "material.diffuse");
        glUniform3fv(materialDiffuse, 1, glm::value_ptr(mat.diffuse)); */

        /* int materialSpecular = glGetUniformLocation(mat.shader->ID, "material.specular");
        glUniform3fv(materialSpecular, 1, glm::value_ptr(mat.specular)); */

        int shininess = glGetUniformLocation(mat.shader->ID, "material.shininess");
        glUniform1f(shininess, mat.shininess);

        // coordinate system
        int viewPos = glGetUniformLocation(mat.shader->ID, "viewPos");
        glUniform3fv(viewPos, 1, glm::value_ptr(camera.Position));

        int viewLoc = glGetUniformLocation(mat.shader->ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));

        int projectionLoc = glGetUniformLocation(mat.shader->ID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(camera.Projection));

        int modelLoc = glGetUniformLocation(mat.shader->ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));

        glDrawElements(GL_TRIANGLES, (int)mesh.Indices.size(), GL_UNSIGNED_INT, 0);
    }

private:
    const std::string pointLightAttribute(const std::string &attribute, int index)
    {
        return std::string("pointLights[") + std::to_string(index) + "]." + attribute;
    }
};
class OpenGLManager
{
public:
    OpenGLVAO loadMesh(Mesh &mesh)
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
                     mesh.Indices.size() * sizeof(unsigned int), &mesh.Indices[0],
                     GL_STATIC_DRAW);

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

        return openGlMesh;
    }
    unsigned int loadTexture2D(const Image &image)
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
            // bind texture
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        return texture;
    }
};
Mesh createCube()
{
    const int rowSize = 5;
    Mesh cube;
    float normals[] = {
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,
        0.0f, 0.0f, -1.0f,

        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,

        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,

        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,

        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,

        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f};
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