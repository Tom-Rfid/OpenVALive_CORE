# 19 — FM-1 ↔ SMK-37 : les commandes côte à côte

L'app commune (`app/`) est écrite pour le panneau du FM-1 ; le SMK-37 y branche ses commandes
(`targets/smk37/smk37_app.c` dans OpenVALive_SMK-37 : `BUTTON_MAP`, `ENCODER_MAP`, `smk_screen_key`, les pads).
**Règle :** un geste ajouté à l'app est ajouté ici, et vérifié sur les deux (script du simulateur FM-1, test hôte
SMK-37). Même fonctionnement sur les deux ; le SMK-37 ajoute seulement ce qu'il a en plus (pads, faders, molettes,
pédale, plus de touches).

## Boutons (mêmes noms sur les deux panneaux)

| Bouton de l'app | FM-1 | SMK-37 | Seul | Tenu (couche) |
|---|---|---|---|---|
| `B_FX` (`BTN_PAGES`) | FX | FX | page de boutons suivante (ordre de MAP CONTROL) | remise à zéro de la ligne (touche 1), molette BEND / PAN (14), carte batterie (15 / 16) |
| `B_SEL` (`BTN_TRACKS`, `BTN_VALIDATE`) | SEL | NOTE REPEAT | prend un son en attente | couche pistes : mutes (tout de suite), puis CLEAR, QUANTIZE, CLOCK, EUCLID, CONTROLLER (validés au relâché) ; touches noires MUTE ALL, A VOL ; clignote vite quand un son l'attend (+SEL) |
| `B_SEQ` (`BTN_PERFORM`) | SEQ | SEQ | tapé : la boucle se relâche | couche jeu (16 effets), titre « ∞ DIV / FILTER » |
| `B_ARP` | ARP | ARP | sur A : mode d'arpège | couche créative |
| `B_GLO` | GLO | BT | — | couche scènes ; GLO + SEL : MAP CONTROL ; GLO + PLAY : chaîne |
| `B_HOME` | HOME | PATCH | panic | PANIC / RESTART / STOP, SOUNDS (8), LOOP EDIT (9), OPTION CONTROL (10) : validés au relâché |
| `B_SAVE` | SAVE | PARA | enregistre le live set | SAVE + touche : son perso |
| `B_ENV` / `B_LFO` | ENV / LFO | SCALE / CHORD | UNDO / REDO | — |
| `B_EDIT` | EDIT | GLOBE | CAPTURE (ENV l'annule) ; dans un menu : valeur par défaut de la ligne ; clignote pendant l'enregistrement | EDIT + note : punch ; LOOP EDIT : EDIT + touches = accord |
| `B_PLAY` / `B_REC` | PLAY / REC | PLAY / REC | transport / enregistrement (REC et CAPTURE seulement en lecture) | SEL + PLAY tap tempo, SEL + REC métronome |
| `B_OCTDN` / `B_OCTUP` | OCT− / OCT+ | OCT− / OCT+ | octave ; saut de famille ; boucle ×2 / ÷2 | — |
| — | — | KNOB BANK = SHIFT | — | SHIFT + K1–K4 : page LIVE ; SHIFT + SELECT : SOUND EDIT ; SHIFT + pad (SCÈNES) : mémoriser la scène ; SHIFT + pad (CLIPS) : effacer le clip |
| — | — | FADER BANK | faders : volumes → macros → USER | — |
| — | — | < (Bluetooth) | TAP tempo | tenu 0,5 s : MAP CONTROL |
| — | — | > (PAD/STEP) | pads : CLIPS → DRUMS → PERFORM → SCÈNES → MIDI | — |
| — | — | pad MODE (en haut à droite) | pareil : une tape = le mode suivant (sa couleur) | MODE + pad (CLIPS) : départ sur le temps |
| — | — | pad REC (en bas à droite) | — | REC + pad, REC lâché en premier : enregistrer un clip / mémoriser une scène |

## Boutons rotatifs

| Encodeur de l'app | FM-1 | SMK-37 |
|---|---|---|
| `EN_SELECT` | SELECT | encodeur 1 |
| `EN_PRESET` | PRESET | encodeur 2 (PRESETS) |
| `EN_K1`..`EN_K4` | K1–K4 | encodeurs 3–6 |
| `EN_ALGO` | ALGO (tempo) | encodeur 7 (PAGE) |
| master | potentiomètre MASTER | encodeur 8 |

## Touches sous une couche

| | FM-1 (27 touches, Fa3–Sol5) | SMK-37 (37 touches, Do3–Do6) |
|---|---|---|
| emplacements 1–16 | les 16 touches blanches Fa3 … Sol5 | les 16 premières blanches Do3 … Ré5 |
| touches noires (`BLACK_KEY[]`) | Fa#3 Sol#3 La#3 Do#4 Ré#4 Fa#4 … (11) | Do#3 Ré#3 Fa#3 Sol#3 La#3 … (la n-ième noire = la n-ième du FM-1) |
| SEL + 1re noire : MUTE ALL | Fa#3 | Do#3 (et 13e pad de jeu en SCÈNES) |
| SEL + 4e noire : A VOL | Do#4 | Sol#3 (et 14e pad de jeu en SCÈNES) |
| touches PERC de batterie (OPTION CONTROL) | PERC 1–4 : Sol#3 Ré#4 Sol#4 Ré#5 | PERC 1–6 : Ré#3 Sol#3 Ré#4 Sol#4 Ré#5 Sol#5 (`APP_PERC_FIRST` / `APP_PERC_COUNT`) |
| lumières des touches | les LED des touches | le clavier à l'écran (KEYS ON SCREEN), couche tenue |

## Seulement sur le SMK-37

Pads (PERFORM / PLAY avec roulements à la pression / SCÈNES avec mutes, MUTE ALL, A VOL), 4 faders (volumes, macros,
emplacements USER), molette de pitch (BEND ou PAN) et de modulation (MORPH par défaut ; les deux réglables), pédale
de sustain, MIDI OUT en jack TRS, courbe de vélocité des touches.
