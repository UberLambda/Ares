#include "PhysModule.hh"

#include <glm/geometric.hpp>
#include <Core/Core.hh>
#include <Core/Debug/Log.hh>
#include <Core/Scene/Scene.hh>
#include <Core/Scene/SceneIterator.hh>

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
    ARES_log(glog, Trace, "Starting up Bullet Physics (version %d)", btGetVersion());

    broadphase_ = new btDbvtBroadphase();

    collisionConfig_ = new btDefaultCollisionConfiguration();
    collisionDispatcher_ = new btCollisionDispatcher(collisionConfig_);

    constraintSolver_ = new btSequentialImpulseConstraintSolver();

    dynamicsWorld_ = new btDiscreteDynamicsWorld(collisionDispatcher_, broadphase_,
                                                 constraintSolver_, collisionConfig_);
    dynamicsWorld_->setGravity(DEFAULT_GRAVITY);

    // Allocate a `CompStore` for the internal `PhysDataComp`s.
    (void)core.g().scene->storeFor<PhysDataComp>();

    ARES_log(glog, Debug, "PhysModule online");
    return true;
}


/// Returns the minimum value of each one in the vector.
template <typename T>
inline constexpr T vec3min(glm::tvec3<T> vec)
{
    return glm::min(glm::min(vec.x, vec.y), vec.z);
}

btCollisionShape* PhysModule::createBulletCollisionShape(const CollisionShape& collisionShape)
{
    switch(collisionShape.type)
    {

    case CollisionShape::Box:
    {
        return new btBoxShape({collisionShape.size.x * 0.5f,
                               collisionShape.size.y * 0.5f,
                               collisionShape.size.z * 0.5f});
    }

    case CollisionShape::Sphere:
    {
        float radius = vec3min(collisionShape.size) * 0.5f;
        return new btSphereShape(radius);
    }

    case CollisionShape::Cylinder:
    {
        float baseRadius = glm::min(collisionShape.size.x, collisionShape.size.z) * 0.5f;
        float halfHeight = collisionShape.size.y * 0.5f;
        return new btCylinderShape({baseRadius, baseRadius, halfHeight});
    }

    case CollisionShape::Capsule:
    {
        // NOTE Bullet capsules' heights do not include the top + bottom sphere
        //      radii, but Ares capsules' `size.y`s do!
        float baseRadius = glm::min(collisionShape.size.x, collisionShape.size.z) * 0.5f;
        float height = collisionShape.size.y - baseRadius * 2.0f; // FIXME Could be a negative value!
        if(height < 0.0f)
        {
            // Invalid height, (2 * base radius) > size.y!
            return nullptr;
        }
        return new btCapsuleShape(baseRadius, height);
    }

    case CollisionShape::Cone:
    {
        float baseRadius = glm::min(collisionShape.size.x, collisionShape.size.z) * 0.5f;
        float height = collisionShape.size.y;
        return new btConeShape(baseRadius, height);
    }

    case CollisionShape::Plane:
    {
        btVector3 normal(collisionShape.normalDist.x,
                         collisionShape.normalDist.y,
                         collisionShape.normalDist.z);
        float constant = collisionShape.normalDist.w;
        return new btStaticPlaneShape(normal, constant);
    }

    case CollisionShape::ConvexHull:
    {
        if(collisionShape.data.size() == 0)
        {
            // No points in the convex hull
            return nullptr;
        }

        auto bulletHull = new btConvexHullShape();
        for(glm::vec4 point : collisionShape.data)
        {
            bulletHull->addPoint({point.x, point.y, point.z}, false);
        }
        bulletHull->recalcLocalAabb();
        return bulletHull;
    }

    case CollisionShape::Spheres:
    {
        size_t n = collisionShape.data.size();
        if(n == 0)
        {
            // No spheres in the collision shape
            return nullptr;
        }

        // TODO Find a way not to allocate these temp buffers and use
        //      `collisionShape.data` directly instead
        auto centers = new btVector3[n];
        auto radii = new btScalar[n];
        for(size_t i = 0; i < n; i ++)
        {
            glm::vec4 sphere = collisionShape.data[i];
            centers[i] = {sphere.x, sphere.y, sphere.z};
            radii[i] = sphere.w;
        }

        auto bulletShape = new btMultiSphereShape(centers, radii, 0);

        delete[] centers;
        delete[] radii;
        return bulletShape;
    }

    default:
    {
        // Unimplemented Ares shape
        return nullptr;
    }

    }
}

