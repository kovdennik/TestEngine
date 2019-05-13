#include <BlackBox/ObjectManager.hpp>
#include <BlackBox/Object.hpp>
#include <BlackBox/Primitives.hpp>
#include <BlackBox/CPlayer.h>

#include <iostream>
using	namespace std;

ObjectManager *ObjectManager::manager = nullptr;

ObjectManager *ObjectManager::instance()
{
  if (manager == nullptr)
  {
    manager = new ObjectManager();
  }
  return manager;
}

Object *ObjectManager::getObject(std::string object, std::string type)
{
  std::string prefix = "res/geom/";
  bool usPrefix = true;
  if (object.find("/") != object.npos)
    usPrefix = false;

  Object *obj;
  {
    std::string oPath;
    if (usPrefix)
      oPath = prefix + object;
    else oPath = object;
    const auto v = cache.find(oPath);
    if (v != cache.end())
    {
      obj = objectFactory(new Object(v->second.get()), type);
    }
    else {
      obj = objectFactory(Object::load(oPath), type);
      cache[oPath] = obj->m_Mesh;
    }
    if (obj == nullptr)
    {
      cout << "Error of load object: " << oPath << endl;
    }
    else
    {
      cout << "Object [" << oPath <<"] loaded" << endl;
    }
  }
  return obj;
}

string ObjectManager::getPathByPointer(Object *object)
{
  for (auto &obj : cache)
  {
    if (obj.second == object->m_Mesh)
      return obj.first;
  }

  //TODO: handle missing pointer
  return string("");
}

Object *ObjectManager::objectFactory(Object *object, string type)
{
  if (type == "player")
    return new CPlayer(object);
  else if (type == "gameobject")
    return new GameObject(object);
  else return object;
}

