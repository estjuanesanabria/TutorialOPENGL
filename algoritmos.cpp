#include "algoritmos.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

// ---------------------------------------------------------------------
// Bresenham para lineas
// Se calculan dx y dy. Si la linea es "acostada" (dx >= dy) x avanza de
// a 1 y el parametro de decision p dice si y tambien avanza o se queda.
// Si la linea es "parada" (dy > dx) se hace lo mismo cambiando x por y.
//   p inicial = 2dy - dx
//   p < 0   -> y se queda,  p = p + 2dy
//   p >= 0  -> y avanza,    p = p + 2dy - 2dx
// pasoX y pasoY valen 1 o -1 para que sirva en cualquier direccion.
// ---------------------------------------------------------------------
std::vector<Celda> bresenhamLinea(int x0, int y0, int x1, int y1)
{
    std::vector<Celda> celdas;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int pasoX = 1;
    int pasoY = 1;
    if (x1 < x0) pasoX = -1;
    if (y1 < y0) pasoY = -1;

    int x = x0;
    int y = y0;

    if (dx >= dy)
    {
        int p = 2 * dy - dx;
        for (int i = 0; i <= dx; i++)
        {
            celdas.push_back({ x, y });
            if (p >= 0)
            {
                y = y + pasoY;
                p = p + 2 * dy - 2 * dx;
            }
            else
            {
                p = p + 2 * dy;
            }
            x = x + pasoX;
        }
    }
    else
    {
        int p = 2 * dx - dy;
        for (int i = 0; i <= dy; i++)
        {
            celdas.push_back({ x, y });
            if (p >= 0)
            {
                x = x + pasoX;
                p = p + 2 * dx - 2 * dy;
            }
            else
            {
                p = p + 2 * dx;
            }
            y = y + pasoY;
        }
    }

    return celdas;
}

// Agrega los 8 puntos simetricos de (x, y) alrededor del centro (un punto por octante)
void agregar8Puntos(std::vector<Celda>& celdas, int xc, int yc, int x, int y)
{
    celdas.push_back({ xc + x, yc + y });
    celdas.push_back({ xc + y, yc + x });
    celdas.push_back({ xc - x, yc + y });
    celdas.push_back({ xc - y, yc + x });
    celdas.push_back({ xc + x, yc - y });
    celdas.push_back({ xc + y, yc - x });
    celdas.push_back({ xc - x, yc - y });
    celdas.push_back({ xc - y, yc - x });
}

// ---------------------------------------------------------------------
// Bresenham para circulos
// Solo se calcula un octante (desde (0, r) hasta que x = y) y los otros
// 7 se sacan por simetria. En cada paso x avanza y d decide si y baja.
//   d inicial = 3 - 2r
//   d < 0   -> y se queda,  d = d + 4x + 6
//   d >= 0  -> y baja,      d = d + 4(x - y) + 10
// ---------------------------------------------------------------------
std::vector<Celda> bresenhamCirculo(int xc, int yc, int r)
{
    std::vector<Celda> celdas;

    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y)
    {
        agregar8Puntos(celdas, xc, yc, x, y);

        if (d < 0)
        {
            d = d + 4 * x + 6;
        }
        else
        {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }

    return celdas;
}

// ---------------------------------------------------------------------
// Relleno por linea de barrido (scan-line) para poligonos
// Se recorre la figura fila por fila. En cada fila se calcula donde corta
// a cada lado del poligono, se ordenan esos cortes de izquierda a derecha
// y se pintan las celdas entre el 1er y 2do corte, entre el 3ro y 4to...
// Corte de la fila y con el lado de (x1, y1) a (x2, y2):
//   x = x1 + (y - y1) * (x2 - x1) / (y2 - y1)
// ---------------------------------------------------------------------
std::vector<Celda> rellenoPoligono(std::vector<Punto> v)
{
    std::vector<Celda> celdas;
    int n = (int)v.size();
    if (n < 3)
        return celdas;

    // fila mas baja y mas alta del poligono
    float yMin = v[0].y;
    float yMax = v[0].y;
    for (int i = 1; i < n; i++)
    {
        if (v[i].y < yMin) yMin = v[i].y;
        if (v[i].y > yMax) yMax = v[i].y;
    }

    for (int y = (int)ceil(yMin); y <= (int)floor(yMax); y++)
    {
        float fila = (float)y;
        std::vector<float> cortes;

        for (int i = 0; i < n; i++)
        {
            Punto a = v[i];
            Punto b = v[(i + 1) % n];   // el ultimo vertice se une con el primero

            if (a.y == b.y)
                continue;   // los lados horizontales no cortan la fila

            float yBajo = std::min(a.y, b.y);
            float yAlto = std::max(a.y, b.y);

            // el y de arriba no se incluye para no contar dos veces un vertice
            if (fila >= yBajo && fila < yAlto)
            {
                float x = a.x + (fila - a.y) * (b.x - a.x) / (b.y - a.y);
                cortes.push_back(x);
            }
        }

        std::sort(cortes.begin(), cortes.end());

        for (int k = 0; k + 1 < (int)cortes.size(); k = k + 2)
        {
            for (int x = (int)ceil(cortes[k]); x <= (int)floor(cortes[k + 1]); x++)
                celdas.push_back({ x, y });
        }
    }

    return celdas;
}

// ---------------------------------------------------------------------
// Relleno del circulo con la misma idea: en la fila que esta dy celdas
// arriba o abajo del centro, el circulo va desde -raiz(r^2 - dy^2)
// hasta +raiz(r^2 - dy^2) (por Pitagoras).
// ---------------------------------------------------------------------
std::vector<Celda> rellenoCirculo(int xc, int yc, int r)
{
    std::vector<Celda> celdas;

    for (int dy = -r; dy <= r; dy++)
    {
        int mitad = (int)floor(sqrt((float)(r * r - dy * dy)));
        for (int dx = -mitad; dx <= mitad; dx++)
            celdas.push_back({ xc + dx, yc + dy });
    }

    return celdas;
}