PhysModule::PhysDataComp* PhysModule::addPhysDataComp(EntityRef entity)
{
    auto transform = entity.comp<TransformComp>();
    auto rigidBody = entity.comp<RigidBodyComp>();

    if(!transform || !rigidBody)
    {
        // Error: this entity does not have a transform or rigid body!
        return nullptr;
    }

    // Get a reference or instantiate the Bullet collision shape corresponding to the Ares one
    Ref<btCollisionShape>& bulletCollisionShape = collisionShapeMap_[rigidBody->collisionShape];
    if(!bulletCollisionShape)
    {
        // It's the first time we are using this Ares collision shape for an entity,
        // create its Bullet counterpart
        btCollisionShape* newBulletCollisionShape = createBulletCollisionShape(*rigidBody->collisionShape);
        bulletCollisionShape = intoRef<btCollisionShape>(newBulletCollisionShape);
    }
    // Else this collision shape has already been used for another entity and so
    // we have the respective Bullet one ready

    btScalar bulletMass = rigidBody->type == RigidBodyComp::Dynamic
                          ? rigidBody->mass
                          : 0; // Static and kinematic rigid bodies have 0 mass in Bullet

    btVector3 bulletLocalInertia;
    bulletCollisionShape->calculateLocalInertia(bulletMass, bulletLocalInertia);

    // Add the new `PhysDataComp`
    PhysDataComp* physDataComp = entity.setComp<PhysDataComp>({});

    physDataComp->bulletCollisionShape = bulletCollisionShape; // (increases refcount)

    // Setup the motion state that will sync `TransformComp`s <=> Bullet's transforms
    // for active Bullet rigid bodies
    physDataComp->bulletMotionState = PhysMotionState(entity);

    btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyInfo(
                bulletMass,
                &physDataComp->bulletMotionState,
                physDataComp->bulletCollisionShape.get(),
                bulletLocalInertia);
    // NOTE: `&physDataComp->bulletMotionState` works because an entity's `PhysDataComp`
    //       is heap-allocated and its address will not change throughout the program;
    //       hence, its `bulletMotionState` field's address will not change, too

    bulletRigidBodyInfo.m_friction = rigidBody->props.friction;
    bulletRigidBodyInfo.m_rollingFriction = rigidBody->props.rollingFriction;
    bulletRigidBodyInfo.m_restitution = rigidBody->props.restitution;
    bulletRigidBodyInfo.m_linearDamping = rigidBody->props.angularDamping;
    bulletRigidBodyInfo.m_angularDamping = rigidBody->props.angularDamping;

    physDataComp->bulletRigidBody = new btRigidBody(bulletRigidBodyInfo);
    if(rigidBody->type == RigidBodyComp::Kinematic)
    {
       int bulletCollisionFlags = physDataComp->bulletRigidBody->getCollisionFlags();
       bulletCollisionFlags |= btRigidBody::CF_KINEMATIC_OBJECT;
       physDataComp->bulletRigidBody->setCollisionFlags(bulletCollisionFlags);
    }

    // Mark the Bullet data as synced to the current `rigidBody`'s values
    physDataComp->cachedRigidBody = *rigidBody;

    return physDataComp;
}

