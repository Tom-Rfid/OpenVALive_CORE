# 03 — Architecture technique

## Les couches

```
              ┌────────────────────────────────────────────────────────────────┐
 cibles et    │ targets/fm1 (app fm1-base)  connectors/desktop  pc-sim  daisy… │  matériel, UI
 connecteurs  └───────────────┬────────────────────────────────────────────────┘
                              │  événements (file SPSC)    audio rendu par blocs
              ┌───────────────▼────────────────────────────────────────────────┐
 core/live    │ midi_in (parseur)    event_queue (UI -> audio, horodatée)      │
              ├────────────────────────────────────────────────────────────────┤
 core/engines │ engine.h : interface commune (notes, paramètres, rendu)        │
              │ va/ : le moteur VA (api, params, presets, voix, rendu)         │
              ├────────────────────────────────────────────────────────────────┤
 core/dsp     │ osc · svf · adsr · lfo · delay · chorus · dsp_common           │
              ├────────────────────────────────────────────────────────────────┤
 core/api     │ ols_synth.h : l'API C simple pour les autres projets           │
              └────────────────────────────────────────────────────────────────┘
```

Règles :
- `core/` ne connaît **aucun matériel**, n'alloue **jamais** de mémoire et n'utilise que `stdint.h` (+ `math.h`
  sur PC). Il compile sans bibliothèque C.
- Une cible ne parle à un moteur qu'à travers `engine.h` ou l'API C.
- L'interface utilisateur ne touche **jamais** directement le moteur : elle pousse des événements.

## Carte des sources

| Fichier | Rôle |
|---|---|
| `core/dsp/dsp_common.h` | maths (libm ou fastmath), modes de fréquence d'échantillonnage, tanh / exp2 / mtof rapides, bruit |
| `core/dsp/osc.h` | dent de scie, carré PWM, triangle PolyBLEP |
| `core/dsp/svf.h` | filtre à variables d'état ZDF |
| `core/dsp/adsr.h` · `lfo.h` | enveloppes, LFO |
| `core/dsp/delay.h` · `chorus.h` | effets |
| `core/engines/va/engine_va.h` | l'interface publique du moteur VA |
| `engine_va.c` | table d'API, init, accesseurs |
| `engine_va_params.c` | conversions 0..1 → Hz / secondes / demi-tons |
| `engine_va_presets.c` | INIT, BASS, PAD, LEAD |
| `engine_va_voices.c` | notes, allocation des voix, pile mono |
| `engine_va_render.c` | rendu au taux de contrôle et audio, effets |
| `core/live/midi_in.*` | parseur MIDI (running status, paquets USB-MIDI, mapping CC) |
| `core/live/event_queue.h` | file SPSC sans verrou d'événements horodatés |
| `core/api/ols_synth.*` | API C simple |

## Le modèle temps réel

Sur le FM-1, l'audio est rendu dans l'**interruption** du DMA audio (I2S ALNK0), 256 frames à la fois (5,8 ms).
L'interface tourne dans la boucle principale (~60 images/s) et peut être interrompue n'importe quand.

```
boucle principale (UI)                  interruption audio (toutes les 5,8 ms)
  touches, encodeurs, presets             1. vide la file d'événements
        │                                 2. lit le MIDI entrant (USB, TRS)
        └──► event_queue (SPSC) ────────► 3. rend 256 frames (par sous-blocs de 32)
                                          4. écrit le DMA, nourrit l'oscilloscope
```

- **Une seule file, un producteur, un consommateur, sans verrou**. Les données sont écrites avant l'index
  (barrière compilateur) : sûr sur un cœur. L'application bureau utilise une barrière matérielle
  (`__sync_synchronize`) ; le second cœur du pi32v2 demanderait `csync`.
- Chaque événement porte un **horodatage en échantillons** (`at`). Aujourd'hui appliqué au début du bloc ; en
  V0.2, au bon échantillon.
- Le MIDI est lu **dans** le rendu : même contexte que le moteur, aucun accès concurrent.
- En surcharge (> 90 % du temps de rendu), fm1-base appelle `app_overload()` et l'app retire une voix au lieu de
  laisser craquer le son.

