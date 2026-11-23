// ====================================================================
//  grilla.cpp
//  Autor: Juan Camilo Forero Paez
//
//  Aqui se arma la grilla de casillas y se pintan sus colores.
// ====================================================================
#include "grilla.h"

int columnas = 40;
int filas    = 40;

std::vector<Vertex> vertices;
std::vector<GLuint> indices;

// --------------------------------------------------------------------
// Esta funcion realiza el armado de la grilla desde cero.
// cada casilla tiene 4 vertices (las esquinas) y 6 indices (2 triangulos):
//
//      v0 ---- v1          triangulo 1: v0, v1, v2
//      |  \     |          triangulo 2: v0, v2, v3
//      |    \   |
//      v3 ---- v2
//
// Las posiciones van de -1 a 1 (asi lo pide OpenGL), la fila 0 queda arriba.
// --------------------------------------------------------------------
void construirGrilla()
{
    vertices.clear();
    indices.clear();

    // este calculo deja un hueco de SEPARACION_PX pixeles entre casillas
    float separacionX = 2.0f * SEPARACION_PX / (columnas * TAMANO_CASILLA_PX);
    float separacionY = 2.0f * SEPARACION_PX / (filas * TAMANO_CASILLA_PX);

    for (int fila = 0; fila < filas; fila++)
    {
        for (int columna = 0; columna < columnas; columna++)
        {
            float x0 = -1.0f + 2.0f * columna / (float)columnas + separacionX;
            float x1 = -1.0f + 2.0f * (columna + 1) / (float)columnas - separacionX;
            float y0 = 1.0f - 2.0f * fila / (float)filas - separacionY;
            float y1 = 1.0f - 2.0f * (fila + 1) / (float)filas + separacionY;

            // base = numero del primer vertice de esta casilla
            GLuint base = static_cast<GLuint>(vertices.size());

            vertices.push_back({ x0, y0, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // arriba-izquierda
            vertices.push_back({ x1, y0, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // arriba-derecha
            vertices.push_back({ x1, y1, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // abajo-derecha
            vertices.push_back({ x0, y1, 0.0f, COLOR_CELDA.r, COLOR_CELDA.g, COLOR_CELDA.b }); // abajo-izquierda

            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 0);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
        }
    }
}

// --------------------------------------------------------------------
// Esta funcion realiza el pintado de una casilla: busco sus 4 vertices y les
// cambio el color. Si la casilla esta fuera de la grilla no hago nada (asi
// una figura puede salirse un poco sin que el programa se rompa).
// --------------------------------------------------------------------
void pintarCelda(int columna, int fila, Color color)
{
    if (columna < 0 || columna >= columnas || fila < 0 || fila >= filas) return;

    int numeroDeCasilla = fila * columnas + columna;   // las casillas estan guardadas fila por fila
    int primerVertice   = numeroDeCasilla * 4;         // cada casilla ocupa 4 vertices seguidos

    for (int i = 0; i < 4; i++)
    {
        vertices[primerVertice + i].r = color.r;
        vertices[primerVertice + i].g = color.g;
        vertices[primerVertice + i].b = color.b;
    }
}

// Esta funcion realiza el pintado de todas las casillas que me calculo un algoritmo.
void pintarCeldas(const std::vector<Celda>& celdas, Color color)
{
    for (size_t i = 0; i < celdas.size(); i++)
        pintarCelda(celdas[i].first, celdas[i].second, color);
}

// Esta funcion realiza el borrado de todo el dibujo antes de repintar las figuras.
void limpiarGrilla()
{
    for (int fila = 0; fila < filas; fila++)
        for (int columna = 0; columna < columnas; columna++)
            pintarCelda(columna, fila, COLOR_CELDA);
}