void PhysModule::updatePhysDataComp(EntityRef entity)
{
    bool massPropsChanged = false; // `true` if mass and/or inertia of the object changed

    auto rigidBodyComp = entity.comp<RigidBodyComp>();
    auto physDataComp = entity.comp<PhysDataComp>();

    if(rigidBodyComp)
    {
        if(physDataComp)
        {
            // The entity has both an Ares RigidBody and Bullet data
            // Check if the two are in sync; if they are not, sync them
            if(physDataComp->cachedRigidBody.collisionShape != rigidBodyComp->collisionShape)
            {
                // Collision shape for the entity changed; update it and its Bullet
                // counterpart in the entity data

                Ref<btCollisionShape>& bulletCollisionShape = collisionShapeMap_[rigidBodyComp->collisionShape];
                if(!bulletCollisionShape)
                {
                    // It's the first time we are using this Ares collision shape for an entity,
                    // create its Bullet counterpart
                    btCollisionShape* newBulletCollisionShape = createBulletCollisionShape(*rigidBodyComp->collisionShape);
                    bulletCollisionShape = intoRef<btCollisionShape>(newBulletCollisionShape);
                }
                // Else this collision shape has already been used for another entity and so
                // we have the respective Bullet one ready

                // Need to recalculate inertia since the shape changed
                massPropsChanged = true;

                // Update the Bullet collision shape
                physDataComp->bulletCollisionShape = bulletCollisionShape;
                physDataComp->bulletRigidBody->setCollisionShape(bulletCollisionShape.get());

                // Mark the Ares and Bullet collision shapes as synced
                physDataComp->cachedRigidBody.collisionShape = rigidBodyComp->collisionShape;
            }

            if(physDataComp->cachedRigidBody.mass != rigidBodyComp->mass)
            {
                // Need to recalculate inertia because the mass changed; this is needed
                // to apply the new mass to the rigid body
                massPropsChanged = true;

                // Mark the Ares and Bullet masses as synced
                physDataComp->cachedRigidBody.mass = rigidBodyComp->mass;
            }

            // FIXME IMPLEMENT IMPORTANT Check if any of `rigidBody.props` changed; apply
            //       each of them that changed to the Bullet rigid body here


            if(massPropsChanged)
            {
                // Apply new mass and inertia to the Bullet rigid body
                btScalar bulletMass = rigidBodyComp->type == RigidBodyComp::Dynamic
                                      ? rigidBodyComp->mass
                                      : 0; // Static and kinematic rigid bodies have 0 mass in Bullet
                btVector3 bulletLocalInertia;
                physDataComp->bulletCollisionShape->calculateLocalInertia(bulletMass, bulletLocalInertia);

                physDataComp->bulletRigidBody->setMassProps(physDataComp->cachedRigidBody.mass, bulletLocalInertia);
            }
        }
        else
        {
            // The entity has had a rigid body added to it but no Bullet data yet.

            // Create the Bullet data
            physDataComp = addPhysDataComp(entity);

            // Add the rigid body to the dynamics world so that Bullet will start
            // simulating the entity next frame
            dynamicsWorld_->addRigidBody(physDataComp->bulletRigidBody);
        }
    }
    else
    {
        // The entity does not have Ares rigid body data

        if(physDataComp)
        {
            // The entity still has Bullet data but does not have a rigid body anymore;
            // remove the Bullet rigid body from the simulation
            dynamicsWorld_->removeRigidBody(physDataComp->bulletRigidBody);

            delete physDataComp->bulletRigidBody; physDataComp->bulletRigidBody = nullptr;
            entity.erase<PhysDataComp>();
        }
    }
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

void PhysModule::mainUpdate(Core& core)
{
}

Task PhysModule::updateTask(Core& core)
{
    core_ = &core;

    static const auto updateFunc = [](TaskScheduler* scheduler, void* data)
    {
        // Step the world
        auto physMod = reinterpret_cast<PhysModule*>(data);
        float interpFactor = physMod->stepWorld();
        // FIXME IMPLEMENT Use the interpolation factor to interp between current
        //       and past state, then apply transforms and velocities to entities
        //       with physics components that are simulated in the dynamics world

        Scene* scene = physMod->core_->g().scene;
        for(auto it = scene->begin(); it != scene->end(); it ++)
        {
            // Check if Bullet data has to be created/deleted/synced with Ares'
            physMod->updatePhysDataComp(*it);
        }
    };

    // Bullet's 3D transforms will automatically be applied to Ares' `TransformComp`s
    // thanks to `PhysMotionState`

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
