// ====================================================================
//  figuras.cpp
//  Autor: Juan Camilo Forero Paez
//
//  Aqui vive la lista de figuras y todo lo que se les hace: crearlas desde
//  los clicks, calcular su contorno (Bresenham) y su interior (inundacion),
//  repintar la grilla, deshacer y aplicar transformaciones lineales.
// ====================================================================
#include "figuras.h"
#include "algoritmos.h"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <algorithm>

std::vector<Figura> figuras;
FiguraEnCurso enCurso = { false, FIGURA_LINEA, { 0.0f, 0.0f, 0.0f }, {} };
bool grillaCambio = false;

static const double PI = 3.14159265358979323846;

// --------------------------------------------------------------------
// DESHACER
// Esta seccion realiza el deshacer de cualquier cosa (dibujar, rellenar,
// rotar, borrar todo): antes de cambiar algo guardo una copia de TODA la
// lista, y deshacer solo vuelve a la ultima copia.
// --------------------------------------------------------------------
static std::vector<std::vector<Figura>> historial;
static const size_t MAXIMO_HISTORIAL = 100;

// Esta funcion realiza el guardado de una copia de la lista antes de un cambio
// (si ya hay 100 copias, borra la mas vieja).
static void guardarParaDeshacer()
{
    historial.push_back(figuras);
    if (historial.size() > MAXIMO_HISTORIAL)
        historial.erase(historial.begin());
}

// Esta funcion realiza el deshacer: vuelve a la ultima copia guardada.
void deshacer()
{
    if (historial.empty()) return;
    figuras = historial.back();
    historial.pop_back();
    grillaCambio = true;
}

// Esta funcion realiza el borrado de todas las figuras (se puede deshacer).
void borrarTodo()
{
    if (figuras.empty()) return;
    guardarParaDeshacer();
    figuras.clear();
    grillaCambio = true;
}

// --------------------------------------------------------------------
// CONVERTIR ENTRE PUNTOS Y CASILLAS
// --------------------------------------------------------------------

// Esta funcion realiza el paso de un punto con decimales a la casilla mas cercana.
static Celda aCelda(const Punto& p)
{
    return Celda((int)std::lround(p.x), (int)std::lround(p.y));
}

// Esta funcion realiza el paso de todos los puntos de una figura a casillas enteras.
static std::vector<Celda> casillasDeLosPuntos(const Figura& f)
{
    std::vector<Celda> lista;
    for (size_t i = 0; i < f.puntos.size(); i++)
        lista.push_back(aCelda(f.puntos[i]));
    return lista;
}

// --------------------------------------------------------------------
// CREAR UNA FIGURA A PARTIR DE LOS CLICKS
// Esta funcion realiza la creacion de una figura a partir de los clicks
// (casillas). Cada tipo usa los clicks de forma distinta.
// --------------------------------------------------------------------
Figura crearFigura(TipoFigura tipo, const std::vector<Celda>& clicks, Color color)
{
    Figura f;
    f.tipo    = tipo;
    f.radio   = 0.0;
    f.relleno = false;
    f.color   = color;
    f.colorRelleno = { color.r * 0.5f, color.g * 0.5f, color.b * 0.5f };

    switch (tipo)
    {
    case FIGURA_LINEA:
    case FIGURA_TRIANGULO:
    case FIGURA_LIBRE:
        // los puntos son los mismos clicks
        for (size_t i = 0; i < clicks.size(); i++)
            f.puntos.push_back({ (double)clicks[i].first, (double)clicks[i].second });
        break;

    case FIGURA_RECTANGULO:
    {
        // los 2 clicks son esquinas opuestas: de ahi saco las 4 esquinas.
        // guardo las 4 (y no solo 2) para que despues se pueda rotar
        int xMin = std::min(clicks[0].first,  clicks[1].first);
        int xMax = std::max(clicks[0].first,  clicks[1].first);
        int yMin = std::min(clicks[0].second, clicks[1].second);
        int yMax = std::max(clicks[0].second, clicks[1].second);
        f.puntos.push_back({ (double)xMin, (double)yMin });   // arriba-izquierda
        f.puntos.push_back({ (double)xMax, (double)yMin });   // arriba-derecha
        f.puntos.push_back({ (double)xMax, (double)yMax });   // abajo-derecha
        f.puntos.push_back({ (double)xMin, (double)yMax });   // abajo-izquierda
        break;
    }

    case FIGURA_CIRCULO:
    {
        // el 1er click es el centro y el 2do es un punto del borde:
        // el radio es la distancia entre los dos (pitagoras)
        double dx = clicks[1].first  - clicks[0].first;
        double dy = clicks[1].second - clicks[0].second;
        f.puntos.push_back({ (double)clicks[0].first, (double)clicks[0].second });
        f.radio = std::sqrt(dx * dx + dy * dy);
        break;
    }
    }
    return f;
}

// --------------------------------------------------------------------
// CALCULAR LAS CASILLAS DE UNA FIGURA
// --------------------------------------------------------------------

