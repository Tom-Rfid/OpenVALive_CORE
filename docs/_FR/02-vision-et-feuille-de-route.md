# 02 — Vision et feuille de route

## Principes

1. **Live First.** Chaque fonction se juge sur une question : est-ce qu'elle aide à jouer, là, maintenant ?
2. **Performance ≠ Configuration.** Le mode Performance n'a aucun menu. La configuration (audio, MIDI,
   contrôles, système) est à part, derrière un geste dédié, avec **2 niveaux de profondeur au maximum**.
3. **La couleur sert l'état.** Vert = actif, jaune = armé, rouge = enregistrement, gris = muet. Pas d'animation décorative.
4. **Peu, mais bien.** Trois formes d'onde, un filtre, trois effets au départ. Chaque ajout doit gagner sa place
   en RAM, en CPU et en clarté.
5. **Un cœur partagé.** Le DSP, le séquenceur et les scènes sont communs à tous les instruments. Un nouveau
   moteur ne réécrit ni l'audio, ni le MIDI, ni les presets.

## Trois couches

```
OpenVALiveCore          DSP Core · système de paramètres · MIDI · horloge · stockage · plateforme · simulateur
OpenVALive         moteurs : VA (fait), puis Acid, FM, Drum…
OpenVALivePerformance   Morph · scènes · capture · undo · effets live · Creative Engine
```

## Hybride

Le FM-1 produit ses propres sons **et** pilote d'autres machines ou un DAW en USB-MIDI, sur une même horloge. Une
piste aura une destination : le moteur interne ou un canal MIDI. En V1 seul le jeu autonome est développé ;
l'architecture prévoit le reste. (Le FM-1 n'a pas de prise MIDI OUT : la sortie MIDI passe par l'USB, et le
Bluetooth si on l'active un jour.)

## Le Creative Engine

| Fonction | Effet |
|---|---|
| **Mutate 5 %** | deux notes de la boucle changent ; encore une fois, deux autres |
| **Humanize** | un peu de microtiming et de vélocité |
| **Fill** | tant que le bouton est tenu : ratchets et notes fantômes |
| **Simplify / Densify** | retire ou ajoute des notes en gardant les temps forts |
| **Rotate, Reverse, Transpose, Expand, Contract** | variations de motif |

## Feuille de route

| Priorité | Contenu | État |
|---|---|---|
| **P0** | Portage FM-1 | **fait** (V0.1) |
| P0 | DSP Core | **fait** |
| P0 | Simulateur PC (moteur) + app FM-1 entière dans le simulateur fm1-base + émulateur web | **fait** |
| P0 | File d'événements horodatée | **fait** |
| P0 | Lissage des paramètres, type d'interpolation (continu / discret) | **fait** (V0.2) |
| P0 | Horloge centrale, MIDI clock in/out, événements à l'échantillon près | **fait** (V0.2) |
| P0 | Presets et projets (live sets « OLSP » versionnés) | **fait** (V0.2) |
| **P1** | 4 pistes, boucles MIDI, capture, undo | **fait** (V0.2) |
| P1 | Morph, 8 scènes, effets de performance « maintenir pour appliquer » | **fait** (V0.2) |
| **P2** | Humanize, Mutate, Fill, Simplify, probabilité, ratchet, euclidien | **fait** (V0.2) |
| **P3** | Moteur batterie | **fait** (V0.2, à la demande du propriétaire) ; Acid = un preset VA ; FM plus tard |
| P3 | Phaser | **fait** (V0.2) |
| P3 | Cible SMK-37 | **faite** (V0.3, GPL-3.0 sur `smk37_open` : doc 17, doc 20) ; à vérifier sur le matériel |
| **P4** | Reverb, moteurs modal et guide d'ondes, matrice de modulation avancée | plus tard |

**V1.0 « le premier instrument »** = VA + Batterie + Loop + Capture + Morph + Scènes + Undo + Creative Engine, sur
le FM-1 et le SMK-37, vérifié sur les deux. C'est la base des forks par mode.

**Pas en V1, volontairement :** MPE, granulaire, wavetable, flanger, routage audio libre, reverb lourde,
crossfader Octatrack, matrice de modulation de plus de 4 slots. Chacun ajoute des menus, de la RAM ou des bugs
avant d'ajouter du jeu.

## Le séquenceur prévu : des événements, pas des pas

```c
typedef struct {
    uint32_t tick;        /* position, en ticks d'horloge */
    uint16_t duration;    /* peut déborder sur la mesure suivante */
    uint8_t  note_count;  /* accords : jusqu'à 4 notes */
    uint8_t  notes[4];
    int8_t   offset;      /* microtiming */
    uint8_t  velocity;
} ols_note_event_t;
```

Accords, contretemps, notes longues et microtiming sont natifs. Les boucles sont du MIDI (quelques octets), pas
de l'audio (des centaines de Ko).

## Les risques identifiés

- **Tout faire trop tôt** (VA + FM + Acid + Drum…) : un seul moteur jusqu'à la V1.
- **Trop de P0** : la V0.1 a validé le matériel d'abord ; chaque version doit rester jouable.
- **Les menus qui reviennent** : toute nouvelle page d'interface se justifie dans sa pull request.

## Les mesures qui orientent les choix

Sur le FM-1 réel, 4 voix coûtent 24–26 % du CPU, une voix ~5 %. Il y a de la place pour 8 voix, ou un deuxième
moteur sur une autre piste, ou une reverb légère. Le choix se fera avec le Live Core, mesures à l'appui.
