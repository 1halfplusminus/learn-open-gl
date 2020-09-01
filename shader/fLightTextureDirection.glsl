#version 330 core

struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  vec3 direction;
};

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform Material material;
uniform Light light;
void main() {

  // ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));

  // diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(-light.direction);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse =
      light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));

  // specular
  vec3 viewDir = normalize(-FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  vec3 specular =
      light.specular * spec * vec3(texture(material.specular, TexCoord));

  vec3 result = ambient + diffuse + specular;

  FragColor = vec4(result, 1.0);
}