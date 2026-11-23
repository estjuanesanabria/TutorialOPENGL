# Paint 2D - Computación Gráfica

**Nombre:** Juan Esteban Sanabria Adame
**Código:** 6014495
**Materia:** Introducción a la Computación Gráfica - Ingeniería en Multimedia, UMNG

## Descripción

Paint en 2D hecho en C++ con OpenGL. El lienzo es una cuadrícula y cada celda funciona como un píxel.
Las figuras se dibujan con los algoritmos de Bresenham, se rellenan con el algoritmo de línea de barrido
(scan-line) y se pueden transformar con matrices de 3x3 en coordenadas homogéneas.

Librerías:
- **OpenGL 4.6**: dibuja las celdas usando shaders, VAO, VBO y EBO.
- **GLFW**: crea la ventana y maneja el mouse.
- **GLAD**: carga las funciones de OpenGL.
- **Dear ImGui**: el panel de herramientas.

## Arquitectura del proyecto

```
TutorialOPENGL/
├── main.cpp               ventana, los 6 momentos de OpenGL, panel de ImGui y mouse
├── lienzo.h / .cpp        cuadrícula, figuras, rellenar, seleccionar y deshacer
├── algoritmos.h / .cpp    Bresenham de líneas y círculos, relleno scan-line
├── transformaciones.h/.cpp matrices de traslación, rotación, escalamiento, reflexión y cizallamiento
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
├── glad.c
└── libraries/             GLAD, GLFW y Dear ImGui
```

Cómo funciona:
1. El mouse se convierte en una celda (x, y) de la cuadrícula.
2. Cada figura guarda sus vértices. Los algoritmos calculan qué celdas pintar.
3. Las celdas pintadas se convierten en vértices (4 por celda) y se mandan a la GPU con VBO y EBO.
4. Los shaders las dibujan en la pantalla.

Las transformaciones multiplican cada vértice por la matriz (`P' = M * P` con `P = [x, y, 1]`) y después
se vuelve a pintar la figura. Para que la figura rote o se escale sobre sí misma se usa
`T(centro) * M * T(-centro)`.

### Los 6 momentos de OpenGL en main.cpp

1. **Construct:** ventana y contexto con GLFW, funciones con GLAD.
2. **Geometry:** las celdas de la cuadrícula como vértices.
3. **Shader compilation:** compilar y enlazar los shaders.
4. **Buffers:** VAO, VBO y EBO.
5. **Continuum:** el render loop.
6. **Free memory space:** liberar buffers, shaders, ImGui y GLFW.

## Funciones

- Puntos, líneas (Bresenham), círculos (Bresenham), dibujo libre, triángulos, cuadrados y rectángulos
- Rellenar figuras cerradas con color sólido (scan-line)
- Selector de color y paleta
- Seleccionar una figura
- Transformaciones: traslación, rotación, escalamiento, reflexión en X y en Y, cizallamiento en X y en Y
- Limpiar, deshacer y cambiar el tamaño de las celdas

## Matrices usadas

| Transformación | Matriz |
|---|---|
| Traslación | `[1 0 tx; 0 1 ty; 0 0 1]` |
| Rotación | `[cos -sen 0; sen cos 0; 0 0 1]` |
| Escalamiento | `[sx 0 0; 0 sy 0; 0 0 1]` |
| Reflexión en X | `[1 0 0; 0 -1 0; 0 0 1]` |
| Reflexión en Y | `[-1 0 0; 0 1 0; 0 0 1]` |
| Cizallamiento en X | `[1 shx 0; 0 1 0; 0 0 1]` |
| Cizallamiento en Y | `[1 0 0; shy 1 0; 0 0 1]` |

## Cómo ejecutarlo

1. Abrir `TutorialOPENGL.slnx` en Visual Studio.
2. Poner la configuración en **Debug** y **x64**.
3. Ejecutar con **Ctrl + F5**.
