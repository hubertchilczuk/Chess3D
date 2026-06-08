# Chess3D

Trójwymiarowy odtwarzacz partii szachowej (PGN viewer) napisany w C++17 / OpenGL 3.3.
Projekt edukacyjny - grafika komputerowa.

## Budowanie (Visual Studio 2022/2026)

1. Otwórz plik `Chess3D.sln` w Visual Studio.
2. Wybierz konfigurację `Release | x64` (lub `Debug | x64`).
3. `Build → Build Solution` (Ctrl+Shift+B).
4. Uruchom `Chess3D.exe` z katalogu głównego projektu (lub przez `Debug → Start Without Debugging`).

**Wszystkie biblioteki są dołączone w folderze `third_party/` - nie jest wymagane pobieranie żadnych zewnętrznych zależności.**

Biblioteki dołączone:
- **GLFW 3.3.9** - obsługa okna i wejścia
- **GLEW 2.2.0** - loader rozszerzeń OpenGL
- **GLM 1.0.1** - matematyka wektorowa/macierzowa
- **Assimp 5.4.2** - wczytywanie modeli 3D
- **ImGui 1.91.0** - interfejs użytkownika
- **stb_image** - wczytywanie tekstur

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
├── Chess3D.sln           # rozwiązanie Visual Studio
├── Chess3D.vcxproj       # plik projektu
├── src/                  # implementacje
├── include/              # nagłówki
├── shaders/              # GLSL (vertex + fragment)
├── resources/
│   ├── models/           # modele figur (.obj / .fbx)
│   └── pgn/              # przykładowe partie PGN
└── third_party/          # biblioteki (dołączone, nie wymagają pobierania)
    ├── glfw/
    ├── glew/
    ├── glm/
    ├── stb/
    ├── imgui/
    └── assimp/
```

Silnik szuka modeli figur w trzech krokach (każdy następny stosowany tylko gdy poprzedni nie wystarczy):

1. **Pliki per-figura** w `resources/models/pawn.{obj,fbx,gltf,glb,dae,ply,blend}` (i analogicznie `knight`, `bishop`, `rook`, `queen`, `king`).
2. **Plik zbiorczy z nazwanymi obiektami** (np. OBJ z `o pawn` / `o knight` ..., albo FBX z nazwanymi nodami) - znaleziony w `resources/` lub `resources/models/`. Renderer wyciąga każdą figurę po nazwie węzła.
3. **Plik zbiorczy bez nazw** (jeden duży mesh łączący wszystkie figury) - silnik dzieli go na spójne komponenty (union-find po indeksach trójkątów), sortuje rosnąco po wysokości i przypisuje w kolejności `pawn → rook → knight → bishop → queen → king`.
