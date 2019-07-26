#pragma once
#include <BlackBox/Render/Opengl.hpp>
#include <BlackBox/IEngine.hpp>
#include <glm/fwd.hpp>
#include <string>
#include <memory>

class CShader; 
class CBaseShaderProgram;

extern CBaseShaderProgram *defaultProgram;

struct ShaderStatus
{
 GLchar infoLog[512]; 
 GLint m_Status;
 CShader *m_Shader;

 ShaderStatus(CShader *shader);
 bool get(GLenum statusType);
};

struct ShaderProgramStatus
{
 GLchar infoLog[512]; 
 GLint m_Status;
 CBaseShaderProgram *m_Program;

 ShaderProgramStatus(CBaseShaderProgram *program);
 bool get(GLenum statusType);
};

class CShader 
{
private:
	GLuint m_Shader;
	std::string m_Path;
	std::string m_Text;
  ShaderStatus m_Status;
public:
  std::shared_ptr<std::string> m_path;
  int m_Type;
  enum type : int{
    E_VERTEX = GL_VERTEX_SHADER,
    E_FRAGMENT = GL_FRAGMENT_SHADER,
    E_UNKNOWN = -1
  };
  CShader(std::string text, CShader::type type);
  ~CShader();
  static CShader *load(std::string path, CShader::type type);
  static bool parseLine(std::ifstream &fin, std::string &buffer);
  static bool loadInternal(std::string &path, std::string& buffer);
  static CShader *loadFromMemory(std::string text, CShader::type type);
  bool create();
  bool compile();
  bool bind();
  void print();
  std::string typeToStr();
  std::string getName();
  GLuint get();
};

template<class T>
class UniformValue {
private:
  GLint m_location;
  T m_value;
};

class CBaseShaderProgram {
public:
  CShader *m_Vertex = nullptr;
  CShader *m_Fragment = nullptr;
  GLuint m_Program;
  GLchar infoLog[512];
  ShaderProgramStatus m_Status;
  bool attached = false;
  bool created = false;

  bool status();
public:
  CBaseShaderProgram();
  CBaseShaderProgram(CShader *vertex, CShader *fragment);
  ~CBaseShaderProgram();

  bool create();
  void attach(CShader *shader);
  bool link();
  void use();
  void unuse();
  void setUniformValue(int value, const char *format, ...);
  void setUniformValue(float value, const char *format, ...);
  void setUniformValue(glm::vec1 value, const char *format, ...);
  void setUniformValue(glm::vec2 value, const char *format, ...);
  void setUniformValue(glm::vec3 value, const char *format, ...);
  void setUniformValue(glm::vec4 value, const char *format, ...);
  void setUniformValue(glm::mat2 value, const char *format, ...);
  void setUniformValue(glm::mat3 value, const char *format, ...);
  void setUniformValue(glm::mat4 value, const char *format, ...);
  GLuint get();
	virtual void setup() = 0;
};
