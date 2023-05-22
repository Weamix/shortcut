#include <avr/io.h>

#define LED     8
#define BOUTON  9

void sleep(int secs) {
#define STEPS_PER_SEC 650000000
    unsigned int i,s;
    for (s=0; s < secs; s++) {
        for (i=0; i < STEPS_PER_SEC; i++) {
        }
    }
}

int main(void){
    DDRB |= 0x01;  // Sortie pour la LED
    DDRB &= ~0x02; // Entrée pour le bouton
    PORTB |= 0x02; // Configuration de la résistance de tirage

    while(1){
        if(PINB & 0x02){
            PORTB &= ~0x01; // LED éteinte
            sleep(1);
        }
        else {
            PORTB |= 0x01; // LED allumée
        }
    }
}

