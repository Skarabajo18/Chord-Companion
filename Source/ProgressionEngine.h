// ProgressionEngine.h
// Generación de progresiones y cola de eventos MIDI para reproducción/exportación

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "Theory.h"
#include "Utils.h"

namespace cc
{
    class ProgressionEngine
    {
    public:
        ProgressionEngine() = default;

        void prepare(double newSampleRate)
        {
            sampleRate = newSampleRate;
            resetPlayback();
        }

        void resetPlayback()
        {
            currentSampleCursor = 0;
            nextIndex = 0;
            playing = false;
        }

        // Construye la progresión en la cola interna
        void buildQueueFromParameters(const juce::AudioProcessorValueTreeState& apvts);

        // Comienza reproducción de la cola (se inyecta en processBlock)
        void startPlayback() { playing = true; }
        void stopPlayback()  { playing = false; }
        bool isPlaying() const { return playing; }

        // Avanza el cursor y añade eventos que caen dentro del bloque
        void injectQueuedEvents(juce::MidiBuffer& midiOut, int numSamples);

        // Exporta progresión actual a archivo MIDI (.mid)
        bool exportProgressionToMidiFile(const juce::AudioProcessorValueTreeState& apvts,
                                         const juce::File& dest,
                                         double bpmIfKnown = 120.0) const;

    private:
        struct TimedEvent
        {
            int sampleOffset = 0; // desde inicio de cola
            juce::MidiMessage msg;
        };

        std::vector<TimedEvent> queue;
        size_t nextIndex = 0;
        bool playing = false;
        double sampleRate = 44100.0;
        int currentSampleCursor = 0; // acumulado desde inicio
        juce::Random rng;
    };
}