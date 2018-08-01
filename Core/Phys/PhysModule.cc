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
    // Resize `enitiesData_` to fit as many entities as there are in the scene
    // (plus dummy data for entity 0)
    // TODO This can be done here since the number of entities in the scene is
    //      constant, but one day it could potentially increase at runtime; think
    //      of a way to solve this issue
    entitiesData_.resize(core.g().scene->maxEntities() + 1);

    ARES_log(glog, Trace, "Starting up Bullet Physics (version %d)", btGetVersion());

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

PhysModule::EntityData PhysModule::createEntityData(const TransformComp& transform,
                                                    const RigidBodyComp& rigidBody)
{
    // Get a reference or instantiate the Bullet collision shape corresponding to the Ares one
    Ref<btCollisionShape>& bulletCollisionShape = collisionShapeMap_[rigidBody.collisionShape];
    if(!bulletCollisionShape)
    {
        // It's the first time we are using this Ares collision shape for an entity,
        // create its Bullet counterpart
        btCollisionShape* newBulletCollisionShape = createBulletCollisionShape(*rigidBody.collisionShape);
        bulletCollisionShape = intoRef<btCollisionShape>(newBulletCollisionShape);
    }
    // Else this collision shape has already been used for another entity and so
    // we have the respective Bullet one ready

    // Create the Bullet motion state from the initial Ares transform and then
    // create the corresponding Bullet rigid body
    btQuaternion bulletRot(transform.rotation.x,
                           transform.rotation.y,
                           transform.rotation.z,
                           transform.rotation.w);
    btVector3 bulletCenter(transform.position.x,
                           transform.position.y,
                           transform.position.z);
    btTransform bulletTransform(bulletRot, bulletCenter);

    btScalar bulletMass = rigidBody.type == RigidBodyComp::Dynamic
                          ? rigidBody.mass
                          : 0; // Static and kinematic rigid bodies have 0 mass in Bullet

    btVector3 bulletLocalInertia;
    bulletCollisionShape->calculateLocalInertia(bulletMass, bulletLocalInertia);

    EntityData data;
    data.rigidBody = rigidBody; // (copies references)
    data.bulletCollisionShape = bulletCollisionShape; // (increases refcount)
    data.bulletMotionState = new btDefaultMotionState(bulletTransform);

    btRigidBody::btRigidBodyConstructionInfo bulletRigidBodyInfo(
                bulletMass, data.bulletMotionState, data.bulletCollisionShape.get(),
                bulletLocalInertia);
    bulletRigidBodyInfo.m_friction = rigidBody.props.friction;
    bulletRigidBodyInfo.m_rollingFriction = rigidBody.props.rollingFriction;
    bulletRigidBodyInfo.m_restitution = rigidBody.props.restitution;
    bulletRigidBodyInfo.m_linearDamping = rigidBody.props.angularDamping;
    bulletRigidBodyInfo.m_angularDamping = rigidBody.props.angularDamping;

    auto bulletRigidBody = data.bulletRigidBody = new btRigidBody(bulletRigidBodyInfo);

    if(rigidBody.type == RigidBodyComp::Kinematic)
    {
       int bulletCollisionFlags = bulletRigidBody->getCollisionFlags();
       bulletCollisionFlags |= btRigidBody::CF_KINEMATIC_OBJECT;
       bulletRigidBody->setCollisionFlags(bulletCollisionFlags);
    }

    return data;
}

void PhysModule::updateEntityData(PhysModule::EntityData& entityData,
                                  const RigidBodyComp& rigidBody)
{
    bool massPropsChanged = false; // `true` if mass and/or inertia of the object changed

    if(entityData.rigidBody.collisionShape != rigidBody.collisionShape)
    {
        // Collision shape for the entity changed; update it and its Bullet
        // counterpart in the entity data

        Ref<btCollisionShape>& bulletCollisionShape = collisionShapeMap_[rigidBody.collisionShape];
        if(!bulletCollisionShape)
        {
            // It's the first time we are using this Ares collision shape for an entity,
            // create its Bullet counterpart
            btCollisionShape* newBulletCollisionShape = createBulletCollisionShape(*rigidBody.collisionShape);
            bulletCollisionShape = intoRef<btCollisionShape>(newBulletCollisionShape);
        }
        // Else this collision shape has already been used for another entity and so
        // we have the respective Bullet one ready

        // Need to recalculate inertia since the shape changed
        massPropsChanged = true;

        entityData.bulletRigidBody->setCollisionShape(bulletCollisionShape.get());

        entityData.rigidBody.collisionShape = rigidBody.collisionShape;
        entityData.bulletCollisionShape = bulletCollisionShape;
    }

    if(entityData.rigidBody.mass != rigidBody.mass)
    {
        // Need to recalculate inertia because the mass changed and to apply the
        // new mass to the rigid body
        massPropsChanged = true;

        entityData.rigidBody.mass = rigidBody.mass;
    }

    // FIXME IMPLEMENT IMPORTANT Check if any of `rigidBody.props` changed; apply
    //       each of them that changed to the Bullet rigid body here


    if(massPropsChanged)
    {
        // Apply new mass and inertia to the Bullet rigid body
        btScalar bulletMass = rigidBody.type == RigidBodyComp::Dynamic
                              ? rigidBody.mass
                              : 0; // Static and kinematic rigid bodies have 0 mass in Bullet
        btVector3 bulletLocalInertia;
        entityData.bulletCollisionShape->calculateLocalInertia(bulletMass, bulletLocalInertia);

        entityData.bulletRigidBody->setMassProps(entityData.rigidBody.mass, bulletLocalInertia);
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
            auto transform = it->comp<TransformComp>();
            if(!transform)
            {
                // An entity needs a transform to be affected by physics
                continue;
            }

            EntityData& entityData = physMod->entitiesData_[it->id()];

            auto rigidBody = it->comp<RigidBodyComp>();
            if(!rigidBody)
            {
                if(entityData.bulletRigidBody != nullptr)
                {
                    // This entity used to have a Ares rigid body that now has
                    // been deleted, but its Bullet counterpart is still alive.
                    // Destroy the Bullet rigid body as it is not needed anymore.
                    physMod->dynamicsWorld_->removeRigidBody(entityData.bulletRigidBody);
                    (void)entityData.~EntityData(); // Delete the Bullet motion state and rigid body
                    (void)entityData.rigidBody.collisionShape.~Ref(); // (for potential cleanup)
                    (void)entityData.bulletCollisionShape.~Ref(); // (for potential cleanup)
                }
                continue;
            }

            if(entityData.bulletRigidBody == nullptr)
            {
                // The entity with this id has a `RigidBodyComp` but no actual Bullet
                // collision shape, motion state or rigid body associated to it.
                entityData = physMod->createEntityData(*transform, *rigidBody);

                // Add the rigid body now so that Bullet will start simulating the
                // entity next frame
                physMod->dynamicsWorld_->addRigidBody(entityData.bulletRigidBody);
            }

            // Check if `RigidBodyComp` properties changed; if they did, apply them
            physMod->updateEntityData(entityData, *rigidBody);

            // Apply the Bullet transform to the Ares TransformComp
            btTransform bulletEntityTransform;
            entityData.bulletMotionState->getWorldTransform(bulletEntityTransform);

            btVector3 bulletPos = bulletEntityTransform.getOrigin();
            transform->position = {bulletPos.x(), bulletPos.y(), bulletPos.z()};

            btQuaternion bulletRot = bulletEntityTransform.getRotation();
            transform->rotation = {bulletRot.x(), bulletRot.y(), bulletRot.z(), bulletRot.w()};
        }
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
