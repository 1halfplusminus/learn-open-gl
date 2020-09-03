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
  float average = 0.2126 * text.r + 0.7152 * text.g + 0.0722 * text.b;
  FragColor = vec4(average, average, average, 1.0);
}