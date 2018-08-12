#pragma once

#include <btBulletDynamicsCommon.h>
#include <Core/Scene/EntityRef.hh>
#include <Core/Scene/Scene.hh>
#include <Core/Comp/TransformComp.hh>

namespace Ares
{

/// A `btMotionState` used by `PhysModule`.
class PhysMotionState : public btMotionState
{
    mutable EntityRef entity_;

public:
    /// Creates a new motion state that is not associated to any entity.
    PhysMotionState()
        : entity_(nullptr)
    {
    }

    /// Creates a new motion state for the given entity.
    PhysMotionState(EntityRef entity)
        : entity_(entity)
    {
    }


    inline void getWorldTransform(btTransform& centerOfMassWorldTrans) const override
    {
        if(!entity_)
        {
            // This motion state is not associated to any entity.
            return;
        }

        TransformComp* transform = entity_.comp<TransformComp>();
        if(!transform)
        {
            // Can't query the transform of an entity without a transform comp!
            return;
        }

        btVector3 bulletPos(transform->position.x,
                            transform->position.y,
                            transform->position.z);
        btQuaternion bulletRot(transform->rotation.x,
                               transform->rotation.y,
                               transform->rotation.z,
                               transform->rotation.w);
        centerOfMassWorldTrans.setOrigin(bulletPos);
        centerOfMassWorldTrans.setRotation(bulletRot);
    }

    inline void setWorldTransform(const btTransform& centerOfMassWorldTrans) override
    {
        if(!entity_)
        {
            // This motion state is not associated to any entity.
            return;
        }

        TransformComp* transform = entity_.comp<TransformComp>();
        if(!transform)
        {
            // Can't change the transform of an entity without a transform comp!
            return;
        }

        btVector3 bulletPos = centerOfMassWorldTrans.getOrigin();
        btQuaternion bulletRot = centerOfMassWorldTrans.getRotation();
        transform->position = {bulletPos.x(), bulletPos.y(), bulletPos.z()};
        transform->rotation = {bulletRot.x(), bulletRot.y(), bulletRot.z(), bulletRot.w()};
    }
};

}
