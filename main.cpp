// Paint 2D sobre una cuadricula
// Computacion Grafica - Juan Esteban Sanabria Adame - 6014495
// C++ con OpenGL 4.6, GLFW, GLAD y Dear ImGui

#include <glad/glad.h>   // GLAD siempre va antes que GLFW
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "lienzo.h"
#include "algoritmos.h"
#include "transformaciones.h"

const int ANCHO_PANEL = 300;   // espacio de la izquierda para el panel
const int MARGEN = 10;

// Herramientas del panel
enum Herramienta
{
    H_PUNTO,
    H_LINEA,
    H_CIRCULO,
    H_LIBRE,
    H_TRIANGULO,
    H_CUADRADO,
    H_RECTANGULO,
    H_RELLENAR,
    H_SELECCIONAR
};

Lienzo lienzo;
int herramienta = H_PUNTO;
Color colorActual = { 0.1f, 0.1f, 0.1f };
int tamCelda = 16;           // tamaño de cada celda en pixeles
bool hayCambios = true;      // cuando es true se vuelve a mandar la geometria a la GPU

// donde queda la cuadricula dentro de la ventana (en pixeles, desde abajo a la izquierda)
int cuadriculaX = 0, cuadriculaY = 0;
int cuadriculaAncho = 0, cuadriculaAlto = 0;

// celda donde esta el mouse
int celdaX = -1, celdaY = -1;
bool mouseEnCuadricula = false;

// figura que se esta dibujando mientras se arrastra el mouse
bool dibujando = false;
Figura figuraTemporal;
Punto puntoInicio;

// figura a la que se le aplican las transformaciones (-1 = ninguna)
int seleccionada = -1;

// valores de las transformaciones
int tx = 2, ty = 2;
float angulo = 45.0f;
float sx = 2.0f, sy = 2.0f;
float shx = 1.0f, shy = 1.0f;
Matriz3 ultimaMatriz;
bool hayMatriz = false;

Color paleta[8] = {
    { 0.10f, 0.10f, 0.10f }, { 0.90f, 0.15f, 0.15f }, { 0.15f, 0.65f, 0.25f }, { 0.15f, 0.35f, 0.90f },
    { 0.95f, 0.80f, 0.10f }, { 0.95f, 0.50f, 0.10f }, { 0.60f, 0.25f, 0.80f }, { 0.10f, 0.75f, 0.80f }
};

// ---------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------

std::string leerArchivo(const char* ruta)
{
    std::ifstream archivo(ruta);
    if (!archivo.is_open())
    {
        std::cout << "No se pudo abrir " << ruta << std::endl;
        return "";
    }
    std::stringstream texto;
    texto << archivo.rdbuf();
    return texto.str();
}

unsigned int compilarShader(GLenum tipo, std::string codigo)
{
    unsigned int shader = glCreateShader(tipo);
    const char* fuente = codigo.c_str();
    glShaderSource(shader, 1, &fuente, NULL);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char error[512];
        glGetShaderInfoLog(shader, 512, NULL, error);
        std::cout << "Error en el shader:\n" << error << std::endl;
    }
    return shader;
}

unsigned int crearPrograma(const char* rutaVertex, const char* rutaFragment)
{
    unsigned int vertex = compilarShader(GL_VERTEX_SHADER, leerArchivo(rutaVertex));
    unsigned int fragment = compilarShader(GL_FRAGMENT_SHADER, leerArchivo(rutaFragment));

    unsigned int programa = glCreateProgram();
    glAttachShader(programa, vertex);
    glAttachShader(programa, fragment);
    glLinkProgram(programa);

    int ok;
    glGetProgramiv(programa, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char error[512];
        glGetProgramInfoLog(programa, 512, NULL, error);
        std::cout << "Error al enlazar los shaders:\n" << error << std::endl;
    }

    // ya quedaron dentro del programa
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return programa;
}

// ---------------------------------------------------------------------
// Geometria. Cada vertice tiene 5 floats: x, y, r, g, b
// ---------------------------------------------------------------------

void agregarVertice(std::vector<float>& v, float x, float y, Color c)
{
    v.push_back(x);
    v.push_back(y);
    v.push_back(c.r);
    v.push_back(c.g);
    v.push_back(c.b);
}

