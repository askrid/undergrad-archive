from pyglet.graphics.shader import Shader, ShaderProgram

# Unlit passthrough: vertex color only
vertex_source_default = """
#version 330
layout(location =0) in vec3 vertices;
layout(location =1) in vec4 colors;

out vec4 newColor;

uniform mat4 view_proj;
uniform mat4 model;

void main()
{
    gl_Position = view_proj * model * vec4(vertices, 1.0f);
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

# Gouraud: per-vertex ambient + Lambert diffuse with point light attenuation.
# mat3(model) is safe for normals since we only use rotation+translation.
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

fragment_source_lit = fragment_source_default


def create_program(vs_source, fs_source):
    return ShaderProgram(Shader(vs_source, "vertex"), Shader(fs_source, "fragment"))


def create_lit_program():
    return create_program(vertex_source_lit, fragment_source_lit)


def create_unlit_program():
    return create_program(vertex_source_default, fragment_source_default)