L'application bureau suit le même modèle avec des fils : principal → file d'événements, MIDI → anneau d'octets,
audio → rendu.

## Deux fréquences de calcul

| Taux | Quoi | Exemple |
|---|---|---|
| audio (44,1–96 kHz) | oscillateurs, filtre, enveloppes, effets | `osc_value`, `svf_process`, `adsr_tick` |
| contrôle (tous les 32 échantillons) | LFO, pitch, glide, cutoff, coefficients du filtre | `svf_set` (tan), `osc_set_freq` |
| paramètre (quand on tourne un bouton) | 0..1 → Hz, secondes, demi-tons | `engine_va_set_param` |

Les fonctions coûteuses (`tan`, `exp`, `pow`) ne sont jamais appelées à la fréquence audio.

## Les algorithmes

| Bloc | Algorithme | Coût | Référence |
|---|---|---|---|
| Saw / carré (PWM) | PolyBLEP 2 points | ~10 flops, 12 o | Välimäki & Huovilainen 2007 |
| Triangle | naïf (harmoniques en 1/n²) | ~3 flops | — |
| Sub | carré PolyBLEP à f/2 | idem | — |
| Filtre | SVF trapézoïdal ZDF, LP/BP/HP/Notch | ~12 flops, 28 o | A. Simper (Cytomic) ; V. Zavalishin, *The Art of VA Filter Design* |
| Enveloppes | `y += c·(cible − y)`, attaque visant 1,3 | ~3 flops | classique (Mutable Instruments, Teensy Audio) |
| Saturation | tanh rationnel Padé 3/2 | ~6 flops | — |
| LFO | sine parabolique, triangle, carré, S&H | contrôle | — |
| Delay | buffer circulaire **int16**, passe-bas dans la boucle | 2 octets/échantillon | — |
| Chorus | 2 lignes modulées en opposition, valeurs Juno-60 | 2 Ko | Roland Juno-60 |

