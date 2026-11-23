// ====================================================================
//  interfaz.cpp
//  Autor: Juan Camilo Forero Paez
//
//  Aqui esta todo lo que el usuario toca: el panel de herramientas (Dear
//  ImGui), la lectura del mouse y del teclado, y lo que pasa con cada click.
//  El panel es una sola ventana de ImGui con widgets normales (botones de
//  opcion, deslizadores y selectores de color), sin nada dibujado a mano.
// ====================================================================
#include "interfaz.h"
#include "figuras.h"
#include "grilla.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Lienzo lienzo = { (float)ANCHO_PANEL_PX, 0.0f,
                  (float)(ANCHO_INICIAL_PX - ANCHO_PANEL_PX), (float)ALTO_INICIAL_PX };

// --------------------------------------------------------------------
// LAS HERRAMIENTAS
// Esta enumeracion guarda las herramientas del panel. Las primeras 6
// dibujan figuras nuevas. Las ultimas 3 se aplican a una figura que ya
// existe (hago click sobre ella).
// --------------------------------------------------------------------
enum Herramienta
{
    HERRAMIENTA_LINEA,
    HERRAMIENTA_CIRCULO,
    HERRAMIENTA_RECTANGULO,
    HERRAMIENTA_CUADRADO,    // es un rectangulo al que se le obligan los lados iguales
    HERRAMIENTA_TRIANGULO,
    HERRAMIENTA_LIBRE,       // forma libre: tantos clicks como quiera, se cierra con Enter
    HERRAMIENTA_RELLENAR,
    HERRAMIENTA_ROTAR,
    HERRAMIENTA_ESCALAR
};

// Esta variable guarda cual herramienta esta elegida ahora mismo.
static Herramienta herramientaActual = HERRAMIENTA_LINEA;

// Estas dos variables guardan el color del contorno y el color del relleno.
static Color colorSeleccionado        = { 0.95f, 0.55f, 0.15f };
static Color colorRellenoSeleccionado = { 0.45f, 0.20f, 0.05f };

// Estas dos variables las muevo con los deslizadores del panel: cuantos
// grados gira (o cuanto escala) la figura cada vez que le hago click.
static int   gradosRotacion = 15;
static float factorEscala   = 1.25f;

// Estas variables guardan la casilla donde esta el mouse (se calcula en cada cuadro).
static int  mouseColumna = 0, mouseFila = 0;
static bool mouseDentroGrilla = false;

// --------------------------------------------------------------------
// datos de cada herramienta
// --------------------------------------------------------------------

// Esta funcion realiza la consulta de que tipo de figura crea cada herramienta de dibujo.
static TipoFigura tipoDeHerramienta(Herramienta h)
{
    switch (h)
    {
    case HERRAMIENTA_LINEA:     return FIGURA_LINEA;
    case HERRAMIENTA_CIRCULO:   return FIGURA_CIRCULO;
    case HERRAMIENTA_TRIANGULO: return FIGURA_TRIANGULO;
    case HERRAMIENTA_LIBRE:     return FIGURA_LIBRE;
    default:                    return FIGURA_RECTANGULO;   // rectangulo y cuadrado
    }
}

// Esta funcion realiza la consulta de cuantos clicks necesita cada herramienta
// de dibujo. Devuelve 0 cuando no hay un numero fijo (la forma libre).
static int clicksNecesarios(Herramienta h)
{
    switch (h)
    {
    case HERRAMIENTA_TRIANGULO: return 3;
    case HERRAMIENTA_LIBRE:     return 0;
    default:                    return 2;
    }
}

// Esta funcion realiza la consulta de si la herramienta crea figuras nuevas
// (true) o si se aplica a una figura que ya existe (false).
static bool esHerramientaDeDibujo(Herramienta h)
{
    return h <= HERRAMIENTA_LIBRE;
}

