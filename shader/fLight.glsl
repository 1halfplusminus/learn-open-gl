#version 330 core

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec3 LightPos; // extra in variable, since we need the light position in view
                  // space we calculate this in the vertex shader

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform Material material;
uniform Light light;
void main() {

  // ambient
  vec3 ambient = light.ambient * material.ambient;

  // diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(LightPos - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * (diff * material.diffuse);

  // specular
  vec3 viewDir = normalize(-FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular = light.specular * (spec * material.specular);

  vec3 result = ambient + diffuse + specular;

  FragColor = vec4(result, 1.0);
}