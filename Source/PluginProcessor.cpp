// PluginProcessor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"
// Incluir .cpp directamente para asegurar que se compilan en este TU
// (útil si el proyecto no ha sido re-exportado desde Projucer para añadir nuevos archivos)
#include "Theory.cpp"
#include "ProgressionEngine.cpp"
#include "Utils.cpp"

ChordCompanionAudioProcessor::ChordCompanionAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, &undo, "PARAMS", cc::createParameterLayout())
{
    // Inicializar propiedad de texto para progressionCustom (no es parámetro estándar)
    if (! apvts.state.hasProperty(cc::ParamID::progressionCustom))
        apvts.state.setProperty(cc::ParamID::progressionCustom, juce::String("1-5-6-4"), nullptr);
    // Inicializa propiedades de notas para la UI
    if (! apvts.state.hasProperty(cc::ParamID::lastChordNotes))
        apvts.state.setProperty(cc::ParamID::lastChordNotes, juce::String(), nullptr);
    if (! apvts.state.hasProperty(cc::ParamID::sequenceNotes))
        apvts.state.setProperty(cc::ParamID::sequenceNotes, juce::String(), nullptr);
}

ChordCompanionAudioProcessor::~ChordCompanionAudioProcessor() = default;

void ChordCompanionAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    engine.prepare(sampleRate);
}

void ChordCompanionAudioProcessor::releaseResources()
{
}

bool ChordCompanionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Permite cualquier combinación; el plugin ignora audio
    juce::ignoreUnused(layouts);
    return true;
}