// Esta funcion realiza el ajuste de un numero para dejarlo dentro de un rango
// (por ejemplo, la columna entre 0 y columnas-1).
static int limitar(int valor, int minimo, int maximo)
{
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

// --------------------------------------------------------------------
// EL LIENZO Y EL MOUSE
// --------------------------------------------------------------------

// Esta funcion realiza el calculo de donde queda la grilla en la ventana. La
// grilla empieza justo a la derecha del panel y ocupa todo el espacio que sobra.
void calcularLienzo()
{
    ImVec2 ventana = ImGui::GetIO().DisplaySize;

    lienzo.x = (float)ANCHO_PANEL_PX;
    lienzo.y = 0.0f;
    lienzo.ancho = std::max(ventana.x - (float)ANCHO_PANEL_PX, 1.0f);
    lienzo.alto  = std::max(ventana.y, 1.0f);
}

// Esta funcion realiza la conversion de los pixeles del mouse a (columna, fila).
// Resto la esquina del lienzo y divido entre el tamano de una casilla.
static void actualizarCasillaDelMouse()
{
    ImVec2 m = ImGui::GetMousePos();

    float x = m.x - lienzo.x;
    float y = m.y - lienzo.y;

    // la grilla solo llega hasta columnas*20 pixeles (puede ser menos que el lienzo)
    mouseDentroGrilla = (x >= 0 && x < columnas * TAMANO_CASILLA_PX &&
                         y >= 0 && y < filas * TAMANO_CASILLA_PX);

    mouseColumna = limitar((int)std::floor(x / TAMANO_CASILLA_PX), 0, columnas - 1);
    mouseFila    = limitar((int)std::floor(y / TAMANO_CASILLA_PX), 0, filas - 1);
}

// Esta funcion realiza el marco blanco sobre la casilla donde esta el mouse, para
// ver exactamente en que casilla voy a hacer click. (Es solo un indicador del
// cursor, no es una figura: las figuras salen de los algoritmos de algoritmos.cpp.)
void dibujarCasillaBajoMouse()
{
    if (!mouseDentroGrilla || ImGui::GetIO().WantCaptureMouse) return;

    ImVec2 a(lienzo.x + mouseColumna * TAMANO_CASILLA_PX, lienzo.y + mouseFila * TAMANO_CASILLA_PX);
    ImVec2 b(a.x + TAMANO_CASILLA_PX, a.y + TAMANO_CASILLA_PX);

    // el foreground dibuja encima de todo
    ImGui::GetForegroundDrawList()->AddRect(a, b, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
}

// --------------------------------------------------------------------
// LO QUE PASA CON CADA CLICK
// --------------------------------------------------------------------

// Esta funcion realiza el ajuste del segundo click del cuadrado: deja los dos
// lados iguales (cuadrado perfecto) sin salirse de la grilla.
static Celda ajustarACuadrado(Celda inicio, Celda fin)
{
    int dx = fin.first  - inicio.first;
    int dy = fin.second - inicio.second;

    int signoX = (dx < 0) ? -1 : 1;
    int signoY = (dy < 0) ? -1 : 1;

    // cuanto espacio queda hacia donde voy, hasta el borde de la grilla
    int libreX = (signoX < 0) ? inicio.first  : (columnas - 1 - inicio.first);
    int libreY = (signoY < 0) ? inicio.second : (filas - 1 - inicio.second);

    int lado = std::max(std::abs(dx), std::abs(dy));
    lado = std::min(lado, std::min(libreX, libreY));

    return Celda(inicio.first + signoX * lado, inicio.second + signoY * lado);
}

// Esta funcion realiza el cierre de la figura: la creo con los clicks que
// llevo y la agrego a la lista de figuras.
static void terminarFiguraEnCurso()
{
    Figura f = crearFigura(enCurso.tipo, enCurso.clicks, enCurso.color);
    enCurso.activa = false;
    enCurso.clicks.clear();
    agregarFigura(f);
}

// Esta funcion realiza el cierre de la forma libre. Una forma libre necesita
// al menos 3 puntos para ser un poligono; si lleva menos, la descarto.
static void cerrarFormaLibre()
{
    if (!enCurso.activa || enCurso.tipo != FIGURA_LIBRE) return;

    if (enCurso.clicks.size() >= 3)
        terminarFiguraEnCurso();    // el poligono se cierra solo uniendo el ultimo punto con el primero
    else
        cancelarFiguraEnCurso();
}

// Esta funcion realiza lo que pasa con un click de una herramienta de dibujo:
// cada click agrega un punto a la figura que estoy haciendo.
static void clickDeDibujo()
{
    Celda click(mouseColumna, mouseFila);

    if (!enCurso.activa)
    {
        // primer click: empiezo una figura nueva
        enCurso.activa = true;
        enCurso.tipo   = tipoDeHerramienta(herramientaActual);
        enCurso.color  = colorSeleccionado;
        enCurso.clicks.clear();
    }
    else if (herramientaActual == HERRAMIENTA_CUADRADO)
    {
        // segundo click del cuadrado: lo corrijo para que los lados sean iguales
        click = ajustarACuadrado(enCurso.clicks[0], click);
    }

    enCurso.clicks.push_back(click);
    grillaCambio = true;   // para que se marque el punto

    // si ya tengo todos los clicks, termino la figura (la libre no termina sola:
    // se cierra con Enter, con clic derecho o con el boton del panel)
    int necesarios = clicksNecesarios(herramientaActual);
    if (necesarios > 0 && (int)enCurso.clicks.size() >= necesarios)
        terminarFiguraEnCurso();
}

// Esta funcion realiza lo que pasa con un click de las 3 herramientas que se
// aplican a una figura que ya existe (rellenar, rotar y escalar).
static void clickSobreFigura()
{
    int indice = figuraEnCelda(mouseColumna, mouseFila);
    if (indice < 0) return;   // hice click en un lugar vacio

    switch (herramientaActual)
    {
    case HERRAMIENTA_RELLENAR:
        aplicarRellenoFigura(indice, colorRellenoSeleccionado);
        break;
    case HERRAMIENTA_ROTAR:
        rotarFigura(indice, (double)gradosRotacion);
        break;
    case HERRAMIENTA_ESCALAR:
        escalarFigura(indice, (double)factorEscala);
        break;
    default:
        break;
    }
}

// Esta funcion realiza la revision del mouse en cada cuadro.
void procesarMouse()
{
    ImGuiIO& io = ImGui::GetIO();
    actualizarCasillaDelMouse();

    // io.WantCaptureMouse es true cuando el mouse esta sobre el panel:
    // ahi NO se dibuja, ahi solo se elige la herramienta
    bool sobreGrilla = mouseDentroGrilla && !io.WantCaptureMouse;

    // clic derecho: cierra la forma libre que estoy haciendo
    if (sobreGrilla && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        cerrarFormaLibre();
        return;
    }

    if (!sobreGrilla || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    if (esHerramientaDeDibujo(herramientaActual))
        clickDeDibujo();
    else
        clickSobreFigura();
}

// Esta funcion realiza la lectura de los atajos del teclado:
//   Esc = cancelar la figura que estoy haciendo, Enter = cerrar la forma libre,
//   Ctrl+Z = deshacer, Supr = borrar todo
void procesarTeclado()
{
    ImGuiIO& io = ImGui::GetIO();

    if (enCurso.activa)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            cancelarFiguraEnCurso();
        else if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
            cerrarFormaLibre();
        return;   // mientras hago una figura no deshago ni borro nada
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
        deshacer();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        borrarTodo();
}

// ====================================================================
//  EL PANEL DE HERRAMIENTAS (Dear ImGui)
//
//  ImGui funciona en "modo inmediato": en CADA cuadro vuelvo a describir
//  el panel y ImGui me dice al instante si tocaron algo. Por eso no
//  guardo botones como objetos: solo escribo
//      if (ImGui::Button(...)) { ... }
// ====================================================================

// Esta funcion realiza la conexion de ImGui con GLFW (eventos) y OpenGL (dibujo).
void iniciarGUI(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// Esta funcion realiza el inicio de un cuadro de ImGui (se llama al empezar cada vuelta).
void comenzarCuadroGUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

// Esta funcion realiza el cierre del cuadro: manda a dibujar todo lo de ImGui.
void terminarCuadroGUI()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Esta funcion realiza la liberacion de Dear ImGui al cerrar el programa.
void cerrarGUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// Esta funcion realiza un boton de opcion para elegir una herramienta. Al
// cambiar de herramienta descarto la figura que estuviera a medias.
static void botonDeHerramienta(const char* texto, Herramienta herramienta)
{
    if (ImGui::RadioButton(texto, herramientaActual == herramienta))
    {
        herramientaActual = herramienta;
        cancelarFiguraEnCurso();
    }
}

// Esta funcion realiza la consulta del texto de ayuda de la herramienta
// elegida (se muestra al final del panel).
static const char* ayudaDeHerramienta()
{
    switch (herramientaActual)
    {
    case HERRAMIENTA_LINEA:      return "2 clics: punto inicial y punto final.";
    case HERRAMIENTA_CIRCULO:    return "2 clics: el centro y un punto del borde.";
    case HERRAMIENTA_RECTANGULO: return "2 clics: dos esquinas opuestas.";
    case HERRAMIENTA_CUADRADO:   return "2 clics: una esquina y la esquina contraria (los lados salen iguales).";
    case HERRAMIENTA_TRIANGULO:  return "3 clics: los 3 vertices.";
    case HERRAMIENTA_LIBRE:      return "1 clic por vertice. Para cerrar: boton, Enter o clic derecho.";
    case HERRAMIENTA_RELLENAR:   return "Clic dentro de una figura para rellenarla con el color de relleno.";
    case HERRAMIENTA_ROTAR:      return "Clic sobre una figura para girarla los grados del deslizador.";
    default:                     return "Clic sobre una figura para escalarla con el factor del deslizador.";
    }
}

// --------------------------------------------------------------------
// Esta funcion realiza el panel completo: una ventana pegada a la izquierda,
// de todo el alto, con las herramientas, los colores y los botones.
// --------------------------------------------------------------------
void dibujarPanel()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2((float)ANCHO_PANEL_PX, ImGui::GetIO().DisplaySize.y));

    ImGuiWindowFlags banderas = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Herramientas", nullptr, banderas);

    // ---- figuras nuevas ----
    ImGui::Text("Dibujar");
    ImGui::Separator();
    botonDeHerramienta("Linea",      HERRAMIENTA_LINEA);
    botonDeHerramienta("Circulo",    HERRAMIENTA_CIRCULO);
    botonDeHerramienta("Rectangulo", HERRAMIENTA_RECTANGULO);
    botonDeHerramienta("Cuadrado",   HERRAMIENTA_CUADRADO);
    botonDeHerramienta("Triangulo",  HERRAMIENTA_TRIANGULO);
    botonDeHerramienta("Forma libre", HERRAMIENTA_LIBRE);

    if (herramientaActual == HERRAMIENTA_LIBRE && ImGui::Button("Cerrar forma"))
        cerrarFormaLibre();

    // ---- figuras que ya existen ----
    ImGui::Spacing();
    ImGui::Text("Editar figura");
    ImGui::Separator();
    botonDeHerramienta("Rellenar", HERRAMIENTA_RELLENAR);
    botonDeHerramienta("Rotar",    HERRAMIENTA_ROTAR);
    ImGui::SliderInt("grados", &gradosRotacion, -180, 180);
    botonDeHerramienta("Escalar",  HERRAMIENTA_ESCALAR);
    ImGui::SliderFloat("factor", &factorEscala, 0.5f, 2.0f, "%.2f");

    // ---- colores ----
    ImGui::Spacing();
    ImGui::Text("Colores");
    ImGui::Separator();
    ImGui::ColorEdit3("borde",   &colorSeleccionado.r,        ImGuiColorEditFlags_NoInputs);
    ImGui::ColorEdit3("relleno", &colorRellenoSeleccionado.r, ImGuiColorEditFlags_NoInputs);

    // ---- acciones generales ----
    ImGui::Spacing();
    if (ImGui::Button("Deshacer") && !enCurso.activa)
        deshacer();
    ImGui::SameLine();
    if (ImGui::Button("Borrar todo") && !enCurso.activa)
        borrarTodo();

    // ---- ayuda y casilla del mouse ----
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextWrapped("%s", ayudaDeHerramienta());
    ImGui::Text("Casilla: (%d, %d)", mouseColumna, mouseFila);

    ImGui::End();
}
