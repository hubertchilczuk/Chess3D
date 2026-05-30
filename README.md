# Chess3D

Trójwymiarowy odtwarzacz partii szachowej (PGN viewer) napisany w C++17 / OpenGL 3.3.
Projekt edukacyjny - grafika komputerowa.

## Budowanie (Visual Studio 2022)

1. Otwórz folder `D:\Desktop\Chess3D` w Visual Studio 2022:
   `File -> Open -> Folder...`
   VS automatycznie wykryje `CMakeLists.txt` i `CMakePresets.json` i skonfiguruje projekt.
2. Z paska narzędzi wybierz konfigurację `x64-debug` lub `x64-release`.
3. `Build -> Build All` (pierwszy build pobiera GLFW, GLM, Assimp, GLEW, ImGui i stb przez `FetchContent` - może trwać kilka minut).
4. `Debug -> Start Without Debugging` (Ctrl+F5).

> Uwaga o loaderze OpenGL: użyto `glew-cmake` zamiast GLAD - obie biblioteki realizują identyczne zadanie (ładowanie funkcji OpenGL 3.3 Core), ale glew-cmake nie wymaga generatora w Pythonie, dzięki czemu konfiguracja CMake działa od razu po `File -> Open -> Folder`.

## Sterowanie

- **WASD** - ruch kamery
- **Q/E** - w dół / w górę
- **Prawym przyciskiem myszy + ruch** - obrót kamery (mouse-look)
- **Kółko myszy** - zoom
- **Spacja** - play/pause partii
- **Strzałka w prawo** - następny ruch
- **Strzałka w lewo** - poprzedni ruch
- **R** - reset partii do pozycji początkowej

Panel ImGui pozwala wczytać dowolny plik PGN i sterować tempem odtwarzania.

## Struktura katalogów

```
Chess3D/
├── CMakeLists.txt
├── src/                  # implementacje
├── include/              # nagłówki
├── shaders/              # GLSL (vertex + fragment)
└── resources/
    ├── models/           # modele figur (.obj / .fbx)
    ├── textures/         # tekstury drewna i materiałów
    └── pgn/              # przykładowe partie PGN
```

Silnik szuka modeli figur w trzech krokach (każdy następny stosowany tylko gdy poprzedni nie wystarczy):

1. **Pliki per-figura** w `resources/models/pawn.{obj,fbx,gltf,glb,dae,ply,blend}` (i analogicznie `knight`, `bishop`, `rook`, `queen`, `king`).
2. **Plik zbiorczy z nazwanymi obiektami** (np. OBJ z `o pawn` / `o knight` ..., albo FBX z nazwanymi nodami) - znaleziony w `resources/` lub `resources/models/`. Renderer wyciąga każdą figurę po nazwie węzła.
3. **Plik zbiorczy bez nazw** (jeden duży mesh łączący wszystkie figury) - silnik dzieli go na spójne komponenty (union-find po indeksach trójkątów), sortuje rosnąco po wysokości i przypisuje w kolejności `pawn → rook → knight → bishop → queen → king`.
