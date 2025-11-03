// Theory.cpp

#include "Theory.h"

namespace cc
{
    std::vector<int> getScaleIntervals(ScaleType scale)
    {
        switch (scale)
        {
            case ScaleType::Major:          return { 0,2,4,5,7,9,11 };
            case ScaleType::NaturalMinor:   return { 0,2,3,5,7,8,10 };
            case ScaleType::HarmonicMinor:  return { 0,2,3,5,7,8,11 };
            case ScaleType::Dorian:         return { 0,2,3,5,7,9,10 };
            case ScaleType::Mixolydian:     return { 0,2,4,5,7,9,10 };
            default:                        return { 0,2,4,5,7,9,11 };
        }
    }

    int degreeToMidi(int degree, int keySemitone, ScaleType scale, int octaveBase)
    {
        jassert(degree >= 1 && degree <= 7);
        const auto intervals = getScaleIntervals(scale);
        const int base = (octaveBase * 12) + keySemitone;
        const int offset = intervals[juce::jlimit(1, 7, degree) - 1];
        return base + offset;
    }

    static int diatonicDegreeToSemitone(const std::vector<int>& intervals, int degree)
    {
        // degree puede ser >7 (para 9, 11, 13). Cada vuelta suma 12.
        const int wraps = (degree - 1) / 7;
        const int idx = (degree - 1) % 7;
        return intervals[idx] + wraps * 12;
    }

    std::vector<int> makeChordNotes(int rootMidi,
                                    ScaleType scale,
                                    ChordQuality quality,
                                    int inversion,
                                    const ExtensionToggles& toggles)
    {
        // Construcción por terceras diatónicas: 1,3,5,7,9,11,13
        const auto intervals = getScaleIntervals(scale);

        std::vector<int> degrees = { 1, 3, 5 };
        if (quality >= ChordQuality::Seventh || toggles.add7)    degrees.push_back(7);
        if (quality >= ChordQuality::Ninth   || toggles.add9)    degrees.push_back(9);
        if (quality >= ChordQuality::Eleventh|| toggles.add11)   degrees.push_back(11);
        if (quality >= ChordQuality::Thirteenth || toggles.add13) degrees.push_back(13);

        std::vector<int> notes;
        notes.reserve(degrees.size());
        for (int d : degrees)
        {
            const int semi = diatonicDegreeToSemitone(intervals, d);
            notes.push_back(rootMidi + semi);
        }

        // Aplicar inversión: mueve la nota más baja 12 semitonos arriba por cada inversión
        const int maxInv = (int) juce::jmin((int) degrees.size() - 1, inversion);
        for (int i = 0; i < maxInv; ++i)
        {
            // mover la más baja al final +12
            std::sort(notes.begin(), notes.end());
            notes.front() += 12;
        }

        // Ajuste de rango agradable (48–84)
        auto clampIntoRange = [](std::vector<int>& ns)
        {
            if (ns.empty()) return;
            for (int iter = 0; iter < 8; ++iter)
            {
                int mn = *std::min_element(ns.begin(), ns.end());
                int mx = *std::max_element(ns.begin(), ns.end());
                if (mn < 48) { for (auto& n : ns) n += 12; continue; }
                if (mx > 84) { for (auto& n : ns) n -= 12; continue; }
                break; // dentro de rango
            }
        };
        clampIntoRange(notes);

        // Ordenar ascendente por estética
        std::sort(notes.begin(), notes.end());
        return notes;
    }
}