#include "SequenceGenerator.h"
#include <random>
#include <algorithm>

void SequenceGenerator::configure(int ppq, int steps, int rootNote,
                                   const std::vector<int>& scaleSemis,
                                   float swingPercent, float density,
                                   int octaveSpread, int seed) {
    ppq_          = ppq > 0 ? ppq : 480;
    steps_        = std::max(1, steps);
    root_         = rootNote;
    scale_        = scaleSemis.empty() ? std::vector<int>{0,2,4,5,7,9,11} : scaleSemis;
    swing_        = std::max(0.f, std::min(0.5f, swingPercent));
    density_      = std::max(0.f, std::min(1.f, density));
    octaveSpread_ = std::max(0, octaveSpread);
    seed_         = seed;
}

// Shared step-pattern builder used by both generateBars() and getStepPattern().
// Returns one bar of StepData.
static std::vector<StepData> buildPattern(int steps, int root,
                                           const std::vector<int>& scale,
                                           float density, int octaveSpread, int seed) {
    std::mt19937 rng((unsigned)seed);
    std::uniform_real_distribution<float> uni(0.f, 1.f);
    std::uniform_int_distribution<int>    octDist(0, std::max(0, octaveSpread));

    std::vector<StepData> pattern(steps);

    for (int s = 0; s < steps; ++s) {
        // Velocity: base + per-step variation seeded by (seed ^ step)
        std::mt19937 velRng((unsigned)(seed * 1000 + s));
        std::uniform_real_distribution<float> velVar(0.55f, 1.0f);

        bool active = uni(rng) <= density;
        int degree  = s % (int)scale.size();
        int octave  = octDist(rng);
        int note    = root + scale[degree] + octave * 12;

        pattern[s].active   = active;
        pattern[s].velocity = active ? velVar(velRng) : 0.f;
        pattern[s].note     = note;
    }
    return pattern;
}

std::vector<NoteEvent> SequenceGenerator::generateBars(int bars) const {
    std::vector<NoteEvent> out;
    if (bars <= 0) return out;

    const uint32_t stepTicks = (uint32_t)((ppq_ * 4) / steps_);
    const auto pattern = buildPattern(steps_, root_, scale_, density_, octaveSpread_, seed_);

    for (int b = 0; b < bars; ++b) {
        const uint32_t barOffset = b * steps_ * stepTicks;

        for (int s = 0; s < steps_; ++s) {
            const auto& step = pattern[s];
            if (!step.active) continue;

            uint32_t start = barOffset + s * stepTicks;
            // Swing on odd steps
            if (s % 2 == 1)
                start += (uint32_t)(stepTicks * swing_);

            NoteEvent ev;
            ev.midiNote   = std::max(0, std::min(127, step.note));
            ev.velocity   = step.velocity;
            ev.startTick  = start;
            ev.lengthTick = (uint32_t)(stepTicks * 0.88f);
            out.push_back(ev);
        }
    }
    return out;
}

std::vector<StepData> SequenceGenerator::getStepPattern() const {
    return buildPattern(steps_, root_, scale_, density_, octaveSpread_, seed_);
}
