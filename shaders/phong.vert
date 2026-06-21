#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 m;
uniform mat4 v;
uniform mat4 p;
uniform mat3 uNormalMatrix;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vUV;

void main()
{
    vec4 worldPos = m * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal  = normalize(uNormalMatrix * aNormal);
    vUV      = aUV;
    gl_Position = p * v * worldPos;
}
