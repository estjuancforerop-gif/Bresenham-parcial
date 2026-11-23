# Paint – Editor gráfico sobre una cuadrícula

## Datos del proyecto

- **Nombre:** Juan Camilo Forero Páez
- **Código estudiantil:** 6014536
- **Lenguaje:** C++
- **Interfaz gráfica:** Dear ImGui
- **Renderizado:** OpenGL 3.3
- **Ventana y eventos:** GLFW
- **Carga de funciones OpenGL:** GLAD

## Descripción general

Paint es una aplicación de dibujo que representa figuras sobre una cuadrícula de casillas. El usuario elige una herramienta en el panel lateral (hecho con Dear ImGui), hace clic sobre la cuadrícula y el programa calcula qué casillas pertenecen a la figura.

Todo el dibujo de contornos se hace con **algoritmos de Bresenham**: las líneas, el círculo y los lados de triángulos, cuadrados y formas libres. 
El programa guarda cada figura como datos independientes (puntos, radio, colores) y **repinta la cuadrícula desde cero** cada vez que algo cambia. Por eso se puede rellenar, rotar, escalar y deshacer sin que la figura se deforme.

## Qué incluye (según la rúbrica)

| Requisito | Dónde está |
|---|---|
| Línea de Bresenham | `algoritmos.cpp` → `dibujarLinea` (usa `lineaHorizontal` y `lineaVertical`, cubre los 8 octantes) |
| Círculo de Bresenham | `algoritmos.cpp` → `dibujarCirculo` |
| Formas libres | `algoritmos.cpp` → `dibujarPoligono`; herramienta de forma libre en el panel |
| Triángulos en la cuadrícula | `FIGURA_TRIANGULO` (3 clics) |
| Cuadrados y rectángulos | `FIGURA_RECTANGULO` (2 clics; con Shift sale cuadrado perfecto) |
| Relleno de primitivas | `algoritmos.cpp` → `rellenarContorno` (inundación) usado por `figuras.cpp` |
| Transformaciones lineales | `figuras.cpp` → `aplicarMatriz`, `rotarFigura`, `escalarFigura` |
| Interfaz con Dear ImGui | `interfaz.cpp` (panel, selector de color, mouse y teclado) |

## Arquitectura del proyecto

El código está separado por responsabilidades. Cada archivo hace una sola cosa:

```
 usuario (mouse / teclado)
        │
        ▼
  interfaz.cpp ──────► figuras.cpp ──────► algoritmos.cpp
  (Dear ImGui:         (lista de figuras,   (Bresenham, círculo,
   panel y clics)       deshacer, matrices)   polígono, relleno)
        │                    │
        │                    ▼
        │               grilla.cpp
        │               (casillas y colores)
        ▼                    │
     main.cpp ◄──────────────┘
     (ventana GLFW, shaders OpenGL, bucle principal)
```

- **`main.cpp`:** punto de entrada. Inicializa GLFW, GLAD, OpenGL y Dear ImGui; crea los buffers de la cuadrícula y ejecuta el bucle principal.
- **`grilla.h` / `grilla.cpp`:** guarda filas, columnas, vértices, índices y colores de cada casilla. Sabe pintar una casilla, una lista de casillas y limpiar todo. No sabe nada de figuras.
- **`algoritmos.h` / `algoritmos.cpp`:** los algoritmos de la clase. No dibujan: devuelven la lista de casillas que forman cada figura.
- **`figuras.h` / `figuras.cpp`:** define los tipos de figura, guarda la lista de figuras, calcula contorno e interior, repinta la cuadrícula, deshace y aplica rotación y escalado.
- **`interfaz.h` / `interfaz.cpp`:** panel de herramientas (Dear ImGui), conversión del mouse a (columna, fila) y atajos de teclado.
- **`librerias/`:** Dear ImGui, GLFW y GLAD incluidos en el proyecto.

### Qué pasa en cada cuadro

1. El usuario elige una herramienta en el panel o hace clic en la cuadrícula.
2. `interfaz.cpp` convierte el clic a coordenadas de columna y fila.
3. Según la herramienta, se crea una figura nueva o se modifica una existente.
4. `figuras.cpp` conserva los datos de la figura y marca que la cuadrícula cambió.
5. `repintarGrilla()` limpia la cuadrícula y vuelve a pintar todas las figuras en orden; para cada una pide a `algoritmos.cpp` su contorno (y su interior si está rellena).
6. `grilla.cpp` actualiza los colores y OpenGL presenta el resultado.

## Algoritmos

