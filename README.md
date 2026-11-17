# GNOME 🧙‍♂️🎹
Sintetizador VST3 de dos osciladores con estética chiptune / pixel art.

- 2 osciladores: seno, cuadrada y diente de sierra
- Volumen independiente por oscilador
- Output Gain (-48 dB a +6 dB)
- 8 voces de polifonía

## Compilar en local
Requiere CMake 3.22+ y un compilador C++17.

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target GNOME_VST3

El `.vst3` queda en `build/GNOME_artefacts/Release/VST3/`.

## Compilar en GitHub
Cada push a `main` lanza el workflow de GitHub Actions. Descarga el VST3 desde
la pestaña **Actions → (ejecución) → Artifacts**, para Windows, macOS y Linux.
