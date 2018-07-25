#include "Scene.hh"

#include <Core/Scene/EntityRef.hh>
#include <Core/Scene/SceneIterator.hh>

namespace Ares
{

Scene::Scene(size_t maxEntities)
    : maxEntities_(maxEntities)
{
}

Scene::~Scene()
{
}


EntityRef Scene::ref(EntityId entity)
{
    return EntityRef(this, entity);
}

bool Scene::has(EntityId entity)
{
    std::lock_guard<std::mutex> compStoresScopedLock(compStoresLock_);

    bool present = false;
    for(auto it = compStores_.begin(); it != compStores_.end(); it ++)
    {
        present |= it->second->has(entity);
    }
    return present;
}

void Scene::erase(EntityId entity)
{
    std::lock_guard<std::mutex> compStoresScopedLock(compStoresLock_);

    for(auto it = compStores_.begin(); it != compStores_.end(); it ++)
    {
        it->second->erase(entity);
    }
}


Scene::iterator Scene::begin()
{
    return iterator(this, 0);
}

Scene::iterator Scene::end()
{
    return iterator(this, maxEntities_); // (one-past-the-end)
}


}
