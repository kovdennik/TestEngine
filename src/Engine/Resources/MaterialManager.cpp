#include <BlackBox/Resources/MaterialManager.hpp>
#include <BlackBox/Resources/TextureManager.hpp>
#include <BlackBox/Resources/ShaderManager.hpp>
#include <BlackBox/Render/ReflectShader.hpp>
#include <BlackBox/IConsole.hpp>
#include <glm/glm.hpp>

#include <tinyxml2.h>
#ifndef XMLCheckResult
  #define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("Error: %i\n", a_eResult); return a_eResult; }
#endif

using	namespace std;
using namespace tinyxml2;

MaterialManager *MaterialManager::manager = nullptr;
Material *defaultMaterial;

MaterialManager *MaterialManager::instance()
{
  if (manager == nullptr)
  {
    manager = new MaterialManager();
  }
  return manager;
}

std::shared_ptr<CShaderProgram> MaterialManager::getProgram(std::string name)
{
	auto p_it = shaders_map.find(name);
	if (p_it == shaders_map.end())
		return ShaderManager::instance()->getDefaultProgram();
	else
		return p_it->second;
}

Material *MaterialManager::getMaterial(std::string name)
{
  Material *material = nullptr;

  /*
  std::string prefix = "res/materials/";
  bool usPrefix = true;
  if (name.find("/") != name.npos)
    usPrefix = false;
  */
  {
    std::string fullName = name;
    /*
    if (usPrefix)
      fullName = prefix + name;
    else fullName = name;
    */

    auto matItor = cache.find(fullName);
    if (matItor != cache.end())
    {
      material = matItor->second;
    }
    else {
      m_pLog->AddLog("[ERROR] Load material\n");
    }
  }

  return material;
}

bool MaterialManager::init(std::string materialLib)
{
  bool status = false;
  status = MaterialManager::instance()->loadLib(materialLib);
  if (status)
  {
    /*
    defaultMaterial = new Material();
    defaultMaterial->diffuse = TextureManager::instance()->getTexture("check.jpg");
    defaultMaterial->program = defaultProgram;
    */
    defaultMaterial = MaterialManager::instance()->getMaterial("default");
    //defaultMaterial->name = std::make_shared<std::string>("default");
  }
  return status;
}

MaterialManager::MaterialManager() : m_pLog(GetIEngine()->getILog())
{
	root_path = GetIEngine()->getIConsole()->GetCVar("materials_path");
}

bool MaterialManager::loadLib(std::string name)
{
  XMLDocument xmlDoc;

  XMLError eResult = xmlDoc.LoadFile((root_path->GetString() + name).c_str());
  XMLCheckResult(eResult);

  XMLNode * materials = xmlDoc.FirstChild();
  if (materials == nullptr) return false;

  XMLElement * material = materials->FirstChildElement("material");
  XMLElement * shaders = materials->FirstChildElement("shaders");

  if (material == nullptr || shaders == nullptr) return false;

  XMLNode * program = shaders->FirstChild();
  while (program != nullptr)
  {
		ProgramDesc pd;
		pd.name = program->Value();

		auto shader = program->FirstChildElement("shader");
		if (shader != nullptr)
		{
			getShaderAttributes(shader, pd);
			shader = shader->NextSiblingElement("shader");
			if (shader != nullptr)
			{
				getShaderAttributes(shader, pd);
				if (!loadProgram(pd, false))
				{
					//TODO: handle this case
					m_pLog->AddLog("[ERROR] Failed load material\n");
				}
			}
		}
    program = program->NextSibling();
  }

  while (material != nullptr)
  {
    if (!loadMaterial(material))
    {
      //TODO: handle this case
      m_pLog->AddLog("[ERROR] Failed load material\n");
    }
    material = material->NextSiblingElement("material");
  }
  if (cache.size() != 0) return true;
  else return false;
}

void MaterialManager::getShaderAttributes(tinyxml2::XMLElement* shader, MaterialManager::ProgramDesc& pd)
{
	std::string type;
	std::string name;

	type = shader->Attribute("type");
	name = shader->Attribute("name");

	if (type == "vertex")
		pd.vs = name;
	else if (type == "fragment")
		pd.fs = name;
}

