#version 330 core

in vec3 position;
in vec2 texcoord;

uniform mat4 matrix;
out vec2 Texcoord;


void main()
{
    Texcoord = texcoord; 
    gl_Position = matrix * vec4(position, 1.0);
    gl_Position[3] = gl_Position[3] + 1.0;
}