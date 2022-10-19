# Drum Companion

Tentative to build a custom Metronom for my drumming practice.

## Build

### Dependencies
- Need to clone [ImGUI](https://github.com/ocornut/imgui) in the ```lib``` directory
- Need to clone [MiniAudio](https://github.com/mackron/miniaudio) in ```libs``` directory

### Use waf
- to configure (clang): ```./waf configure --out=cbuild --check-cxx-compiler=clang++ --compil_db```
- to build: ```./waf build```
- to run: ```cbuild/test/xxx```

# Documentation

## Paramètres de l'application

``` c++
Usage:
      drum_companion [-h -v -g -b <int> -s <str>] [-p <str>]... [-l <str> -o <str>]
      drum_companion [-h -v -g -b <uint>] <infile>


    Options:
      -h --help              Show this screen
      -v --verbose           Display some info
      -g --gui               With GUI (but ONE pattern, NO loop)
      -b --bpm=<uint>        BPM (default is 90)
      -s --sig=<str>         signature [default: 4x2]
      -p --pattern=<str>     patterns, can be REPEATED [default: 2x1x1x1x]
      -l --loop=<str>        sequence of patterns (like 2x(p0+P1)) [default: p0]
      -o --outfile=<str>     file to save looper (or pattern)
```

## Pattern
Un ```Pattern``` émet des sons selon une séquence d'intervales. A chaque intervale, on peut jouer un son (déterminé par son index) ou rien.

Un ```Pattern``` possède une ```Signature``` composée de 

``` c++
struct Signature {
    uint bpm           = 120;   // nombre de pulsations par minute
    uint beats         = 4;     // nombre de pulsations dans le Pattern
    uint subdivisions  = 1;     // nombre de subdivisions dans une pulsation
}
```
Un ```Pattern``` possède un état interne.

``` c++
enum AudioState { ready, running, paused, ended, empty };
- ready : prêt à être joué
- running : en train d'être joué
- paused : en pause
- ended : fini d'être joué
- empty : vide (les sons et les variables internes pas définies)
```

En interne, un ```Pattern``` est une succession de ```Note```.

``` c++
struct Note
{
  uint val              = 1;     // indice de la note à jouer
  uint length           = 10;    // durée avant la prochaine note/son
}
```
mais maintient aussi une ```Timeline = std::vector<uint>``` qui indique, pour chaque ```subdivision```, quel est l'```index``` du son qui doit être joué (ou ```0```).

## Looper
Un ```Looper``` permet d'enchaîner une séquence de ```Pattern``` en boucle.

Les différents ```Pattern``` sont stockés dans un ```vector all_patterns``` et sont repéré par leur ```index```.

La séquence de ```Pattern``` à jouer est stockée dans une ```PatternList sequence```, et le ```Pattern``` courant est suivit/pointé par un ```Iterator```.

Un ```Looper``` possède un état interne.

``` c++
enum LooperState { ready, running, paused, empty };
- ready : prêt à être joué
- running : en train d'être joué
- paused : en pause
- empty : la séquence de Pattern n'est pas prête
```

## SoundEngine
Un ```SoundEngine``` stocke les différent sons utilisés et peut les jouer en utilisant la bibliothèque ```miniaudio```.

Les différents sons sont stockés dans un ```SoundVec sounds``` et sont repérés par leur ```index```.

Il est alors possible d'utiliser (oui, on peut jouer plusieurs sons en même temps)
- ```play_sound( const size_t& idx )```
- ```pause_sound( const size_t& idx )```
- ```stop_soundplay( const size_t& idx )```

## Analyzer
Analyse une chaîne de caractère pour en faire un ```Looper```. Les opérations permises sont : 

``` c++
- repeat: nb x expr    // répéter nb fois une expression
- concat: expr + expr  // enchaîner deux expression
- parenthesis : (expr) // isoler une expression
```
Une ```Expr``` est soit une opération valide soit une référence à un ```Pattern``` et dans ce cas à la forme ```pxx``` ou ```Pxx``` (les 'x' sont des chiffres).

On utilise un algorithme de [Shunting Yard algorithm](https://en.wikipedia.org/wiki/Shunting_yard_algorithm). Cela génère une liste de ```UInt``` qui est ensuite associée à une ```PatternList``` et donc compatible avec le ```Looper```.

L'analyse est assez verbeuse et les messages d'erreurs nombreux.
