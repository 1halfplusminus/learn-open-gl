#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;
layout(location = 2) in vec3 aNormal;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 transform;

void main() {
  gl_Position = model * vec4(aPos.x, aPos.y, 0.0, 1.0);
  TexCoord = aTexCoords;
}