#include <BlackBox/Render/Shader.hpp>
#include <BlackBox/Render/OpenGL/Debug.hpp>
#include <BlackBox/ISystem.hpp>
#include <BlackBox/Render/Pipeline.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

CShaderProgram::CShaderProgram() : CBaseShaderProgram()
{

}

CShaderProgram::CShaderProgram(ShaderRef vs, ShaderRef fs) : CBaseShaderProgram(vs, fs)
{

}

CShaderProgram::CShaderProgram(ShaderInfo& vs, ShaderInfo& fs) : CBaseShaderProgram(vs, fs)
{
}

#if 0
CShaderProgram::CShaderProgram(std::string vs, std::string fs) 
	:
	CBaseShaderProgram(CShader::load(vs, CShader::E_VERTEX), CShader::load(fs, CShader::E_FRAGMENT))
{
}
#endif

CShaderProgram::CShaderProgram(ShaderInfo& vs, ShaderInfo& fs, ShaderInfo& gs) : CBaseShaderProgram(vs, fs, gs)
{
}

CShaderProgram::CShaderProgram(ShaderInfo& vs, ShaderInfo& fs, ShaderInfo& gs, ShaderInfo& cs) : CBaseShaderProgram(vs, fs, gs, cs)
{

}

void CShaderProgram::setup()
{
  Uniform( Pipeline::instance()->model,"model");
  Uniform( Pipeline::instance()->view,"view");
  Uniform( Pipeline::instance()->projection,"projection");
  Uniform( Pipeline::instance()->view_pos,"viewPos");
  Uniform( 128.0f,"material.shininess");
}

CShaderProgram::~CShaderProgram()
{
}
