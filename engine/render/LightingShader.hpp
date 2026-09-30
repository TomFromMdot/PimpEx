#pragma once

// Prosty shader oświetlenia Blinna-Phonga (do 4 świateł) dla scen 3D.
// Wzorowany na raylib/examples/shaders/lighting, wbudowany w binarkę.
namespace PimpEx::render {

inline constexpr const char *LightingVertexShader = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main() {
  fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
  fragTexCoord = vertexTexCoord;
  fragColor = vertexColor;
  fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
  gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

inline constexpr const char *LightingFragmentShader = R"(#version 330
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

#define MAX_LIGHTS 4
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1

struct Light {
  int enabled;
  int type;
  vec3 position;
  vec3 target;
  vec4 color;
};

uniform Light lights[MAX_LIGHTS];
uniform vec4 ambient;
uniform vec3 viewPos;

out vec4 finalColor;

void main() {
  vec4 texelColor = texture(texture0, fragTexCoord);
  vec3 normal = normalize(fragNormal);
  vec3 viewD = normalize(viewPos - fragPosition);
  vec3 lighting = ambient.rgb;
  vec3 specular = vec3(0.0);

  for (int i = 0; i < MAX_LIGHTS; i++) {
    if (lights[i].enabled == 0) continue;
    vec3 light = lights[i].type == LIGHT_DIRECTIONAL
                     ? -normalize(lights[i].target - lights[i].position)
                     : normalize(lights[i].position - fragPosition);
    float NdotL = max(dot(normal, light), 0.0);
    lighting += lights[i].color.rgb * NdotL;
    if (NdotL > 0.0)
      specular += lights[i].color.rgb * 0.25 * pow(max(0.0, dot(viewD, reflect(-light, normal))), 16.0);
  }

  vec4 base = texelColor * colDiffuse * fragColor;
  finalColor = vec4(base.rgb * lighting + specular, base.a);
}
)";

} // namespace PimpEx::render
