// Utils.h
// Funciones auxiliares: humanización, parsing de progresiones y utilidades varias

#pragma once

#include <juce_core/juce_core.h>

namespace cc
{
    // Convierte ms a samples (entero)
    inline int msToSamples(double sampleRate, int ms)
    {
        return static_cast<int>(std::round(sampleRate * static_cast<double>(ms) / 1000.0));
    }

    // Devuelve un delta aleatorio en samples dentro de ±maxMs
    inline int humanizeMsToSamples(double sampleRate, int maxMs, juce::Random& rng)
    {
        if (maxMs <= 0) return 0;
        const int range = msToSamples(sampleRate, maxMs);
        return rng.nextInt(juce::Range<int>(-range, range + 1));
    }

    // Humaniza la velocidad con clamp 1–127
    inline int humanizeVelocity(int base, int maxDelta, juce::Random& rng)
    {
        if (maxDelta <= 0) return juce::jlimit(1, 127, base);
        const int delta = rng.nextInt(juce::Range<int>(-maxDelta, maxDelta + 1));
        return juce::jlimit(1, 127, base + delta);
    }

    // Parser de progresión: "1-5-6-4" -> {1,5,6,4}
    inline std::vector<int> parseProgressionString(const juce::String& s)
    {
        juce::String cleaned = s.trim();
        cleaned = cleaned.replaceCharacter(',', '-');
        cleaned = cleaned.replaceCharacter(' ', '-');

        std::vector<int> degrees;
        for (auto token : juce::StringArray::fromTokens(cleaned, "-", ""))
        {
            auto t = token.trim();
            if (t.isEmpty()) continue;
            if (t.containsOnly("0123456789"))
            {
                int d = t.getIntValue();
                if (d >= 1 && d <= 7)
                    degrees.push_back(d);
            }
        }
        return degrees;
    }

    // Mapea grados a numerales romanos (mayúsculas para mayor, minúsculas para menor)
    inline juce::String degreesToRoman(const std::vector<int>& degrees, bool minor)
    {
        static const char* romanMajor[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
        static const char* romanMinor[] = { "i", "ii", "iii", "iv", "v", "vi", "vii" };
        juce::StringArray out;
        for (int d : degrees)
        {
            if (d >= 1 && d <= 7)
                out.add(minor ? romanMinor[d - 1] : romanMajor[d - 1]);
        }
        // Guión ASCII para separación
        return out.joinIntoString("-");
    }

    // Convierte número de nota MIDI a nombre (ej. C4) con sostenidos
    inline juce::String midiNoteToName(int midiNote)
    {
        return juce::MidiMessage::getMidiNoteName(midiNote, true /*useSharps*/, true /*includeOctave*/, 4 /*middleC*/);
    }

    // Convierte vector de notas a string legible "C4 E4 G4"
    inline juce::String notesToString(const std::vector<int>& notes)
    {
        juce::StringArray arr;
        for (int n : notes) arr.add(midiNoteToName(n));
        return arr.joinIntoString(" ");
    }
}