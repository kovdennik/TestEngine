#include <BlackBox/Render/BaseShader.hpp>
#include <BlackBox/ISystem.hpp>
#include <BlackBox/ILog.hpp>
#include <BlackBox/Render/OpenGL/Debug.hpp>
#include <BlackBox/Render/Pipeline.hpp>
#include <BlackBox/IConsole.hpp>

#include <fstream>
#include <string>
#include <cstdarg>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

char* CBaseShaderProgram::buffer = nullptr;

CShader::type str2typ(std::string type)
{
  if (type == "vertex")
    return CShader::type::E_VERTEX;
  else if (type == "fragment")
    return CShader::type::E_FRAGMENT;
  else if (type == "geometry")
    return CShader::type::E_GEOMETRY;
  else if (type == "compute")
    return CShader::type::E_COMPUTE;
  else
    return CShader::type::E_UNKNOWN;
}

ShaderStatus::ShaderStatus(CShader *shader) :
  m_Shader(shader)
{

}

bool ShaderStatus::get(GLenum statusType) {
  glCheck(glCheck(glGetShaderiv(m_Shader->get(), statusType, &m_Status)));
  if(m_Status != GL_TRUE)
  {
    glCheck(glGetShaderInfoLog(m_Shader->get(), 512, NULL, infoLog));
    GetISystem()->GetILog()->Log("[ERROR] Shader %s \n %s\n", m_Shader->getName().c_str(), infoLog);;
    return false;
  }
  return true;
 
}

ShaderProgramStatus::ShaderProgramStatus(CBaseShaderProgram *program) :
  m_Program(program)
{

}

bool ShaderProgramStatus::get(GLenum statusType) {
  GLsizei size;
  glCheck(glGetProgramiv(m_Program->get(), statusType, &m_Status));
	if (m_Status != GL_TRUE)
	{
		glCheck(glGetProgramInfoLog(m_Program->get(), 512, &size, infoLog));
		auto log = GetISystem()->GetILog();
		if (log != nullptr)
		{
			log->Log("[ERROR] Shader::programm: %s\n", infoLog);
			std::vector<char> label(1);
			GLsizei length = 0;
			glCheck(glGetObjectLabel(GL_PROGRAM, this->m_Program->get(), 1, &length, label.data()));
			if (length > 0)
			{
				label.resize(length);
				glCheck(glGetObjectLabel(GL_PROGRAM, this->m_Program->get(), length, &length, label.data()));
				log->Log("[INFO] Shader::programm label: %s\n", label.data());
			}
		}
		else
			GetISystem()->Log((std::string("[ERROR] Shader::programm: ") +  infoLog).c_str());
    return false;
  }
  return true;
 
}
CShader::CShader(string text, CShader::type type) :
  m_Text(text), m_Type(type), m_Status(this), m_Empty(true)
{
  
}

CShader::~CShader() {
  glCheck(glDeleteShader(m_Shader));
}

bool CShader::create() {
  m_Shader = glCreateShader(m_Type);
  if (m_Shader != 0) { return true; }
  else { return false; }
  // return m_Status.get(GL_VALIDATE_STATUS);
}


std::shared_ptr<CShader> CShader::load(ShaderDesc& desc) {
  string text;

	auto path = ShaderDesc::root + desc.name;
	if (!loadInternal(path, text)) return nullptr;

	if (desc.macro.size() > 0)
	{
		auto pos = text.find("#version");
		if (pos != std::string::npos)
		{
			auto end = text.find_first_of('\n', pos + 1);
			std::string defines;
			for (auto& define : desc.macro)
			{
				defines = "#define " + define.first + " " + define.second + "\n";
			}
			text.insert(end + 1, defines);
			std::cout << text << endl;
		}
	}

  auto shader = std::make_shared<CShader>(text, str2typ(desc.type));
  if (!shader->create())
    return nullptr;
  shader->compile();
  shader->print();
	shader->m_Empty = false;
	debuger::shader_label(shader->get(), path);
  return shader;
}

bool CShader::parseLine(std::ifstream &fin, std::string& buffer)
{
	if (!getline(fin, buffer))
		return false;
	size_t pos = 0;
	if ((pos = buffer.find("#include")) != buffer.npos)
	{
		size_t begin, end;
		if ((begin = buffer.find_first_of('<')) != buffer.npos)
			end = buffer.find_first_of('>');
		else if ((begin = buffer.find_first_of('"')) != buffer.npos)
			end = buffer.find('"', begin + 1);
		else
			return false;

		std::string file(buffer.substr(begin + 1, end - begin - 1));
		std::string buff;
		if (!loadInternal("res/shaders/" + file, buff)) return false;
		buffer.clear();
		buffer += buff;
	}
		
	return true;
}

