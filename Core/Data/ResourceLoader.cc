#include "ResourceLoader.hh"

#include <assert.h>

namespace Ares
{

ResourceLoader::ResourceLoader(FileStore* fileStore)
    : fileStore_(fileStore)
{
}

ResourceLoader::~ResourceLoader()
{
    cleanup();
}

size_t ResourceLoader::cleanup()
{
    std::lock_guard<std::mutex> allStoresLock(resourceStoresLock_);

    size_t nCleaned = 0;
    for(auto it = resourceStores_.begin(); it != resourceStores_.end(); it ++)
    {
        auto store = reinterpret_cast<ResourceStoreBase*>(it->second);
        nCleaned += store->cleanup();
    }

    return nCleaned;
}

}
