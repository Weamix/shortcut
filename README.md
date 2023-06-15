# Systeme embarqué

## Projet

Voir le wiki pour plus d'informations. [Wiki](https://wiki-se.plil.fr/mediawiki/index.php/I2L_2022_Groupe4)

## Dépendences

- avr-gcc
- libusb
- dfu-programmer

## Les deux programmes pour le projets

- [Programme Host pour envoyer des shortcuts sur la carte](./USBprogI2L/)

> Pour lancer le programme, compilez avec `make` puis lancez le programme avec `./prog_echo`

- [Programme LUFA qui reçoit les shortcuts et les exécute en tant que clavier](./lufa-copy-paste/Demos//Device//LowLevel/Keyboard)

> Flashez le programme sur la carte avec `make dfu`

## Membres du groupe

- [DJAMAA Wassim](https://github.com/WassimDjamaa)
- [LEBAS Axel](https://github.com/alebas1)
- [VITSE Maxime](https://github.com/Weamix)
