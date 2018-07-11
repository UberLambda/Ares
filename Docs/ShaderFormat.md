# Ares shader (.arsh) file 
Describes a shader program.  

.arsh files use JSON syntax, while the shader sources themselves are GLSL 330 core.  

The root of the JSON should be an object containing atleast one of these keys:

- `"vert"`: Vertex shader.
- `"frag"`: Fragment shader.
- `"geom"`: Geometry shader.
- `"tes"`: Tessellation evaluation shader.
- `"tec"`: Tessellation control shader.

Each defined key must point to a string value. Each value contains a path to
a resource file containing source code for the respective shader.  
The paths can either be relative to the .arsh file or absolute.
