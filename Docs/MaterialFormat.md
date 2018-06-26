# Ares material (.armat) file 
Describes a material, i.e. the shaders, textures and other effects applied to a renderable object.
Uses `Config` syntax.

## [material] section
**Required?** **Yes**

Describes common properties of the material.

| Key | Type | Required? | Description |
|:-:|:-:|:-:|:-:|
| name | string | **Yes** | A short name for the material. |
| descr | string | No | A short description of the material. |

## [shaders] section
**Required?** No

Used to store shaders or references to the shader program used to render the material.
Shaders use GLSL 330 core syntax.  
If the whole section or any of the shader sources are missing defaults are used
in their place.

| Key | Type | Required? | Description |
|:-:|:-:|:-:|:-:|
| vertSrc | string | No | The source code of the vertex shader. |
| fragSrc | string | No | The source code of the fragment shader. |
| geomSrc | string | No | The source code of the geometry shader. |
