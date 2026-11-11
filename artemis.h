#ifndef ARTEMIS_H
#define ARTEMIS_H

#include <stdbool.h>

// Códigos de color ANSI para interfaz gráfica en consola
#define COLOR_RESET   "\033[0m"
#define COLOR_ROJO    "\033[1;31m"
#define COLOR_VERDE   "\033[1;32m"
#define COLOR_AMARILLO "\033[1;33m"
#define COLOR_AZUL    "\033[1;34m"
#define COLOR_CIAN    "\033[1;36m"
#define COLOR_BLANCO  "\033[1;37m"

// Estructura principal que guarda el estado de la cápsula Orión
typedef struct {
    double altitud;             // En kilómetros (ej. 120.0 km inicial)
    double velocidad;           // En km/h (ej. 39000.0 km/h hiperbólica)
    double temperatura_escudo;  // En grados Celsius (Límite crítico: 2800°C)
    double integridad_escudo;   // Porcentaje 0.0 a 100.0%
    int calidad_angulo;         // 1: Malo (Rojo), 2: Regular (Amarillo), 3: Óptimo (Verde)
    bool drogue_desplegado;     // Paracaídas de estabilización
    bool main_desplegado;       // Paracaídas principales
    bool mision_exitosa;
} MisionArtemis;

// Funciones de utilidad de interfaz
void limpiarPantalla(void);
void pausar(void);
void mostrarBanner(void);
void mostrarTelemetria(const MisionArtemis *mision);

// Inicialización y menú
void inicializarMision(MisionArtemis *mision);
int mostrarMenuPrincipal(void);

// Los 3 Minijuegos
bool jugarAnguloReingreso(MisionArtemis *mision);
bool jugarFrenadoAerodinamico(MisionArtemis *mision);
bool jugarAperturaParacaidas(MisionArtemis *mision);

// Ejecución de campaña completa
void ejecutarMisionCompleta(MisionArtemis *mision);

#endif
