#include <stdio.h>
#include "validaciones.h"

void limpiarBufferEntrada(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int leerEnteroRango(const char *mensaje, int min, int max) {
    int valor;
    int resultado;

    while (1) {
        printf("%s", mensaje);
        resultado = scanf("%d", &valor);

        if (resultado == 1 && valor >= min && valor <= max) {
            limpiarBufferEntrada();
            return valor;
        }

        printf("\033[1;31m[!] Entrada invalida. Ingrese un numero entre %d y %d.\033[0m\n", min, max);
        limpiarBufferEntrada();
    }
}
