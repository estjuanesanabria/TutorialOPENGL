#pragma once
#include <vector>

// Una celda de la cuadricula (columna y fila)
struct Celda
{
    int x, y;
};

// Un punto con decimales. Los vertices de las figuras se guardan asi
// para que al rotar o escalar no se pierdan los decimales.
struct Punto
{
    float x, y;
};

// Algoritmos de rasterizacion: devuelven la lista de celdas que hay que pintar
std::vector<Celda> bresenhamLinea(int x0, int y0, int x1, int y1);
std::vector<Celda> bresenhamCirculo(int xc, int yc, int r);
std::vector<Celda> rellenoPoligono(std::vector<Punto> vertices);
std::vector<Celda> rellenoCirculo(int xc, int yc, int r);
