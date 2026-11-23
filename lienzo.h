#pragma once
#include <vector>
#include "algoritmos.h"

// El lienzo es una cuadricula de celdas. La celda (0, 0) es la de abajo a la
// izquierda, x crece hacia la derecha y y hacia arriba.
// Lo que dibuja el usuario se guarda como figuras (con sus vertices) y las
// celdas se pintan a partir de esas figuras.

struct Color
{
    float r, g, b;
};

enum TipoFigura
{
    PUNTO,
    LINEA,
    CIRCULO,
    LIBRE,
    TRIANGULO,
    CUADRADO,
    RECTANGULO,
    POLIGONO
};

struct Figura
{
    TipoFigura tipo = PUNTO;
    std::vector<Punto> vertices;     // el circulo usa 2: el centro y un punto del borde
    Color color = { 0, 0, 0 };
    bool rellena = false;
    Color colorRelleno = { 0, 0, 0 };
};

struct Lienzo
{
    int columnas = 0;
    int filas = 0;
    std::vector<Color> celdas;                   // color de cada celda
    std::vector<int> duenio;                     // que figura pinto cada celda (-1 = ninguna)
    std::vector<Figura> figuras;                 // todo lo que se ha dibujado
    std::vector<std::vector<Figura>> historial;  // copias anteriores para deshacer
};

const Color COLOR_FONDO = { 1.0f, 1.0f, 1.0f };

void crearLienzo(Lienzo& l, int columnas, int filas);
bool dentroDelLienzo(Lienzo& l, int x, int y);
void pintarCelda(Lienzo& l, int x, int y, Color c, int id);
void limpiarCeldas(Lienzo& l);

int redondear(float valor);
int radioCirculo(Figura& f);
bool esCerrada(Figura& f);
const char* nombreFigura(TipoFigura tipo);
std::vector<Celda> celdasInteriores(Figura& f);
void dibujarFigura(Lienzo& l, Figura& f, int id);
void redibujarTodo(Lienzo& l);
void agregarFigura(Lienzo& l, Figura f);
bool puntoDentro(Figura& f, float x, float y);
int buscarFigura(Lienzo& l, int x, int y, bool soloCerradas);

void guardarHistorial(Lienzo& l);
void deshacer(Lienzo& l);
void borrarTodo(Lienzo& l);
