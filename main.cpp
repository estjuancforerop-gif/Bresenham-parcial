// ====================================================================
//  PAINT  -  main.cpp
//  Autor: Juan Camilo Forero Paez
//
//  Este archivo realiza la apertura de la ventana, la preparacion de
//  OpenGL y el bucle principal. La logica de cada cosa esta en su propio archivo:
//
//     grilla.cpp      -> las casillas y sus colores
//     algoritmos.cpp  -> linea y circulo de Bresenham, poligonos y relleno
//     figuras.cpp     -> la lista de figuras, rellenar, rotar, escalar, deshacer
//     interfaz.cpp    -> el panel de herramientas (Dear ImGui) y el mouse
//     main.cpp        -> la ventana, los shaders y el bucle (este archivo)
// ====================================================================
#include <iostream>
#include <vector>
#include <cmath>
#include <cstddef>   // offsetof
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "grilla.h"
#include "figuras.h"
#include "interfaz.h"

// --------------------------------------------------------------------
// SHADERS
// Estos dos textos le dicen a la tarjeta grafica como dibujar:
//  - el vertex shader coloca cada vertice y le pasa su color
//  - el fragment shader pinta cada pixel con ese color
// --------------------------------------------------------------------
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"    ourColor = aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 ourColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(ourColor, 1.0f);\n"
"}\n\0";

// Estas variables guardan los objetos de OpenGL donde vive la grilla:
//  VBO guarda los vertices, EBO guarda los indices de los triangulos
//  y VAO recuerda como leer el VBO
GLuint VAO = 0, VBO = 0, EBO = 0;
GLuint programaShader = 0;

// --------------------------------------------------------------------
// VENTANA
// --------------------------------------------------------------------

