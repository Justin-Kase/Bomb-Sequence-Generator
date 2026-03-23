# Illbomb Sequence Generator (VST3 + CLI) – v0.2.0

A JUCE-based VST3 MIDI step sequencer by **Illbomb** for Bitwig and other VST3 hosts. Generates step-sequenced note patterns with scale selection, per-step velocity variation, and a seed-based generative engine. Includes a standalone CLI for exporting MIDI clips without a DAW.

## Features

- **VST3 MIDI effect** — no audio, pure MIDI note output
- **10 scales** — Major, Minor, Pentatonic Major/Minor, Blues, Dorian, Phrygian, Lydian, Mixolydian, Locrian
- **Seed parameter (0–999)** — each seed generates a unique pattern and velocity curve
- **Per-step velocity variation** — humanized feel, driven by seed
- **7 controls** — Steps (4–32), Swing, Density, Root, Octaves, Scale, Seed
- **Live step grid** — shows active steps with velocity-driven brightness and amber playhead
- **Dark UI** — custom look-and-feel matching Illbomb branding
- **CLI `midigen`** — export `.mid` files from the terminal without opening a DAW

## macOS Prerequisites

- Xcode command line tools
- CMake 3.21+

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Artifacts:
- Plugin: `build/Source/IllbombSeqGenerator_artefacts/Release/VST3/Illbomb Sequence Generator.vst3`
- CLI: `build/Source/midigen`

## Install to Bitwig

```bash
cp -r "build/Source/IllbombSeqGenerator_artefacts/Release/VST3/Illbomb Sequence Generator.vst3" \
      ~/Library/Audio/Plug-Ins/VST3/
```

Then in Bitwig: **Settings → Plug-ins → Rescan**. Insert as a **Note FX** on any instrument track.

## Using the Plugin

1. Drop it before an instrument in Bitwig's device chain
2. Hit play — it generates one-bar sequences synced to host tempo
3. Dial in your scale, seed, and density
4. Change **Seed** to instantly flip to a new pattern with the same feel
5. The step grid lights up in real time — brightness = velocity, amber = playhead position

## CLI Usage

```bash
./build/Source/midigen \
  --steps 16 \
  --bars 4 \
  --root 60 \
  --swing 0.1 \
  --density 0.7 \
  --octaves 1 \
  --scale 4 \
  --seed 77 \
  --out sequence.mid
```

### Scale index reference

| Index | Scale |
|-------|-------|
| 0 | Major |
| 1 | Minor |
| 2 | Pentatonic Major |
| 3 | Pentatonic Minor |
| 4 | Blues |
| 5 | Dorian |
| 6 | Phrygian |
| 7 | Lydian |
| 8 | Mixolydian |
| 9 | Locrian |

## Roadmap

- Multi-bar pattern generation with sample-accurate scheduling
- Preset save/load
- Euclidean rhythm mode
- Per-step pitch lock / note override
- Windows & Linux builds via GitHub Actions
