#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <map>

struct NoteEvent {
    int midiNote   = 60;
    float velocity = 0.8f;
    uint32_t startTick  = 0;   // PPQ ticks
    uint32_t lengthTick = 480; // default 1 beat at 480 PPQ
};

// ─── Scale definitions ────────────────────────────────────────────────────────
struct ScaleInfo {
    std::string name;
    std::vector<int> semitones;
};

inline const std::vector<ScaleInfo>& getAllScales() {
    static const std::vector<ScaleInfo> scales = {
        { "Major",          {0,2,4,5,7,9,11}   },
        { "Minor",          {0,2,3,5,7,8,10}   },
        { "Pent. Major",    {0,2,4,7,9}        },
        { "Pent. Minor",    {0,3,5,7,10}       },
        { "Blues",          {0,3,5,6,7,10}     },
        { "Dorian",         {0,2,3,5,7,9,10}   },
        { "Phrygian",       {0,1,3,5,7,8,10}   },
        { "Lydian",         {0,2,4,6,7,9,11}   },
        { "Mixolydian",     {0,2,4,5,7,9,10}   },
        { "Locrian",        {0,1,3,5,6,8,10}   },
    };
    return scales;
}

inline const ScaleInfo& getScale(int index) {
    auto& s = getAllScales();
    if (index < 0 || index >= (int)s.size()) return s[0];
    return s[index];
}

// ─── Step data (shared with UI for display) ───────────────────────────────────
struct StepData {
    bool    active   = false;
    float   velocity = 0.f;  // 0..1
    int     note     = 60;   // absolute MIDI note
};

// ─── Generator ────────────────────────────────────────────────────────────────
class SequenceGenerator {
public:
    /// Configure the generator. Call before generateBars().
    void configure(int ppq, int steps, int rootNote,
                   const std::vector<int>& scaleSemis,
                   float swingPercent = 0.0f,
                   float density      = 0.7f,
                   int   octaveSpread = 1,
                   int   seed         = 42);

    /// Generate events for N bars.
    std::vector<NoteEvent> generateBars(int bars) const;

    /// Return per-step display data for one bar (for UI step grid).
    std::vector<StepData> getStepPattern() const;

private:
    int              ppq_          = 480;
    int              steps_        = 16;
    int              root_         = 60;
    std::vector<int> scale_        {0,2,4,5,7,9,11};
    float            swing_        = 0.0f;
    float            density_      = 0.7f;
    int              octaveSpread_ = 1;
    int              seed_         = 42;
};
