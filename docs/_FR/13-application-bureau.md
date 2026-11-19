# 13 — L'application bureau (Windows, Linux, macOS)

**`openvalive-desktop`** fait tourner sur un ordinateur le même Live Core que le FM-1 : quatre pistes (synthés et
batterie), boucles, CAPTURE, annulation, scènes, Morph et Creative Engine. On en joue avec un clavier MIDI (un
SMK-37, le FM-1 lui-même en USB, n'importe quel clavier MIDI) ou avec le clavier de l'ordinateur. C'est aussi le
moyen le plus rapide d'essayer un live set sans rien flasher.

## Windows : télécharger et lancer

`prebuilt/windows/openvalive-desktop.exe` (ou les fichiers de la release). Double-clic sur `PLAY-OPENVALIVE.bat`
ou sur l'`.exe`. Une console s'ouvre, le son sort sur la sortie par défaut et le premier clavier MIDI est utilisé.
Il n'y a rien à installer. Un clavier français est détecté : les touches de notes passent en AZERTY.

```
OpenVALive desktop
Audio: <ta carte son>, 48000 Hz
MIDI: <ton clavier MIDI>

PLAY 120 | T4 DRUMS 808  step  5/16   4 notes | TUNE  50 | voices  3 | CPU  2% | MIDI: SMK-37 | euclid 4/16
```

## Touches

Les commandes évitent toutes les touches de notes des deux dispositions : elles sont les mêmes en QWERTY et en
AZERTY.

| Touches | Action |
|---|---|
| `q z s e d f t g y h u j k o l p m` (QWERTY `a w s e d f t g y h u j k o l p ;`) | notes, do4 à mi5, sur la piste sélectionnée |
| `b` / `n` | piste précédente / suivante |
| `Espace` | lecture / arrêt |
| `r` (ou Entrée) | enregistrer dans la piste |
| `c` | **CAPTURE** de la phrase qu'on vient de jouer |
| `x` / `X` | annuler / rétablir |
| `v` | piste muette |
| `i` | quantize oui / non |
| `!` | panique |
| Haut / Bas | octave |
| Gauche / Droite | son de la piste : 17 synthés, 4 kits de batterie |
| `Tab`, `+` / `-` | choisir un réglage et le changer : les quatre macros de la piste, puis MORPH, SWING, TEMPO, LEVEL |
| `M H F S R E C P T` (Maj) | Creative Engine : mutate, humanize, fill, simplify, reverse, expand, contract, chance, ratchet |
| `<` `>` / `U` `D` / `G` | rotation / transposition / Euclide (un coup de plus à chaque appui) |
| `W` / `Q` | mémoriser une scène (case suivante) / rappeler la suivante ; MORPH glisse entre les deux dernières |
| `L` / `A` | métronome (REC depuis l'arrêt compte alors une mesure) / tap tempo |
| `K` | enregistrer le live set dans `openvalive.set` (rechargé au prochain lancement) |
| `h`, `Échap` | aide, quitter |

Un clavier MIDI joue la piste sélectionnée, avec la vélocité, et il est enregistré et capturé. L'horloge MIDI en
entrée marche aussi. Un terminal signale les appuis mais pas les relâchements, donc une touche d'ordinateur
tapée joue 0,7 s. Un clavier MIDI donne la vraie longueur des notes.

## Options

```
--list              liste des entrées MIDI
--midi N|CHEMIN     entrée MIDI (Windows : son numéro ; Linux : un chemin /dev/snd/midi)
--azerty            disposition française pour les notes
--preset NOM        son de la piste 1 : ACID (303) BASS SUB LEAD PLUCK BRASS ORGAN PAD STRINGS SWEEP GRUNGE SCREECH DRONE WIND HOLD WHALE INIT
--set FICHIER       fichier du live set (openvalive.set par défaut) : chargé au lancement, K l'enregistre
--rate HZ           fréquence d'échantillonnage (par défaut : celle de la carte son)
--render F.wav      rendu hors ligne au lieu de jouer (--seconds N, --demo : un set de 4 pistes)
```

## Linux / macOS : compiler

```bash
make desktop
build/openvalive-desktop --list
build/openvalive-desktop --midi /dev/snd/midiC1D0
build/openvalive-desktop --render demo.wav --seconds 9 --demo     # écouter le Live Core sans carte son
```

| Système | Audio | MIDI |
|---|---|---|
| Windows | WASAPI (miniaudio) | WinMM, intégré |
| Linux | PulseAudio / ALSA (miniaudio) | périphériques ALSA bruts `/dev/snd/midiC*D*` (utilisateur dans le groupe `audio`) |
| macOS | Core Audio (miniaudio) | pas encore (CoreMIDI : contributions bienvenues) ; le clavier de l'ordinateur marche |

## Comment c'est construit

`connectors/desktop/` est un petit exemple lisible d'hôte temps réel du Live Core :

```
fil principal  clavier, commandes, ligne d'état ──► file d'événements ──┐
fil MIDI       callback WinMM / lecteur ALSA     ──► anneau d'octets   ──┼──► fil audio (callback miniaudio)
                                                                         │    applique les deux, rend ols_live
```

- `engine_bridge.c` est le seul code qui change les moteurs et le Live Core (fil audio). Il enregistre et charge
  aussi les live sets (`core/live/project.h`).
- `key_commands.c` transforme les touches en commandes `EV_LIVE`. Le FM-1 envoie les mêmes commandes depuis ses
  boutons.
- `render_offline.c` prend le même chemin sans carte son. Les tests l'utilisent, et `--demo` construit un set
  avec les fonctions du Live Core.

Compiler pour Windows depuis Linux : `make windows` (mingw-w64) produit un seul `.exe` statique.

## Testé

La compilation Linux et le rendu hors ligne tournent dans la CI. L'`.exe` Windows est compilé avec mingw-w64 et
lancé sous Wine (rendu hors ligne, liste MIDI). Le son en direct sur un vrai PC Windows et avec un vrai clavier
MIDI n'a pas encore été testé : merci de dire ce que vous constatez.
