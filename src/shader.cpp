
#include <shader.h>
// clang-format off
#include <glad/glad.h>
// clang-format on
#include <vector>

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
  // build and compile our shader program
  // ------------------------------------
  // vertex shader
  std::string vertexShaderSource = readFile(vertexPath);
  const GLchar *vertexShaderSourceChar = vertexShaderSource.c_str();
  // check for shader compile errors
  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSourceChar, NULL);
  glCompileShader(vertexShader);
  checkCompileErrors(vertexShader, "VERTEX");
  // fragment shader
  std::string fragmentShaderSource = readFile(fragmentPath);
  const GLchar *fragmentShaderSourceChar = fragmentShaderSource.c_str();
  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSourceChar, NULL);
  glCompileShader(fragmentShader);
  checkCompileErrors(fragmentShader, "FRAGMENT");
  // link shaders
  ID = glCreateProgram();
  glAttachShader(ID, vertexShader);
  glAttachShader(ID, fragmentShader);
  glLinkProgram(ID);
  checkCompileErrors(ID, "PROGRAM");
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
}

// activate the shader
// ------------------------------------------------------------------------
void Shader::use() { glUseProgram(ID); }
// utility uniform functions
// ------------------------------------------------------------------------
void Shader::setBool(const std::string &name, bool value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
// ------------------------------------------------------------------------
void Shader::setInt(const std::string &name, int value) const {
  glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
// ------------------------------------------------------------------------
void Shader::setFloat(const std::string &name, float value) const {
  glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}
// utility function for checking shader compilation/linking errors.
// ------------------------------------------------------------------------
void Shader::checkCompileErrors(unsigned int shader, std::string type) {
  int success;
  char infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      std::cout
          << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, NULL, infoLog);
      std::cout
          << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
          << infoLog
          << "\n -- --------------------------------------------------- -- "
          << std::endl;
    }
  }
}
const std::string Shader::readFile(const std::string path) {
  std::ifstream ifs(path.c_str(),
                    std::ios::in | std::ios::binary | std::ios::ate);

  if (ifs.bad()) {
    std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ " << std::endl;
  }
  std::ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  std::vector<char> bytes(fileSize);
  ifs.read(&bytes[0], fileSize);
  return std::string(&bytes[0], fileSize);
}