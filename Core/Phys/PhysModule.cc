#include "PhysModule.hh"

#include <Core/Core.hh>
#include <Core/Debug/Log.hh>

namespace Ares
{

/// The default gravity of the newly-created dynamics world.
static const btVector3 DEFAULT_GRAVITY{0.0f, -9.81f, 0.0f};

constexpr const PhysModule::Seconds PhysModule::MAX_PERUPDATE_TIME;

PhysModule::PhysModule()
    : broadphase_(nullptr), collisionConfig_(nullptr), collisionDispatcher_(nullptr),
      constraintSolver_(nullptr), dynamicsWorld_(nullptr),
      dt_(1.0f / 60.0f)
{
}

PhysModule::~PhysModule()
{
}

#define glog (*core.g().log)

bool PhysModule::init(Core& core)
{
    broadphase_ = new btDbvtBroadphase();

    collisionConfig_ = new btDefaultCollisionConfiguration();
    collisionDispatcher_ = new btCollisionDispatcher(collisionConfig_);

    constraintSolver_ = new btSequentialImpulseConstraintSolver();

    dynamicsWorld_ = new btDiscreteDynamicsWorld(collisionDispatcher_, broadphase_,
                                                 constraintSolver_, collisionConfig_);
    dynamicsWorld_->setGravity(DEFAULT_GRAVITY);

    ARES_log(glog, Debug, "PhysModule online");
    return true;
}

void PhysModule::mainUpdate(Core& core)
{
}

float PhysModule::stepWorld()
{
    // See: the infamous https://gafferongames.com/post/fix_your_timestep/ article

    auto tNow = Clock::now();
    auto dtUpdate = std::chrono::duration_cast<Seconds>(tNow - tLastUpdate_);

    if(dtUpdate > MAX_PERUPDATE_TIME)
    {
        dtUpdate = MAX_PERUPDATE_TIME;
    }
    tLastUpdate_ = tNow;

    updateTimeAccumulator_ += dtUpdate;
    while(updateTimeAccumulator_ >= dt_)
    {
        dynamicsWorld_->stepSimulation(dt_.count(), 1, dt_.count());
        updateTimeAccumulator_ -= dt_;
    }

    float interpFactor = updateTimeAccumulator_ / dt_;
    return interpFactor;
}

Task PhysModule::updateTask(Core& core)
{
    static const auto updateFunc = [](TaskScheduler* scheduler, void* data)
    {
        auto physMod = reinterpret_cast<PhysModule*>(data);
        float interpFactor = physMod->stepWorld();
        // FIXME IMPLEMENT Use the interpolation factor to interp between current
        //       and past state, then apply transforms and velocities to entities
        //       with physics components that are simulated in the dynamics world
    };
    return {updateFunc, this};
}

void PhysModule::halt(Core& core)
{
    delete dynamicsWorld_; dynamicsWorld_ = nullptr;
    delete constraintSolver_; constraintSolver_ = nullptr;
    delete collisionDispatcher_; collisionDispatcher_ = nullptr;
    delete collisionConfig_; collisionConfig_ = nullptr;
    delete broadphase_; broadphase_ = nullptr;

    ARES_log(glog, Debug, "PhysModule offline");
}

}
