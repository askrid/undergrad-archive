from pyglet.graphics.shader import Shader, ShaderProgram


# ---- Flat shader: pos + color only. Used for lines (cage) and points. ----
vertex_source_default = """
#version 330
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec4 colors;

out vec4 newColor;

uniform mat4 view_proj;
uniform mat4 model;

void main()
{
    gl_Position = view_proj * model * vec4(vertices, 1.0);
    gl_PointSize = 10.0;
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


# ---- Gouraud shader: per-vertex Phong lighting computed in vertex stage. ----
# Material diffuse color comes from per-vertex `colors`; ambient and specular
# scale that base color by uniform coefficients. Single point light with
# distance attenuation 1/(a + b*d + c*d^2).
vertex_source_gouraud = """
#version 330
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec4 colors;
layout(location = 2) in vec3 normals;

out vec4 newColor;

uniform mat4 view_proj;
uniform mat4 model;

uniform vec3 light_pos;
uniform vec3 light_color;
uniform vec3 eye_pos;

uniform float k_a;
uniform float k_d;
uniform float k_s;
uniform float shininess;

uniform float atten_a;
uniform float atten_b;
uniform float atten_c;

void main()
{
    vec4 world_pos4 = model * vec4(vertices, 1.0);
    vec3 P = world_pos4.xyz;

    // Transform normal by model (assumes uniform scale; OK here).
    vec3 N = normalize(mat3(model) * normals);
    vec3 L = light_pos - P;
    float d = length(L);
    L = L / max(d, 1e-6);
    vec3 V = normalize(eye_pos - P);
    vec3 R = reflect(-L, N);

    float atten = 1.0 / (atten_a + atten_b * d + atten_c * d * d);

    vec3 base = colors.rgb;
    vec3 ambient  = k_a * base;
    vec3 diffuse  = k_d * base * max(dot(N, L), 0.0);
    vec3 specular = k_s * vec3(1.0) * pow(max(dot(R, V), 0.0), shininess);

    vec3 lit = ambient + atten * light_color * (diffuse + specular);
    newColor = vec4(lit, colors.a);
    gl_Position = view_proj * world_pos4;
}
"""

fragment_source_gouraud = fragment_source_default


def create_program(vs_source, fs_source):
    vert_shader = Shader(vs_source, "vertex")
    frag_shader = Shader(fs_source, "fragment")
    return ShaderProgram(vert_shader, frag_shader)
