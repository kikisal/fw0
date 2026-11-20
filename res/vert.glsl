#version 440 core

layout (location = 0) in vec3  position;
layout (location = 1) in vec3  color;
layout (location = 2) in vec2  uv;

uniform mat4 uMVP;

out vec3 frag_color;

void main()
{
    gl_Position = uMVP * vec4(position, 1.0);
    frag_color  = vec3(uv, 0.0);
    // frag_color  = color;
}