// Esta funcion realiza la creacion de la ventana y la carga de OpenGL con GLAD.
GLFWwindow* crearVentana()
{
    glfwInit();

    // pido OpenGL 3.3 en perfil core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);   // la ventana se puede agrandar

    GLFWwindow* window = glfwCreateWindow(ANCHO_INICIAL_PX, ALTO_INICIAL_PX,
                                          "Paint - Juan Camilo Forero Paez", NULL, NULL);
    if (window == nullptr)
    {
        std::cout << "Fallo en la creacion de la ventana" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    // tamano minimo para que el panel siempre quepa
    glfwSetWindowSizeLimits(window, ANCHO_MINIMO_PX, ALTO_MINIMO_PX, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    gladLoadGL();
    glfwSwapInterval(1);   // sincroniza con el monitor para no gastar CPU de mas
    return window;
}

// --------------------------------------------------------------------
// SHADERS Y BUFFERS
// --------------------------------------------------------------------

// Esta funcion realiza la compilacion de los dos shaders y los junta en un programa.
GLuint crearShaders()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    GLuint programa = glCreateProgram();
    glAttachShader(programa, vertexShader);
    glAttachShader(programa, fragmentShader);
    glLinkProgram(programa);

    // ya estan dentro del programa, los sueltos ya no hacen falta
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return programa;
}

// Esta funcion realiza la subida de TODA la grilla a la GPU. La uso al inicio
// y cada vez que cambia la cantidad de casillas (porque cambia el tamano del arreglo).
void subirBuffers()
{
    glBindVertexArray(VAO);

    // GL_DYNAMIC_DRAW porque los colores cambian mientras corre el programa
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    // los indices no cambian mientras la grilla tenga el mismo tamano
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

// Esta funcion realiza la creacion de los objetos y le dice a OpenGL como leer cada vertice.
void crearBuffers()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // atributo 0 = posicion (3 floats al inicio del vertice)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    // atributo 1 = color (3 floats despues de la posicion)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    subirBuffers();   // ahora si, subo la grilla
}

// Esta funcion realiza el envio de solo los colores nuevos (el arreglo mide lo mismo).
void actualizarVertices()
{
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// --------------------------------------------------------------------
// LA GRILLA SEGUN LA VENTANA
// --------------------------------------------------------------------

// Esta funcion realiza el ajuste de la grilla al tamano de la ventana: cuando
// agrando la ventana aparecen MAS casillas (no se estiran las casillas).
// Reviso cuantas caben y, si cambio, reconstruyo la grilla. Redondeo hacia
// arriba para que no sobre espacio vacio: la ultima casilla puede quedar cortada.
void ajustarGrillaAlLienzo()
{
    int nuevasColumnas = std::max(1, (int)std::ceil(lienzo.ancho / TAMANO_CASILLA_PX));
    int nuevasFilas    = std::max(1, (int)std::ceil(lienzo.alto  / TAMANO_CASILLA_PX));

    if (nuevasColumnas == columnas && nuevasFilas == filas) return;   // nada cambio

    columnas = nuevasColumnas;
    filas    = nuevasFilas;

    construirGrilla();    // vertices nuevos (todos vacios)
    subirBuffers();       // los subo a la GPU
    grillaCambio = true;  // hay que volver a pintar las figuras
}

// Esta funcion realiza el dibujo de la grilla con OpenGL en la zona del lienzo.
void dibujarGrilla(GLFWwindow* window)
{
    // en pantallas de alta resolucion los pixeles reales son mas que los de
    // la ventana, por eso calculo la escala
    int anchoFb, altoFb;
    glfwGetFramebufferSize(window, &anchoFb, &altoFb);
    ImVec2 ventana = ImGui::GetIO().DisplaySize;
    float escalaX = anchoFb / ventana.x;
    float escalaY = altoFb  / ventana.y;

    // tamano exacto de la grilla en pixeles
    float anchoGrilla = (float)(columnas * TAMANO_CASILLA_PX);
    float altoGrilla  = (float)(filas * TAMANO_CASILLA_PX);

    // glViewport dice en que parte de la ventana se dibuja. OpenGL cuenta la
    // Y desde ABAJO (el mouse la cuenta desde ARRIBA), por eso la invierto
    int vx = (int)std::lround(lienzo.x * escalaX);
    int vw = (int)std::lround(anchoGrilla * escalaX);
    int vh = (int)std::lround(altoGrilla * escalaY);
    int vy = altoFb - (int)std::lround((lienzo.y + altoGrilla) * escalaY);
    glViewport(vx, vy, vw, vh);

    glUseProgram(programaShader);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// --------------------------------------------------------------------
// UN CUADRO COMPLETO
// Esta funcion realiza todo lo que pasa en cada vuelta, en este orden:
//   1) empiezo el cuadro de ImGui y calculo donde esta el lienzo
//   2) leo mouse y teclado, y armo el panel
//   3) si algo cambio, repinto la grilla y subo los colores
//   4) dibujo la grilla y encima la interfaz
// --------------------------------------------------------------------
void dibujarCuadro(GLFWwindow* window)
{
    comenzarCuadroGUI();
    calcularLienzo();
    ajustarGrillaAlLienzo();

    // Primero se construye la interfaz para que WantCaptureMouse refleje
    // los controles y popups visibles en ESTE cuadro.
    dibujarPanel();
    procesarMouse();
    procesarTeclado();

    if (grillaCambio)
    {
        repintarGrilla();
        actualizarVertices();
        grillaCambio = false;
    }

    // primero limpio TODA la ventana, luego dibujo la grilla y encima la interfaz
    int anchoFb, altoFb;
    glfwGetFramebufferSize(window, &anchoFb, &altoFb);
    glViewport(0, 0, anchoFb, altoFb);
    glClear(GL_COLOR_BUFFER_BIT);

    dibujarGrilla(window);
    dibujarCasillaBajoMouse();
    terminarCuadroGUI();

    glfwSwapBuffers(window);
}

// Esta funcion realiza el redibujo de la ventana en Windows mientras arrastro
// el borde para cambiarle el tamano (ahi el bucle principal se detiene).
void alRefrescarVentana(GLFWwindow* window)
{
    dibujarCuadro(window);
}

// --------------------------------------------------------------------
// MAIN
// --------------------------------------------------------------------
int main()
{
    GLFWwindow* window = crearVentana();
    if (window == nullptr) return -1;

    programaShader = crearShaders();

    construirGrilla();   // grilla inicial, todas las casillas vacias
    crearBuffers();      // la subo a la GPU
    iniciarGUI(window);  // arranco Dear ImGui

    glfwSetWindowRefreshCallback(window, alRefrescarVentana);

    // color de fondo (se ve si la ventana es mas grande que la grilla)
    glClearColor(0.110f, 0.118f, 0.140f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();        // revisa los eventos de la ventana
        dibujarCuadro(window);   // un cuadro completo
    }

    // libero todo lo que cree
    cerrarGUI();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(programaShader);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
