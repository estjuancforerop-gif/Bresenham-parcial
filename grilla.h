// ====================================================================
//  grilla.h
//  Autor: Juan Camilo Forero Paez
//
//  Este archivo guarda todo lo que tiene que ver con la grilla (los
//  vertices, los colores y las funciones para pintar casillas).
//  Nada de aqui sabe de figuras ni de la interfaz, solo de casillas.
// ====================================================================
#pragma once
#include <vector>
#include <utility>
#include <glad/glad.h>

// Estas constantes deciden cuanto mide cada casilla en pantalla. Como son
// constantes, las casillas siempre son cuadradas aunque cambie la ventana.
const int   TAMANO_CASILLA_PX = 20;
const float SEPARACION_PX     = 0.8f;   // el espacio entre casillas, asi se nota la grilla

// Esta estructura guarda la posicion y el color de cada vertice (lo lee OpenGL).
struct Vertex
{
    GLfloat x, y, z;
    GLfloat r, g, b;
};

// Esta estructura guarda un color RGB (cada valor va de 0 a 1).
struct Color
{
    GLfloat r, g, b;
};

// Este tipo guarda una casilla como (columna, fila).
typedef std::pair<int, int> Celda;

// Esta constante guarda el color de una casilla vacia (azul oscuro).
const Color COLOR_CELDA = { 0.212f, 0.278f, 0.463f };

// Estas variables guardan cuantas casillas caben ahora mismo (cambian cuando se agranda la ventana).
extern int columnas;
extern int filas;

// Estos dos arreglos guardan la grilla completa: 4 vertices y 6 indices por cada casilla.
extern std::vector<Vertex>  vertices;
extern std::vector<GLuint>  indices;

void construirGrilla();                                            // Esta funcion realiza el armado de todos los vertices e indices
void pintarCelda(int columna, int fila, Color color);              // Esta funcion realiza el pintado de UNA casilla
void pintarCeldas(const std::vector<Celda>& celdas, Color color);  // Esta funcion realiza el pintado de una lista de casillas
void limpiarGrilla();                                              // Esta funcion realiza la limpieza: deja todas las casillas vacias
