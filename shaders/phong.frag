#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform DirectionalLight uDirLight;
uniform PointLight       uPointLight;
uniform vec3             uViewPos;

uniform sampler2D uDiffuseTex;
uniform bool      uHasTexture;
uniform vec3      uMaterialDiffuse;
uniform vec3      uMaterialSpecular;
uniform float     uMaterialShininess;

uniform vec3  uTintColor;
uniform float uHighlight;

vec3 computeDirectional(DirectionalLight L, vec3 N, vec3 V, vec3 baseColor)
{
    vec3 Ld = normalize(-L.direction);
    float diff = max(dot(N, Ld), 0.0);
    vec3 R = reflect(-Ld, N);
    float spec = pow(max(dot(V, R), 0.0), uMaterialShininess);

    vec3 ambient  = L.ambient  * baseColor;
    vec3 diffuse  = L.diffuse  * diff * baseColor;
    vec3 specular = L.specular * spec * uMaterialSpecular;
    return ambient + diffuse + specular;
}

vec3 computePoint(PointLight L, vec3 N, vec3 V, vec3 baseColor)
{
    vec3 Ld = normalize(L.position - vFragPos);
    float diff = max(dot(N, Ld), 0.0);
    vec3 R = reflect(-Ld, N);
    float spec = pow(max(dot(V, R), 0.0), uMaterialShininess);

    float d = length(L.position - vFragPos);
    float attenuation = 1.0 / (L.constant + L.linear * d + L.quadratic * d * d);

    vec3 ambient  = L.ambient  * baseColor;
    vec3 diffuse  = L.diffuse  * diff * baseColor;
    vec3 specular = L.specular * spec * uMaterialSpecular;
    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vFragPos);

    vec3 baseColor = uMaterialDiffuse * uTintColor;
    if (uHasTexture) {
        baseColor *= texture(uDiffuseTex, vUV).rgb;
    }

    vec3 color = computeDirectional(uDirLight, N, V, baseColor)
               + computePoint(uPointLight, N, V, baseColor);

    // additive highlight (golden glow)
    color += uHighlight * vec3(1.0, 0.85, 0.3) * 0.55;

    // gamma
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