bool CShader::loadInternal(std::string &path, std::string& buffer)
{
  ifstream fin(path);
  string buff;

  if (!fin.is_open()) return false;
  
  while (parseLine(fin, buff)) {
    buffer += buff;
    buffer += '\n';
  }
	fin.close();
	return true;
}

std::shared_ptr<CShader> CShader::loadFromMemory(std::string text, CShader::type type)
{
  auto shader = std::make_shared<CShader>(text, type);
  if (!shader->create())
    return nullptr;
  shader->compile();
  shader->print();
  return shader;
}

bool CShader::compile() {
  const char *text = m_Text.c_str();
  glCheck(glShaderSource(m_Shader, 1, &text, nullptr));
  glCheck(glCompileShader(m_Shader));
  //glCompileShaderIncludeARB(m_Shader, )
  return m_Status.get(GL_COMPILE_STATUS);
}

bool CShader::bind() {
 return true; 
}

bool CShader::empty()
{
	return m_Empty;
}

void CShader::print() {
  //cout << m_Text << endl;
}

string CShader::typeToStr()
{
  switch (m_Type) {
  case E_VERTEX:
    return "vertex";
  case E_FRAGMENT:
    return "fragment";
  case E_GEOMETRY:
    return "geometry";
  case E_COMPUTE:
    return "compute";
	default:
    return "unknown";
  }
}

string CShader::getName() {
  return m_Path;
}

GLuint CShader::get() {
  return m_Shader;
}

CBaseShaderProgram::CBaseShaderProgram() : m_Status(this)
{
  m_Program = glCreateProgram();
  created = true;
  buffer = new char[BUFFER_SIZE];
}

CBaseShaderProgram::CBaseShaderProgram(
  ShaderRef vs, ShaderRef fs)
	:
	CBaseShaderProgram()
{
  m_Vertex.shader = vs;
  m_Fragment.shader = fs;
}

CBaseShaderProgram::CBaseShaderProgram(ShaderInfo& vs, ShaderInfo& fs)
	:
	CBaseShaderProgram()
{
	m_Vertex = vs;
	m_Fragment = fs;
}

CBaseShaderProgram::CBaseShaderProgram(ShaderInfo& vs, ShaderInfo& fs, ShaderInfo& gs)
	:
	CBaseShaderProgram()
{
	m_Vertex = vs;
	m_Fragment = fs;
	m_Geometry = gs;
}

CBaseShaderProgram::CBaseShaderProgram(ShaderInfo& vs, ShaderInfo& fs, ShaderInfo& gs, ShaderInfo& cs)
	:
	CBaseShaderProgram()
{
	m_Vertex = vs;
	m_Fragment = fs;
	m_Geometry = gs;
	m_Compute = cs;
}

CBaseShaderProgram::~CBaseShaderProgram() {
	glDeleteProgram(m_Program);
}

bool CBaseShaderProgram::create(const char *label) {
	if (!created) {
		created = true;
		m_Program = glCreateProgram();
	}
	debuger::program_label(m_Program, label);
	attach(m_Vertex);
	attach(m_Fragment);
	attach(m_Geometry);
	attach(m_Compute);
	return Link();
}

void CBaseShaderProgram::attach(ShaderInfo& info) {
	ShaderInfo &attached = info;
	
	if (!info.used) return;
  switch (info.shader->m_Type) {
  case CShader::type::E_VERTEX:
		attached = attachInternal(info, m_Vertex);
    break;
  case CShader::type::E_FRAGMENT:
		attached = attachInternal(info, m_Fragment);
    break;
  case CShader::type::E_GEOMETRY:
		attached = attachInternal(info, m_Geometry);
     break;
  case CShader::type::E_COMPUTE:
		attached = attachInternal(info, m_Compute);
    break;
  }
	attached.attached = true;
  glCheck(glAttachShader(m_Program, info.shader->get()));
}

CBaseShaderProgram::ShaderInfo& CBaseShaderProgram::attachInternal(ShaderInfo& src, ShaderInfo& dst)
{
	if (dst.shader == nullptr)
		dst.shader = src.shader;
	if (dst.attached)
		Detach(src);
	return dst;
}

void CBaseShaderProgram::Detach(ShaderInfo& info)
{
  if (info.attached)
		glCheck(glDetachShader(m_Program, info.shader->get()));
}

bool CBaseShaderProgram::Dispatch(int x, int y, int z, GLbitfield barriers)
{
	if (m_Compute.attached)
	{
		glCheck(glDispatchCompute(x, y, z));
		if (barriers) glCheck(glMemoryBarrier(barriers));
		return true;
	}
	return false;
}

bool CBaseShaderProgram::DispatchInderect()
{
	return false;
}

bool CBaseShaderProgram::Link() {
  glCheck(glLinkProgram(m_Program));
	return m_Status.get(GL_LINK_STATUS);
}

void CBaseShaderProgram::Use() {
    glCheck(glUseProgram(m_Program));
}

