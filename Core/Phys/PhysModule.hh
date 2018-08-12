#pragma once

#include <vector>
#include <unordered_map>
#include <chrono>
#include <utility>
#include <btBulletDynamicsCommon.h>
#include <Core/Api.h>
#include <Core/Module/Module.hh>
#include <Core/Scene/EntityId.hh>
#include <Core/Comp/TransformComp.hh>
#include <Core/Comp/RigidBodyComp.hh>
#include <Core/Phys/PhysMotionState.hh>

namespace Ares
{

/// A module used for simulating 3D physics for entities.
///
/// Implemented on top of Bullet Physics (3.x).
class ARES_API PhysModule : public Module
{
public:
    /// The `std::chrono` clock to use for timing calculations.
    using Clock = std::chrono::high_resolution_clock;

    /// A `std::chrono::duration` representing a fractional amount of seconds.
    using Seconds = std::chrono::duration<float>;

    /// The maximum amount of time to advance the physics simulation by in a single
    /// {`updateTask()`, `mainUpdate()`} cycle, in seconds.
    /// Used to guarantee that the physics simulation won't drop the framerate
    /// below a certain threshold when processing intensive scenes.
    static constexpr const Seconds MAX_PERUPDATE_TIME{0.25f};

private:
    Core* core_; ///< Set every time `updateTask` is called.

    btBroadphaseInterface* broadphase_;
    btCollisionConfiguration* collisionConfig_;
    btCollisionDispatcher* collisionDispatcher_;
    btConstraintSolver* constraintSolver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;

    /// A map of each Ares collision shape -> its Bullet counterpart
    std::unordered_map<Ref<CollisionShape>, Ref<btCollisionShape>> collisionShapeMap_;

    // TODO ^ Use a lockless map to be able to multithread collision shape creation

    /// An internal `Comp` to store Bullet data associated to an Ares entity.
    struct PhysDataComp
    {
        // NOTE `PhysDataComp` resides on the heap; no need to `new`
        //      `PhysMotionState` one!
        RigidBodyComp cachedRigidBody; ///< The `RigidBodyComp` as it was the last time
                                       ///  `updatePhysDataComp()` was called on this.

        Ref<btCollisionShape> bulletCollisionShape; // (from `collisionShapeMap_`)
        PhysMotionState bulletMotionState;
        btRigidBody* bulletRigidBody = nullptr; // If null this entity does not
                                                // have an active rigid body and
                                                // its `PhysDataComp` can be disposed of.
    };

    Seconds dt_; ///< By how much time to (ideally; see `updateTask()` impl)
                 ///  advance the physics simulation each update cycle.
    Clock::time_point tLastUpdate_; ///< The time When the last simulation step was started.
    Seconds updateTimeAccumulator_; ///< (See `updateTask()` impl)


    /// Attempts to create a Bullet collision shape from an Ares one. Returns null on error.
    static btCollisionShape* createBulletCollisionShape(const CollisionShape& collisionShape);

    /// Attachs an appropriate `PhysDataComp` to the given entity, which must
    /// already have a `TransformComp` and `RigidBodyComp`. Returns a pointer to
    /// the newly-added `PhysDataComp`.
    PhysDataComp* addPhysDataComp(EntityRef entity);

    /// Updates the `PhysDataComp` for an entity so that it will match what the
    /// `RigidBodyComp` expects from it.
    void updatePhysDataComp(EntityRef entity);

    /// Steps `dynamicsWorld_`'s simulation as many times as necessary to make
    /// the simulation catch up (but only up to `MAX_PERUPDATE_TIME` worth of
    /// simulated time). Returns the interpolation factor to use to interpolate
    /// between the current and past physics state.
    float stepWorld();

public:
    PhysModule();
    ~PhysModule() override;

    bool init(Core& core) override;
    void mainUpdate(Core& core) override;
    Task updateTask(Core& core) override;
    void halt(Core& core) override;
};

}
