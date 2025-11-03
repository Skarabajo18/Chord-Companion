// ProgressionEngine.cpp

#include "ProgressionEngine.h"

namespace cc
{
    static std::vector<int> presetDegrees(ProgressionPreset preset)
    {
        switch (preset)
        {
            case ProgressionPreset::I_V_vi_IV: return {1,5,6,4};
            case ProgressionPreset::ii_V_I:    return {2,5,1};
            case ProgressionPreset::I_vi_IV_V: return {1,6,4,5};
            case ProgressionPreset::vi_IV_I_V: return {6,4,1,5};
            case ProgressionPreset::Custom:    return {}; // se ignora aquí
            default:                           return {1,5,6,4};
        }
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

    void ProgressionEngine::buildQueueFromParameters(const juce::AudioProcessorValueTreeState& apvts)
    {
        queue.clear();
        nextIndex = 0;
        currentSampleCursor = 0;

        const int keyIdx     = loadIntParam(apvts, ParamID::key);
        const int scaleIdx   = loadIntParam(apvts, ParamID::scale);
        const int presetIdx  = loadIntParam(apvts, ParamID::progressionPreset);
        const juce::String customStr = apvts.state.getProperty(ParamID::progressionCustom, juce::String("1-5-6-4")).toString();
        const int qualityIdx = loadIntParam(apvts, ParamID::chordQuality);

        const bool add7  = loadBoolParam(apvts, ParamID::add7);
        const bool add9  = loadBoolParam(apvts, ParamID::add9);
        const bool add11 = loadBoolParam(apvts, ParamID::add11);
        const bool add13 = loadBoolParam(apvts, ParamID::add13);
        const int inversion    = loadIntParam(apvts, ParamID::inversion);
        const int velocity     = loadIntParam(apvts, ParamID::velocity);
        const int noteLengthMs = loadIntParam(apvts, ParamID::noteLengthMs);
        const int humanizeMs   = loadIntParam(apvts, ParamID::humanizeMs);
        const int humanizeVel  = loadIntParam(apvts, ParamID::humanizeVel);
        const int octave       = loadIntParam(apvts, ParamID::octave);

        std::vector<int> degrees;
        if ((ProgressionPreset) presetIdx == ProgressionPreset::Custom)
            degrees = cc::parseProgressionString(customStr);
        else
            degrees = presetDegrees((ProgressionPreset) presetIdx);

        if (degrees.empty())
            degrees = {1, 5, 6, 4};

        const auto scale      = (ScaleType) scaleIdx;
        const auto quality    = (ChordQuality) qualityIdx;
        const int keySemitone = keyIdx; // 0..11

        const int chordLenSamples = cc::msToSamples(sampleRate, noteLengthMs);

        int timeCursor = 0;
        for (int deg : degrees)
        {
            const int root = degreeToMidi(deg, keySemitone, scale, octave);
            const ExtensionToggles togg { add7, add9, add11, add13 };
            auto notes = makeChordNotes(root, scale, quality, inversion, togg);

            for (int n : notes)
            {
                const int vel = cc::humanizeVelocity(velocity, humanizeVel, rng);
                const int hOn = cc::humanizeMsToSamples(sampleRate, humanizeMs, rng);
                const int hOff = cc::humanizeMsToSamples(sampleRate, humanizeMs, rng);

                // Usa canal 2 para eventos generados por el motor,
                // para diferenciarlos de NoteOn entrantes del host
                juce::MidiMessage on = juce::MidiMessage::noteOn(2, n, (juce::uint8) vel);
                juce::MidiMessage off = juce::MidiMessage::noteOff(2, n);

                const int onPos  = std::max(0, timeCursor + hOn);
                const int offPos = std::max(0, timeCursor + chordLenSamples + hOff);
                queue.push_back(TimedEvent { onPos, on });
                queue.push_back(TimedEvent { offPos, off });
            }

            timeCursor += chordLenSamples;
        }

        // Ordenar por sampleOffset
        std::sort(queue.begin(), queue.end(), [](const TimedEvent& a, const TimedEvent& b){ return a.sampleOffset < b.sampleOffset; });
    }

    void ProgressionEngine::injectQueuedEvents(juce::MidiBuffer& midiOut, int numSamples)
    {
        if (!playing || queue.empty())
        {
            currentSampleCursor += numSamples;
            return;
        }

        const int blockStart = currentSampleCursor;
        const int blockEnd   = currentSampleCursor + numSamples;

        while (nextIndex < queue.size())
        {
            const auto& ev = queue[nextIndex];
            if (ev.sampleOffset >= blockStart && ev.sampleOffset < blockEnd)
            {
                const int posInBlock = ev.sampleOffset - blockStart;
                midiOut.addEvent(ev.msg, posInBlock);
                ++nextIndex;
            }
            else if (ev.sampleOffset >= blockEnd)
            {
                break; // pendiente para próximos bloques
            }
            else
            {
                // Evento anterior al bloque actual: si es el primer bloque, clampa a 0
                if (blockStart == 0 && ev.sampleOffset < 0)
                {
                    midiOut.addEvent(ev.msg, 0);
                }
                ++nextIndex;
            }
        }

        currentSampleCursor += numSamples;
        if (nextIndex >= queue.size())
            playing = false; // cola agotada
    }

    bool ProgressionEngine::exportProgressionToMidiFile(const juce::AudioProcessorValueTreeState& apvts,
                                                        const juce::File& dest,
                                                        double bpmIfKnown) const
    {
        const int keyIdx     = loadIntParam(apvts, ParamID::key);
        const int scaleIdx   = loadIntParam(apvts, ParamID::scale);
        const int presetIdx  = loadIntParam(apvts, ParamID::progressionPreset);
        const juce::String customStr = apvts.state.getProperty(ParamID::progressionCustom, juce::String("1-5-6-4")).toString();
        const int qualityIdx = loadIntParam(apvts, ParamID::chordQuality);

        const bool add7  = loadBoolParam(apvts, ParamID::add7);
        const bool add9  = loadBoolParam(apvts, ParamID::add9);
        const bool add11 = loadBoolParam(apvts, ParamID::add11);
        const bool add13 = loadBoolParam(apvts, ParamID::add13);
        const int inversion    = loadIntParam(apvts, ParamID::inversion);
        const int velocity     = loadIntParam(apvts, ParamID::velocity);
        const int noteLengthMs = loadIntParam(apvts, ParamID::noteLengthMs);
        const int octave       = loadIntParam(apvts, ParamID::octave);

        std::vector<int> degrees;
        if ((ProgressionPreset) presetIdx == ProgressionPreset::Custom)
            degrees = cc::parseProgressionString(customStr);
        else
            degrees = presetDegrees((ProgressionPreset) presetIdx);

        if (degrees.empty())
            degrees = {1, 5, 6, 4};

        const auto scale      = (ScaleType) scaleIdx;
        const auto quality    = (ChordQuality) qualityIdx;
        const int keySemitone = keyIdx; // 0..11

        const double qnMs = 60000.0 / juce::jmax(1.0, bpmIfKnown);
        const double chordLenQN = static_cast<double>(noteLengthMs) / qnMs; // cuántas negras dura
        const int ticksPerQN = 960;
        const int chordLenTicks = (int) std::round(chordLenQN * ticksPerQN);

        juce::MidiMessageSequence seq;
        int tickCursor = 0;
        for (int deg : degrees)
        {
            const int root = degreeToMidi(deg, keySemitone, scale, octave);
            const ExtensionToggles togg { add7, add9, add11, add13 };
            auto notes = makeChordNotes(root, scale, quality, inversion, togg);

            for (int n : notes)
            {
                juce::MidiMessage on = juce::MidiMessage::noteOn(1, n, (juce::uint8) velocity);
                juce::MidiMessage off = juce::MidiMessage::noteOff(1, n);
                on.setTimeStamp(tickCursor);
                off.setTimeStamp(tickCursor + chordLenTicks);
                seq.addEvent(on);
                seq.addEvent(off);
            }

            tickCursor += chordLenTicks;
        }

        juce::MidiFile mf;
        mf.setTicksPerQuarterNote(ticksPerQN);
        mf.addTrack(seq);

        juce::FileOutputStream fos(dest);
        if (!fos.openedOk())
            return false;
        mf.writeTo(fos);
        return true;
    }
}