// ====================================================================
//  algoritmos.h
//  Autor: Juan Camilo Forero Paez
//
//  Este archivo declara los algoritmos de la clase. Ninguno dibuja
//  nada en pantalla: solo calculan QUE casillas hay que pintar y las
//  devuelven en una lista (vector de Celda). Despues otra funcion
//  (pintarCeldas en grilla.cpp) es la que las colorea.
//
//  Todo el dibujo de contornos sale de Bresenham:
//     - linea     -> Bresenham de lineas (enteros, sin decimales)
//     - circulo   -> Bresenham de circunferencias (enteros)
//     - poligonos -> varias lineas de Bresenham unidas
//  El relleno se hace con inundacion sobre ese contorno.
// ====================================================================
#pragma once
#include <vector>
#include "grilla.h"

// Esta funcion realiza la linea de Bresenham entre dos casillas.
// Sirve para los 8 octantes (cualquier inclinacion y direccion).
void dibujarLinea(int x0, int y0, int x1, int y1, std::vector<Celda>& celdas);

// Esta funcion realiza el circulo de Bresenham: recibe el centro (cx, cy)
// y el radio r, y devuelve las casillas de la circunferencia.
void dibujarCirculo(int cx, int cy, int r, std::vector<Celda>& celdas);

// Esta funcion realiza un poligono uniendo los puntos con lineas de
// Bresenham (sirve para triangulos, cuadrados/rectangulos y formas libres).
// Si cerrado es true, tambien une el ultimo punto con el primero.
void dibujarPoligono(const std::vector<Celda>& esquinas, bool cerrado, std::vector<Celda>& celdas);

// Esta funcion realiza el relleno por inundacion: recibe el contorno de
// una figura y devuelve las casillas de ADENTRO (el contorno no va incluido).
std::vector<Celda> rellenarContorno(const std::vector<Celda>& contorno);
