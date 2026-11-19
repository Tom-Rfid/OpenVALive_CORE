# 09 — Licences

*Ceci n'est pas un avis juridique : c'est le résumé des licences en jeu et des règles du projet.*

## OpenVALive

Le code de ce dépôt est sous **licence MIT** ([LICENSE](../../LICENSE)) : chacun peut le réutiliser, y compris
dans un produit commercial ou fermé, en gardant la mention de copyright.

## Ce sur quoi il s'appuie

| Composant | Rôle | Licence | Conséquence |
|---|---|---|---|
| [fm1-base](https://github.com/charlesvestal/fm1-base) | plateforme FM-1 (écran, audio, MIDI, stockage, simulateur, outils, site, installateur web) | **MPL-2.0** | nos fichiers gardent leur licence ; une modification d'un fichier de fm1-base se publie en MPL-2.0 |
| [felucca-bsp](https://github.com/hugelton/felucca-bsp) | couche matériel (puce, carte FM-1, chargeur de mise à jour) | **MPL-2.0** | idem |
| SDK JieLi AC79 (3 fichiers) | nécessaires au paquet `.fwsc` | Apache-2.0 | téléchargés au build, jamais dans ce dépôt |
| Toolchain JieLi | compilateur | propriétaire, téléchargée par `get_toolchain.sh` | jamais dans ce dépôt |
| Polices de fm1-base (Barlow, Terminus) | écran | SIL OFL 1.1 | incluses dans le firmware par fm1-base |
| [miniaudio](https://miniaud.io) | audio de l'application bureau | domaine public ou MIT-0 | copié dans `third_party/` |

Le firmware (`.fwsc`) combine donc du MIT (OpenVALive), du MPL-2.0 (fm1-base, felucca-bsp) et des fichiers
Apache-2.0. Pour le distribuer : donner le lien vers ce dépôt et vers fm1-base / felucca-bsp fournit les sources
MPL. Les firmwares publiés sont compilés par la CI des dépôts des appareils, depuis leurs sources et celles de ce dépôt.

## Ce qu'on ne copie pas

**Felucca** (le firmware complet) est sous **GPL-3.0-only**. Copier son code obligerait à publier tout le projet
en GPL-3.0. On l'étudie, on s'en inspire, on ne le copie pas. Ses moteurs **Felucca-Engines** (ANALOG, LOFI,
TRIO, WHEEL, NOISE, PHASE, `dsp.c`) sont en revanche en **MIT** : réutilisables avec leur mention.

## Avant d'importer du code externe

1. Vérifier la licence **du fichier** (pas seulement du dépôt : Felucca mélange GPL, MIT, MPL, Apache et LGPL).
2. Compatibles avec MIT sans conditions : MIT, BSD, zlib, Apache-2.0 (en gardant le NOTICE), domaine public /
   CC0 / MIT-0.
3. GPL, LGPL : non, sauf décision explicite de changer la licence du projet.
4. Garder l'en-tête de copyright d'origine et ajouter une ligne dans `third_party/README.md` ou le README.

## Les dépôts des appareils

[OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) est aussi sous MIT. [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37)
est en **GPL-3.0** (avec l'exception de liaison du SDK), parce qu'il est bâti sur `smk37_open` de M-VAVE : le
firmware SMK-37 dans son ensemble est en GPL-3.0 et inclut le code MIT de ce dépôt, ce que la GPL permet. La règle
ici : **aucun code GPL dans ce dépôt.**

## Marques

M-VAVE, FM-1 et SMK-37 sont des marques de leurs propriétaires. OpenVALive n'est ni affilié, ni soutenu par
M-VAVE, JieLi, Hügelton Instruments ou les auteurs de fm1-base.
