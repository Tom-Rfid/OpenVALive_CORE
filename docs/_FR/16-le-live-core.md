# 16 — Le Live Core : boucles, CAPTURE, scènes, Creative Engine

Le Live Core (`core/live/`) fait d'OpenVALive un instrument plutôt qu'un synthé. C'est du C99 portable, sans
allocation et sans bibliothèque C. Le même code tourne sur le FM-1, dans l'application bureau, dans l'émulateur
web et dans les tests. Cette page explique comment il marche et comment l'utiliser dans votre propre hôte.

## Les pièces

| Fichier | Rôle |
|---|---|
| `live_types.h` | unités de temps (96 ticks par noire), notes, motifs, un petit générateur aléatoire |
| `clock.c` | l'horloge maître : tempo interne ou horloge MIDI, un compteur libre pour CAPTURE |
| `pattern.c` | une boucle : jusqu'à 96 notes triées, de 1 pas à 8 mesures |
| `live.c` | pistes, transport, séquenceur et mixage (le rendu) |
| `live_record.c` | REC (overdub) et CAPTURE |
| `live_history.c` | annuler / rétablir, effacer, Creative Engine et Euclide sur une piste |
| `live_scenes.c` | scènes, Morph, maintiens (overrides), stutter |
| `live_cmd.c` | commandes venues de la file d'événements ; l'entrée MIDI (`ols_live_input`) |
| `creative.c` | le Creative Engine : des fonctions pures sur un motif |
| `project.c` | un live set entier en octets (flash, fichier), avec un CRC |
| `midi_in.c`, `event_queue.h` | lecture MIDI, file sans verrou entre l'interface et l'audio |

Les moteurs (`core/engines/`) se branchent par `engine.h`. `bank.h` liste tous les sons (12 presets de synthé, 3
kits de batterie) et fait passer une piste du moteur VA au moteur batterie.

## Le temps, à l'échantillon près

Un tick vaut 1/96 de noire (5,2 ms à 120 BPM). Le rendu demande à l'horloge combien d'échantillons restent avant
le prochain tick et coupe son bloc à cet endroit. Les notes de ce tick démarrent sur leur échantillon exact, quelle
que soit la taille de bloc de la plateforme (256 sur le FM-1, 512 sur PC). Le test `test_sample_accurate_notes`
le vérifie à l'échantillon près.

Le rythme de contrôle du moteur VA (toutes les 32 échantillons) continue d'un appel à l'autre. Un bloc coupé
n'importe où sonne exactement, bit pour bit, comme un seul appel (`test_va_split_render`).

Le swing retarde les doubles-croches à contretemps, jusqu'aux deux tiers d'un pas. Chaque note peut aussi avoir
une **probabilité** (0–100 %, tirée une fois par tour) et un **ratchet** (2 à 4 frappes rapides dans sa durée).

L'horloge MIDI en entrée (24 impulsions par noire = 4 ticks chacune) est suivie en douceur entre deux impulsions,
au tempo mesuré, et rattrapée si elle prend de l'avance. L'horloge MIDI en sortie envoie une impulsion tous les
4 ticks, plus start et stop.

## Annulation par piste, boucles calées

- **Annuler / rétablir par piste** : l'historique de 8 pas est commun, mais chaque pas connaît sa piste. L'UNDO d'une
  piste prend son propre pas le plus récent et laisse les autres pistes tranquilles ; `OLS_TRACK_ALL` prend le pas le
  plus récent de n'importe quelle piste (`ols_live_undo_track`, `LIVE_UNDO` avec une cible).
- **Boucles (stutter) par piste, relâchées sur la grille** : une boucle tourne sur une piste ou sur toutes
  (`ols_live_stutter_track`). Relâchée, elle joue jusqu'à la fin de sa tranche, puis la musique reprend là où elle en
  serait, calée sur la mesure et la MIDI clock ; `id = 1` relâche tout de suite (panique).

## REC et CAPTURE

- **REC** (overdub) : une note entre dans la boucle quand on relâche la touche, avec sa vraie durée et sa place
  telle qu'elle a été jouée. Avec **quantize**, elle est calée à la double-croche. Une note calée juste devant la
  tête de lecture n'est pas jouée deux fois. Une prise entière (REC allumé puis éteint) = un pas d'annulation.
- **CAPTURE** : les 128 dernières notes sont toujours gardées, qu'on enregistre ou pas, transport lancé ou pas.
  CAPTURE remonte depuis la note la plus récente de la piste sélectionnée et s'arrête à un silence d'une mesure et
  demie, ou à 8 mesures. La longueur est arrondie à 1, 2, 4 ou 8 mesures, puis :
  - si le transport **tournait**, les notes gardent leur place dans les mesures ;
  - s'il était **arrêté**, la première note devient le premier temps et le transport démarre à l'endroit de la
    phrase où l'on se trouve. La boucle continue en rythme avec ce qu'on jouait.

## Annuler

L'historique est un anneau de 8 pas. Annuler **échange** le motif sauvé avec celui de la piste : la même case
contient alors ce qu'il faut pour rétablir. Cela fait 8 × 0,8 Ko en tout. Ces gestes créent un pas : une prise
REC, CAPTURE, EFFACER, un changement de longueur, chaque geste du Creative Engine et Euclide.

