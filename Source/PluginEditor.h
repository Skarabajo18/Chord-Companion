// PluginEditor.h
// Editor de UI para ChordCompanion

#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "Parameters.h"
#include "Utils.h"

class ChordCompanionAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit ChordCompanionAudioProcessorEditor(ChordCompanionAudioProcessor&);
    ~ChordCompanionAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateProgressionLabel();

    ChordCompanionAudioProcessor& processor;

    // Controles
    juce::ComboBox keyBox, scaleBox, progPresetBox, qualityBox, inversionBox;
    juce::TextEditor progressionCustom;
    juce::Slider velocitySlider, noteLenSlider, humanizeMsSlider, humanizeVelSlider, octaveSlider;
    juce::ToggleButton add7Toggle {"Add 7"}, add9Toggle {"Add 9"}, add11Toggle {"Add 11"}, add13Toggle {"Add 13"};
    juce::ToggleButton followHostToggle {"Follow Host"};
    juce::ToggleButton generateToggle {"Generate"};
    juce::ToggleButton exportToggle {"Export MIDI"};

    juce::Label progressionLabel;
    juce::Label lastNotesLabel;
    juce::Label sequenceLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt, presetAtt, qualityAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> velocityAtt, noteLenAtt, humanizeMsAtt, humanizeVelAtt, octaveAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> add7Att, add9Att, add11Att, add13Att, followHostAtt, generateAtt, exportAtt;
    // No existe TextEditorAttachment en APVTS; se vincula manualmente al ValueTree

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordCompanionAudioProcessorEditor)
};