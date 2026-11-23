// ====================================================================
//  interfaz.h
//  Autor: Juan Camilo Forero Paez
//
//  Este archivo declara todo lo que el usuario ve y toca: el panel de
//  herramientas (hecho con Dear ImGui) y la lectura del mouse y teclado.
// ====================================================================
#pragma once

struct GLFWwindow;   // solo aviso que existe, no necesito todo GLFW aqui

// Estas constantes guardan los tamanos de la ventana (en pixeles)
const int ANCHO_PANEL_PX   = 230;   // ancho del panel de herramientas (queda a la izquierda)
const int ANCHO_INICIAL_PX = 1030;  // ancho de la ventana al abrir (panel + 800 de grilla)
const int ALTO_INICIAL_PX  = 800;   // alto de la ventana al abrir
const int ANCHO_MINIMO_PX  = 630;   // ancho minimo para que quepa el panel y algo de grilla
const int ALTO_MINIMO_PX   = 600;   // alto minimo para que quepa todo el panel

// Esta estructura guarda en que parte de la ventana esta la grilla
// (esta a la derecha del panel y ocupa todo lo que sobra).
struct Lienzo
{
    float x, y;          // esquina de arriba-izquierda
    float ancho, alto;   // tamano
};
extern Lienzo lienzo;

void calcularLienzo();          // Esta funcion realiza el recalculo del lienzo con el tamano actual de la ventana

void iniciarGUI(GLFWwindow* window);
void comenzarCuadroGUI();
void terminarCuadroGUI();
void cerrarGUI();

void dibujarPanel();             // Esta funcion realiza el panel de herramientas
void procesarMouse();            // Esta funcion realiza la lectura de clicks sobre la grilla
void procesarTeclado();          // Esta funcion realiza la lectura de atajos (Ctrl+Z, Supr, Esc, Enter)
void dibujarCasillaBajoMouse();  // Esta funcion realiza el borde blanco en la casilla donde esta el mouse
