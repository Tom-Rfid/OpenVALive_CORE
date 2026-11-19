# 01 — Présentation

**OpenVALive CORE** est le cœur d'OpenVALive : un synthétiseur pensé pour le live, en C99 portable, et l'app
qui le joue sur de petits appareils. Il n'a pas de code matériel : les appareils ont leurs propres dépôts, qui
incluent celui-ci comme sous-module git.

| Dépôt | Quoi | Licence |
|---|---|---|
| **OpenVALive_CORE** (celui-ci) | le DSP, les moteurs, le Live Core, l'app commune, l'app bureau | MIT |
| [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) | le firmware du FM-1 de M-VAVE (sur fm1-base) | MIT |
| [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) | le firmware des SMK-37 Elite / Pro de M-VAVE (sur smk37_open de M-VAVE) | GPL-3.0 |

## Ce qu'il contient

| Partie | Dossier | En bref |
|---|---|---|
| Blocs DSP | `core/dsp/` | oscillateurs PolyBLEP, filtre ZDF SVF, ADSR, LFO, delay, chorus, phaser |
| Moteur VA | `core/engines/va/` | 2 oscillateurs + sub + bruit, filtre multimode avec drive, 2 enveloppes, 2 LFO (libres ou calés sur le tempo, retrig, un cycle), **matrice de modulation** (4 entrées), delay, chorus façon Juno, phaser, bitcrusher, 17 presets |
| Moteur batterie | `core/engines/drum/` | famille 808 synthétisée : grosse caisse, caisse claire, clap, charlestons, toms, rim, cowbell ; 4 kits |
| Live Core | `core/live/` | horloge (MIDI clock entrée / sortie), 4 pistes en boucle, REC, CAPTURE, undo, scènes, Morph, effets de jeu, Creative Engine, macros, sidechain, chaîne de scènes, live sets |
| L'app | `app/` | l'interface de l'instrument pour un écran 240 × 240 et le panneau du FM-1 (pages, couches, MAP CONTROL, SOUND EDIT), sur l'API `plat.h` de fm1-base |
| Connecteurs | `connectors/` | app bureau (Windows / Linux), simulateur PC, API C, modèles Daisy / Teensy |

Comment c'est câblé : [03 — Architecture](03-architecture-technique.md). Le Live Core : [16](16-le-live-core.md).
Écrire un moteur : [07](07-creer-un-moteur.md).

## Les règles qui le rendent rapide

C99 et `float` seulement, pas de `malloc`, pas de bibliothèque C sur l'appareil ; les calculs coûteux seulement au
rythme de contrôle (toutes les 32 échantillons) ; chaque moteur dort quand il se tait et rend n'importe quelle taille
de bloc à l'identique ; une refonte garde le son identique au bit près (le rendu de démo est comparé). Détails :
[CLAUDE.md](../../CLAUDE.md).