// Cada celda es un cuadrado de 4 vertices que se dibuja con 2 triangulos.
// Con el EBO se usan 6 indices sin repetir vertices:
//   3 --- 2     triangulo 1: 0 1 2
//   |   / |     triangulo 2: 0 2 3
//   0 --- 1
void generarCeldas(std::vector<float>& vertices, std::vector<unsigned int>& indices)
{
    vertices.clear();
    indices.clear();

    for (int y = 0; y < lienzo.filas; y++)
    {
        for (int x = 0; x < lienzo.columnas; x++)
        {
            Color c = lienzo.celdas[y * lienzo.columnas + x];
            unsigned int n = (unsigned int)(vertices.size() / 5);   // numero del primer vertice de la celda

            agregarVertice(vertices, (float)x, (float)y, c);
            agregarVertice(vertices, (float)x + 1, (float)y, c);
            agregarVertice(vertices, (float)x + 1, (float)y + 1, c);
            agregarVertice(vertices, (float)x, (float)y + 1, c);

            indices.push_back(n);
            indices.push_back(n + 1);
            indices.push_back(n + 2);
            indices.push_back(n);
            indices.push_back(n + 2);
            indices.push_back(n + 3);
        }
    }
}

// Lineas grises entre las celdas y un rectangulo rojo alrededor de la figura seleccionada
void generarRejilla(std::vector<float>& v)
{
    v.clear();
    Color gris = { 0.82f, 0.82f, 0.82f };

    for (int x = 0; x <= lienzo.columnas; x++)
    {
        agregarVertice(v, (float)x, 0, gris);
        agregarVertice(v, (float)x, (float)lienzo.filas, gris);
    }
    for (int y = 0; y <= lienzo.filas; y++)
    {
        agregarVertice(v, 0, (float)y, gris);
        agregarVertice(v, (float)lienzo.columnas, (float)y, gris);
    }

    if (seleccionada >= 0)
    {
        Figura& f = lienzo.figuras[seleccionada];
        int xMin, yMin, xMax, yMax;
        if (f.tipo == CIRCULO)
        {
            int r = radioCirculo(f);
            xMin = redondear(f.vertices[0].x) - r;
            xMax = redondear(f.vertices[0].x) + r;
            yMin = redondear(f.vertices[0].y) - r;
            yMax = redondear(f.vertices[0].y) + r;
        }
        else
        {
            xMin = xMax = redondear(f.vertices[0].x);
            yMin = yMax = redondear(f.vertices[0].y);
            for (int i = 1; i < (int)f.vertices.size(); i++)
            {
                int x = redondear(f.vertices[i].x);
                int y = redondear(f.vertices[i].y);
                if (x < xMin) xMin = x;
                if (x > xMax) xMax = x;
                if (y < yMin) yMin = y;
                if (y > yMax) yMax = y;
            }
        }

        Color rojo = { 0.9f, 0.1f, 0.1f };
        float x0 = (float)xMin, x1 = (float)xMax + 1;
        float y0 = (float)yMin, y1 = (float)yMax + 1;
        agregarVertice(v, x0, y0, rojo); agregarVertice(v, x1, y0, rojo);
        agregarVertice(v, x1, y0, rojo); agregarVertice(v, x1, y1, rojo);
        agregarVertice(v, x1, y1, rojo); agregarVertice(v, x0, y1, rojo);
        agregarVertice(v, x0, y1, rojo); agregarVertice(v, x0, y0, rojo);
    }
}

// ---------------------------------------------------------------------
// Coordenadas
// ---------------------------------------------------------------------

// Calcula cuantas celdas caben en la ventana y donde va la cuadricula
void calcularTamano(int anchoVentana, int altoVentana)
{
    int columnas = (anchoVentana - ANCHO_PANEL - 2 * MARGEN) / tamCelda;
    int filas = (altoVentana - 2 * MARGEN) / tamCelda;
    if (columnas < 1) columnas = 1;
    if (filas < 1) filas = 1;

    if (columnas != lienzo.columnas || filas != lienzo.filas)
    {
        crearLienzo(lienzo, columnas, filas);
        hayCambios = true;
    }

    cuadriculaAncho = lienzo.columnas * tamCelda + 1;   // +1 para que se vea la ultima linea
    cuadriculaAlto = lienzo.filas * tamCelda + 1;
    cuadriculaX = ANCHO_PANEL + MARGEN;
    cuadriculaY = altoVentana - MARGEN - cuadriculaAlto;   // OpenGL cuenta desde abajo
}

