#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

struct Material {
  sampler2D texture_diffuse1;
  sampler2D texture_diffuse2;
  sampler2D texture_diffuse3;
  sampler2D texture_specular1;
  sampler2D texture_specular2;
  float shininess;
};

uniform Material material;

void main() {
  vec4 text = texture(material.texture_diffuse1, TexCoord);
  FragColor = vec4(vec3(1.0 - text), 1.0);
}