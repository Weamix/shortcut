#include <avr/io.h>
#include <util/delay.h>

int main(void){
    CLKSEL0 = 0b00010101;   // sélection de l'horloge externe
    CLKSEL1 = 0b00001111;   // minimum de 8Mhz
    CLKPR = 0b10000000;     // modification du diviseur d'horloge (CLKPCE=1)
    CLKPR = 0;              // 0 pour pas de diviseur (diviseur de 1)

    DDRB |= 0x01;  // direction sortie pour PB0
    PORTB &= ~0x01;

    PORTD &= ~0x78; // entrees
    PORTD |= ~0x78;

    // lire pin d sur les bits 40 20 10 et 08

    //DDRB &= ~0b00000011; // Entrée pour le bouton
    //PORTB |= 0x02; // Configuration de la résistance de tirage

    // row 4 sortie 0
    // lire sur les colonnes
    // 4 colonnes en entrees
    // pull up : par défaut à 1 et 0 qunad on appuie

    while(1){

        if ((PIND & 0x40))
            PORTB &= ~0x80; // LED éteinte
        else
            PORTB |= 0x80; // LED allumée

        if ((PIND & 0x20))
            // open led on pb6
            PORTB &= ~0x40; // LED éteinte
        else
            PORTB |= 0x40; // LED allumée

        if ((PIND & 0x10))
            // open led on pb5
            PORTB &= ~0x20; // LED éteinte
        else
            PORTB |= 0x20; // LED allumée

        if ((PIND & 0x08))
            // open led on pb4
            PORTB &= ~0x10; // LED éteinte
        else
            PORTB |= 0x10; // LED allumée
    }
}