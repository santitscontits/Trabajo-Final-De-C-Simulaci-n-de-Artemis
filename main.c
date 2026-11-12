#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "artemis.h"
#include "validaciones.h"

// Compatibilidad multiplataforma para lectura de teclado en tiempo real y pausas
#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    #define DORMIR_MS(ms) Sleep(ms)
    int teclaPresionada(void) { return _kbhit(); }
    int obtenerTecla(void) { return _getch(); }
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
    #define DORMIR_MS(ms) usleep((ms) * 1000)

    int teclaPresionada(void) {
        struct termios oldt, newt;
        int ch;
        int oldf;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        fcntl(STDIN_FILENO, F_SETFL, oldf);
        if (ch != EOF) {
            ungetc(ch, stdin);
            return 1;
        }
        return 0;
    }

    int obtenerTecla(void) {
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }
#endif

void limpiarPantalla(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pausar(void) {
    printf("\n%sPresione ENTER para continuar...%s", COLOR_CIAN, COLOR_RESET);
    getchar();
}

void mostrarBanner(void) {
    printf("%s", COLOR_CIAN);
    printf("====================================================================\n");
    printf("   _   ___ _____ ___ __  __ ___ ___     ___  ___ ___ ___  _  _      \n");
    printf("  /_\\ | _ \\_   _| __|  \\/  |_ _/ __|   / _ \\| _ \\_ _/ _ \\| \\| |     \n");
    printf(" / _ \\|   / | | | _|| |\\/| || |\\__ \\  | (_) |   /| | (_) | .` |     \n");
    printf("/_/ \\_\\_|_\\ |_| |___|_|  |_|___|___/   \\___/|_|_\\___\\___/|_|\\_|     \n");
    printf("               SIMULADOR DE REINGRESO ATMOSFERICO                   \n");
    printf("====================================================================\n");
    printf("%s", COLOR_RESET);
}

void inicializarMision(MisionArtemis *mision) {
    mision->altitud = 120.0;             // 120 km (Interfaz de entrada)
    mision->velocidad = 39000.0;         // 39,000 km/h
    mision->temperatura_escudo = 200.0;  // 200 °C iniciales
    mision->integridad_escudo = 100.0;   // 100% intacto
    mision->calidad_angulo = 2;          // Por defecto regular si se juega suelto
    mision->drogue_desplegado = false;
    mision->main_desplegado = false;
    mision->mision_exitosa = false;
}

void mostrarTelemetria(const MisionArtemis *mision) {
    printf("%s+------------------------------------------------------------------+\n", COLOR_AZUL);
    printf("| TELEMETRIA DE CAPSULA ORION                                      |\n");
    printf("+------------------------------------------------------------------+\n");
    printf("| Altitud: %7.2f km        | Velocidad: %8.1f km/h            |\n", mision->altitud, mision->velocidad);
    printf("| Temp. Escudo: %6.1f C     | Integridad Escudo: %5.1f%%           |\n", mision->temperatura_escudo, mision->integridad_escudo);
    printf("+------------------------------------------------------------------+%s\n\n", COLOR_RESET);
}

int mostrarMenuPrincipal(void) {
    limpiarPantalla();
    mostrarBanner();
    printf("\n%s  [ MENU DE CONTROL DE MISION ]%s\n\n", COLOR_BLANCO, COLOR_RESET);
    printf("  %s1.%s Iniciar Mision Completa (Fases 1 -> 2 -> 3)\n", COLOR_VERDE, COLOR_RESET);
    printf("  %s2.%s Practicar Fase 1: Angulo de Reingreso (Barra Espaciadora)\n", COLOR_CIAN, COLOR_RESET);
    printf("  %s3.%s Practicar Fase 2: Frenado Aerodinamico (Gestion Termica)\n", COLOR_CIAN, COLOR_RESET);
    printf("  %s4.%s Practicar Fase 3: Apertura de Paracaidas (Secuencia)\n", COLOR_CIAN, COLOR_RESET);
    printf("  %s5.%s Salir del Simulador\n\n", COLOR_ROJO, COLOR_RESET);

    return leerEnteroRango("  Seleccione una opcion [1-5]: ", 1, 5);
}

/* =========================================================================
   MINIJUEGO 1: ANGULO DE REINGRESO
   Barra de 40 posiciones:
   - 60% Rojo (24 posiciones: 12 izq + 12 der)
   - 25% Amarillo (10 posiciones: 5 izq + 5 der)
   - 15% Verde (6 posiciones centrales)
   ========================================================================= */
bool jugarAnguloReingreso(MisionArtemis *mision) {
    const int ANCHO_BARRA = 40;
    int pos_aguja = 0;
    int direccion = 1;
    bool detenido = false;

    limpiarPantalla();
    mostrarBanner();
    printf("\n%s[ FASE 1: CALIBRACION DE ANGULO DE REINGRESO ]%s\n", COLOR_AMARILLO, COLOR_RESET);
    printf("Presione la %sBARRA ESPACIADORA%s para fijar el angulo de entrada.\n", COLOR_BLANCO, COLOR_RESET);
    printf("Zona %sVERDE%s (15%%): Escudo optimo | %sAMARILLA%s (25%%): Normal | %sROJA%s (60%%): Critico\n\n",
           COLOR_VERDE, COLOR_RESET, COLOR_AMARILLO, COLOR_RESET, COLOR_ROJO, COLOR_RESET);

    // Ocultar cursor
    printf("\033[?25l");

    while (!detenido) {
        // Dibujar línea de la aguja
        printf("\r  ");
        for (int i = 0; i < ANCHO_BARRA; i++) {
            if (i == pos_aguja) {
                printf("%sV%s", COLOR_BLANCO, COLOR_RESET);
            } else {
                printf(" ");
            }
        }
        printf("\n  ");

        // Dibujar barra de colores (60% Rojo, 25% Amarillo, 15% Verde)
        // Indices: 0-11 Rojo (12), 12-16 Amarillo (5), 17-22 Verde (6), 23-27 Amarillo (5), 28-39 Rojo (12)
        for (int i = 0; i < ANCHO_BARRA; i++) {
            if (i >= 17 && i <= 22) {
                printf("%s#%s", COLOR_VERDE, COLOR_RESET);
            } else if ((i >= 12 && i <= 16) || (i >= 23 && i <= 27)) {
                printf("%s=%s", COLOR_AMARILLO, COLOR_RESET);
            } else {
                printf("%s-%s", COLOR_ROJO, COLOR_RESET);
            }
        }
        fflush(stdout);

        // Verificar si se presionó la barra espaciadora (ASCII 32)
        if (teclaPresionada()) {
            int tecla = obtenerTecla();
            if (tecla == ' ') {
                detenido = true;
                break;
            }
        }

        DORMIR_MS(35); // Velocidad de oscilación de la aguja

        // Mover cursor hacia arriba 1 línea para redibujar limpio
        printf("\033[1A");

        pos_aguja += direccion;
        if (pos_aguja >= ANCHO_BARRA - 1 || pos_aguja <= 0) {
            direccion *= -1;
        }
    }

    // Mostrar cursor nuevamente
    printf("\033[?25h\n\n");

    // Evaluar zona donde cayó la aguja
    if (pos_aguja >= 17 && pos_aguja <= 22) {
        printf("%s[EXITO PERFECTO]%s Angulo de reingreso optimo (6.2 grados).\n", COLOR_VERDE, COLOR_RESET);
        printf("Ventaja tactica: Escudo termico reforzado y velocidad inicial controlada.\n");
        mision->calidad_angulo = 3;
        mision->integridad_escudo = 100.0;
        mision->velocidad = 36000.0;
    } else if ((pos_aguja >= 12 && pos_aguja <= 16) || (pos_aguja >= 23 && pos_aguja <= 27)) {
        printf("%s[ACEPTABLE]%s Angulo ligeramente desviado. El escudo sufrira estres adicional.\n", COLOR_AMARILLO, COLOR_RESET);
        mision->calidad_angulo = 2;
        mision->integridad_escudo = 85.0;
        mision->velocidad = 39000.0;
    } else {
        printf("%s[ALERTA CRITICA]%s Angulo muy pronunciado (Zona Roja).\n", COLOR_ROJO, COLOR_RESET);
        printf("Penalizacion: Temperatura inicial elevada e integridad del escudo al 65%%.\n");
        mision->calidad_angulo = 1;
        mision->integridad_escudo = 65.0;
        mision->temperatura_escudo = 650.0;
        mision->velocidad = 42000.0;
    }

    pausar();
    return true;
}

/* =========================================================================
   MINIJUEGO 2: FRENADO AERODINAMICO
   Objetivo: Bajar velocidad debajo de 2500 km/h antes de llegar a 10 km
   sin que la temperatura supere los 2800°C o la integridad llegue a 0%.
   ========================================================================= */
bool jugarFrenadoAerodinamico(MisionArtemis *mision) {
    int turno = 1;
    const double TEMP_MAXIMA = 2800.0;
    const double VELOCIDAD_OBJETIVO = 2500.0;
    const double ALTITUD_LIMITE_FASE = 10.0;

    while (mision->altitud > ALTITUD_LIMITE_FASE) {
        limpiarPantalla();
        mostrarBanner();
        printf("\n%s[ FASE 2: FRENADO AERODINAMICO - TURNO %d ]%s\n", COLOR_AMARILLO, turno, COLOR_RESET);
        mostrarTelemetria(mision);

        // Barra visual de temperatura del escudo
        int barras_temp = (int)((mision->temperatura_escudo / TEMP_MAXIMA) * 30);
        if (barras_temp > 30) barras_temp = 30;
        printf("  Carga Termica Escudo: [");
        for (int i = 0; i < 30; i++) {
            if (i < barras_temp) {
                if (barras_temp > 22) printf("%s#%s", COLOR_ROJO, COLOR_RESET);
                else if (barras_temp > 14) printf("%s#%s", COLOR_AMARILLO, COLOR_RESET);
                else printf("%s#%s", COLOR_VERDE, COLOR_RESET);
            } else {
                printf(".");
            }
        }
        printf("] %.0f / %.0f C\n\n", mision->temperatura_escudo, TEMP_MAXIMA);

        printf("  Seleccione perfil de inclinacion de la capsula:\n");
        printf("  %s1. Frenado Agresivo%s   (Desacelera mucho | Dispara temperatura y desgaste)\n", COLOR_ROJO, COLOR_RESET);
        printf("  %s2. Frenado Balanceado%s (Desacelera normal | Calor moderado)\n", COLOR_AMARILLO, COLOR_RESET);
        printf("  %s3. Planeo Suave%s      (Desacelera poco  | Enfria el escudo termico)\n\n", COLOR_VERDE, COLOR_RESET);

        int decision = leerEnteroRango("  Orden de vuelo [1-3]: ", 1, 3);

        // Física simplificada del turno
        double factor_escudo = (mision->calidad_angulo == 3) ? 0.85 : (mision->calidad_angulo == 1 ? 1.25 : 1.0);

        if (decision == 1) {
            mision->velocidad -= 7800.0;
            mision->altitud -= 16.0;
            mision->temperatura_escudo += 680.0 * factor_escudo;
            mision->integridad_escudo -= 14.0 * factor_escudo;
        } else if (decision == 2) {
            mision->velocidad -= 5200.0;
            mision->altitud -= 18.0;
            mision->temperatura_escudo += 260.0 * factor_escudo;
            mision->integridad_escudo -= 5.0 * factor_escudo;
        } else {
            mision->velocidad -= 2400.0;
            mision->altitud -= 22.0;
            mision->temperatura_escudo -= 320.0;
            if (mision->temperatura_escudo < 200.0) mision->temperatura_escudo = 200.0;
        }

        if (mision->velocidad < 800.0) mision->velocidad = 800.0;

        // Verificar condiciones de derrota térmica
        if (mision->temperatura_escudo >= TEMP_MAXIMA || mision->integridad_escudo <= 0.0) {
            limpiarPantalla();
            mostrarBanner();
            printf("\n%s[ FALLO CATASTROFICO ]%s\n", COLOR_ROJO, COLOR_RESET);
            printf("El escudo termico ha colapsado por sobrecalentamiento (%.1f C).\n", mision->temperatura_escudo);
            printf("La capsula Orion se ha desintegrado en la atmosfera.\n");
            pausar();
            return false;
        }

        turno++;
    }

    // Evaluar velocidad al salir de la fase atmosférica alta (10 km)
    limpiarPantalla();
    mostrarBanner();
    if (mision->velocidad > VELOCIDAD_OBJETIVO) {
        printf("\n%s[ FALLO DE FRENADO ]%s\n", COLOR_ROJO, COLOR_RESET);
        printf("Llegaste a 10 km de altitud con una velocidad excesiva (%.1f km/h).\n", mision->velocidad);
        printf("Es imposible desplegar paracaidas a esta velocidad. Impacto inminente.\n");
        pausar();
        return false;
    }

    printf("\n%s[ FRENADO AERODINAMICO COMPLETADO ]%s\n", COLOR_VERDE, COLOR_RESET);
    printf("Velocidad reducida a %.1f km/h. Listo para secuencia de paracaidas.\n", mision->velocidad);
    pausar();
    return true;
}

/* =========================================================================
   MINIJUEGO 3: APERTURA DE PARACAIDAS
   Secuencia obligatoria:
   1. Drogue Chutes (Estabilizacion): Seguro desplegar debajo de 1200 km/h y < 8.5 km
   2. Main Chutes (Principales): Seguro desplegar debajo de 350 km/h y < 3.5 km
   ========================================================================= */
bool jugarAperturaParacaidas(MisionArtemis *mision) {
    // Asegurar condiciones iniciales coherentes si se juega de forma independiente
    if (mision->altitud > 10.0) mision->altitud = 10.0;
    if (mision->velocidad > 2500.0) mision->velocidad = 1400.0;
    mision->drogue_desplegado = false;
    mision->main_desplegado = false;

    while (mision->altitud > 0.0) {
        limpiarPantalla();
        mostrarBanner();
        printf("\n%s[ FASE 3: DESCENSO FINAL Y PARACAIDAS ]%s\n", COLOR_AMARILLO, COLOR_RESET);
        mostrarTelemetria(mision);

        // Calcular presión dinámica relativa (Q)
        double presion_dinamica = (mision->velocidad * mision->velocidad) / 10000.0;

        printf("  Estado Paracaidas Estabilizacion (Drogue): %s\n",
               mision->drogue_desplegado ? COLOR_VERDE "[DESPLEGADO]" COLOR_RESET : COLOR_ROJO "[PLEGADO]" COLOR_RESET);
        printf("  Estado Paracaidas Principales (Main):      %s\n",
               mision->main_desplegado ? COLOR_VERDE "[DESPLEGADO]" COLOR_RESET : COLOR_ROJO "[PLEGADO]" COLOR_RESET);
        printf("  Presion Dinamica (Tension): %.1f kPa ", presion_dinamica);

        if (presion_dinamica > 140.0) printf("%s(PELIGRO EXTREMO - NO ABRIR)%s\n\n", COLOR_ROJO, COLOR_RESET);
        else if (presion_dinamica > 15.0) printf("%s(APTO PARA DROGUE CHUTES)%s\n\n", COLOR_AMARILLO, COLOR_RESET);
        else printf("%s(APTO PARA MAIN CHUTES)%s\n\n", COLOR_VERDE, COLOR_RESET);

        printf("  Acciones disponibles:\n");
        printf("  1. Esperar / Continuar descenso libre\n");
        printf("  2. Desplegar Paracaidas de Estabilizacion (Drogue Chutes)\n");
        printf("  3. Desplegar Paracaidas Principales (Main Chutes)\n\n");

        int accion = leerEnteroRango("  Seleccione accion [1-3]: ", 1, 3);

        if (accion == 2) {
            if (mision->drogue_desplegado) {
                printf("\n%s[!] Los paracaidas Drogue ya estan desplegados.%s\n", COLOR_AMARILLO, COLOR_RESET);
                DORMIR_MS(1000);
            } else if (mision->velocidad > 1200.0) {
                limpiarPantalla();
                mostrarBanner();
                printf("\n%s[ FALLO ESTRUCTURAL ]%s\n", COLOR_ROJO, COLOR_RESET);
                printf("Desplegaste los Drogue Chutes demasiado pronto (%.1f km/h).\n", mision->velocidad);
                printf("La presion dinamica destrozo las cuerdas. Caida libre sin control.\n");
                pausar();
                return false;
            } else {
                mision->drogue_desplegado = true;
                printf("\n%s[OK] Drogue Chutes desplegados correctamente. Estabilizando nave...%s\n", COLOR_VERDE, COLOR_RESET);
                DORMIR_MS(1200);
            }
        } else if (accion == 3) {
            if (!mision->drogue_desplegado) {
                limpiarPantalla();
                mostrarBanner();
                printf("\n%s[ FALLO DE SECUENCIA ]%s\n", COLOR_ROJO, COLOR_RESET);
                printf("Intentaste abrir los Main Chutes sin estabilizar la nave con los Drogue.\n");
                printf("Los paracaidas principales se enredaron y colapsaron.\n");
                pausar();
                return false;
            } else if (mision->main_desplegado) {
                printf("\n%s[!] Los paracaidas principales ya estan desplegados.%s\n", COLOR_AMARILLO, COLOR_RESET);
                DORMIR_MS(1000);
            } else if (mision->velocidad > 360.0) {
                limpiarPantalla();
                mostrarBanner();
                printf("\n%s[ FALLO ESTRUCTURAL ]%s\n", COLOR_ROJO, COLOR_RESET);
                printf("Velocidad demasiado alta (%.1f km/h) para los Main Chutes.\n", mision->velocidad);
                printf("La tension desgarro la tela de los paracaidas principales.\n");
                pausar();
                return false;
            } else {
                mision->main_desplegado = true;
                printf("\n%s[OK] Main Chutes desplegados. Frenando para amerizaje seguro...%s\n", COLOR_VERDE, COLOR_RESET);
                DORMIR_MS(1200);
            }
        }

        // Actualizar física de descenso por ciclo
        if (mision->main_desplegado) {
            mision->velocidad -= 140.0;
            if (mision->velocidad < 28.0) mision->velocidad = 28.0; // Velocidad segura de amerizaje (~28 km/h)
            mision->altitud -= 1.2;
        } else if (mision->drogue_desplegado) {
            mision->velocidad -= 280.0;
            if (mision->velocidad < 240.0) mision->velocidad = 240.0; // Velocidad terminal con drogue
            mision->altitud -= 1.8;
        } else {
            // Caída libre aerodinámica
            mision->velocidad -= 180.0;
            if (mision->velocidad < 650.0) mision->velocidad = 650.0;
            mision->altitud -= 2.2;
        }
    }

    // Evaluación de impacto / amerizaje (Altitud <= 0)
    limpiarPantalla();
    mostrarBanner();
    if (mision->main_desplegado && mision->velocidad <= 45.0) {
        mision->mision_exitosa = true;
        printf("\n%s====================================================================\n", COLOR_VERDE);
        printf("                  AMERIZAJE EXITOSO EN EL OCEANO                    \n");
        printf("====================================================================%s\n", COLOR_RESET);
        printf("Velocidad de contacto: %.1f km/h (Dentro del margen seguro).\n", mision->velocidad);
        printf("Integridad final del escudo termico: %.1f%%\n", mision->integridad_escudo);
        printf("La tripulacion de Artemis ha regresado a salvo a la Tierra.\n");
        pausar();
        return true;
    } else {
        printf("\n%s[ IMPACTO FATAL CONTRA EL OCEANO ]%s\n", COLOR_ROJO, COLOR_RESET);
        printf("La capsula impacto el agua a %.1f km/h sin frenado completo.\n", mision->velocidad);
        pausar();
        return false;
    }
}

void ejecutarMisionCompleta(MisionArtemis *mision) {
    inicializarMision(mision);

    if (!jugarAnguloReingreso(mision)) return;
    if (!jugarFrenadoAerodinamico(mision)) return;
    jugarAperturaParacaidas(mision);
}