// Esta funcion realiza el calculo del contorno: llama al algoritmo de
// Bresenham que le toca a cada tipo (linea, circulo o poligono).
static std::vector<Celda> contornoDeFigura(const Figura& f)
{
    std::vector<Celda> celdas;
    std::vector<Celda> pts = casillasDeLosPuntos(f);

    switch (f.tipo)
    {
    case FIGURA_LINEA:
        dibujarLinea(pts[0].first, pts[0].second, pts[1].first, pts[1].second, celdas);
        break;
    case FIGURA_CIRCULO:
        dibujarCirculo(pts[0].first, pts[0].second, (int)std::lround(f.radio), celdas);
        break;
    default:
        // rectangulo, triangulo y libre: poligono cerrado
        dibujarPoligono(pts, true, celdas);
        break;
    }
    return celdas;
}

// Esta funcion realiza el calculo del interior de una figura. Primero saca
// el contorno con Bresenham y luego se lo pasa al relleno por inundacion
// (rellenarContorno). La linea no tiene interior, por eso devuelve vacio.
static std::vector<Celda> interiorDeFigura(const Figura& f)
{
    if (f.tipo == FIGURA_LINEA || f.puntos.empty()) return std::vector<Celda>();
    return rellenarContorno(contornoDeFigura(f));
}

// --------------------------------------------------------------------
// REPINTAR LA GRILLA
// Esta funcion realiza el dibujo de todo: limpio la grilla y pinto las
// figuras en orden (la ultima queda encima). Si la figura esta rellena,
// pinto primero el interior y luego el contorno encima.
// --------------------------------------------------------------------
void repintarGrilla()
{
    limpiarGrilla();

    for (size_t i = 0; i < figuras.size(); i++)
    {
        if (figuras[i].relleno)
            pintarCeldas(interiorDeFigura(figuras[i]), figuras[i].colorRelleno);

        pintarCeldas(contornoDeFigura(figuras[i]), figuras[i].color);
    }

    // los clicks de la figura que todavia no termino: pinto cada punto y
    // las lineas que ya quedaron entre ellos (sin seguir al mouse)
    if (enCurso.activa)
    {
        std::vector<Celda> celdas;
        dibujarPoligono(enCurso.clicks, false, celdas);
        pintarCeldas(celdas, enCurso.color);
    }
}

// --------------------------------------------------------------------
// AGREGAR UNA FIGURA (y mostrarla por consola)
// --------------------------------------------------------------------

// Esta funcion realiza la escritura de una descripcion corta de la figura.
static void describirFigura(const Figura& f, char* texto, size_t tamano)
{
    std::vector<Celda> p = casillasDeLosPuntos(f);

    switch (f.tipo)
    {
    case FIGURA_LINEA:
        snprintf(texto, tamano, "Linea: (%d, %d) a (%d, %d)", p[0].first, p[0].second, p[1].first, p[1].second);
        break;
    case FIGURA_RECTANGULO:
    {
        int ancho = p[1].first - p[0].first + 1;
        int alto  = p[3].second - p[0].second + 1;
        if (ancho == alto)
            snprintf(texto, tamano, "Cuadrado: lado %d", ancho);
        else
            snprintf(texto, tamano, "Rectangulo: %d x %d casillas", ancho, alto);
        break;
    }
    case FIGURA_CIRCULO:
        snprintf(texto, tamano, "Circulo: centro (%d, %d), radio %d", p[0].first, p[0].second, (int)std::lround(f.radio));
        break;
    case FIGURA_TRIANGULO:
        snprintf(texto, tamano, "Triangulo: (%d, %d) (%d, %d) (%d, %d)",
                 p[0].first, p[0].second, p[1].first, p[1].second, p[2].first, p[2].second);
        break;
    default:
        snprintf(texto, tamano, "Forma libre: %d vertices", (int)p.size());
        break;
    }
}

// Esta funcion realiza el guardado de la figura nueva y ademas imprime por
// consola, paso a paso, las casillas que calculo el algoritmo (sirve para
// explicarlo en el video).
void agregarFigura(const Figura& figura)
{
    guardarParaDeshacer();
    figuras.push_back(figura);
    grillaCambio = true;

    char titulo[128];
    describirFigura(figura, titulo, sizeof(titulo));

    std::vector<Celda> celdas = contornoDeFigura(figura);
    std::cout << titulo << " (" << celdas.size() << " casillas):" << std::endl;
    for (size_t k = 0; k < celdas.size(); k++)
        std::cout << "  paso " << k << " -> (col " << celdas[k].first
                  << ", fila " << celdas[k].second << ")" << std::endl;
}

// Esta funcion realiza la cancelacion de la figura que estaba a medias.
void cancelarFiguraEnCurso()
{
    if (!enCurso.activa) return;
    enCurso.activa = false;
    enCurso.clicks.clear();
    grillaCambio = true;
}

