#version 330 core

struct Material {
  sampler2D texture_diffuse1;
  sampler2D texture_diffuse2;
  sampler2D texture_diffuse3;
  sampler2D texture_specular1;
  sampler2D texture_specular2;
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

// Light
#define MAX_NR_POINT_LIGHTS 20
uniform Light light;

uniform int nbPointLight;
uniform Light pointLights[MAX_NR_POINT_LIGHTS];

uniform int nbSpotLight;
uniform Light spotLights[MAX_NR_POINT_LIGHTS];

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

vec3 CalcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
  vec3 result;
  // ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoord));
  // diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light.position - fragPos);
  float theta = dot(lightDir, normalize(-light.direction));
  float epsilon = light.cutOff - light.outerCutOff;
  float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
  if (theta > light.cutOff) {
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse =
        light.diffuse * diff * vec3(texture(material.diffuse, TexCoord));

    // specular
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular =
        light.specular * spec * vec3(texture(material.specular, TexCoord));
    diffuse *= intensity;
    specular *= intensity;
    result = ambient + diffuse + specular;
  } else {
    result = ambient;
  }
  return result;
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
  // phase 3: Spot lights
  for (int i = 0; i < nbSpotLight; i++)
    result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);

  FragColor = vec4(result, 1.0);
}
