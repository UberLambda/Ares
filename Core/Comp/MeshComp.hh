#pragma once

#include "../Base/Ref.hh"
#include "../Resource/Mesh.hh"
#include "../Resource/Material.hh"

namespace Ares
{

/// A triangle mesh attached to an entity for rendering.
/// The mesh's origin will match the entity's.
struct MeshComp
{
    /// A reference to the mesh's data.
    Ref<Mesh> mesh;

    /// The gfx material the mesh is made of.
    Ref<Material> material;
};

}
