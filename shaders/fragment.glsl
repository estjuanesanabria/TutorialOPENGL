#version 460 core

// Fragment shader: se ejecuta por cada pixel y le pone el color.

in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
