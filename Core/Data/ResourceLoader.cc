#include "ResourceLoader.hh"

#include <assert.h>
#include "ResourceRef.hh"

namespace Ares
{

ResourceLoader::ResourceLoader(FileStore* fileStore)
    : fileStore_(fileStore), nextResId_(0)
{
}

ResourceLoader::~ResourceLoader()
{
    size_t nStoredRes = resMap_.size();
    size_t nCleanedRes = cleanup();
    assert(nCleanedRes == nStoredRes && "cleanup() did not free all resources on exit!");
}

size_t ResourceLoader::cleanup()
{
    std::lock_guard<std::mutex> resMapScopedLock(resMapLock_);
    std::lock_guard<std::mutex> pathMapScopedLock(pathMapLock_);

    size_t nCleaned = 0;
    for(auto it = resMap_.begin(); it != resMap_.end();)
    {
        if(it->second.refCount == 0) // (atomic)
        {
            delete it->second.resource; // Note: `~TResHandler()` will automatically
                                        //       destroy its inner `T` here
            it = resMap_.erase(it);
            nCleaned ++;
        }
        else
        {
            it ++;
        }
    }
    return nCleaned;
}

}
