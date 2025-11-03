// PluginProcessor.h
// Procesador principal de ChordCompanion: efecto MIDI VST3

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "Theory.h"
#include "Utils.h"
#include "ProgressionEngine.h"

class ChordCompanionAudioProcessor : public juce::AudioProcessor
{
public:
    ChordCompanionAudioProcessor();
    ~ChordCompanionAudioProcessor() override;

    // JUCE AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ChordCompanion"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // APVTS
    juce::AudioProcessorValueTreeState apvts;
    juce::UndoManager undo;

    // Exportación (usado por Editor)
    bool exportProgressionToMidiFile(const juce::File& dest);

    // One-shot Generate desde Editor (opcional)
    void triggerGenerateNow();

private:
    cc::ProgressionEngine engine;

    // Tracking de progresión en tiempo real
    size_t currentDegreeIndex = 0;
    std::vector<int> cachedDegrees; // cache de preset/custom

    // Utilidad para leer posición del host (API moderna)
    bool getHostInfo(juce::AudioPlayHead::PositionInfo& outInfo) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordCompanionAudioProcessor)
};