#ifndef SOM_H
#define SOM_H

// Inicializa o PWM do buzzer usado para os beeps de sucesso/falha.
void setup_som(void);

// Toca um pequeno "jingle" ascendente indicando sucesso no teste.
void toca_som_sucesso(void);

// Toca um som grave/descendente indicando falha no teste.
void toca_som_falha(void);

#endif
