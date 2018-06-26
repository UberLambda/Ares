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

## [shaders.<api>] section[s]
**Required?** No

Used to store shaders or references to shaders to use to render the material on a certain graphics API.  
<api> should be one of:

- `gl33`: OpenGL 3.3 core (`#version 330 core`)

Unrecognized <api>s are ignored.

| Key | Type | Required? | Description |
|:-:|:-:|:-:|:-:|
| vertSrc | string | No | The source code of a vertex shader for the given API. |
| fragSrc | string | No | The source code of a fragment shader for the given API. |
| geomSrc | string | No | The source code of a geometry shader for the given API. |
