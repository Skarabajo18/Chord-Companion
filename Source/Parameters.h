// Parameters.h
// Definición de parámetros APVTS y enums estables para ChordCompanion

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace cc // ChordCompanion
{
    // Enumeraciones de dominio musical
    enum class Key : int
    {
        C = 0, Cs = 1, D = 2, Ds = 3, E = 4, F = 5, Fs = 6,
        G = 7, Gs = 8, A = 9, As = 10, B = 11
    };

    enum class ScaleType : int
    {
        Major = 0,
        NaturalMinor,
        HarmonicMinor,
        Dorian,
        Mixolydian
    };

    enum class ProgressionPreset : int
    {
        I_V_vi_IV = 0,
        ii_V_I,
        I_vi_IV_V,
        vi_IV_I_V,
        Custom
    };

    enum class ChordQuality : int
    {
        Triad = 0,
        Seventh,
        Ninth,
        Eleventh,
        Thirteenth
    };

    // IDs de parámetros (estables)
    namespace ParamID
    {
        static constexpr const char* key = "key";
        static constexpr const char* scale = "scale";
        static constexpr const char* progressionPreset = "progressionPreset";
        static constexpr const char* progressionCustom = "progressionCustom"; // string
        static constexpr const char* chordQuality = "chordQuality";
        static constexpr const char* add7 = "add7";
        static constexpr const char* add9 = "add9";
        static constexpr const char* add11 = "add11";
        static constexpr const char* add13 = "add13";
        static constexpr const char* inversion = "inversion";
        static constexpr const char* velocity = "velocity";
        static constexpr const char* noteLengthMs = "noteLengthMs";
        static constexpr const char* humanizeMs = "humanizeMs";
        static constexpr const char* humanizeVel = "humanizeVel";
        static constexpr const char* octave = "octave";
        static constexpr const char* followHost = "followHost";
        static constexpr const char* generateNow = "generateNow"; // botón/flag one-shot
        static constexpr const char* exportMidi = "exportMidi";   // botón para FileChooser
        // Propiedades auxiliares (no parámetros): strings para mostrar notas
        static constexpr const char* lastChordNotes = "lastChordNotes"; // e.g. "C4 E4 G4"
        static constexpr const char* sequenceNotes  = "sequenceNotes";  // resumen de cola
    }

    inline juce::StringArray getKeyChoices()
    {
        return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    }

    inline juce::StringArray getScaleChoices()
    {
        return { "Major", "NaturalMinor", "HarmonicMinor", "Dorian", "Mixolydian" };
    }

    inline juce::StringArray getProgressionChoices()
    {
        // Usar guiones ASCII para evitar problemas de codificación en literales
        return { "I-V-vi-IV", "ii-V-I", "I-vi-IV-V", "vi-IV-I-V", "Custom" };
    }

    inline juce::StringArray getChordQualityChoices()
    {
        return { "Triad", "Seventh", "Ninth", "Eleventh", "Thirteenth" };
    }

    // Construye el layout de parámetros para APVTS
    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        // Enums (choices)
        params.push_back(std::make_unique<AudioParameterChoice>(ParamID::key, "Key", getKeyChoices(), (int) Key::C));
        params.push_back(std::make_unique<AudioParameterChoice>(ParamID::scale, "Scale", getScaleChoices(), (int) ScaleType::Major));
        params.push_back(std::make_unique<AudioParameterChoice>(ParamID::progressionPreset, "Progression Preset", getProgressionChoices(), (int) ProgressionPreset::I_V_vi_IV));
        params.push_back(std::make_unique<AudioParameterChoice>(ParamID::chordQuality, "Chord Quality", getChordQualityChoices(), (int) ChordQuality::Triad));

        // Nota: JUCE no ofrece AudioParameterString estándar.
        // Usaremos una propiedad en el ValueTree (apvts.state) para progressionCustom.
        // Se inicializa en el Processor con valor por defecto "1-5-6-4".

        // Bool toggles
        params.push_back(std::make_unique<AudioParameterBool>(ParamID::add7, "Add 7", false));
        params.push_back(std::make_unique<AudioParameterBool>(ParamID::add9, "Add 9", false));
        params.push_back(std::make_unique<AudioParameterBool>(ParamID::add11, "Add 11", false));
        params.push_back(std::make_unique<AudioParameterBool>(ParamID::add13, "Add 13", false));
        params.push_back(std::make_unique<AudioParameterBool>(ParamID::followHost, "Follow Host", true));

        // Botones/flags one-shot
        params.push_back(std::make_unique<AudioParameterBool>(ParamID::generateNow, "Generate Now", false));
        params.push_back(std::make_unique<AudioParameterBool>(ParamID::exportMidi, "Export MIDI", false));

        // Int ranges
        params.push_back(std::make_unique<AudioParameterInt>(ParamID::inversion, "Inversion", 0, 3, 0));
        params.push_back(std::make_unique<AudioParameterInt>(ParamID::velocity, "Velocity", 1, 127, 96));
        params.push_back(std::make_unique<AudioParameterInt>(ParamID::noteLengthMs, "Note Length (ms)", 10, 4000, 600));
        params.push_back(std::make_unique<AudioParameterInt>(ParamID::humanizeMs, "Humanize (ms)", 0, 25, 0));
        params.push_back(std::make_unique<AudioParameterInt>(ParamID::humanizeVel, "Humanize Velocity", 0, 15, 0));
        params.push_back(std::make_unique<AudioParameterInt>(ParamID::octave, "Octave", 3, 6, 4));

        return { params.begin(), params.end() };
    }
}