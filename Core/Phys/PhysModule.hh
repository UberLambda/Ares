#pragma once

#include <chrono>
#include <btBulletDynamicsCommon.h>
#include <Core/Api.h>
#include <Core/Module/Module.hh>

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
    btBroadphaseInterface* broadphase_;
    btCollisionConfiguration* collisionConfig_;
    btCollisionDispatcher* collisionDispatcher_;
    btConstraintSolver* constraintSolver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;

    Seconds dt_; ///< By how much time to (ideally; see `updateTask()` impl)
                 ///  advance the physics simulation each update cycle.
    Clock::time_point tLastUpdate_; ///< The time When the last simulation step was started.
    Seconds updateTimeAccumulator_; ///< (See `updateTask()` impl)


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
