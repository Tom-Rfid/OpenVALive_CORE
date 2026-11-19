# 00 — La stratégie d'OpenVALive, simplement

## En une phrase

> Ne pas construire un synthé plus complexe. Construire un instrument plus inspirant — et donner les pièces.

## Trois paris

**1. Le live d'abord, pas le synthé d'abord.**
Les petits synthés ne manquent pas de sons, ils manquent de *jeu*. Chaque fonction est jugée sur une seule
question : est-ce qu'elle aide à jouer, maintenant ? Donc : aucun menu en mode performance, quatre boutons qui
comptent, des boucles et des scènes avant d'autres oscillateurs. Nos voisins sont le Dirtywave M8, l'OP-1, le
Circuit Tracks, l'Electribe et le Deluge — des machines d'improvisation — pas Serum ou Diva.

**2. Ouvrir les pièces, pas seulement le produit.**
Un firmware pour un appareil aide quelques centaines de personnes. Un cœur DSP propre et portable aide tous les
projets sur la même puce, et tous ceux qui ont une Daisy, une Teensy ou un SDK de plugin. Le travail est donc
découpé en couches réutilisables séparément :

```
core/        le son : briques DSP, moteurs, MIDI, événements temps réel   (C99 portable, MIT)
connectors/  le même son sur PC, en plugin, sur Daisy, Teensy...
appareils    leurs propres dépôts : OpenVALive_FM-1, OpenVALive_SMK-37 (celui-ci en sous-module)
```

**3. Ne jamais flasher pour voir.**
Chaque modification s'écoute, se voit et se mesure d'abord sur l'ordinateur : le simulateur du moteur, l'app FM-1
entière dans le simulateur fm1-base (écran, touches, MIDI, stockage), l'émulateur web, les tests audio
automatiques (justesse, aliasing, compilation sans libc). Flasher est la dernière étape, pas le débogueur.

## Comment on choisit quoi faire

| Priorité | Quoi | Pourquoi d'abord |
|---|---|---|
| P0 | portage FM-1, DSP Core, simulateur, file d'événements — **fait** | tout le reste repose dessus |
| P0 | système de paramètres lissés, horloge centrale | Morph, scènes, MIDI, automation et séquenceur en ont besoin |
| P1 | 4 pistes, boucles MIDI, capture, undo, Morph, scènes | le premier vrai *instrument* (V1.0) |
| P2 | Creative Engine : Mutate, Humanize, Fill, Simplify | ce qui rend OpenVALive différent |
| P3 | d'autres moteurs (Acid, FM, Drum) | seulement quand un moteur est excellent |
| P4 | reverb, grosse matrice de modulation, wavetables | peut-être, s'ils méritent leur RAM et leurs menus |

Et une liste de refus volontaires pour la V1 : MPE, granulaire, wavetables, phaser, flanger, routage audio
libre, grosse reverb, crossfader façon Octatrack, matrice de modulation géante.

## Le Creative Engine : notre signature

Des transformations appliquées à une boucle pendant qu'elle joue, sans ouvrir d'éditeur : *Mutate 5 %* change
deux notes, *Humanize* ajoute du microtiming, *Fill* ajoute des ratchets tant qu'on tient un bouton, *Simplify*
retire des notes en gardant les temps forts. Pas d'IA, pas de cloud : de petits algorithmes prévisibles sur des
événements MIDI, à coût DSP nul.

## Hybride par conception

Le FM-1 joue ses propres sons **et** peut piloter d'autres machines ou un DAW en USB-MIDI, sur une même horloge.
En V1 seul l'instrument autonome est développé, mais une piste a déjà une *destination* (moteur interne ou sortie
MIDI) : rien ne sera à réécrire.

## Fait pour être forké

Tout ce qu'il faut est dans le dépôt ou téléchargé par un script : sources, firmware et application bureau déjà
compilés, scripts de build, tests, une CI qui compile et publie un fork exactement comme l'original, et un
[guide des forks](12-guide-des-forks.md). Les licences sont choisies pour ça : notre code est en MIT, la
plateforme FM-1 en dessous en MPL-2.0, et on ne copie jamais de code GPL dans le projet.

## À quoi ressemble la réussite

Quelqu'un allume un FM-1 et improvise une heure sans rien lire. Quelqu'un d'autre prend `core/` et sort un
plugin. Une troisième personne forke le dépôt, ajoute un moteur Acid et le renvoie.