static float loadParam(const juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
{
    if (auto* p = apvts.getRawParameterValue(id))
        return p->load();
    return 0.0f;
}

static int loadIntParam(const juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
{
    return (int) std::lrintf(loadParam(apvts, id));
}

static bool loadBoolParam(const juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
{
    return loadParam(apvts, id) >= 0.5f;
}

static std::vector<int> getDegreesFromParameters(const juce::AudioProcessorValueTreeState& apvts)
{
    const int presetIdx = loadIntParam(apvts, cc::ParamID::progressionPreset);
    const juce::String customStr = apvts.state.getProperty(cc::ParamID::progressionCustom, juce::String("1-5-6-4")).toString();
    const auto preset = (cc::ProgressionPreset) presetIdx;
    if (preset == cc::ProgressionPreset::Custom)
        return cc::parseProgressionString(customStr);
    switch (preset)
    {
        case cc::ProgressionPreset::I_V_vi_IV: return {1,5,6,4};
        case cc::ProgressionPreset::ii_V_I:    return {2,5,1};
        case cc::ProgressionPreset::I_vi_IV_V: return {1,6,4,5};
        case cc::ProgressionPreset::vi_IV_I_V: return {6,4,1,5};
        default: return {1,5,6,4};
    }
}

void ChordCompanionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    // Este plugin no produce audio: limpia el buffer
    buffer.clear();

    // One-shot generate/Export mediante flags APVTS
    {
        auto* genParam = apvts.getParameter(cc::ParamID::generateNow);
        if (genParam && genParam->getValue() > 0.5f)
        {
            // Construir cola y también preparar resumen de notas
            engine.buildQueueFromParameters(apvts);

            // Crear resumen de la secuencia
            auto degrees = getDegreesFromParameters(apvts);
            const int keyIdx     = loadIntParam(apvts, cc::ParamID::key);
            const int scaleIdx   = loadIntParam(apvts, cc::ParamID::scale);
            const int qualityIdx = loadIntParam(apvts, cc::ParamID::chordQuality);
            const bool add7  = loadBoolParam(apvts, cc::ParamID::add7);
            const bool add9  = loadBoolParam(apvts, cc::ParamID::add9);
            const bool add11 = loadBoolParam(apvts, cc::ParamID::add11);
            const bool add13 = loadBoolParam(apvts, cc::ParamID::add13);
            const int inversion    = loadIntParam(apvts, cc::ParamID::inversion);
            const int octave       = loadIntParam(apvts, cc::ParamID::octave);

            const auto scale      = (cc::ScaleType) scaleIdx;
            const auto quality    = (cc::ChordQuality) qualityIdx;
            const int keySemitone = keyIdx;
            cc::ExtensionToggles togg { add7, add9, add11, add13 };

            juce::StringArray chordStrings;
            for (int deg : degrees)
            {
                const int root = cc::degreeToMidi(deg, keySemitone, scale, octave);
                auto notes = cc::makeChordNotes(root, scale, quality, inversion, togg);
                chordStrings.add("[" + cc::notesToString(notes) + "]");
            }
            apvts.state.setProperty(cc::ParamID::sequenceNotes, chordStrings.joinIntoString(" | "), nullptr);
            engine.startPlayback();
            genParam->beginChangeGesture();
            genParam->setValueNotifyingHost(0.0f);
            genParam->endChangeGesture();
        }

        auto* expParam = apvts.getParameter(cc::ParamID::exportMidi);
        if (expParam && expParam->getValue() > 0.5f)
        {
            // El editor abrirá el FileChooser; aquí solo reseteamos el flag tras exportar
            // (Export real se realiza en el editor para interactuar con la UI)
        }
    }

    // Reproducir cola generada si aplica
    engine.injectQueuedEvents(midi, buffer.getNumSamples());

    // Manejo en tiempo real: intercepta NoteOn entrante y genera acorde del grado activo
    // Avanza el índice de grado por barra o por longitud si no hay reloj
    juce::AudioPlayHead::PositionInfo pos;
    const bool hasHost = getHostInfo(pos);
    // BPM disponible en pos si el host lo proporciona (no usado aquí)
    const int noteLengthMs = loadIntParam(apvts, cc::ParamID::noteLengthMs);
    const bool followHost = loadBoolParam(apvts, cc::ParamID::followHost);

    const int blocksamples = buffer.getNumSamples();

    // Cachear grados para RT
    cachedDegrees = getDegreesFromParameters(apvts);
    if (cachedDegrees.empty())
        cachedDegrees = {1,5,6,4};

    const int keyIdx     = loadIntParam(apvts, cc::ParamID::key);
    const int scaleIdx   = loadIntParam(apvts, cc::ParamID::scale);
    const int qualityIdx = loadIntParam(apvts, cc::ParamID::chordQuality);
    const bool add7  = loadBoolParam(apvts, cc::ParamID::add7);
    const bool add9  = loadBoolParam(apvts, cc::ParamID::add9);
    const bool add11 = loadBoolParam(apvts, cc::ParamID::add11);
    const bool add13 = loadBoolParam(apvts, cc::ParamID::add13);
    const int inversion    = loadIntParam(apvts, cc::ParamID::inversion);
    const int velocity     = loadIntParam(apvts, cc::ParamID::velocity);
    const int humanizeMs   = loadIntParam(apvts, cc::ParamID::humanizeMs);
    const int humanizeVel  = loadIntParam(apvts, cc::ParamID::humanizeVel);
    const int octave       = loadIntParam(apvts, cc::ParamID::octave);

    const auto scale      = (cc::ScaleType) scaleIdx;
    const auto quality    = (cc::ChordQuality) qualityIdx;
    const int keySemitone = keyIdx;
    cc::ExtensionToggles togg { add7, add9, add11, add13 };

    juce::Random rng;

    // Determinar avance de grado cuando no hay reloj
    const int lenSamples = cc::msToSamples(getSampleRate(), noteLengthMs);
    static int samplesUntilAdvance = 0;
    samplesUntilAdvance -= blocksamples;
    if (samplesUntilAdvance <= 0)
    {
        if (!followHost || !hasHost)
        {
            currentDegreeIndex = (currentDegreeIndex + 1) % cachedDegrees.size();
            samplesUntilAdvance = lenSamples;
        }
        else
        {
            // Si followHost: avanzará por barra/negra según DAW (simplificado aquí)
            currentDegreeIndex = (currentDegreeIndex + 1) % cachedDegrees.size();
            samplesUntilAdvance = lenSamples; // aproximación a negra
        }
    }

    // Iterar mensajes entrantes, generar acordes y preservar otros
    juce::MidiBuffer output;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int samplePos = meta.samplePosition;
        if (msg.isNoteOn())
        {
            const int degree = cachedDegrees[currentDegreeIndex];
            const int root = cc::degreeToMidi(degree, keySemitone, scale, octave);
            auto notes = cc::makeChordNotes(root, scale, quality, inversion, togg);
            // Publicar notas actuales para la UI
            apvts.state.setProperty(cc::ParamID::lastChordNotes, cc::notesToString(notes), nullptr);
            for (int n : notes)
            {
                const int vel = cc::humanizeVelocity(velocity, humanizeVel, rng);
                const int hOn = cc::humanizeMsToSamples(getSampleRate(), humanizeMs, rng);
                const int hOff= cc::humanizeMsToSamples(getSampleRate(), humanizeMs, rng);

                output.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8) vel), juce::jmax(0, samplePos + hOn));
                output.addEvent(juce::MidiMessage::noteOff(1, n), juce::jmax(0, samplePos + hOn + lenSamples + hOff));
            }
        }
        else
        {
            // Preservar clock/CC/otros
            output.addEvent(msg, samplePos);
        }
    }

    midi.swapWith(output);
}

bool ChordCompanionAudioProcessor::getHostInfo(juce::AudioPlayHead::PositionInfo& outInfo) const
{
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            outInfo = *pos;
            return true;
        }
    }
    return false;
}

bool ChordCompanionAudioProcessor::exportProgressionToMidiFile(const juce::File& dest)
{
    // Algunas versiones de JUCE no exponen BPM en PositionInfo; usamos 120 por defecto.
    double bpm = 120.0;
    return engine.exportProgressionToMidiFile(apvts, dest, bpm);
}

void ChordCompanionAudioProcessor::triggerGenerateNow()
{
    engine.buildQueueFromParameters(apvts);
    engine.startPlayback();
}

void ChordCompanionAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChordCompanionAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* ChordCompanionAudioProcessor::createEditor()
{
    return new ChordCompanionAudioProcessorEditor(*this);
}

// Fábrica requerida por JUCE para crear la instancia del procesador
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChordCompanionAudioProcessor();
}