bool MaterialManager::loadMaterial(XMLElement *material)
{
  Material *result = new Material();

  const char *materialName = nullptr;
	isSkyBox = false;

  materialName = material->Attribute("name");
  if (materialName == nullptr)
    return false;
	if (material->Attribute("type", "skybox"))
		isSkyBox = true;
	if (material->Attribute("shakaled"))
		alpha_shakaled = true;
	else
		alpha_shakaled = false;

  result->name = std::make_shared<std::string>(materialName);
  //============ TEXTURES LOADING =======================//
  XMLElement *textures = material->FirstChildElement("textures");
  XMLElement *colorElement = material->FirstChildElement("color");
  result->diffuseColor = glm::vec3(1.0, 0.5, 0.31);
  if (colorElement != nullptr)
  {
    result->diffuseColor.r = colorElement->FloatAttribute("red");
    result->diffuseColor.g = colorElement->FloatAttribute("green");
    result->diffuseColor.b = colorElement->FloatAttribute("blue");
  }
  if (textures != nullptr)
  {
    result->hasTexture = true;
    XMLElement *image = textures->FirstChildElement("texture");
    if (image == nullptr) return false;
    while (image != nullptr)
    {
      BaseTexture *t = loadTexture(image);
      switch(t->type)
      {
      case TextureType::DIFFUSE:
				t->setUnit(0);
        result->diffuse.push_back(t);
        break;
      case TextureType::SPECULAR:
				t->setUnit(1);
        result->specular = t;
        break;
      case TextureType::BUMP:
				t->setUnit(2);
        result->bump = t;
        break;
      case TextureType::NORMAL:
				t->setUnit(3);
        result->normal = t;
        break;
      case TextureType::MASK:
				t->setUnit(4);
        result->mask = t;
        break;
      default:
      {
        m_pLog->AddLog("[ERROR] Unknown texture type\n");
      }
      }
      image = image->NextSiblingElement("texture");
    }
  }
  else {
    result->hasTexture = false;
  }
	result->current_diffuse = 0;
  //============ SHADERS LOADING =======================//
  XMLElement *shader = material->FirstChildElement("shader");
  if (shader == nullptr) return false;
	const char * shader_name = nullptr;
	shader_name = shader->Attribute("name");
	if (shader_name == nullptr)
		return false;
	
	auto shader_it = shaders_map.find(shader_name);
	if (shader_it == shaders_map.end())
		return false;
  result->program = shader_it->second;
	result->program_name = shader_name;
  cache[materialName] = result;
  m_pLog->AddLog("[INFO] Created material: %s\n", materialName);
  return true;
}

bool MaterialManager::loadProgram(ProgramDesc &desc, bool isReload)
{
	auto shader_it = shaders_map.find(desc.name);
	if (shader_it != shaders_map.end() && !isReload)
		return true;
	auto vs = loadShader(ShaderDesc("vertex", desc.vs), isReload);
	if (vs == nullptr) return false;

	auto fs = loadShader(ShaderDesc("fragment", desc.fs), isReload);
	if (fs == nullptr) return false;

	auto shaderProgram = std::make_shared<CShaderProgram>(vs,fs);
  if (!shaderProgram->create())
    return false;
	shaderProgram->vertex_name = desc.vs;
	shaderProgram->fragment_name = desc.fs;
	shaders_map[desc.name] = shaderProgram;
	return true;
}

bool MaterialManager::reloadShaders()
{
	for (auto shader : shaders_map)
	{
		ProgramDesc pd;
		pd.name = shader.first;
		pd.vs = shader.second->vertex_name;
		pd.fs = shader.second->fragment_name;
		reloadShader(pd);
	}
	return true;
}

bool MaterialManager::reloadShaders(std::vector<std::string> names)
{
	for (auto shader : names)
	{
		ProgramDesc pd;
		pd.name = shader;

		auto s_it = shaders_map.find(shader);
		if (s_it == shaders_map.end())
			continue;
		pd.vs = s_it->second->vertex_name;
		pd.fs = s_it->second->fragment_name;
		reloadShader(pd);
	}
	return true;
}

void MaterialManager::reloadShader(MaterialManager::ProgramDesc& pd)
{
	//delete shader.second;
	loadProgram(pd, true);
	for (auto& mat : cache)
	{
		if (mat.second->program_name == pd.name)
			mat.second->program = shaders_map[pd.name];
	}
}

BaseTexture *MaterialManager::loadTexture(XMLElement *texture)
{
  BaseTexture *result;
  TextureManager *textureManager = TextureManager::instance();
  const char *type = nullptr;
  const char *name = nullptr;

  type = texture->Attribute("type");
  name = texture->Attribute("name");

  result = textureManager->getTexture(name, isSkyBox);
  if (result == nullptr)
    return result;
  result->setType(type);
  return result;
}

XMLElement *MaterialManager::saveTexture(XMLDocument &xmlDoc, Texture *texture)
{
  XMLElement *textureElement = xmlDoc.NewElement("shader");

  textureElement->SetAttribute("type", texture->typeToStr().c_str());
  textureElement->Attribute("name", texture->path->c_str());

  return textureElement;

}

std::shared_ptr<CShader> MaterialManager::loadShader(ShaderDesc &sd, bool isReload)
{
  return ShaderManager::instance()->getShader(sd.name, sd.type, isReload);
}

XMLElement *MaterialManager::saveShader(XMLDocument &xmlDoc, CShader *shader)
{
  XMLElement *shaderElement = xmlDoc.NewElement("shader");

  shaderElement->SetAttribute("type", shader->typeToStr().c_str());
  shaderElement->Attribute("name", shader->m_Path.c_str());

  return shaderElement;
}