// Pasa la posicion del mouse a la celda (x, y).
// El mouse tiene y hacia abajo y la cuadricula y hacia arriba, por eso se invierte.
// Ejemplo: tamCelda = 16, cuadriculaX = 310 y el mouse en x = 350
//          (350 - 310) / 16 = 2.5  ->  columna 2
bool mouseACelda(float mouseX, float mouseY, int altoVentana)
{
    float xLocal = mouseX - (float)cuadriculaX;
    float yLocal = ((float)altoVentana - mouseY) - (float)cuadriculaY;

    celdaX = (int)floor(xLocal / (float)tamCelda);
    celdaY = (int)floor(yLocal / (float)tamCelda);

    return dentroDelLienzo(lienzo, celdaX, celdaY);
}

// Calcula los vertices de la figura que se esta dibujando, desde donde se
// presiono el mouse (puntoInicio) hasta donde esta ahora (actual)
void actualizarTemporal(Punto actual)
{
    Punto a = puntoInicio;
    std::vector<Punto>& v = figuraTemporal.vertices;

    if (figuraTemporal.tipo == LINEA || figuraTemporal.tipo == CIRCULO)
    {
        // linea: los 2 extremos. circulo: el centro y un punto del borde
        v = { a, actual };
    }
    else if (figuraTemporal.tipo == LIBRE)
    {
        // se guarda cada celda nueva por donde pasa el mouse;
        // despues se unen con Bresenham para que no queden huecos
        if (v.size() == 0 || v.back().x != actual.x || v.back().y != actual.y)
            v.push_back(actual);
    }
    else if (figuraTemporal.tipo == RECTANGULO)
    {
        v = { a, { actual.x, a.y }, actual, { a.x, actual.y } };
    }
    else if (figuraTemporal.tipo == CUADRADO)
    {
        // el lado es el mayor desplazamiento del mouse
        float ladoX = actual.x - a.x;
        float ladoY = actual.y - a.y;
        if (ladoX < 0) ladoX = -ladoX;
        if (ladoY < 0) ladoY = -ladoY;
        float lado = ladoX;
        if (ladoY > lado)
            lado = ladoY;

        float bx = a.x + lado;
        float by = a.y + lado;
        if (actual.x < a.x) bx = a.x - lado;
        if (actual.y < a.y) by = a.y - lado;
        v = { a, { bx, a.y }, { bx, by }, { a.x, by } };
    }
    else if (figuraTemporal.tipo == TRIANGULO)
    {
        // triangulo con la base abajo y la punta arriba en el centro
        float xMin = a.x, xMax = actual.x;
        float yMin = a.y, yMax = actual.y;
        if (xMin > xMax) { xMin = actual.x; xMax = a.x; }
        if (yMin > yMax) { yMin = actual.y; yMax = a.y; }
        float punta = (float)(int)((xMin + xMax) / 2);   // x de la punta (entera)
        v = { { xMin, yMin }, { xMax, yMin }, { punta, yMax } };
    }
}

// Aplica una matriz a la figura seleccionada
void aplicarMatriz(Matriz3 m, bool deformaCirculo)
{
    if (seleccionada < 0)
        return;

    guardarHistorial(lienzo);
    Figura& f = lienzo.figuras[seleccionada];

    if (f.tipo == CIRCULO && deformaCirculo)
        circuloAPoligono(f);

    transformarFigura(f, m);
    redibujarTodo(lienzo);

    ultimaMatriz = m;
    hayMatriz = true;
    hayCambios = true;
}

// ---------------------------------------------------------------------
// Panel con ImGui
// ---------------------------------------------------------------------

void selectorColor()
{
    ImGui::ColorEdit3("Color", &colorActual.r);
    for (int i = 0; i < 8; i++)
    {
        ImGui::PushID(i);
        if (ImGui::ColorButton("##paleta", ImVec4(paleta[i].r, paleta[i].g, paleta[i].b, 1), 0, ImVec2(26, 26)))
            colorActual = paleta[i];
        ImGui::PopID();
        if (i < 7)
            ImGui::SameLine();
    }
}

