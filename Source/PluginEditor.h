#pragma once
#include "PluginProcessor.h"

// Look and feel que dibuja las perillas sobre el fondo del diseno NKL TUNE
class NKLLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NKLLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override;

    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override {}

    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override {}
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

class NKLTuneEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit NKLTuneEditor (NKLTuneProcessor&);
    ~NKLTuneEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    float scaleFactor() const { return (float) getWidth() / 1536.0f; }
    juce::Rectangle<int> nativeRect (float x, float y, float w, float h) const;
    void drawValue (juce::Graphics&, const juce::String&, float cx, float cy, float fontH, juce::Colour);
    void setupKnob (juce::Slider&, const char* paramId, bool big = false);

    NKLTuneProcessor& proc;
    NKLLookAndFeel laf;

    juce::Image bg, bgScaled;

    juce::ComboBox keyBox, scaleBox;
    juce::Slider speedKnob, amountKnob, humanizeKnob;
    juce::Slider formantKnob, vibratoKnob, attackKnob, releaseKnob, mixKnob;
    juce::Slider outputSlider;
    juce::TextButton suaveBtn { "SUAVE" }, underBtn { "UNDER" };

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SliderAtt>> sliderAtts;
    std::unique_ptr<ComboAtt> keyAtt, scaleAtt;
    std::unique_ptr<juce::ParameterAttachment> modeAtt;

    float meterLevel = 0.0f;
    std::vector<float> lastValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NKLTuneEditor)
};
