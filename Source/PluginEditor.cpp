// PluginEditor.cpp

#include "PluginEditor.h"

ChordCompanionAudioProcessorEditor::ChordCompanionAudioProcessorEditor(ChordCompanionAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    setSize(660, 420);

    // Combos
    keyBox.addItemList(cc::getKeyChoices(), 1);
    scaleBox.addItemList(cc::getScaleChoices(), 1);
    progPresetBox.addItemList(cc::getProgressionChoices(), 1);
    qualityBox.addItemList(cc::getChordQualityChoices(), 1);

    inversionBox.addItemList({ "Root", "1st", "2nd", "3rd" }, 1);

    addAndMakeVisible(keyBox);
    addAndMakeVisible(scaleBox);
    addAndMakeVisible(progPresetBox);
    addAndMakeVisible(qualityBox);
    addAndMakeVisible(inversionBox);

    // TextEditor
    progressionCustom.setText(processor.apvts.state.getProperty(cc::ParamID::progressionCustom, juce::String("1-5-6-4")).toString());
    addAndMakeVisible(progressionCustom);

    // Sliders
    auto setupSlider = [](juce::Slider& s){ s.setSliderStyle(juce::Slider::LinearHorizontal); s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20); };
    setupSlider(velocitySlider);
    setupSlider(noteLenSlider);
    setupSlider(humanizeMsSlider);
    setupSlider(humanizeVelSlider);
    setupSlider(octaveSlider);
    addAndMakeVisible(velocitySlider);
    addAndMakeVisible(noteLenSlider);
    addAndMakeVisible(humanizeMsSlider);
    addAndMakeVisible(humanizeVelSlider);
    addAndMakeVisible(octaveSlider);

    // Toggles
    addAndMakeVisible(add7Toggle);
    addAndMakeVisible(add9Toggle);
    addAndMakeVisible(add11Toggle);
    addAndMakeVisible(add13Toggle);
    addAndMakeVisible(followHostToggle);
    addAndMakeVisible(generateToggle);
    addAndMakeVisible(exportToggle);

    // Label de progresión
    progressionLabel.setText("Progression: I-V-vi-IV", juce::dontSendNotification);
    addAndMakeVisible(progressionLabel);
    lastNotesLabel.setText("Notes: ", juce::dontSendNotification);
    addAndMakeVisible(lastNotesLabel);
    sequenceLabel.setText("Sequence: ", juce::dontSendNotification);
    addAndMakeVisible(sequenceLabel);

    // Attachments
    auto& apvts = processor.apvts;
    keyAtt.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, cc::ParamID::key, keyBox));
    scaleAtt.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, cc::ParamID::scale, scaleBox));
    presetAtt.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, cc::ParamID::progressionPreset, progPresetBox));
    qualityAtt.reset(new juce::AudioProcessorValueTreeState::ComboBoxAttachment(apvts, cc::ParamID::chordQuality, qualityBox));

    velocityAtt.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, cc::ParamID::velocity, velocitySlider));
    noteLenAtt.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, cc::ParamID::noteLengthMs, noteLenSlider));
    humanizeMsAtt.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, cc::ParamID::humanizeMs, humanizeMsSlider));
    humanizeVelAtt.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, cc::ParamID::humanizeVel, humanizeVelSlider));
    octaveAtt.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(apvts, cc::ParamID::octave, octaveSlider));

    add7Att.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, cc::ParamID::add7, add7Toggle));
    add9Att.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, cc::ParamID::add9, add9Toggle));
    add11Att.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, cc::ParamID::add11, add11Toggle));
    add13Att.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, cc::ParamID::add13, add13Toggle));
    followHostAtt.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, cc::ParamID::followHost, followHostToggle));

    generateAtt.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, cc::ParamID::generateNow, generateToggle));
    exportAtt.reset(new juce::AudioProcessorValueTreeState::ButtonAttachment(apvts, cc::ParamID::exportMidi, exportToggle));

    // Binding manual del TextEditor al ValueTree
    progressionCustom.onTextChange = [this]
    {
        const bool isCustom = (progPresetBox.getSelectedId() - 1) == (int) cc::ProgressionPreset::Custom;
        if (isCustom)
            processor.apvts.state.setProperty(cc::ParamID::progressionCustom, progressionCustom.getText(), nullptr);
        updateProgressionLabel();
    };

    // Interacciones adicionales
    progPresetBox.onChange = [this]
    {
        const bool isCustom = (progPresetBox.getSelectedId() - 1) == (int) cc::ProgressionPreset::Custom;
        progressionCustom.setEnabled(isCustom);
        updateProgressionLabel();
    };

    exportToggle.onClick = [this]
    {
        if (exportToggle.getToggleState())
        {
            juce::FileChooser chooser("Export MIDI", juce::File(), "*.mid");
            chooser.launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                                [this](const juce::FileChooser& fc)
                                {
                                    auto file = fc.getResult();
                                    if (file != juce::File())
                                        processor.exportProgressionToMidiFile(file);

                                    // resetear flag
                                    exportToggle.setToggleState(false, juce::dontSendNotification);
                                    if (auto* p = processor.apvts.getParameter(cc::ParamID::exportMidi))
                                    {
                                        p->beginChangeGesture();
                                        p->setValueNotifyingHost(0.0f);
                                        p->endChangeGesture();
                                    }
                                });
        }
    };

    generateToggle.onClick = [this]
    {
        if (generateToggle.getToggleState())
        {
            processor.triggerGenerateNow();
            generateToggle.setToggleState(false, juce::dontSendNotification);
            if (auto* p = processor.apvts.getParameter(cc::ParamID::generateNow))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost(0.0f);
                p->endChangeGesture();
            }
        }
    };

    startTimerHz(10); // refrescar label/estado
}

