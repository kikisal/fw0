#version 440 core

in vec3  frag_color;
out vec4 frag_out;

void main()
{
    frag_out = vec4(frag_color, 1.0);
}