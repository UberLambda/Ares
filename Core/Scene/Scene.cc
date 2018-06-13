#include "Scene.hh"

namespace Ares
{

Scene::Scene(size_t nEntities)
    : nEntities_(nEntities)
{
}

Scene::~Scene()
{
}


void Scene::erase(Entity entity)
{
    for(auto it = compStores_.begin(); it != compStores_.end(); it ++)
    {
        it->second->erase(entity);
    }
}

bool Scene::has(Entity entity) const
{
    for(auto it = compStores_.begin(); it != compStores_.end(); it ++)
    {
        if(it->second->has(entity))
        {
            return true;
        }
    }
    return false;
}

}