Mesures des tests : aliasing −36 dB à 1 760 Hz (15–20 dB de mieux qu'une dent de scie naïve), justesse < 0,1 cent
à 44,1 / 48 / 96 kHz.

## Le moteur VA

```
OSC1 ─┐
OSC2 ─┤                                                    par voix (×4)
SUB  ─┼─► MIX ─► SVF ─► DRIVE ─► VCA (ADSR)
NOISE─┘           ▲
        FENV + LFO2 + keytrack + glide

Σ voix ─► DELAY (mono, int16) ─► CHORUS (stéréo) ─► soft-clip ─► L/R
```

- 40 paramètres **normalisés 0..1** (`va_param_t`) : encodeur, CC MIDI, preset et futures scènes écrivent la
  même valeur ; les conversions sont faites une fois, dans `set_param`, vers `e->d` (valeurs dérivées).
- Poly 4 voix (vol : même note > voix libre > plus ancienne relâchée > plus ancienne) ou mono legato (pile de 8
  notes, glide).
- `voice_limit` : délestage dynamique.

## Fréquence d'échantillonnage

Deux modes (`core/dsp/dsp_common.h`) :
- **fixe** à la compilation avec `DSP_SR_F` (FM-1 : `24 MHz / 544 = 44 117,647 Hz`, la vraie horloge de l'ALNK0,
  pas 44 100) → constantes ;
- **réglable** avec `dsp_set_sample_rate()` avant l'init (plugins, application bureau, Daisy…) → compiler
  `core/dsp/dsp_runtime.c`. Une fréquence par programme.

## Pas de libm sur l'appareil

Le firmware FM-1 n'a pas de libm et son build refuse tout `double`. Avec `OLS_FASTMATH`, le cœur utilise
`fastmath.h` de fm1-base. Un test rend la démo des deux façons : écart de plus de 80 dB sous le signal.

## Le FM-1 en chiffres

| | Valeur | Source |
|---|---|---|
| CPU | pi32v2, 360 MHz, FPU simple précision | fm1-base `plat_cpu_khz` |
| Audio | I2S, stéréo, **Q23 dans des int32**, 44 117,647 Hz | felucca-bsp `docs/BOARD_IO.md` |
| Bloc audio | 256 frames (5,8 ms), double buffer | fm1-base `APP_BLOCK` |
| Flash app | 581 Ko | README fm1-base |
| RAM `.data/.bss` | 96 Ko | idem |
| Pool (gros buffers) | 336 Ko | idem |
| MIDI | entrée USB + TRS ; sortie USB (+ BLE) ; **pas de prise OUT** | fm1-base `plat.h` |

OpenVALive aujourd'hui (build réel) : flash 100 Ko, RAM 36 Ko, pool 100 Ko (32 Ko de delay + écran), code en
RAM 5 Ko sur 32. CPU sur l'appareil : 4 % à vide, ~5 % par voix, 26 % au pire.

## L'app (`app/`, commune à tous les appareils)

| Fichier | Rôle |
|---|---|
| `openvalive_app.h` | ce que toutes les cibles partagent : objets de stockage, points d'entrée (le `app.h` d'une cible l'inclut) |
| `openvalive_app.c` | points d'entrée (`app_init`, `app_frame`…), inclut `src/` |
| `src/app_state.c` | état partagé et règles de propriété |
| `src/app_audio.c` | interruption audio : événements, MIDI, rendu, sortie Q23, oscilloscope |
| `src/app_events.c` | événements UI → audio, sortie USB-MIDI |
| `src/app_settings.c` | chargement / sauvegarde en flash |
| `src/ui_text.c` · `ui_input.c` · `ui_draw.c` | texte, contrôles, écran |
| `src/control_map.c` · `src/sound_edit.c` | les pages CONTROL MAP et SOUND EDIT |
| `ols_dsp.c` · `ols_config.h` | le cœur compilé à -O2, fastmath, fréquence fixe |

Les dépôts des appareils ajoutent le reste : [OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1)
(`targets/fm1` : `app.c` / `app.h`, `fm1.toml`, site, manuel, scripts du simulateur, sur fm1-base) et
[OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37) (sa propre plateforme sur le SDK de M-VAVE). Chacun
inclut ce dépôt comme sous-module git (`va/`).

fm1-base compile l'app en une seule unité : `app.c` inclut les fichiers de `src/` dans l'ordre, l'état `static`
est donc partagé.

## Ce que la V0.2 change

1. **Système de paramètres** : chaque paramètre a `value`, `target`, un lissage et un type (linéaire,
   exponentiel, discret) qui dit aussi comment l'interpoler pour Morph et les scènes.
2. **Horloge centrale** : un tempo en ticks (24 PPQN pour le MIDI clock, plus fin en interne), suit une horloge
   externe, envoie le MIDI clock en USB.
3. **Événements à l'échantillon près** : le rendu découpe le bloc aux horodatages.

## V0.2 : le Live Core et plusieurs moteurs

```
Interface (boucle principale) ──EV_LIVE / notes / params──► file d'événements ──► côté audio
                                                                                 ols_live_render
                                                                                 ├── horloge : couper le bloc à chaque tick
                                                                                 ├── séquenceur : 4 pistes, notes on/off
                                                                                 ├── moteur des pistes 1..4 (VA ou BATTERIE, via bank.h)
                                                                                 └── mixage + limiteur doux
```

- `core/live/` contient l'horloge, les pistes, l'enregistrement et CAPTURE, l'annulation, les scènes et Morph, le
  Creative Engine et le format de projet. Détails : [16 — Le Live Core](16-le-live-core.md).
- Les moteurs exposent des **rôles** (quel paramètre est le cutoff, le drive...) et des **macros** (les quatre
  paramètres des boutons) : le Live Core et les interfaces n'ont jamais besoin de savoir quel moteur une piste
  utilise.
- `core/engines/bank.*` liste les sons et change le moteur d'une piste. Chaque piste possède un moteur VA et un
  moteur batterie ; seul l'actif calcule, et un moteur silencieux s'endort.
