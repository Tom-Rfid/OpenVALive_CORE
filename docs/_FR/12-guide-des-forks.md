# 12 — Guide des forks

OpenVALive est fait pour être forké : ton propre instrument sur le FM-1, une autre interface, un nouveau
moteur, une autre carte. Cette page liste ce qu'il faut changer pour que ton fork compile, se publie et
s'installe proprement, et reste facile à resynchroniser avec l'original.

## 1. Forker et compiler

1. Clique sur **Fork** sur GitHub.
2. Active les Actions dans ton fork (onglet Actions). Les workflows `tests` et `build` tournent comme ici.
3. Active Pages : **Settings → Pages → Build and deployment → Branch : `gh-pages`**. Après le prochain push sur
   `main`, ton site (émulateur + installateur web + manuel) est à `https://<toi>.github.io/<dépôt>/`.

## 2. Donner une identité propre à ton firmware

Dans `targets/fm1/fm1.toml` d'[OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1) (un fork de firmware part d'un dépôt d'appareil ; un changement de moteur ou du Live Core part d'ici) :

| Clé | À changer en |
|---|---|
| `id_prefix` | **une nouvelle valeur `FM-1_` + chiffres**, utilisée par aucun autre firmware. Les installateurs reconnaissent le paquet par elle ; deux firmwares avec la même identité embrouillent utilisateurs et outils. |
| `name` | le nom de ton firmware (fichiers, écran, splash) — et `APP_NAME` dans `targets/fm1/app.h` |
| `usb_manufacturer`, `usb_product` | le nom du FM-1 vu par l'ordinateur |
| `repo`, `install`, `site` | les adresses de ton dépôt et de ton Pages |
| `author`, `tagline`, `description`, `tags`, `screenshot` | la fiche de ton firmware |

Change aussi `SET_MAGIC` dans `app/src/app_state.c` si ton format de réglages diffère : un FM-1 qui a fait
tourner OpenVALive chargera les valeurs par défaut au lieu de mal lire tes données.

## 3. Où faire les modifications

| Tu veux... | Modifier | Garder intact si possible |
|---|---|---|
| un nouveau moteur | ajouter `core/engines/<nom>/` ([07](07-creer-un-moteur.md)) | les API de `core/dsp/` |
| une autre interface sur le FM-1 | `app/src/ui_*.c` | `app_audio.c` (les règles temps réel) |
| d'autres boutons / CC MIDI | `KNOB_PARAM` dans `app_state.c`, `midi_in_map_cc` dans `app.c` | — |
| une autre carte | un nouveau `targets/<carte>/` ou `connectors/<carte>/` | `core/` |
| un meilleur DSP | `core/dsp/*` + un test dans `tests/` | noms et signatures des fonctions |

Plus tes changements vivent dans de **nouveaux fichiers**, plus il est facile de récupérer les corrections de
l'original (`git pull upstream main`).

## 4. Les règles qui font que ça marche

- Lancer `FM1_BASE=~/fm1-base sh tests/run_tests.sh` avant de pousser ; la CI le fait de toute façon.
- Respecter les règles de `core/` ([CLAUDE.md](../../CLAUDE.md)) : float, sans libc, sans malloc, pas de maths
  coûteuses par échantillon.
- Ne jamais bloquer dans `app_render` (c'est l'interruption audio).
- Licences : tes ajouts peuvent être sous toute licence compatible MIT ; garder les mentions existantes ; ne pas
  coller de code GPL (Felucca, Dexed) sauf si ton fork passe en GPL ([09](09-licences.md)).

## 5. Publier une version

```bash
echo 0.2.0 > VERSION
git commit -am "Release 0.2.0" && git tag v0.2.0 && git push --tags
```
La CI compile et publie la release GitHub (firmware, fichiers d'installation, applications bureau, zip du site)
et le site. `bash scripts/update-prebuilt.sh v0.2.0` met ensuite à jour `prebuilt/` dans le dépôt.

## 6. Contribuer en retour

Les corrections et moteurs utiles à tous sont bienvenus : un sujet par pull request, mesures incluses (CPU sur
le FM-1 si c'est du DSP), docs et `STATUS.md` à jour. Voir [CONTRIBUTING](../../CONTRIBUTING.md).

## 7. Faire connaître ton firmware

fm1-base tient une liste des firmwares construits dessus (ouvrir une issue avec le formulaire « List your
firmware »), et les catalogues de firmwares FM-1 lisent `firmware.json` (écrit par chaque build).