### Línea de Bresenham
Usa solo enteros. Un parámetro de decisión `D` indica si la siguiente casilla queda en la misma fila (o columna) o en la vecina. Se divide en dos casos según la inclinación:

- `|dx| >= |dy|`: se avanza una casilla en X (`lineaHorizontal`).
- `|dy| > |dx|`: se avanza una casilla en Y (`lineaVertical`).

`dibujarLinea` elige el caso automáticamente, así que funciona en cualquier dirección.

### Círculo de Bresenham
Calcula solo un octavo de la circunferencia y obtiene los otros siete por simetría (`puntosSimetricos`). Empieza con `D = 3 - 2r`; en cada paso, si `D > 0` la coordenada `y` baja una casilla y `D` aumenta `4(x - y) + 10`, y si no, solo aumenta `4x + 6`. No usa raíces ni decimales.

### Polígonos (triángulo, cuadrado, forma libre)
Un polígono es una lista de puntos unidos de dos en dos con la línea de Bresenham. El rectángulo guarda sus 4 esquinas (no solo 2) para poder rotarlo.

### Relleno por inundación
Busca el exterior en lugar del interior: desde una esquina que está afuera se inunda de vecino en vecino (4 vecinos) sin cruzar el contorno. Lo que el agua nunca alcanza es el interior. Se usan 4 vecinos porque la línea de Bresenham es 8-conexa y así el agua no se cuela por las diagonales. Funciona con figuras cóncavas (por ejemplo una forma en "C").

### Transformaciones lineales
Cada punto se multiplica por una matriz 2×2 alrededor del centro de la figura (se resta el centro, se multiplica y se vuelve a sumar):

```
| x' |   | a  b |   | x |
| y' | = | c  d | * | y |
```

- **Rotación:** `| cos θ  -sin θ ; sin θ  cos θ |`. Como la Y de la pantalla crece hacia abajo, un ángulo positivo gira en el sentido de las manecillas del reloj.
- **Escala:** `| s  0 ; 0  s |`.

Los puntos se guardan con decimales para que varias rotaciones seguidas no deformen la figura; solo se redondean a casillas al momento de dibujar.

## Interfaz y controles

El panel de la izquierda (una sola ventana de Dear ImGui) tiene tres bloques:

| Bloque | Widget | Qué hace |
|---|---|---|
| Dibujar | Línea | 2 clics: inicio y fin |
| | Círculo | 2 clics: centro y un punto del borde |
| | Rectángulo | 2 clics: esquinas opuestas |
| | Cuadrado | 2 clics: una esquina y la contraria (los lados salen iguales) |
| | Triángulo | 3 clics: los 3 vértices |
| | Forma libre | Un clic por vértice. Se cierra con el botón **Cerrar forma**, **Enter** o **clic derecho** (mínimo 3 puntos) |
| Editar figura | Rellenar | Clic dentro de una figura: la rellena con el color de relleno |
| | Rotar + deslizador `grados` | Clic sobre una figura: la gira esos grados (de -180 a 180; negativo gira al otro lado) |
| | Escalar + deslizador `factor` | Clic sobre una figura: la escala por ese factor (de 0.5 a 2.0) |
| Colores | `borde` y `relleno` | Selectores de color (clic en el cuadrito) |

Abajo del panel hay dos botones (**Deshacer** y **Borrar todo**), un texto de ayuda de la herramienta elegida y la casilla donde está el mouse.

**Teclado:** `Esc` cancela la figura en curso, `Enter` cierra la forma libre, `Ctrl+Z` deshace (hasta 100 pasos) y `Supr` borra todo (también se puede deshacer).

**Consola:** cada vez que se termina una figura, el programa imprime paso a paso las casillas que calculó el algoritmo. Sirve para explicar el procedimiento.

**Nota:** el marco blanco que sigue al mouse es un indicador de Dear ImGui para ver en qué casilla se va a hacer clic; no es una figura.

## Compilación

El proyecto incluye `Paint.slnx` y `Paint.vcxproj` para abrirlo en Visual Studio (Windows, x64). Se deben conservar las carpetas de `librerias/` (incluyen `glfw3.lib`, los encabezados de GLFW/GLAD y Dear ImGui). El enlazado usa `glfw3.lib`, `opengl32.lib`, `user32.lib`, `gdi32.lib` y `shell32.lib`.

## Estructura de carpetas

```
Bresenham/
├── main.cpp
├── grilla.h / grilla.cpp
├── algoritmos.h / algoritmos.cpp
├── figuras.h / figuras.cpp
├── interfaz.h / interfaz.cpp
├── glad.c
├── Paint.slnx / Paint.vcxproj
├── README.md
└── librerias/   (imgui, include, lib)
```