void panelTransformaciones()
{
    if (seleccionada >= 0)
        ImGui::Text("Figura: %s %d", nombreFigura(lienzo.figuras[seleccionada].tipo), seleccionada);
    else
        ImGui::TextDisabled("Selecciona o dibuja una figura");

    // traslacion
    ImGui::InputInt("Tx", &tx);
    ImGui::InputInt("Ty", &ty);
    if (ImGui::Button("Trasladar"))
        aplicarMatriz(traslacion((float)tx, (float)ty), false);

    // rotacion
    ImGui::InputFloat("Ángulo", &angulo, 15, 45, "%.1f");
    if (ImGui::Button("Rotar"))
        aplicarMatriz(rotacion(angulo), false);

    // escalamiento
    ImGui::InputFloat("Sx", &sx, 0.5f, 1, "%.2f");
    ImGui::InputFloat("Sy", &sy, 0.5f, 1, "%.2f");
    if (ImGui::Button("Escalar") && sx > 0 && sy > 0)
        aplicarMatriz(escalamiento(sx, sy), sx != sy);   // escala distinta deforma el circulo

    // reflexion
    if (ImGui::Button("Reflejar en X"))
        aplicarMatriz(reflexionX(), false);
    ImGui::SameLine();
    if (ImGui::Button("Reflejar en Y"))
        aplicarMatriz(reflexionY(), false);

    // cizallamiento
    ImGui::InputFloat("Shx", &shx, 0.5f, 1, "%.2f");
    if (ImGui::Button("Cizallar en X"))
        aplicarMatriz(cizallamientoX(shx), true);
    ImGui::InputFloat("Shy", &shy, 0.5f, 1, "%.2f");
    if (ImGui::Button("Cizallar en Y"))
        aplicarMatriz(cizallamientoY(shy), true);

    // matriz que se uso
    if (hayMatriz)
    {
        ImGui::Text("Última matriz:");
        for (int i = 0; i < 3; i++)
            ImGui::Text("| %6.2f  %6.2f  %6.2f |", ultimaMatriz.m[i][0], ultimaMatriz.m[i][1], ultimaMatriz.m[i][2]);
    }
}

