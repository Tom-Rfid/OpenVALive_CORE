# 18 — Règles : code, git et publication

Les règles que tout le monde suit ici, humains et agents IA. Elles gardent OpenVALive rapide, lisible et publiable.
La version pour les agents, avec les commandes, est [CLAUDE.md](../../CLAUDE.md).

## Branches

| Branche | Ce qui y va |
|---|---|
| `dev` (par défaut) | le travail : chaque pull request vise `dev` |
| `feat/<sujet>`, `fix/<sujet>` | un sujet chacune, depuis `dev`, retour dans `dev` par pull request |
| `main` | **les releases seulement** : `dev` est fusionné dans `main` par une pull request « Release vX.Y.Z », puis publié |

Pas de push direct sur `main`. Un correctif urgent sur une release : `fix/<sujet>` depuis `main`, PR vers `main`,
puis `main` fusionné dans `dev`.

## Commits et pull requests

- Un sujet par commit ; un résumé court en anglais (impératif, ≤ 72 caractères), puis le *pourquoi* dans le corps.
- Dire **où** c'est testé : PC, simulateur, vrai appareil (modèle, firmware).
- `FM1_BASE=~/fm1-base sh tests/run_tests.sh` doit finir par « All tests pass. » avant une PR ; la CI le relance.
- Un travail fait avec un agent IA le dit : une ligne `Co-Authored-By:` dans le commit.

## Code

- **C99, `float` seulement** (suffixe `f`), pas de `double` : le compilateur JieLi le refuse (`-Wdouble-promotion -Werror`).
- **Pas de bibliothèque C, pas de `malloc`** dans le cœur et l'app : `<stdint.h>` seulement ; les maths par `dsp_*`.
- Les calculs coûteux (tan, exp, pow) seulement au **rythme de contrôle** (toutes les 32 échantillons) ou quand un
  paramètre change.
- L'interruption audio ne bloque jamais : ni stockage, ni écran. L'interface ne touche jamais un moteur : elle envoie
  des événements.
- Les moteurs dorment en silence et rendent n'importe quelle taille de bloc à l'identique. **Une refonte garde le son
  identique au bit près.**
- Les nouveaux paramètres vont **à la fin** (les live sets sauvés les rangent par numéro).
- Petites fonctions avec un seul rôle, des commentaires qui disent *pourquoi*, l'anglais dans le code.
- Licence de ce dépôt : **MIT**. Ne jamais copier de code dont la licence ne le permet pas ; le code MIT
  d'ailleurs garde sa notice.

## Docs

- **Anglais et français ensemble** : `docs/_EN` et `docs/_FR`, mêmes numéros, même contenu.
- Le dépôt décrit ce qui est **fait ou en cours** ; les idées pas encore décidées restent en dehors.
- Chaque « testé » dit où et comment. `STATUS.md` et `CHANGELOG.md` suivent le travail.

## Publier une release

1. `dev` est vert ; `VERSION`, `CHANGELOG.md` et `STATUS.md` sont à jour.
2. Pull request `dev` → `main` « Release vX.Y.Z », fusionnée.
3. Actions > ci > Run workflow, branche `main`, « release » cochée : le tag `vX.Y.Z` et la release sont créés.
4. Tester la release sur le vrai appareil (installer, jouer, sauver, éteindre et rallumer, ressortir) et le noter
   dans `STATUS.md`.

## Les releases de ce dépôt

Ce dépôt n'a pas de firmware : ses releases sont des **versions du synthé** que les dépôts des appareils fixent
(`v0.4.0`), avec les applis bureau jointes. Une release d'appareil fixe toujours l'une d'elles.
