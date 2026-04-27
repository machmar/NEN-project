# Project Charter

## 1. Základní informace

**Název projektu:** NEN Board Engine / Demo Game for PIC18F4550  
**Typ projektu:** Embedded game engine a demonstrační hra pro vlastní hardware platformu  
**Datum:** 2026-04-21  
**Status:** Rozpracovaný funkční prototyp / vertical slice

## 2. Kontext projektu

Projekt představuje firmware a herní engine pro desku NEN postavenou na mikrokontroleru PIC18F4550 s monochromatickým LCD 128x64. Cílem není pouze jednorázová demonstrace, ale vytvoření znovupoužitelného základu, ze kterého lze odvozovat další hry pro stejný hardware.

Současná implementace už obsahuje základní herní smyčku, raycasting renderer, fixed-point matematiku, HUD, dialogový systém, mapové eventy a nástroje pro přípravu obsahu. Repo zároveň slouží jako referenční demo a technický základ pro další rozvoj.

## 3. Účel projektu

Účelem projektu je navrhnout a dodat kompaktní 3D-like herní framework pro velmi omezený embedded hardware tak, aby:

- běžel přímo na cílové desce s PIC18F4550,
- umožňoval vytvářet jednoduché first-person hry s mapami, událostmi a dialogy,
- oddělil engine logiku od herního obsahu,
- poskytl opakovaně použitelný základ pro další studentské nebo experimentální projekty.

## 4. Vize a cíle

### Hlavní vize

Vytvořit lehký a prakticky použitelný engine pro retro-style first-person hru na specializovaném hardwaru s minimální pamětí a výkonem.

### Hlavní cíle

- Zprovoznit stabilní vykreslování pseudo-3D prostředí pomocí raycastingu.
- Implementovat kompletní gameplay smyčku: pohyb, kolize, interakce, HUD a dialogy.
- Umožnit definici herního obsahu přes mapy, assety a předzpracované datové soubory.
- Udržet projekt rozšiřitelný tak, aby šel použít jako základ pro nové hry forked z tohoto repozitáře.

### Měřitelné cíle

- Aplikace korektně běží na desce s PIC18F4550 a displejem 128x64.
- Hráč se může pohybovat v mapě, narážet do stěn a aktivovat eventy nebo dialogové dlaždice.
- HUD zobrazuje minimálně minimapu, item slot, kompas, statistiky a dialogové UI.
- Build proces automaticky generuje `assets_precomputed.h` ze zdrojových assetů.
- Projekt lze sestavit pomocí MPLAB X / XC8 a Python pre-build kroku bez ruční editace generovaných souborů.

## 5. Rozsah projektu

### In scope

- Firmware pro cílovou desku NEN.
- Raycasting renderer optimalizovaný pro LCD 128x64.
- Fixed-point matematika pro pohyb a kameru.
- Mapový systém s kolizemi, spawn body, event tiles a dialogue tiles.
- HUD vrstva pro minimapu, banner, item slot, kompas, statistiky a dialogy.
- Nástroje pro tvorbu obsahu, zejména precompute pipeline a map editor.
- Demo obsah ověřující funkčnost enginu na reálném hardware.

### Out of scope

- Obecný multiplatformní game engine.
- Full-color grafika nebo high-resolution rendering.
- Pokročilý entity/sprite pipeline jako finálně uzavřená část produktu.
- Síťové funkce, audio subsystém nebo externí runtime frameworky.
- Portace na jiný mikrokontroler bez úprav low-level vrstvy.

## 6. Hlavní výstupy

- Funkční embedded herní engine pro PIC18F4550.
- Hratelná demonstrační hra / vertical slice.
- Zdrojové kódy rendereru, HUD, inputu, asset systému a board integrace.
- Dokumentace k použití projektu a k tvorbě nové hry nad enginem.
- Podpůrné nástroje pro authoring map a precompute asset pipeline.

## 7. Stakeholdeři

- **Vývojový tým projektu:** implementace enginu, gameplay logiky a asset pipeline.
- **Vedoucí cvičení / vyučující / hodnotitel:** posouzení technického návrhu, funkčnosti a kvality řešení.
- **Budoucí autoři obsahu nebo forků projektu:** využití enginu pro nové hry na stejné platformě.
- **Uživatel / hráč na cílovém zařízení:** ověření použitelnosti ovládání, čitelnosti HUD a stability běhu.

## 8. Technická omezení a předpoklady

### Klíčová omezení

- Mikrokontroler PIC18F4550.
- Přibližně 2 KB RAM a 32 KB flash.
- Monochromatický LCD displej 128x64.
- Vstup omezený na 5 tlačítek a 5 LED.
- Nutnost používat fixed-point aritmetiku a paměťově úsporné datové struktury.
- Build závisí na toolchainu MPLAB X / XC8 a Python 3 pro generování assetů.

### Předpoklady

- Cílový hardware je dostupný a funkční.
- Build prostředí obsahuje XC8 a Python 3 dostupný jako `py -3`.
- Herní obsah je spravován přes `assets.c` a generované soubory nejsou ručně upravovány.
- Budoucí rozvoj bude respektovat oddělení engine vrstvy od konkrétního demo obsahu.

## 9. Rizika projektu

- **Paměťová omezení:** nové funkce mohou překročit dostupnou RAM nebo flash.
- **Výkon rendereru:** složitější scény nebo entity mohou snížit plynulost na cílovém HW.
- **Křehkost toolchainu:** závislost na MPLAB/XC8 a pre-build skriptu komplikuje onboarding.
- **Nedokončený sprite pipeline:** některé plánované gameplay prvky mohou být omezené aktuálním stavem rendereru.
- **Těsné vazby na konkrétní hardware:** omezená přenositelnost a vyšší cena změn v low-level vrstvě.

## 10. Kritéria úspěchu

Projekt bude považován za úspěšný, pokud:

- se spolehlivě sestaví a nasadí na cílovou desku,
- renderer, pohyb a HUD fungují stabilně v rámci definovaného demo scénáře,
- mapy, dialogy a eventy lze měnit bez zásahu do low-level renderovací vrstvy,
- nový autor dokáže podle přiložené dokumentace vytvořit fork projektu a nahradit demo obsah vlastním,
- projekt prokáže, že i na silně omezeném embedded hardware lze realizovat ucelený first-person herní zážitek.

## 11. Návrh milníků

1. **Základ platformy:** inicializace desky, displeje, vstupů a build pipeline.
2. **Renderovací jádro:** raycasting, fixed-point výpočty, kolize a pohyb hráče.
3. **Gameplay vrstva:** event tiles, dialogue tiles, HUD a herní stav.
4. **Obsah a tooling:** mapy, assety, precompute skript a editor map.
5. **Stabilizace a demonstrace:** odladění demo scénáře a příprava projektu pro další forky.

## 12. Shrnutí charteru

Projekt NEN Board Engine je zaměřen na vývoj znovupoužitelného embedded herního enginu a demonstrační hry pro specifickou hardwarovou platformu. Hlavní hodnotou projektu je kombinace technické realizovatelnosti na velmi omezeném zařízení, oddělení engine a obsahu a připravenost projektu pro další rozšíření nebo přímé použití jako základ nové hry.