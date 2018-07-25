#pragma once

#include <Core/Api.h>
#include <Core/Base/Ref.hh>
#include <Core/Resource/Mesh.hh>
#include <Core/Resource/Material.hh>

namespace Ares
{

/// A triangle mesh attached to an entity for rendering.
/// The mesh's origin will match the entity's.
struct ARES_API MeshComp
{
    /// A reference to the mesh's data.
    Ref<Mesh> mesh;

    /// The gfx material the mesh is made of.
    Ref<Material> material;
};

}