## Scènes, Morph, effets maintenus

- Une **scène** garde chaque paramètre de chaque piste (sur 16 bits), les pistes muettes et le type de moteur de
  chaque piste.
- **Morph** (0..1) entre deux scènes : les paramètres continus sont interpolés et les commutateurs (forme d'onde,
  mode) basculent à mi-course. Les moteurs lissent chaque changement (5 ms), donc Morph ne claque jamais. Une
  piste qui a changé de type de moteur depuis la scène n'est pas touchée.
- Les **maintiens** (overrides) sont désignés par un **rôle** (`OLS_ROLE_CUTOFF`, `_DRIVE`, `_DELAY_MIX`...).
  « Filtre fermé sur toutes les pistes » marche donc sur les synthés comme sur la batterie, et saute un moteur qui
  n'a pas ce rôle. Au relâchement, la valeur d'avant revient.
- **Stutter** répète la tranche en cours (1/4 à 1/32) sur toutes les pistes. Au relâchement, la musique reprend
  là où elle en serait, comme un beat repeat.

## Le Creative Engine

Ne jamais partir d'une grille vide : jouer quelque chose, puis le transformer. Chaque opération garde la matière de
la boucle (ses hauteurs, ses vélocités, son groove). `amount` 0..1 règle la force ; sur le FM-1, elle vient de la
vélocité de la touche.

| Geste | Ce qu'il fait |
|---|---|
| MUTATE | change la hauteur de quelques notes (parmi celles de la boucle), accents ou notes fantômes, un pas plus tôt ou plus tard (jamais le premier temps) |
| HUMANIZE | ±6 ticks et ±18 de vélocité au plus |
| FILL | ajoute des notes dans les pas vides, avec les hauteurs de la boucle, plutôt vers la fin |
| SIMPLIFY | enlève les notes les plus faibles (à contretemps et douces d'abord), sans jamais vider la boucle |
| ROTATE, REVERSE, TRANSPOSE | décaler de quelques pas, jouer à l'envers, transposer en demi-tons ou en octaves |
| EXPAND / CONTRACT | deux fois plus long et lent / deux fois plus court et rapide (l'un défait l'autre) |
| CHANCE | les notes à contretemps reçoivent une probabilité ; les temps jouent toujours |
| RATCHET | quelques notes deviennent 2 à 4 frappes rapides |
| EUCLIDE | k frappes réparties sur n pas (3/8 = tresillo), sur la dernière note jouée |

Ce sont toutes des fonctions pures (`creative.h`). Les tests vérifient l'ordre des notes, les limites et les
identités (`rotate +3 −3`, deux `reverse`, `expand` + `contract`).

## Live sets (projets)

`ols_project_save` écrit « OLSP » version 1 : tempo, swing, quantize, puis pour chaque piste l'état muet, le
volume, le type de moteur, tous les paramètres et la boucle, puis les scènes, et un CRC-32 à la fin.
`ols_project_load` vérifie tout avant de changer quoi que ce soit. Un set fait avec moins de paramètres (les 40 de
la 0.1) charge ce qui correspond. Sur le FM-1, un set occupe deux objets en flash. Sur l'ordinateur, c'est le
fichier `openvalive.set`.

## L'utiliser dans votre hôte

```c
ols_live_t live;                 /* ~16 Ko, aucune allocation */
engine_va_t va[4]; engine_drum_t drum[4]; ols_slot_t slot[4];
ols_engine_t eng[4];
for (int t = 0; t < 4; t++) {
    engine_va_init(&va[t], delay_ram[t], DELAY_LEN);
    engine_drum_init(&drum[t]);
    slot[t].va = &va[t]; slot[t].drum = &drum[t];
    eng[t] = ols_slot_engine(&slot[t], OLS_KIND_VA);
}
ols_live_init(&live, eng, 4, sample_rate);
ols_bank_attach(&live, slot);
ols_bank_select(&live, slot, 3, OLS_SOUND_FIRST_DRUM);      /* piste 4 : DRUMS 808 */

/* fil audio */
while (evq_pop(&file, &e)) ols_live_apply_event(&live, &e);
ols_live_render(&live, gauche, droite, n);

/* fil interface : ne jamais toucher `live`, envoyer des commandes */
push(EV_LIVE, LIVE_CAPTURE, ...);
```

La règle est la même que pour les moteurs. Seul le côté audio modifie le Live Core. L'interface envoie des
commandes `EV_LIVE` par la file et ne fait que lire l'état pour l'afficher.

## CPU

| Cas | Coût (mesure sur PC, rapporté à une voix de synthé) |
|---|---|
| une piste silencieuse | presque rien : un moteur sans voix et sans traîne s'endort |
| quatre pistes au repos | environ un huitième d'un pad à 4 voix |
| le Live Core lui-même (horloge, séquenceur, mixage) | quelques % d'une voix |

Sur le FM-1, une voix de synthé coûte environ 5 % du CPU. Les limites de voix (4 + 1 + 2 + 4 = 11) gardent quatre
pistes chargées dans le budget. Au-dessus de 90 %, la piste la plus chargée perd une voix.
