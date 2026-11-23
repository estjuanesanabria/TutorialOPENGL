#pragma once
#include "lienzo.h"

// Matriz 3x3 para transformaciones 2D en coordenadas homogeneas.
// El punto (x, y) se usa como la columna [x, y, 1] y se calcula P' = M * P
struct Matriz3
{
    float m[3][3];   // m[fila][columna]
};

Matriz3 identidad();
Matriz3 traslacion(float tx, float ty);
Matriz3 rotacion(float grados);
Matriz3 escalamiento(float sx, float sy);
Matriz3 reflexionX();
Matriz3 reflexionY();
Matriz3 cizallamientoX(float shx);
Matriz3 cizallamientoY(float shy);

Matriz3 multiplicar(Matriz3 a, Matriz3 b);
Punto multiplicarPunto(Matriz3 m, Punto p);

Punto centroFigura(Figura& f);
void circuloAPoligono(Figura& f);
void transformarFigura(Figura& f, Matriz3 m);
