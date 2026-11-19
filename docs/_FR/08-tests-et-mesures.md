# 08 — Tests et mesures

## Lancer les tests

```bash
make test                                   # tests PC
FM1_BASE=~/fm1-base sh tests/run_tests.sh   # + tests FM-1
```

Prérequis : un compilateur C, `make`, Python 3 avec **numpy** (sinon les mesures audio sont sautées), et fm1-base
pour les tests FM-1. La CI les lance tous à chaque push et pull request.

## Ce qui est vérifié

| Test | Vérifie | Seuil |
|---|---|---|
| compilation stricte | le cœur compile avec `-Wall -Wextra -Wdouble-promotion -Werror` | 0 avertissement |
| simulateur | démo de 33 s (basse mono + pad 4 voix, par de vrais octets MIDI) | 0 NaN, 0 échantillon écrêté |
| API C | l'exemple compile et tourne à 44,1 kHz | — |
| application bureau | compile et rend un WAV hors ligne | 0 échantillon invalide |
| justesse | La 440 rendu à 44,1 / 48 / 96 kHz, mesuré par FFT | ±2 cents (mesuré : < 0,1) |
| aliasing | dent de scie à 1 760 Hz, énergie hors harmoniques | < −25 dB (mesuré : −36) |
| FM-1 sans libc | `app/ols_dsp.c` compilé en `-ffreestanding -nostdinc` | comme le compilateur JieLi |
| fastmath = libm | démo calculée avec les maths du FM-1 et celles du PC | > 40 dB d'écart (mesuré : 87) |
| app FM-1 | l'app entière dans le simulateur fm1-base : boot, accord, presets, MIDI, CC, sauvegarde, redémarrage | 13 vérifications |

## Les tests des appareils

L'app entière tourne dans le simulateur fm1-base (écran, touches, MIDI, stockage) dans
[OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) (`targets/fm1/tests/basic.txt`), et dans le test hôte du
SMK-37 dans [OpenVALive_SMK-37](https://github.com/Tom-Rfid/OpenVALive_SMK-37). Les deux tournent dans leur CI, avec la
version de ce dépôt qu'ils fixent.

## Le simulateur du moteur

```bash
make demo                    # build/demo.wav
build/ols_sim sweep x.wav    # balayage 50 Hz -> 15 kHz : écouter l'aliasing
make bench                   # coût CPU par cas, RAM statique
```

## Mesures sur le FM-1 réel (V0.1, 17/11/2026)

| Preset | À vide | 4 touches | Remarque |
|---|---|---|---|
| INIT | 4 % | 24 % | 4 voix |
| PAD | 6 % | 26 % | + chorus |
| LEAD | 4 % | 9 % | mono : 1 voix |
| BASS | 4 % | 10 % | mono + sub + drive |

Environ 4 % de coût fixe (delay, chorus, interface), ~5 % par voix.

Build réel (toolchain JieLi) : image 100 Ko, `.data+.bss` 36 / 96 Ko, pool 100 / 336 Ko, code en RAM 5 / 32 Ko,
aucun `double`.

## À ajouter

- Test de clics : variations rapides d'un paramètre, détection de discontinuités (avec la V0.2).
- Réponse fréquentielle et stabilité du filtre à résonance maximale.
- Simulateur / appareil : enregistrer le FM-1 par l'audio USB et comparer au WAV du simulateur.

## Ajouts de la V0.2

| Test | Ce qu'il prouve |
|---|---|
| `live core: 170 checks` (`tests/live_test.c`) | les notes du séquenceur partent sur leur échantillon exact ; ratchet, probabilité, muet, swing ; REC avec quantize (pas de note doublée) ; CAPTURE à l'arrêt et en lecture ; profondeur d'annulation ; chaque geste du Creative Engine laisse un motif valide, et les identités tiennent ; scènes, Morph, maintiens par rôle ; un projet enregistré → chargé → enregistré donne les mêmes octets et un fichier abîmé est refusé ; horloge MIDI en entrée (à un tick près, tempo mesuré) ; stutter ; le rendu VA coupé n'importe où est identique bit à bit ; chaque son de batterie reste dans les limites, s'arrête seul, grosse caisse grave, charley brillant, charley fermé qui coupe l'ouvert ; changement de moteur et projets avec une piste batterie |
| `FM-1: app in the fm1-base simulator (80 checks)` | l'appli FM-1 V0.2 : pistes, MIDI vers la piste sélectionnée, CAPTURE, annuler/rétablir, créatif, REC, scènes, Euclide, muets, batterie, mode contrôleur, changement de son, tempo, enregistrement et redémarrage |