void dibujarPanel(int altoVentana)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)ANCHO_PANEL, (float)altoVentana));
    ImGui::Begin("Paint 2D", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    ImGui::SeparatorText("Herramientas");
    ImGui::RadioButton("Punto", &herramienta, H_PUNTO);      ImGui::SameLine(150); ImGui::RadioButton("Libre", &herramienta, H_LIBRE);
    ImGui::RadioButton("Línea", &herramienta, H_LINEA);      ImGui::SameLine(150); ImGui::RadioButton("Triángulo", &herramienta, H_TRIANGULO);
    ImGui::RadioButton("Círculo", &herramienta, H_CIRCULO);  ImGui::SameLine(150); ImGui::RadioButton("Cuadrado", &herramienta, H_CUADRADO);
    ImGui::RadioButton("Rellenar", &herramienta, H_RELLENAR); ImGui::SameLine(150); ImGui::RadioButton("Rectángulo", &herramienta, H_RECTANGULO);
    ImGui::RadioButton("Seleccionar", &herramienta, H_SELECCIONAR);

    ImGui::SeparatorText("Color");
    selectorColor();

    ImGui::SeparatorText("Transformaciones");
    panelTransformaciones();

    ImGui::SeparatorText("Lienzo");
    ImGui::SliderInt("Celda (px)", &tamCelda, 8, 40);
    if (ImGui::Button("Limpiar"))
    {
        borrarTodo(lienzo);
        seleccionada = -1;
        hayCambios = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Deshacer"))
    {
        deshacer(lienzo);
        if (seleccionada >= (int)lienzo.figuras.size())
            seleccionada = -1;
        hayCambios = true;
    }

    ImGui::SeparatorText("Información");
    ImGui::Text("Cuadrícula: %d x %d", lienzo.columnas, lienzo.filas);
    if (mouseEnCuadricula)
        ImGui::Text("Celda: (%d, %d)", celdaX, celdaY);
    else
        ImGui::Text("Celda: -");

    // datos de Bresenham mientras se dibuja una linea o un circulo
    if (dibujando && figuraTemporal.tipo == LINEA)
    {
        int x0 = redondear(figuraTemporal.vertices[0].x), y0 = redondear(figuraTemporal.vertices[0].y);
        int x1 = redondear(figuraTemporal.vertices[1].x), y1 = redondear(figuraTemporal.vertices[1].y);
        int dx = abs(x1 - x0), dy = abs(y1 - y0);
        ImGui::Text("Línea de (%d, %d) a (%d, %d)", x0, y0, x1, y1);
        ImGui::Text("dx = %d  dy = %d", dx, dy);
        if (dx >= dy)
            ImGui::Text("p0 = 2dy - dx = %d", 2 * dy - dx);
        else
            ImGui::Text("p0 = 2dx - dy = %d", 2 * dx - dy);
    }
    if (dibujando && figuraTemporal.tipo == CIRCULO)
    {
        int r = radioCirculo(figuraTemporal);
        ImGui::Text("Centro (%d, %d)  radio = %d", redondear(figuraTemporal.vertices[0].x), redondear(figuraTemporal.vertices[0].y), r);
        ImGui::Text("d0 = 3 - 2r = %d", 3 - 2 * r);
    }

    ImGui::End();
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------
int main()
{
    // ================= MOMENTO 1: CONSTRUCT =================
    // GLFW crea la ventana y el contexto de OpenGL, GLAD carga las funciones
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* ventana = glfwCreateWindow(1280, 800, "Paint 2D", NULL, NULL);
    if (ventana == NULL)
    {
        std::cout << "No se pudo crear la ventana" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(ventana);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "No se pudo cargar GLAD" << std::endl;
        return -1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

    // ImGui
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = NULL;
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(ventana, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    // ================= MOMENTO 2: GEOMETRY =================
    // La geometria son las celdas de la cuadricula (4 vertices por celda)
    int anchoVentana, altoVentana;
    glfwGetFramebufferSize(ventana, &anchoVentana, &altoVentana);
    calcularTamano(anchoVentana, altoVentana);

    std::vector<float> verticesCeldas;
    std::vector<unsigned int> indicesCeldas;
    std::vector<float> verticesRejilla;
    generarCeldas(verticesCeldas, indicesCeldas);
    generarRejilla(verticesRejilla);

    // ================= MOMENTO 3: SHADER COMPILATION =================
    unsigned int shaderProgram = crearPrograma("shaders/vertex.glsl", "shaders/fragment.glsl");
    int uTamCelda = glGetUniformLocation(shaderProgram, "uTamCelda");
    int uTamViewport = glGetUniformLocation(shaderProgram, "uTamViewport");

    // ================= MOMENTO 4: BUFFERS =================
    // VBO: vertices en la GPU. EBO: indices. VAO: como leer el VBO.
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesCeldas.size() * sizeof(float), verticesCeldas.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCeldas.size() * sizeof(unsigned int), indicesCeldas.data(), GL_DYNAMIC_DRAW);
    // atributo 0 = posicion (2 floats), atributo 1 = color (3 floats)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // la rejilla son lineas, no necesita EBO
    unsigned int VAOrejilla, VBOrejilla;
    glGenVertexArrays(1, &VAOrejilla);
    glGenBuffers(1, &VBOrejilla);
    glBindVertexArray(VAOrejilla);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrejilla);
    glBufferData(GL_ARRAY_BUFFER, verticesRejilla.size() * sizeof(float), verticesRejilla.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    hayCambios = false;

    // ================= MOMENTO 5: CONTINUUM (RENDER LOOP) =================
    while (!glfwWindowShouldClose(ventana))
    {
        glfwPollEvents();
        glfwGetFramebufferSize(ventana, &anchoVentana, &altoVentana);
        calcularTamano(anchoVentana, altoVentana);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        dibujarPanel(altoVentana);

        // ---- mouse ----
        ImGuiIO& io = ImGui::GetIO();
        mouseEnCuadricula = false;
        if (ImGui::IsMousePosValid())
            mouseEnCuadricula = mouseACelda(io.MousePos.x, io.MousePos.y, altoVentana);

        // io.WantCaptureMouse es true cuando el mouse esta sobre el panel
        bool clic = ImGui::IsMouseClicked(0) && mouseEnCuadricula && !io.WantCaptureMouse;
        bool solto = ImGui::IsMouseReleased(0);
        Punto actual = { (float)celdaX, (float)celdaY };

        if (clic)
        {
            if (herramienta == H_PUNTO)
            {
                guardarHistorial(lienzo);
                Figura punto;
                punto.tipo = PUNTO;
                punto.vertices.push_back(actual);
                punto.color = colorActual;
                agregarFigura(lienzo, punto);
                seleccionada = (int)lienzo.figuras.size() - 1;
                hayCambios = true;
            }
            else if (herramienta == H_RELLENAR)
            {
                int id = buscarFigura(lienzo, celdaX, celdaY, true);
                if (id >= 0)
                {
                    guardarHistorial(lienzo);
                    lienzo.figuras[id].rellena = true;
                    lienzo.figuras[id].colorRelleno = colorActual;
                    redibujarTodo(lienzo);
                    seleccionada = id;
                    hayCambios = true;
                }
            }
            else if (herramienta == H_SELECCIONAR)
            {
                seleccionada = buscarFigura(lienzo, celdaX, celdaY, false);
                hayCambios = true;
            }
            else
            {
                // empieza una figura nueva
                dibujando = true;
                puntoInicio = actual;
                figuraTemporal = Figura();
                figuraTemporal.color = colorActual;
                if (herramienta == H_LINEA) figuraTemporal.tipo = LINEA;
                if (herramienta == H_CIRCULO) figuraTemporal.tipo = CIRCULO;
                if (herramienta == H_LIBRE) figuraTemporal.tipo = LIBRE;
                if (herramienta == H_TRIANGULO) figuraTemporal.tipo = TRIANGULO;
                if (herramienta == H_CUADRADO) figuraTemporal.tipo = CUADRADO;
                if (herramienta == H_RECTANGULO) figuraTemporal.tipo = RECTANGULO;
            }
        }

        if (dibujando)
        {
            actualizarTemporal(actual);
            hayCambios = true;

            if (solto)
            {
                // al soltar el mouse la figura queda guardada
                guardarHistorial(lienzo);
                agregarFigura(lienzo, figuraTemporal);
                seleccionada = (int)lienzo.figuras.size() - 1;
                dibujando = false;
            }
        }

        // ---- si algo cambio se vuelve a mandar la geometria a la GPU ----
        if (hayCambios)
        {
            if (dibujando)
                dibujarFigura(lienzo, figuraTemporal, -1);   // vista previa encima

            generarCeldas(verticesCeldas, indicesCeldas);
            generarRejilla(verticesRejilla);

            if (dibujando)
                redibujarTodo(lienzo);   // se quita la vista previa del lienzo

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, verticesCeldas.size() * sizeof(float), verticesCeldas.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCeldas.size() * sizeof(unsigned int), indicesCeldas.data(), GL_DYNAMIC_DRAW);

            glBindVertexArray(VAOrejilla);
            glBindBuffer(GL_ARRAY_BUFFER, VBOrejilla);
            glBufferData(GL_ARRAY_BUFFER, verticesRejilla.size() * sizeof(float), verticesRejilla.data(), GL_DYNAMIC_DRAW);

            glBindVertexArray(0);
            hayCambios = false;
        }

        // ---- dibujar ----
        glViewport(0, 0, anchoVentana, altoVentana);
        glClearColor(0.92f, 0.92f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // solo se dibuja en el rectangulo de la cuadricula
        glViewport(cuadriculaX, cuadriculaY, cuadriculaAncho, cuadriculaAlto);
        glUseProgram(shaderProgram);
        glUniform1f(uTamCelda, (float)tamCelda);
        glUniform2f(uTamViewport, (float)cuadriculaAncho, (float)cuadriculaAlto);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indicesCeldas.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(VAOrejilla);
        glDrawArrays(GL_LINES, 0, (GLsizei)(verticesRejilla.size() / 5));
        glBindVertexArray(0);

        // el panel va encima de todo
        glViewport(0, 0, anchoVentana, altoVentana);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ventana);
    }

    // ================= MOMENTO 6: FREE MEMORY SPACE =================
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAOrejilla);
    glDeleteBuffers(1, &VBOrejilla);
    glDeleteProgram(shaderProgram);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(ventana);
    glfwTerminate();
    return 0;
}
