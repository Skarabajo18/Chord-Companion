// Theory.h
// Motor musical: mapeo de escalas, grados y construcción de acordes diatónicos

#pragma once

#include <juce_core/juce_core.h>
#include "Parameters.h"

namespace cc
{
    // Devuelve los offsets en semitonos desde la tónica para grados 1..7
    std::vector<int> getScaleIntervals(ScaleType scale);

    // Grado (1..7) a nota MIDI raíz basándose en keySemitone y octaveBase
    int degreeToMidi(int degree, int keySemitone, ScaleType scale, int octaveBase);

    struct ExtensionToggles
    {
        bool add7 = false;
        bool add9 = false;
        bool add11 = false;
        bool add13 = false;
    };

    // Construye notas del acorde por terceras diatónicas hasta quality, aplicando inversiones.
    // Ajusta las notas a un rango agradable (48–84 aprox.)
    std::vector<int> makeChordNotes(int rootMidi,
                                    ScaleType scale,
                                    ChordQuality quality,
                                    int inversion,
                                    const ExtensionToggles& toggles);
}