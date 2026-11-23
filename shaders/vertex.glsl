#version 460 core

// Vertex shader: se ejecuta una vez por cada vertice.
// Pasa la posicion de celdas a coordenadas de OpenGL (de -1 a 1).

layout (location = 0) in vec2 aPos;     // posicion en celdas
layout (location = 1) in vec3 aColor;   // color

uniform float uTamCelda;      // tamaño de una celda en pixeles
uniform vec2 uTamViewport;    // tamaño de la cuadricula en pixeles

out vec3 vColor;

void main()
{
    // celdas -> pixeles (+0.5 para que las lineas se vean nitidas)
    vec2 pixel = aPos * uTamCelda + 0.5;

    // pixeles -> coordenadas de OpenGL
    vec2 ndc = (pixel / uTamViewport) * 2.0 - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = aColor;
}
