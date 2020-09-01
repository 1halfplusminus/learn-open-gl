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

  float constant;
  float linear;
  float quadratic;
  float cutOff;
  float outerCutOff;
};

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform Material material;
uniform Light light;
uniform int nbPointLight;
#define MAX_NR_POINT_LIGHTS 100
uniform Light pointLights[MAX_NR_POINT_LIGHTS];
uniform vec3 viewPos;

vec3 CalcDirLight(Light light, vec3 normal, vec3 viewDir) {
  vec3 lightDir = normalize(-light.direction);
  // diffuse shading
  float diff = max(dot(normal, lightDir), 0.0);
  // specular shading
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  // combine results
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));
  vec3 diffuse =
      light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));
  vec3 specular =
      light.specular * spec * vec3(texture(material.specular, TexCoord));
  return (ambient + diffuse + specular);
}
vec3 CalcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
  vec3 lightDir = normalize(light.position - fragPos);
  // diffuse shading
  float diff = max(dot(normal, lightDir), 0.0);
  // specular shading
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
  // attenuation
  float distance = length(light.position - fragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance +
                             light.quadratic * (distance * distance));
  // combine results
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));
  vec3 diffuse =
      light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));
  vec3 specular =
      light.specular * spec * vec3(texture(material.specular, TexCoord));
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;
  return (ambient + diffuse + specular);
}
void main() {

  // properties
  vec3 norm = normalize(Normal);
  vec3 viewDir = normalize(-FragPos);

  // phase 1: Directional lighting
  vec3 result = CalcDirLight(light, norm, viewDir);
  // phase 2: Point lights
  for (int i = 0; i < nbPointLight; i++)
    result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);

  FragColor = vec4(result, 1.0);
}
