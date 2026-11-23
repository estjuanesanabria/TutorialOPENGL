# Paint 2D con C++ y OpenGL

**Nombre:** Juan Esteban Sanabria Adame
**Código:** 6014495
**Materia:** Introducción a la Computación Gráfica
**Programa:** Ingeniería en Multimedia, Universidad Militar Nueva Granada (UMNG)

---

## Video explicativo

[![Video explicativo del proyecto](https://img.youtube.com/vi/0VSzSrdqTR0/0.jpg)](https://www.youtube.com/watch?v=0VSzSrdqTR0)

Enlace: https://www.youtube.com/watch?v=0VSzSrdqTR0

En el video explico el código archivo por archivo y al final muestro el programa funcionando.

---

## Descripción

Es un Paint en 2D hecho en C++ con OpenGL. El lienzo es una cuadrícula donde cada celda funciona como un píxel, y todas las herramientas pintan sobre esas celdas.

- Las líneas y los círculos se dibujan con los algoritmos de Bresenham.
- Las figuras cerradas se rellenan con el algoritmo de línea de barrido (scan-line).
- Las figuras se pueden transformar con matrices de 3x3 en coordenadas homogéneas, hechas a mano, sin librerías de matemáticas.

---

## Funciones

**Herramientas de dibujo**
- Punto
- Línea (Bresenham)
- Círculo (Bresenham)
- Dibujo libre (los puntos del mouse se unen con líneas de Bresenham)
- Triángulo, cuadrado y rectángulo

**Color y relleno**
- Selector de color y paleta de 8 colores
- Herramienta Rellenar: con un clic dentro de una figura cerrada, se rellena con color sólido

**Transformaciones** (se aplican a la figura seleccionada)
- Traslación
- Rotación
- Escalamiento
- Reflexión en X y en Y
- Cizallamiento en X y en Y

**Lienzo**
- Seleccionar figuras (la figura seleccionada se marca con un recuadro rojo)
- Deshacer (hasta 50 pasos)
- Limpiar el lienzo
- Cambiar el tamaño de las celdas (de 8 a 40 píxeles)
- Información en pantalla: la celda donde está el mouse y los valores iniciales de Bresenham mientras se dibuja

---

## Tecnologías

| Herramienta | Uso |
|---|---|
| C++ | Lenguaje del proyecto |
| OpenGL 4.6 (Core Profile) | Dibuja las celdas con shaders, VAO, VBO y EBO |
| GLSL 4.60 | Vertex shader y fragment shader |
| GLFW | Crea la ventana y maneja el mouse |
| GLAD | Carga las funciones de OpenGL |
| Dear ImGui 1.93.0 | Panel de herramientas |
| Visual Studio | Compilación (Debug, x64) |

---

## Arquitectura del proyecto

```
TutorialOPENGL/
├── main.cpp                  ventana, los 6 momentos de OpenGL, panel de ImGui y manejo del mouse
├── lienzo.h / lienzo.cpp     cuadrícula, figuras, rellenar, seleccionar y deshacer
├── algoritmos.h / .cpp       Bresenham de líneas y círculos, relleno scan-line
├── transformaciones.h / .cpp matrices 3x3 y aplicación de transformaciones
├── shaders/
│   ├── vertex.glsl           convierte la posición de la celda a coordenadas de pantalla
│   └── fragment.glsl         pinta cada celda con su color
├── glad.c
└── libraries/                GLAD, GLFW y Dear ImGui
```

### Paradigma

El proyecto usa **programación procedimental modular**:

- Los datos están en `struct` sin métodos: `Figura`, `Lienzo` y `Matriz3`.
- Las acciones son funciones que reciben esos datos, por ejemplo `dibujarFigura(lienzo, figura)`.
- Cada módulo tiene una sola responsabilidad:

| Módulo | Responsabilidad |
|---|---|
| `algoritmos` | Calcula qué celdas pintar. No dibuja nada. |
| `lienzo` | Guarda las figuras y pinta las celdas. |
| `transformaciones` | Arma las matrices y las aplica a los vértices. |
| `main` | Maneja la ventana, el mouse, el panel y el ciclo de dibujo. |

### Cómo funciona

1. La posición del mouse se convierte en una celda (x, y) de la cuadrícula. El punto (0, 0) está abajo a la izquierda.
2. Cada figura guarda solo sus vértices. Los algoritmos calculan qué celdas pintar.
3. Cada celda se convierte en un cuadrado de 4 vértices y 2 triángulos (6 índices). Esos datos se mandan a la tarjeta de video con VBO y EBO.
4. Los shaders dibujan las celdas en la pantalla.
5. Los datos solo se vuelven a mandar a la tarjeta de video cuando algo cambia, no en todos los frames.

Como el proyecto guarda los vértices de cada figura y no solo los píxeles, se puede transformar cualquier figura y volver a dibujarla con los mismos algoritmos.

---

## Los 6 momentos de OpenGL

Están marcados con comentarios en `main.cpp`:

1. **Construct:** se inicia GLFW, se crea la ventana con contexto OpenGL 4.6 Core, se cargan las funciones con GLAD y se inicia ImGui.
2. **Geometry:** las celdas de la cuadrícula y las líneas de la rejilla se convierten en vértices.
3. **Shader compilation:** se leen, compilan y enlazan el vertex shader y el fragment shader.
4. **Buffers:** se crean el VAO, el VBO y el EBO y se definen los atributos: posición (2 floats) y color (3 floats).
5. **Continuum:** el render loop lee el mouse, actualiza las figuras, dibuja con `glDrawElements` y `glDrawArrays` y muestra el frame.
6. **Free memory space:** se liberan los buffers y el programa de shaders, y se cierran ImGui y GLFW.

---

## Algoritmos

### Bresenham para líneas

En cada paso se elige entre dos celdas usando solo sumas de enteros.

```
p0 = 2dy - dx
si p < 0   ->  y se queda,  p = p + 2dy
si p >= 0  ->  y avanza,    p = p + 2dy - 2dx
```

Cuando la línea es más vertical que horizontal (`dy > dx`), se hace lo mismo cambiando `x` por `y`.

### Bresenham para círculos

Se calcula solo un octante, desde (0, r) hasta que x = y. Los otros 7 octantes se sacan por simetría.

```
d0 = 3 - 2r
si d < 0   ->  y se queda,  d = d + 4x + 6
si d >= 0  ->  y baja,      d = d + 4(x - y) + 10
```

### Relleno scan-line

La figura se recorre fila por fila:

1. En cada fila se calcula dónde la corta cada lado del polígono.
2. Los cortes se ordenan de izquierda a derecha.
3. Se pintan las celdas entre cada par de cortes.

El corte de una fila con un lado se calcula así:

```
x = x1 + (y - y1) * (x2 - x1) / (y2 - y1)
```

El círculo se rellena con la misma idea. En cada fila, el círculo va desde `-raiz(r² - dy²)` hasta `+raiz(r² - dy²)`.

### Seleccionar y rellenar

Para saber si un clic cayó dentro de una figura, se traza una línea horizontal desde el punto y se cuentan los lados que cruza. Si el número es impar, el punto está adentro.

---

## Transformaciones

Cada vértice se multiplica por la matriz con el punto en forma de columna:

```
P' = M * P      con P = [x, y, 1]
```

### Matrices usadas

```
Traslación           Rotación                   Escalamiento
| 1  0  tx |         | cos  -sen  0 |           | sx  0   0 |
| 0  1  ty |         | sen   cos  0 |           | 0   sy  0 |
| 0  0  1  |         |  0     0   1 |           | 0   0   1 |

Reflexión en X       Reflexión en Y
| 1   0  0 |         | -1  0  0 |
| 0  -1  0 |         |  0  1  0 |
| 0   0  1 |         |  0  0  1 |

Cizallamiento en X   Cizallamiento en Y
| 1  shx  0 |        |  1   0  0 |
| 0   1   0 |        | shy  1  0 |
| 0   0   1 |        |  0   0  1 |
```

### Transformar sobre el centro de la figura

Las matrices rotan y escalan alrededor del punto (0, 0). Para que la figura se transforme sobre su propio centro `c`, se combinan tres matrices:

```
total = T(c) * M * T(-c)
```

1. `T(-c)` lleva el centro de la figura al origen.
2. `M` aplica la transformación.
3. `T(c)` devuelve la figura a su lugar.

Si un círculo se escala distinto en X y en Y, o se cizalla, se convierte en un polígono de 36 lados. Así se puede dibujar la elipse que resulta.

---

## Cómo ejecutarlo

**Requisitos:** Windows y Visual Studio con el paquete *Desarrollo para el escritorio con C++*. Las librerías ya vienen dentro de la carpeta `libraries`.

1. Clonar o descargar este repositorio.
2. Abrir `TutorialOPENGL.slnx` en Visual Studio.
3. Poner la configuración en **Debug** y la plataforma en **x64**.
4. Ejecutar con **Ctrl + F5**.

### Problemas comunes

- **No se encuentran los shaders:** el programa busca la carpeta `shaders` desde la carpeta del proyecto. Hay que ejecutarlo desde Visual Studio, o copiar la carpeta `shaders` junto al `.exe`.
- **Error al enlazar `glfw3.lib`:** la librería está compilada para Debug x64. Hay que usar esa configuración.
- **Windows bloquea el ejecutable** (mensaje «Una directiva de Control de aplicaciones bloqueó este archivo»): es el Control inteligente de aplicaciones de Windows. Hay que desactivarlo en Seguridad de Windows o ejecutar el programa en otro equipo.

---

## Cómo usarlo

| Herramienta | Uso |
|---|---|
| Punto | Clic en una celda. |
| Línea, rectángulo, cuadrado, triángulo | Clic y arrastrar. La figura queda al soltar el mouse. |
| Círculo | Se presiona en el centro y se arrastra hasta el radio que se quiere. |
| Libre | Arrastrar el mouse. |
| Rellenar | Elegir un color y hacer clic dentro de una figura cerrada. |
| Seleccionar | Clic sobre una figura. La última figura dibujada queda seleccionada automáticamente. |
| Transformaciones | Escribir los valores en el panel y presionar el botón. El panel muestra la última matriz aplicada. |
| Deshacer / Limpiar | Botones de la sección Lienzo. |
