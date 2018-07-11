# Ares shader (.arsh) file 
Describes a shader program. Uses `Config` syntax.

## [shader] section
**Required?** **Yes**

Shader source code uses GLSL 330 core syntax.  

Inside of this section, source code for atleast one of any of these
shader types must be defined:

- vert: Vertex shader.
- frag: Fragment shader.
- geom: Geometry shader.
- tes: Tessellation evaluation shader.
- tec: Tessellation control shader.

If the source code for a shader is stored directly in a string value
it should be stored in a key named `${type}.src`.  
If the source code for a shader is stored in another resource file
the path to that file should be stored in a key named `${type}`.
The path can either be relative to the .arsh file or absolute.
