#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <atomic>

class NKLTuneProcessor : public juce::AudioProcessor
{
public:
    NKLTuneProcessor();
    ~NKLTuneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NKL TUNE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    static const juce::StringArray keyNames;

    // Modo 0 = SUAVE, 1 = UNDER (ajusta todos los knobs)
    void applyPreset (int mode);

    // Nivel de salida para el medidor de la interfaz
    std::atomic<float> outPeak { 0.0f };

private:
    // Tiempo de correccion (seg) cuando retune speed = 0. Con speed 20 da ~190 ms.
    static constexpr float kSlowestTime = 0.30f;

    void analyse (int key, int scale, float amount, float humanize);
    void setParam (const char* id, float plainValue);
    static int snapToScale (float midiNote, int key, int scale);

    double sr = 44100.0;

    // Deteccion de pitch (YIN)
    static constexpr int kRingSize = 8192;
    static constexpr int kRingMask = kRingSize - 1;
    static constexpr int kWindow   = 1024;
    std::vector<float> inRing;
    std::vector<float> analysisBuf;
    std::vector<float> yinDiff;
    int ringPos = 0;
    int hopSize = 256;
    int hopCounter = 0;
    std::array<float, 3> midiHist { 0.f, 0.f, 0.f };
    int histCount = 0;

    // Pitch shifter (dos taps con delay modulado)
    static constexpr int kDelaySize = 16384;
    static constexpr int kDelayMask = kDelaySize - 1;
    std::array<std::vector<float>, 2> delayBuf;
    int writePos = 0;
    float phase = 0.0f;
    float grainLen = 1440.0f;

    float targetSemis = 0.0f;
    float curSemis = 0.0f;
    bool  voicedNow = false;
    float corrGain = 0.0f;
    float vibPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NKLTuneProcessor)
};
