from pyglet.graphics.shader import Shader, ShaderProgram

# create vertex and fragment shader sources
vertex_source_default = """
#version 330
layout(location =0) in vec3 vertices;
layout(location =1) in vec4 colors;

out vec4 newColor;

// add a view-projection uniform and multiply it by the vertices
uniform mat4 view_proj;
uniform mat4 model;

void main()
{
    gl_Position = view_proj * model * vec4(vertices, 1.0f); // local->world->vp
    newColor = colors;
}
"""

fragment_source_default = """
#version 330
in vec4 newColor;

out vec4 outColor;

void main()
{
    outColor = newColor;
}
"""


# --- Gouraud lit shader: point light, ambient + Lambert diffuse, no specular ---
# Lighting computed per-vertex in the vertex shader and smoothly interpolated
# via the `out vec4 newColor` varying -- that interpolation is what makes this
# Gouraud shading. The fragment shader is pure passthrough.
vertex_source_lit = """
#version 330
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec4 colors;
layout(location = 2) in vec3 normals;

out vec4 newColor;

uniform mat4 view_proj;
uniform mat4 model;

uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 ambient;
uniform float light_const;
uniform float light_linear;
uniform float light_quad;

void main()
{
    vec4 world_pos4 = model * vec4(vertices, 1.0);
    vec3 world_pos  = world_pos4.xyz;
    // Model matrices in this project are rotation+translation only (no
    // non-uniform scale), so mat3(model) rotates normals correctly without
    // needing the inverse-transpose.
    vec3 N = normalize(mat3(model) * normals);

    vec3 Lvec  = light_pos - world_pos;
    float dist = length(Lvec);
    vec3 L     = Lvec / dist;

    float atten = 1.0 / (light_const + light_linear * dist + light_quad * dist * dist);
    float ndotl = max(dot(N, L), 0.0);

    vec3 base = colors.rgb;
    vec3 lit  = base * (ambient + light_color * ndotl * atten);

    newColor    = vec4(lit, colors.a);
    gl_Position = view_proj * world_pos4;
}
"""

# The lit fragment shader is identical to the unlit one -- it only interpolates
# the per-vertex color. Reused via the same source.
fragment_source_lit = fragment_source_default


def create_program(vs_source, fs_source):
    # compile the vertex and fragment sources to a shader program
    vert_shader = Shader(vs_source, "vertex")
    frag_shader = Shader(fs_source, "fragment")
    return ShaderProgram(vert_shader, frag_shader)


def create_lit_program():
    return create_program(vertex_source_lit, fragment_source_lit)


def create_unlit_program():
    return create_program(vertex_source_default, fragment_source_default)