void CBaseShaderProgram::Unuse()
{
    glCheck(glUseProgram(0));
}

void CBaseShaderProgram::DeleteProgram()
{
	glDeleteProgram(m_Program);
	created = false;
}

GLint CBaseShaderProgram::GetUniformLocation(const char* format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = -1;
	if (use_cache->GetIVal())
	{
		auto it = m_Cache.find(name);
		if (it != m_Cache.end())
			loc = it->second;
		else
		{
			loc = glGetUniformLocation(m_Program, name);
			m_Cache[name] = loc;
		}
	}
	else
	{
    loc = glGetUniformLocation(m_Program, name);
		m_Cache[name] = loc;
	}
  return loc;
}

GLint CBaseShaderProgram::GetUniformLocation(std::string& name)
{
  return GetUniformLocation(name.c_str());
}

UniformValue CBaseShaderProgram::GetUniformValue(const char* name)
{
  UniformValue result;
  auto location = GetUniformLocation(name);

  result.location = location;
	result.program = m_Program;
  return result;
}

void CBaseShaderProgram::Uniform(int value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
      glCheck(glUniform1i(loc, value));
  }
}

void CBaseShaderProgram::Uniform(unsigned int value, const char* format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
      glCheck(glUniform1ui(loc, value));
  }
}

void CBaseShaderProgram::Uniform(float value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
        glCheck(glUniform1f(loc, value));
  }
}

void CBaseShaderProgram::Uniform(Vec1 value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
        glCheck(glUniform1f(loc, value[0]));
  }
}

void CBaseShaderProgram::Uniform(Vec2 value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
		glCheck(glUniform2fv(loc, 1, glm::value_ptr(value)));
		if (print_loc_name->GetIVal())
			std::cout << name << std::endl;
  }
}

void CBaseShaderProgram::Uniform(Vec3 value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
      glCheck(glUniform3fv(loc, 1, glm::value_ptr(value)));
  }
}

void CBaseShaderProgram::Uniform(Vec4 value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
        glCheck(glUniform4fv(loc, 1, glm::value_ptr(value)));
  }
}

void CBaseShaderProgram::Uniform(glm::ivec4 value, const char* format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
        glCheck(glUniform4iv(loc, 1, glm::value_ptr(value)));
  }
}

void CBaseShaderProgram::Uniform(Mat2 value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
    glCheck(glUniformMatrix2fv(loc, 1, GL_FALSE, glm::value_ptr(value)));
  }
}

void CBaseShaderProgram::Uniform(Mat3 value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
    glCheck(glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(value)));
  }
}

void CBaseShaderProgram::Uniform(Mat4 value, const char * format, ...)
{
  va_list ptr;
  va_start(ptr, format);
  auto name = buildName(format, ptr);
  va_end(ptr);

  GLint loc = GetUniformLocation(name);
  if (loc != -1){
    glCheck(glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value)));
  }
}

void CBaseShaderProgram::reload(ShaderRef v, ShaderRef f, ShaderRef g, ShaderRef c, const char* label)
{
	Detach(m_Vertex);
	//reset(m_Vertex);
	Detach(m_Fragment);
	//reset(m_Fragment);
	Detach(m_Geometry);
	//reset(m_Geometry);
	Detach(m_Compute);
	//reset(m_Compute);
	DeleteProgram();

	m_Vertex.shader = v;
	m_Fragment.shader = f;
	if (g != nullptr)
		m_Geometry.shader = g;
	if (c != nullptr)
		m_Compute.shader = c;
	create(label);
}

void CBaseShaderProgram::BindTexture2D(GLuint texture, GLint unit, const char* sampler)
{
	BindTextureUnit2D(texture, unit);
	Uniform(unit, sampler);
}

void CBaseShaderProgram::BindTextureUnit2D(GLuint texture, GLint unit)
{
	gl::ActiveTexture(GL_TEXTURE0 + unit);
	gl::BindTexture2D(texture);
}

GLuint CBaseShaderProgram::get() {
  return m_Program;
}

void CBaseShaderProgram::Dump()
{
	int len = 0;
	GLenum bFormat;
	glGetProgramiv(m_Program, GL_PROGRAM_BINARY_LENGTH, &len);

	std::unique_ptr<uint8_t> binary(new uint8_t[len]);
	glGetProgramBinary(m_Program, len, &len, &bFormat, binary.get());

	FILE* shader = fopen("dump.bin", "wb");
	if (!shader)
	{
		//GetISystem()->Log("Cannot open file for dump");
		return;
	}

	fwrite(binary.get(), 1, len, shader);
	fclose(shader);
}

void CBaseShaderProgram::reset(ShaderInfo src)
{
	if (src.shader != nullptr)
		src.shader.reset();
}

const char* CBaseShaderProgram::buildName(const char* format, va_list args)
{
  vsprintf(buffer, format, args);
  return buffer;
}