// --------------------------------------------------------------------
// ELEGIR UNA FIGURA CON EL MOUSE
// Esta funcion realiza la busqueda de la figura sobre la que hice click.
// Reviso desde la ultima (la que esta encima) hasta la primera, y miro
// tanto el contorno como el interior (asi puedo hacer click adentro de una
// figura sin rellenar). Devuelve su posicion en la lista, o -1 si no hay.
// --------------------------------------------------------------------
int figuraEnCelda(int columna, int fila)
{
    Celda buscada(columna, fila);
    for (int i = (int)figuras.size() - 1; i >= 0; --i)
    {
        const Figura& f = figuras[i];

        std::vector<Celda> contorno = contornoDeFigura(f);
        if (std::find(contorno.begin(), contorno.end(), buscada) != contorno.end())
            return i;

        std::vector<Celda> interior = interiorDeFigura(f);
        if (std::find(interior.begin(), interior.end(), buscada) != interior.end())
            return i;
    }
    return -1;
}

// Esta funcion realiza el cambio de relleno: si estaba rellena la deja vacia y viceversa.
void alternarRelleno(int indice)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;
    guardarParaDeshacer();
    figuras[indice].relleno = !figuras[indice].relleno;
    grillaCambio = true;
}

// Esta funcion realiza el cambio de color del contorno y activa el relleno de la figura.
void aplicarColorFigura(int indice, Color color, Color colorRelleno)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;
    guardarParaDeshacer();
    figuras[indice].color = color;
    figuras[indice].colorRelleno = colorRelleno;
    figuras[indice].relleno = true;
    grillaCambio = true;
}

// Esta funcion realiza el relleno de la figura con el color elegido sin cambiar el color del borde.
void aplicarRellenoFigura(int indice, Color colorRelleno)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;
    if (figuras[indice].tipo == FIGURA_LINEA) return;
    guardarParaDeshacer();
    figuras[indice].colorRelleno = colorRelleno;
    figuras[indice].relleno = true;
    grillaCambio = true;
}

// --------------------------------------------------------------------
// TRANSFORMACIONES LINEALES
// Una transformacion lineal en 2D es multiplicar cada punto por una
// matriz 2x2:
//
//      | x' |   | a  b |   | x |
//      |    | = |      | * |   |        es decir:  x' = a*x + b*y
//      | y' |   | c  d |   | y |                   y' = c*x + d*y
//
// OJO: eso gira o escala alrededor del origen (0, 0), que es la esquina
// de la grilla. Yo quiero que la figura gire sobre SU PROPIO centro, entonces:
//   1) resto el centro a cada punto (muevo la figura al origen)
//   2) multiplico por la matriz
//   3) vuelvo a sumar el centro
// --------------------------------------------------------------------

// Esta funcion realiza el calculo del centro de la figura (el promedio de sus puntos).
static Punto centroDeFigura(const Figura& f)
{
    Punto centro = { 0.0, 0.0 };
    for (size_t i = 0; i < f.puntos.size(); i++)
    {
        centro.x += f.puntos[i].x;
        centro.y += f.puntos[i].y;
    }
    centro.x /= f.puntos.size();
    centro.y /= f.puntos.size();
    return centro;
}

// Esta funcion realiza la multiplicacion de cualquier matriz 2x2 por todos los
// puntos de una figura, tomando como eje su propio centro.
static void aplicarMatriz(Figura& f, double a, double b, double c, double d)
{
    Punto centro = centroDeFigura(f);

    for (size_t i = 0; i < f.puntos.size(); i++)
    {
        double dx = f.puntos[i].x - centro.x;   // 1) llevo el punto al origen
        double dy = f.puntos[i].y - centro.y;

        f.puntos[i].x = centro.x + a * dx + b * dy;   // 2) multiplico y 3) regreso
        f.puntos[i].y = centro.y + c * dx + d * dy;
    }

    // el radio del circulo cambia por la raiz del determinante de la matriz
    // (con una rotacion el determinante es 1, asi que el radio no cambia)
    if (f.tipo == FIGURA_CIRCULO)
        f.radio = f.radio * std::sqrt(std::fabs(a * d - b * c));
}

// Esta funcion realiza la rotacion de una figura. La matriz de rotacion es
//                                                  | cos  -sin |
//                                                  | sin   cos |
// Como la Y de la pantalla crece hacia abajo, un angulo positivo gira
// en el sentido de las manecillas del reloj.
void rotarFigura(int indice, double grados)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;

    guardarParaDeshacer();
    double radianes = grados * PI / 180.0;
    aplicarMatriz(figuras[indice], std::cos(radianes), -std::sin(radianes),
                                   std::sin(radianes),  std::cos(radianes));
    grillaCambio = true;
}

// Esta funcion realiza el escalado de una figura (agrandar o achicar).
// La matriz de escala es                           | s  0 |
//                                                  | 0  s |
void escalarFigura(int indice, double factor)
{
    if (indice < 0 || indice >= (int)figuras.size()) return;

    guardarParaDeshacer();
    aplicarMatriz(figuras[indice], factor, 0.0, 0.0, factor);
    grillaCambio = true;
}
