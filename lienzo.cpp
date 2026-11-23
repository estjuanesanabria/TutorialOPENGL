#include "lienzo.h"
#include <cmath>

// ---------------------------------------------------------------------
// Cuadricula
// Las celdas se guardan en un vector de una dimension, fila por fila.
// La celda (x, y) esta en la posicion y * columnas + x.
// ---------------------------------------------------------------------

void crearLienzo(Lienzo& l, int columnas, int filas)
{
    l.columnas = columnas;
    l.filas = filas;
    l.celdas.assign(columnas * filas, COLOR_FONDO);
    l.duenio.assign(columnas * filas, -1);
    redibujarTodo(l);   // las figuras se conservan, solo cambia el tamaño
}

bool dentroDelLienzo(Lienzo& l, int x, int y)
{
    return x >= 0 && x < l.columnas && y >= 0 && y < l.filas;
}

void pintarCelda(Lienzo& l, int x, int y, Color c, int id)
{
    if (!dentroDelLienzo(l, x, y))
        return;   // lo que queda fuera de la cuadricula no se pinta

    l.celdas[y * l.columnas + x] = c;
    l.duenio[y * l.columnas + x] = id;
}

void limpiarCeldas(Lienzo& l)
{
    for (int i = 0; i < (int)l.celdas.size(); i++)
    {
        l.celdas[i] = COLOR_FONDO;
        l.duenio[i] = -1;
    }
}

// ---------------------------------------------------------------------
// Figuras
// ---------------------------------------------------------------------

int redondear(float valor)
{
    return (int)floor(valor + 0.5f);
}

// El radio es la distancia entre el centro y el punto del borde (Pitagoras)
int radioCirculo(Figura& f)
{
    float dx = f.vertices[1].x - f.vertices[0].x;
    float dy = f.vertices[1].y - f.vertices[0].y;
    return redondear((float)sqrt(dx * dx + dy * dy));
}

// Figuras que encierran un area (las unicas que se pueden rellenar)
bool esCerrada(Figura& f)
{
    return f.tipo == CIRCULO || f.tipo == TRIANGULO || f.tipo == CUADRADO ||
           f.tipo == RECTANGULO || f.tipo == POLIGONO;
}

const char* nombreFigura(TipoFigura tipo)
{
    if (tipo == PUNTO) return "Punto";
    if (tipo == LINEA) return "Línea";
    if (tipo == CIRCULO) return "Círculo";
    if (tipo == LIBRE) return "Trazo libre";
    if (tipo == TRIANGULO) return "Triángulo";
    if (tipo == CUADRADO) return "Cuadrado";
    if (tipo == RECTANGULO) return "Rectángulo";
    return "Polígono";
}

// Celdas de adentro de una figura cerrada (relleno scan-line)
std::vector<Celda> celdasInteriores(Figura& f)
{
    if (f.tipo == CIRCULO)
        return rellenoCirculo(redondear(f.vertices[0].x), redondear(f.vertices[0].y), radioCirculo(f));

    // se usan los vertices redondeados, los mismos del borde, para que el
    // relleno no se salga del borde cuando la figura esta rotada
    std::vector<Punto> v;
    for (int i = 0; i < (int)f.vertices.size(); i++)
        v.push_back({ (float)redondear(f.vertices[i].x), (float)redondear(f.vertices[i].y) });
    return rellenoPoligono(v);
}

void pintarLista(Lienzo& l, std::vector<Celda> celdas, Color c, int id)
{
    for (int i = 0; i < (int)celdas.size(); i++)
        pintarCelda(l, celdas[i].x, celdas[i].y, c, id);
}

// Pinta las celdas de una figura: primero el relleno y despues el borde encima
void dibujarFigura(Lienzo& l, Figura& f, int id)
{
    if (f.rellena && esCerrada(f))
        pintarLista(l, celdasInteriores(f), f.colorRelleno, id);

    if (f.tipo == CIRCULO)
    {
        int xc = redondear(f.vertices[0].x);
        int yc = redondear(f.vertices[0].y);
        pintarLista(l, bresenhamCirculo(xc, yc, radioCirculo(f)), f.color, id);
        return;
    }

    int n = (int)f.vertices.size();
    if (n == 1)
    {
        pintarCelda(l, redondear(f.vertices[0].x), redondear(f.vertices[0].y), f.color, id);
        return;
    }

    // las demas figuras son vertices unidos con lineas de Bresenham
    int lados = n - 1;
    if (esCerrada(f))
        lados = n;   // las cerradas tambien unen el ultimo vertice con el primero

    for (int i = 0; i < lados; i++)
    {
        Punto a = f.vertices[i];
        Punto b = f.vertices[(i + 1) % n];
        pintarLista(l, bresenhamLinea(redondear(a.x), redondear(a.y), redondear(b.x), redondear(b.y)), f.color, id);
    }
}

// Borra las celdas y vuelve a pintar todas las figuras en orden
void redibujarTodo(Lienzo& l)
{
    limpiarCeldas(l);
    for (int i = 0; i < (int)l.figuras.size(); i++)
        dibujarFigura(l, l.figuras[i], i);
}

void agregarFigura(Lienzo& l, Figura f)
{
    l.figuras.push_back(f);
    int id = (int)l.figuras.size() - 1;
    dibujarFigura(l, l.figuras[id], id);
}

// ¿El punto (x, y) esta dentro de la figura?
// Circulo: la distancia al centro es menor o igual al radio.
// Poligono: se traza una linea horizontal desde el punto hacia la derecha y se
// cuentan los lados que cruza. Si es impar esta adentro.
bool puntoDentro(Figura& f, float x, float y)
{
    if (f.tipo == CIRCULO)
    {
        float dx = x - f.vertices[0].x;
        float dy = y - f.vertices[0].y;
        float r = (float)radioCirculo(f);
        return dx * dx + dy * dy <= r * r;
    }

    bool adentro = false;
    int n = (int)f.vertices.size();
    for (int i = 0; i < n; i++)
    {
        Punto a = f.vertices[i];
        Punto b = f.vertices[(i + 1) % n];
        if ((a.y > y) != (b.y > y))
        {
            float xCorte = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (x < xCorte)
                adentro = !adentro;
        }
    }
    return adentro;
}

// Busca la figura que esta en la celda (x, y). Devuelve su numero o -1.
// Primero mira que figura pinto esa celda; si no hay ninguna, busca una figura
// cerrada que tenga el punto adentro.
int buscarFigura(Lienzo& l, int x, int y, bool soloCerradas)
{
    if (dentroDelLienzo(l, x, y))
    {
        int id = l.duenio[y * l.columnas + x];
        if (id >= 0 && (!soloCerradas || esCerrada(l.figuras[id])))
            return id;
    }

    for (int i = (int)l.figuras.size() - 1; i >= 0; i--)
    {
        if (esCerrada(l.figuras[i]) && puntoDentro(l.figuras[i], (float)x, (float)y))
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------
// Deshacer: antes de cada cambio se guarda una copia de las figuras
// ---------------------------------------------------------------------

void guardarHistorial(Lienzo& l)
{
    l.historial.push_back(l.figuras);
    if (l.historial.size() > 50)
        l.historial.erase(l.historial.begin());
}

void deshacer(Lienzo& l)
{
    if (l.historial.size() == 0)
        return;

    l.figuras = l.historial.back();
    l.historial.pop_back();
    redibujarTodo(l);
}

void borrarTodo(Lienzo& l)
{
    guardarHistorial(l);
    l.figuras.clear();
    redibujarTodo(l);
}
