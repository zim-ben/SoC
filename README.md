## Objectif

Ce TP consiste à comprendre comment intégrer un composant matériel personnalisé dans un système **Nios II / Qsys** sur carte **DE1**.  
Le tutoriel montre comment créer un registre 16 bits accessible par le processeur via le bus **Avalon Memory-Mapped** et comment exporter sa valeur vers les afficheurs 7 segments.

## Architecture utilisée

Le système est organisé autour d’un processeur **Nios II** connecté à un **Avalon Interconnect**.

Les principaux blocs sont :

- **Nios II processor** : exécute le programme logiciel.
- **Avalon Interconnect** : relie le processeur aux mémoires et périphériques.
- **On-chip memory** : mémoire interne du FPGA.
- **SRAM / SDRAM controllers** : interfaces vers les mémoires externes.
- **PIO** : ports parallèles pour LEDs, boutons, switches et afficheurs.
- **reg16_avalon_interface** : composant personnalisé connecté au bus Avalon-MM.
- **Conduit `to_hex_export`** : sortie directe vers le décodeur 7 segments.

## Principe du tutoriel

Le tutoriel explique comment transformer un simple bloc VHDL en composant Qsys.

Le composant `reg16` est un registre 16 bits.  
Il est encapsulé dans `reg16_avalon_interface`, qui lui ajoute une interface **Avalon-MM slave**.

Ainsi, le processeur Nios II peut lire ou écrire dans ce registre comme s’il s’agissait d’une adresse mémoire.

## Interface Avalon-MM

L’interface Avalon-MM permet une communication par adresses entre le Nios II et les périphériques.

Signaux importants :

| Signal | Rôle |
|---|---|
| `chipselect` | sélectionne le composant |
| `write` | demande une écriture |
| `read` | demande une lecture |
| `writedata` | donnée envoyée par le Nios II |
| `readdata` | donnée renvoyée au Nios II |
| `byteenable` | sélection des octets à écrire |

Dans ce TP, `reg16_avalon_interface` est un **slave Avalon-MM**. Le Nios II peut donc écrire une valeur dans le registre.

## Interface Conduit

Le signal `to_hex_export` est un **Conduit**.  
Contrairement à Avalon-MM, ce n’est pas un bus adressé. C’est simplement un signal exporté hors du système Qsys.
