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


# ---- Phong shader: per-fragment lighting. ----
# Material diffuse color comes from per-vertex `colors`; ambient and specular
# scale that base color by uniform coefficients. Single point light with
# distance attenuation 1/(a + b*d + c*d^2).
vertex_source_phong = """
#version 330
layout(location = 0) in vec3 vertices;
layout(location = 1) in vec4 colors;
layout(location = 2) in vec3 normals;

out vec3 fragPos;
out vec3 fragNormal;
out vec4 fragColor;

uniform mat4 view_proj;
uniform mat4 model;

void main()
{
    vec4 world_pos4 = model * vec4(vertices, 1.0);
    fragPos = world_pos4.xyz;
    fragNormal = mat3(model) * normals;
    fragColor = colors;
    gl_Position = view_proj * world_pos4;
}
"""

fragment_source_phong = """
#version 330
in vec3 fragPos;
in vec3 fragNormal;
in vec4 fragColor;

out vec4 outColor;

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
    vec3 N = normalize(fragNormal);
    vec3 L = light_pos - fragPos;
    float d = length(L);
    L = L / max(d, 1e-6);
    vec3 V = normalize(eye_pos - fragPos);
    vec3 R = reflect(-L, N);

    float atten = 1.0 / (atten_a + atten_b * d + atten_c * d * d);

    vec3 base = fragColor.rgb;
    vec3 ambient  = k_a * base;
    vec3 diffuse  = k_d * base * max(dot(N, L), 0.0);
    vec3 specular = k_s * vec3(1.0) * pow(max(dot(R, V), 0.0), shininess);

    vec3 lit = ambient + atten * light_color * (diffuse + specular);
    outColor = vec4(lit, fragColor.a);
}
"""


def create_program(vs_source, fs_source):
    vert_shader = Shader(vs_source, "vertex")
    frag_shader = Shader(fs_source, "fragment")
    return ShaderProgram(vert_shader, frag_shader)
