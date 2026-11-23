#include "transformaciones.h"
#include <cmath>

const float PI = 3.14159265f;

//  | 1 0 0 |
//  | 0 1 0 |
//  | 0 0 1 |
Matriz3 identidad()
{
    Matriz3 r;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            r.m[i][j] = 0;
    r.m[0][0] = 1;
    r.m[1][1] = 1;
    r.m[2][2] = 1;
    return r;
}

// Traslacion: x' = x + tx,  y' = y + ty
//  | 1 0 tx |
//  | 0 1 ty |
//  | 0 0 1  |
Matriz3 traslacion(float tx, float ty)
{
    Matriz3 r = identidad();
    r.m[0][2] = tx;
    r.m[1][2] = ty;
    return r;
}

// Rotacion (angulo positivo = antihorario):
// x' = x cos - y sen,  y' = x sen + y cos
//  | cos -sen 0 |
//  | sen  cos 0 |
//  |  0    0  1 |
Matriz3 rotacion(float grados)
{
    float a = grados * PI / 180.0f;   // cos y sin trabajan en radianes
    Matriz3 r = identidad();
    r.m[0][0] = (float)cos(a);
    r.m[0][1] = (float)-sin(a);
    r.m[1][0] = (float)sin(a);
    r.m[1][1] = (float)cos(a);
    return r;
}

// Escalamiento: x' = x * sx,  y' = y * sy  (mayor a 1 agranda, menor a 1 reduce)
//  | sx 0  0 |
//  | 0  sy 0 |
//  | 0  0  1 |
Matriz3 escalamiento(float sx, float sy)
{
    Matriz3 r = identidad();
    r.m[0][0] = sx;
    r.m[1][1] = sy;
    return r;
}

// Reflexion respecto al eje X: y cambia de signo
//  | 1  0 0 |
//  | 0 -1 0 |
//  | 0  0 1 |
Matriz3 reflexionX()
{
    Matriz3 r = identidad();
    r.m[1][1] = -1;
    return r;
}

// Reflexion respecto al eje Y: x cambia de signo
//  | -1 0 0 |
//  |  0 1 0 |
//  |  0 0 1 |
Matriz3 reflexionY()
{
    Matriz3 r = identidad();
    r.m[0][0] = -1;
    return r;
}

// Cizallamiento en X: x' = x + shx * y
//  | 1 shx 0 |
//  | 0  1  0 |
//  | 0  0  1 |
Matriz3 cizallamientoX(float shx)
{
    Matriz3 r = identidad();
    r.m[0][1] = shx;
    return r;
}

// Cizallamiento en Y: y' = y + shy * x
//  |  1  0 0 |
//  | shy 1 0 |
//  |  0  0 1 |
Matriz3 cizallamientoY(float shy)
{
    Matriz3 r = identidad();
    r.m[1][0] = shy;
    return r;
}

// Multiplicacion de matrices 3x3: cada posicion es fila de A por columna de B
Matriz3 multiplicar(Matriz3 a, Matriz3 b)
{
    Matriz3 r;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            r.m[i][j] = 0;
            for (int k = 0; k < 3; k++)
                r.m[i][j] = r.m[i][j] + a.m[i][k] * b.m[k][j];
        }
    }
    return r;
}

// Matriz por punto: [x', y', 1] = M * [x, y, 1]
Punto multiplicarPunto(Matriz3 m, Punto p)
{
    Punto r;
    r.x = m.m[0][0] * p.x + m.m[0][1] * p.y + m.m[0][2] * 1;
    r.y = m.m[1][0] * p.x + m.m[1][1] * p.y + m.m[1][2] * 1;
    return r;
}

// Centro de la figura. En el circulo es su centro; en las demas es el
// centro del rectangulo que encierra todos sus vertices.
Punto centroFigura(Figura& f)
{
    if (f.tipo == CIRCULO)
        return f.vertices[0];

    float xMin = f.vertices[0].x, xMax = f.vertices[0].x;
    float yMin = f.vertices[0].y, yMax = f.vertices[0].y;
    for (int i = 1; i < (int)f.vertices.size(); i++)
    {
        if (f.vertices[i].x < xMin) xMin = f.vertices[i].x;
        if (f.vertices[i].x > xMax) xMax = f.vertices[i].x;
        if (f.vertices[i].y < yMin) yMin = f.vertices[i].y;
        if (f.vertices[i].y > yMax) yMax = f.vertices[i].y;
    }
    return { (xMin + xMax) / 2, (yMin + yMax) / 2 };
}

// Si un circulo se escala diferente en x y en y, o se cizalla, queda como una
// elipse y ya no se puede dibujar con el Bresenham de circulos. Entonces se
// cambia por un poligono de 36 lados que sigue la circunferencia.
void circuloAPoligono(Figura& f)
{
    Punto c = f.vertices[0];
    float r = (float)radioCirculo(f);

    f.vertices.clear();
    for (int i = 0; i < 36; i++)
    {
        float a = 2 * PI * (float)i / 36.0f;
        float x = c.x + r * (float)cos(a);
        float y = c.y + r * (float)sin(a);
        f.vertices.push_back({ x, y });
    }
    f.tipo = POLIGONO;
}

// Aplica la matriz a todos los vertices de la figura.
// Las matrices rotan, escalan, reflejan y cizallan respecto al punto (0, 0).
// Para que la figura lo haga sobre su propio centro c se combinan 3 matrices:
//   total = T(c) * M * T(-c)
// T(-c) lleva el centro al (0, 0), M transforma y T(c) la devuelve a su lugar.
void transformarFigura(Figura& f, Matriz3 m)
{
    Punto c = centroFigura(f);
    Matriz3 total = multiplicar(traslacion(c.x, c.y), multiplicar(m, traslacion(-c.x, -c.y)));

    for (int i = 0; i < (int)f.vertices.size(); i++)
        f.vertices[i] = multiplicarPunto(total, f.vertices[i]);
}
