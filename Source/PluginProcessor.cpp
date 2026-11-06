#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

const juce::StringArray NKLTuneProcessor::keyNames
    { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

juce::AudioProcessorValueTreeState::ParameterLayout NKLTuneProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", keyNames, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale",
                                                         StringArray { "Major", "Minor", "Chromatic" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
                                                         StringArray { "SUAVE", "UNDER" }, 1));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "speed", 1 }, "Retune Speed",
                                                        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "amount", 1 }, "Pitch Correction",
                                                        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "humanize", 1 }, "Humanize",
                                                        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "formant", 1 }, "Formant",
                                                        NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "vibrato", 1 }, "Vibrato",
                                                        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack",
                                                        NormalisableRange<float> (1.0f, 200.0f, 1.0f, 0.5f), 5.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release",
                                                        NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.5f), 40.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                        NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                                                        NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));

    return { p.begin(), p.end() };
}

void NKLTuneProcessor::setParam (const char* id, float plainValue)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
}

void NKLTuneProcessor::applyPreset (int mode)
{
    setParam ("mode", (float) mode);

    if (mode == 0)   // SUAVE: natural, retune speed 20
    {
        setParam ("speed", 20.0f);   setParam ("amount", 100.0f);
        setParam ("humanize", 30.0f); setParam ("vibrato", 0.0f);
        setParam ("attack", 10.0f);  setParam ("release", 100.0f);
        setParam ("mix", 100.0f);    setParam ("formant", 0.0f);
    }
    else             // UNDER: robotico total, retune speed al maximo
    {
        setParam ("speed", 100.0f);  setParam ("amount", 100.0f);
        setParam ("humanize", 0.0f); setParam ("vibrato", 0.0f);
        setParam ("attack", 5.0f);   setParam ("release", 40.0f);
        setParam ("mix", 100.0f);    setParam ("formant", 0.0f);
    }
}

NKLTuneProcessor::NKLTuneProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool NKLTuneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void NKLTuneProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    inRing.assign (kRingSize, 0.0f);
    analysisBuf.assign (kRingSize, 0.0f);
    yinDiff.assign (4096, 0.0f);
    for (auto& d : delayBuf) d.assign (kDelaySize, 0.0f);

    ringPos = 0;
    writePos = 0;
    phase = 0.0f;
    hopSize = juce::jmax (128, (int) (sr / 180.0));
    hopCounter = 0;
    histCount = 0;
    targetSemis = 0.0f;
    curSemis = 0.0f;
    voicedNow = false;
    corrGain = 0.0f;
    vibPhase = 0.0f;

    grainLen = (float) (0.030 * sr);
    grainLen = juce::jmin (grainLen, (float) (kDelaySize / 2 - 8));
    setLatencySamples ((int) (grainLen * 0.5f));
}

int NKLTuneProcessor::snapToScale (float midiNote, int key, int scale)
{
    static const bool major[12] = { true,false,true,false,true,true,false,true,false,true,false,true };
    static const bool minor[12] = { true,false,true,true,false,true,false,true,true,false,true,false };

    const int centre = (int) std::lround (midiNote);
    int best = centre;
    float bestDist = 1.0e9f;

    for (int n = centre - 3; n <= centre + 3; ++n)
    {
        const int pc = (((n - key) % 12) + 12) % 12;
        const bool ok = (scale == 0) ? major[pc] : (scale == 1) ? minor[pc] : true;
        if (! ok) continue;
        const float dist = std::abs ((float) n - midiNote);
        if (dist < bestDist) { bestDist = dist; best = n; }
    }
    return best;
}

// Deteccion de pitch YIN sobre las ultimas muestras del ring buffer
void NKLTuneProcessor::analyse (int key, int scale, float amount, float humanize)
{
    const int W = kWindow;
    int tauMax = (int) (sr / 70.0);
    tauMax = juce::jmin (tauMax, 4000, kRingSize - W - 2);
    const int tauMin = juce::jmax (2, (int) (sr / 1000.0));
    const int total = W + tauMax;

    const int start = ringPos - total;
    for (int i = 0; i < total; ++i)
        analysisBuf[(size_t) i] = inRing[(size_t) ((start + i) & kRingMask)];

    // Gate de silencio
    float energy = 0.0f;
    for (int i = tauMax; i < total; ++i)
        energy += analysisBuf[(size_t) i] * analysisBuf[(size_t) i];
    const float rms = std::sqrt (energy / (float) W);

    bool voiced = false;
    float freq = 0.0f;

    if (rms > 0.004f)
    {
        // Funcion de diferencia
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float sum = 0.0f;
            const float* a = analysisBuf.data();
            const float* b = analysisBuf.data() + tau;
            for (int j = 0; j < W; ++j)
            {
                const float d = a[j] - b[j];
                sum += d * d;
            }
            yinDiff[(size_t) tau] = sum;
        }

        // Diferencia normalizada (CMND)
        yinDiff[0] = 1.0f;
        float running = 0.0f;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            running += yinDiff[(size_t) tau];
            yinDiff[(size_t) tau] = (running > 0.0f) ? yinDiff[(size_t) tau] * (float) tau / running : 1.0f;
        }

        const float threshold = 0.15f;
        int tauEst = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (yinDiff[(size_t) tau] < threshold)
            {
                while (tau + 1 < tauMax && yinDiff[(size_t) tau + 1] < yinDiff[(size_t) tau])
                    ++tau;
                tauEst = tau;
                break;
            }
        }

        if (tauEst > 1 && tauEst < tauMax)
        {
            const float s0 = yinDiff[(size_t) tauEst - 1];
            const float s1 = yinDiff[(size_t) tauEst];
            const float s2 = yinDiff[(size_t) tauEst + 1];
            const float denom = (s0 + s2 - 2.0f * s1);
            float shift = 0.0f;
            if (std::abs (denom) > 1.0e-9f)
                shift = 0.5f * (s0 - s2) / denom;
            const float tauF = (float) tauEst + juce::jlimit (-1.0f, 1.0f, shift);
            freq = (float) sr / tauF;
            voiced = (freq > 60.0f && freq < 1100.0f);
        }
    }

    if (voiced)
    {
        const float midi = 69.0f + 12.0f * std::log2 (freq / 440.0f);

        // Mediana de 3 para evitar saltos de octava sueltos
        midiHist[(size_t) (histCount % 3)] = midi;
        ++histCount;
        float m = midi;
        if (histCount >= 3)
        {
            auto h = midiHist;
            std::sort (h.begin(), h.end());
            m = h[1];
        }

        const int snapped = snapToScale (m, key, scale);
        float raw = juce::jlimit (-12.0f, 12.0f, (float) snapped - m);

        // Humanize: zona muerta que deja pasar la variacion natural de la voz
        const float deadband = humanize * 0.0075f;           // hasta 0.75 semitonos
        const float mag = juce::jmax (0.0f, std::abs (raw) - deadband);
        raw = (raw < 0.0f ? -mag : mag);

        targetSemis = raw * amount * 0.01f;
        voicedNow = true;
    }
    else
    {
        histCount = 0;
        voicedNow = false;       // mantiene el ultimo target y se desvanece con Release
    }
}

void NKLTuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    auto val = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const int   key      = (int) val ("key");
    const int   scale    = (int) val ("scale");
    const float speed    = val ("speed");
    const float amount   = val ("amount");
    const float humanize = val ("humanize");
    const float vibrato  = val ("vibrato") * 0.01f;
    const float attackMs = val ("attack");
    const float releaseMs= val ("release");
    const float mix      = val ("mix") * 0.01f;
    const float outGain  = juce::Decibels::decibelsToGain (val ("output"));

    const float tauSec = (speed >= 100.0f)
                           ? 0.0f
                           : kSlowestTime * (1.0f - speed / 100.0f) * (1.0f - speed / 100.0f);
    const float coef = (tauSec <= 0.0f)
                           ? 1.0f
                           : 1.0f - std::exp (-1.0f / (tauSec * (float) sr));

    const float coefA = 1.0f - std::exp (-1.0f / (juce::jmax (0.5f, attackMs)  * 0.001f * (float) sr));
    const float coefR = 1.0f - std::exp (-1.0f / (juce::jmax (0.5f, releaseMs) * 0.001f * (float) sr));
    const float vibInc = juce::MathConstants<float>::twoPi * 5.5f / (float) sr;
    const int   dryDelay = (int) (grainLen * 0.5f) + 2;

    const float twoPi = juce::MathConstants<float>::twoPi;
    float blockPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float x = buffer.getReadPointer (ch)[i];
            delayBuf[(size_t) ch][(size_t) writePos] = x;
            mono += x;
        }
        mono /= (float) juce::jmax (1, numCh);

        inRing[(size_t) ringPos] = mono;
        ringPos = (ringPos + 1) & kRingMask;

        if (++hopCounter >= hopSize)
        {
            hopCounter = 0;
            analyse (key, scale, amount, humanize);
        }

        // Retune speed (suavizado de la correccion)
        curSemis += coef * (targetSemis - curSemis);

        // Attack / Release de la correccion
        const float gTarget = voicedNow ? 1.0f : 0.0f;
        corrGain += (gTarget - corrGain) * (gTarget > corrGain ? coefA : coefR);

        // Vibrato artificial
        vibPhase += vibInc;
        if (vibPhase > twoPi) vibPhase -= twoPi;
        const float vib = std::sin (vibPhase) * vibrato * 0.5f;

        const float effSemis = (curSemis + vib) * corrGain;
        const float ratio = std::exp2 (effSemis / 12.0f);

        phase += (1.0f - ratio) / grainLen;
        while (phase >= 1.0f) phase -= 1.0f;
        while (phase < 0.0f)  phase += 1.0f;

        float phase2 = phase + 0.5f;
        if (phase2 >= 1.0f) phase2 -= 1.0f;

        const float w1 = 0.5f - 0.5f * std::cos (twoPi * phase);
        const float w2 = 0.5f - 0.5f * std::cos (twoPi * phase2);
        const float d1 = 2.0f + phase  * grainLen;
        const float d2 = 2.0f + phase2 * grainLen;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const auto& buf = delayBuf[(size_t) ch];

            auto readTap = [&] (float delay)
            {
                float rp = (float) writePos - delay;
                if (rp < 0.0f) rp += (float) kDelaySize;
                const int i0 = (int) rp;
                const float frac = rp - (float) i0;
                const int a = i0 & kDelayMask;
                const int b = (i0 + 1) & kDelayMask;
                return buf[(size_t) a] * (1.0f - frac) + buf[(size_t) b] * frac;
            };

            const float wet = readTap (d1) * w1 + readTap (d2) * w2;
            const float dry = buf[(size_t) ((writePos - dryDelay) & kDelayMask)];
            const float y = (dry * (1.0f - mix) + wet * mix) * outGain;

            buffer.getWritePointer (ch)[i] = y;
            blockPeak = juce::jmax (blockPeak, std::abs (y));
        }

        writePos = (writePos + 1) & kDelayMask;
    }

    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (blockPeak > outPeak.load()) outPeak.store (blockPeak);
}

juce::AudioProcessorEditor* NKLTuneProcessor::createEditor()
{
    return new NKLTuneEditor (*this);
}

void NKLTuneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NKLTuneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NKLTuneProcessor();
}
