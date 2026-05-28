"""Single uber shader. Mode uniform selects:
    0 wireframe, 1 Gouraud, 2 Phong, 3 Phong+textures, 4 Phong+normal map.
Up to 4 point lights.
"""

from pyglet.graphics.shader import Shader, ShaderProgram

MAX_LIGHTS = 4

_VS = """
#version 330
#define MAX_LIGHTS 4

layout(location = 0) in vec3 vertices;
layout(location = 1) in vec3 normals;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec3 tangents;

uniform mat4 view_proj;
uniform mat4 model;
uniform vec3 cam_pos;
uniform int  mode;
uniform int  num_lights;

struct Light { vec3 position; vec3 color; float intensity; };
uniform Light lights[MAX_LIGHTS];
uniform vec3 ambient_color;
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float shininess;

out vec3 v_pos;
out vec3 v_normal;
out vec2 v_uv;
out vec3 v_tangent;
out vec3 v_gouraud;

vec3 phong(vec3 P, vec3 N, vec3 V, vec3 ka, vec3 kd, vec3 ks, float n) {
    vec3 color = ka * ambient_color;
    for (int i = 0; i < num_lights; ++i) {
        vec3 L = lights[i].position - P;
        float dist = length(L);
        L /= max(dist, 1e-5);
        float atten = lights[i].intensity / (1.0 + 0.05 * dist + 0.01 * dist * dist);
        vec3 lc = lights[i].color * atten;
        float diff = max(dot(N, L), 0.0);
        vec3 R = reflect(-L, N);
        float spec = pow(max(dot(R, V), 0.0), max(n, 1.0));
        color += lc * (kd * diff + ks * spec);
    }
    return color;
}

void main() {
    vec4 world = model * vec4(vertices, 1.0);
    v_pos = world.xyz;
    v_normal = normalize(mat3(model) * normals);
    v_tangent = normalize(mat3(model) * tangents);
    v_uv = uvs;
    if (mode == 1) {
        v_gouraud = phong(v_pos, v_normal, normalize(cam_pos - v_pos), Ka, Kd, Ks, shininess);
    } else {
        v_gouraud = vec3(0.0);
    }
    gl_Position = view_proj * world;
}
"""

_FS = """
#version 330
#define MAX_LIGHTS 4

in vec3 v_pos;
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_tangent;
in vec3 v_gouraud;

uniform int  mode;
uniform int  num_lights;
uniform vec3 cam_pos;
uniform vec3 ambient_color;
uniform vec3 wire_color;

struct Light { vec3 position; vec3 color; float intensity; };
uniform Light lights[MAX_LIGHTS];
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float shininess;

uniform sampler2D base_color_map;
uniform sampler2D ao_map;
uniform sampler2D specular_map;
uniform sampler2D roughness_map;
uniform sampler2D normal_map;
uniform float rough_a;
uniform float rough_b;

out vec4 outColor;

vec3 phong(vec3 P, vec3 N, vec3 V, vec3 ka, vec3 kd, vec3 ks, float n) {
    vec3 color = ka * ambient_color;
    for (int i = 0; i < num_lights; ++i) {
        vec3 L = lights[i].position - P;
        float dist = length(L);
        L /= max(dist, 1e-5);
        float atten = lights[i].intensity / (1.0 + 0.05 * dist + 0.01 * dist * dist);
        vec3 lc = lights[i].color * atten;
        float diff = max(dot(N, L), 0.0);
        vec3 R = reflect(-L, N);
        float spec = pow(max(dot(R, V), 0.0), max(n, 1.0));
        color += lc * (kd * diff + ks * spec);
    }
    return color;
}

void main() {
    if (mode == 0) { outColor = vec4(wire_color, 1.0); return; }
    if (mode == 1) { outColor = vec4(v_gouraud, 1.0); return; }

    vec3 N = normalize(v_normal);
    vec3 V = normalize(cam_pos - v_pos);
    vec3 ka = Ka, kd = Kd, ks = Ks;
    float n = shininess;

    if (mode >= 3) {
        vec3 base = texture(base_color_map, v_uv).rgb;
        kd = base;
        ka = base * texture(ao_map, v_uv).rgb;
        ks = texture(specular_map, v_uv).rgb;
        n  = 1.0 / max(rough_a * texture(roughness_map, v_uv).r + rough_b, 1e-3);
    }
    if (mode == 4) {
        vec3 T = normalize(v_tangent - N * dot(N, v_tangent));
        vec3 B = cross(N, T);
        vec3 ts = texture(normal_map, v_uv).rgb * 2.0 - 1.0;
        N = normalize(mat3(T, B, N) * ts);
    }

    outColor = vec4(phong(v_pos, N, V, ka, kd, ks, n), 1.0);
}
"""

_program: ShaderProgram | None = None


def get_program() -> ShaderProgram:
    global _program
    if _program is None:
        _program = ShaderProgram(Shader(_VS, "vertex"), Shader(_FS, "fragment"))
        _program.use()
        # sampler bindings never change
        _program["base_color_map"] = 0
        _program["ao_map"] = 1
        _program["specular_map"] = 2
        _program["roughness_map"] = 3
        _program["normal_map"] = 4
    return _program