ChordCompanionAudioProcessorEditor::~ChordCompanionAudioProcessorEditor()
{
    stopTimer();
}

void ChordCompanionAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("ChordCompanion - MIDI Effect", getLocalBounds().reduced(10), juce::Justification::centredTop, true);
}

void ChordCompanionAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto row = area.removeFromTop(28);
    keyBox.setBounds(row.removeFromLeft(120));
    scaleBox.setBounds(row.removeFromLeft(140));
    progPresetBox.setBounds(row.removeFromLeft(160));
    qualityBox.setBounds(row.removeFromLeft(120));
    inversionBox.setBounds(row.removeFromLeft(100));

    auto row2 = area.removeFromTop(28);
    progressionCustom.setBounds(row2.removeFromLeft(200));
    followHostToggle.setBounds(row2.removeFromLeft(140));
    generateToggle.setBounds(row2.removeFromLeft(120));
    exportToggle.setBounds(row2.removeFromLeft(120));

    auto slidersA = area.removeFromTop(28);
    velocitySlider.setBounds(slidersA.removeFromLeft(220));
    noteLenSlider.setBounds(slidersA.removeFromLeft(220));
    octaveSlider.setBounds(slidersA.removeFromLeft(200));

    auto slidersB = area.removeFromTop(28);
    humanizeMsSlider.setBounds(slidersB.removeFromLeft(220));
    humanizeVelSlider.setBounds(slidersB.removeFromLeft(220));

    auto toggles = area.removeFromTop(28);
    add7Toggle.setBounds(toggles.removeFromLeft(100));
    add9Toggle.setBounds(toggles.removeFromLeft(100));
    add11Toggle.setBounds(toggles.removeFromLeft(100));
    add13Toggle.setBounds(toggles.removeFromLeft(100));

    progressionLabel.setBounds(area.removeFromTop(22));
    lastNotesLabel.setBounds(area.removeFromTop(22));
    sequenceLabel.setBounds(area.removeFromTop(22));
}

void ChordCompanionAudioProcessorEditor::timerCallback()
{
    updateProgressionLabel();
    // Refrescar labels de notas
    lastNotesLabel.setText("Notes: " + processor.apvts.state.getProperty(cc::ParamID::lastChordNotes).toString(), juce::dontSendNotification);
    sequenceLabel.setText("Sequence: " + processor.apvts.state.getProperty(cc::ParamID::sequenceNotes).toString(), juce::dontSendNotification);
}

void ChordCompanionAudioProcessorEditor::updateProgressionLabel()
{
    const int presetIdx = (int) std::lrintf(processor.apvts.getRawParameterValue(cc::ParamID::progressionPreset)->load());
    const juce::String customStr = processor.apvts.state.getProperty(cc::ParamID::progressionCustom, juce::String("1-5-6-4")).toString();
    std::vector<int> degrees;
    if ((cc::ProgressionPreset) presetIdx == cc::ProgressionPreset::Custom)
        degrees = cc::parseProgressionString(customStr);
    else
    {
        switch ((cc::ProgressionPreset) presetIdx)
        {
            case cc::ProgressionPreset::I_V_vi_IV: degrees = {1,5,6,4}; break;
            case cc::ProgressionPreset::ii_V_I:    degrees = {2,5,1}; break;
            case cc::ProgressionPreset::I_vi_IV_V: degrees = {1,6,4,5}; break;
            case cc::ProgressionPreset::vi_IV_I_V: degrees = {6,4,1,5}; break;
            default: degrees = {1,5,6,4}; break;
        }
    }
    const int scaleIdx = processor.apvts.getParameterAsValue(cc::ParamID::scale).toString().getIntValue();
    const bool minorLike = (cc::ScaleType) scaleIdx != cc::ScaleType::Major;
    progressionLabel.setText("Progression: " + cc::degreesToRoman(degrees, minorLike), juce::dontSendNotification);
}