#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
    const juce::Colour kGreen      { 0xff2bff3c };
    const juce::Colour kGreenText  { 0xff4dff5e };
    const float kStartAngle = juce::degreesToRadians (-125.0f) + juce::MathConstants<float>::twoPi;
    const float kEndAngle   = juce::degreesToRadians ( 125.0f) + juce::MathConstants<float>::twoPi;

    // Posiciones en el diseno original (1536 x 1024)
    constexpr float kBigCx = 768.0f,  kBigCy = 487.0f,  kBigSize = 410.0f;
    constexpr float kTopY = 497.0f,   kTopSize = 160.0f;
    constexpr float kPitchX = 1107.0f, kHumanX = 1356.0f;
    constexpr float kBotY = 795.0f,   kBotSize = 140.0f;
    constexpr float kBotX[5] = { 296.0f, 523.0f, 751.0f, 978.0f, 1210.0f };
}

// ============================================================ Look and feel
NKLLookAndFeel::NKLLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff06120a));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffd8ffd8));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff12701f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffeaffea));
}

juce::Font NKLLookAndFeel::getComboBoxFont (juce::ComboBox& b)
{
    return juce::Font (juce::FontOptions ((float) b.getHeight() * 0.44f, juce::Font::bold));
}

void NKLLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds ((int) (box.getWidth() * 0.07f), 0, (int) (box.getWidth() * 0.70f), box.getHeight());
    label.setFont (getComboBoxFont (box));
}

juce::Font NKLLookAndFeel::getTextButtonFont (juce::TextButton&, int h)
{
    return juce::Font (juce::FontOptions ((float) h * 0.42f, juce::Font::bold));
}

void NKLLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                           bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();
    const float cr = r.getHeight() * 0.3f;

    g.setColour (juce::Colour (on ? 0xff0e4a18 : 0xff050c07).withAlpha (on ? 0.95f : 0.85f));
    g.fillRoundedRectangle (r, cr);

    if (on)   // brillo
    {
        g.setColour (kGreen.withAlpha (0.18f));
        g.drawRoundedRectangle (r.expanded (2.0f), cr + 2.0f, 4.0f);
    }
    g.setColour (kGreen.withAlpha (on ? 1.0f : (highlighted || down ? 0.85f : 0.5f)));
    g.drawRoundedRectangle (r, cr, 1.6f);
}

void NKLLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                                       float startAngle, float endAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = bounds.getCentre();
    const float R = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const juce::Point<float> dir (std::sin (angle), -std::cos (angle));

    auto arcPath = [&] (float radius, float from, float to)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, radius, radius, 0.0f, from, to, true);
        return p;
    };

    // ---------------- Perilla grande (Retune Speed): se dibuja sobre el arte del fondo
    if ((bool) slider.getProperties().getWithDefault ("big", false))
    {
        const float s = R / 205.0f;

        if (sliderPos < 0.995f)
        {
            // tapa la parte "apagada" del arco pintado
            g.setColour (juce::Colour (0xf4050c07));
            g.strokePath (arcPath (155.0f * s, angle, endAngle),
                          juce::PathStrokeType (58.0f * s, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

            g.setColour (juce::Colour (0xff1c5a26).withAlpha (0.75f));
            g.strokePath (arcPath (126.0f * s, angle, endAngle), juce::PathStrokeType (1.6f * s));
            g.strokePath (arcPath (184.0f * s, angle, endAngle), juce::PathStrokeType (1.6f * s));

            g.setColour (juce::Colour (0xff12401c));
            g.strokePath (arcPath (153.0f * s, angle, endAngle),
                          juce::PathStrokeType (8.0f * s, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }

        // marca en el borde del arco
        g.setColour (kGreen.withAlpha (0.25f));
        g.drawLine (c.x + dir.x * 126.0f * s, c.y + dir.y * 126.0f * s,
                    c.x + dir.x * 184.0f * s, c.y + dir.y * 184.0f * s, 9.0f * s);
        g.setColour (juce::Colour (0xffd9ffd9));
        g.drawLine (c.x + dir.x * 126.0f * s, c.y + dir.y * 126.0f * s,
                    c.x + dir.x * 184.0f * s, c.y + dir.y * 184.0f * s, 3.5f * s);

        // puntero sobre la cara
        g.setColour (kGreenText);
        g.drawLine (c.x + dir.x * 92.0f * s, c.y + dir.y * 92.0f * s,
                    c.x + dir.x * 114.0f * s, c.y + dir.y * 114.0f * s, 5.0f * s);
        return;
    }

    // ---------------- Perillas chicas (dibujadas completas)
    const bool bipolar = slider.getMinimum() < 0.0;
    const float rArc = R * 0.74f;
    const float thick = R * 0.13f;

    // cara
    {
        juce::ColourGradient grad (juce::Colour (0xff2a2e2c), c.x - R * 0.2f, c.y - R * 0.25f,
                                   juce::Colour (0xff080a09), c.x + R * 0.5f, c.y + R * 0.6f, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - R * 0.60f, c.y - R * 0.60f, R * 1.20f, R * 1.20f);
        g.setColour (juce::Colour (0xff4a504c));
        g.drawEllipse (c.x - R * 0.62f, c.y - R * 0.62f, R * 1.24f, R * 1.24f, 1.4f);
    }

    // marcas
    g.setColour (kGreen.withAlpha (0.55f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const juce::Point<float> d (std::sin (a), -std::cos (a));
        g.drawLine (c.x + d.x * R * 0.90f, c.y + d.y * R * 0.90f,
                    c.x + d.x * R * 0.99f, c.y + d.y * R * 0.99f, 1.3f);
    }

    // pista apagada
    g.setColour (juce::Colour (0xff343a35));
    g.strokePath (arcPath (rArc, startAngle, endAngle),
                  juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // arco activo con brillo
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.01f)
    {
        const float a0 = juce::jmin (from, angle), a1 = juce::jmax (from, angle);
        const auto p = arcPath (rArc, a0, a1);
        g.setColour (kGreen.withAlpha (0.10f));
        g.strokePath (p, juce::PathStrokeType (thick * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (kGreen.withAlpha (0.18f));
        g.strokePath (p, juce::PathStrokeType (thick * 1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (kGreen);
        g.strokePath (p, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // puntero
    g.setColour (juce::Colour (0xffc8ffcc));
    g.drawLine (c.x + dir.x * R * 0.18f, c.y + dir.y * R * 0.18f,
                c.x + dir.x * R * 0.50f, c.y + dir.y * R * 0.50f, juce::jmax (2.0f, R * 0.045f));
}

// ============================================================ Editor
NKLTuneEditor::NKLTuneEditor (NKLTuneProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    bg = juce::ImageCache::getFromMemory (BinaryData::background_png, BinaryData::background_pngSize);

    // Key / Scale
    keyBox.addItemList (NKLTuneProcessor::keyNames, 1);
    scaleBox.addItemList (juce::StringArray { "Major", "Minor", "Chromatic" }, 1);
    for (auto* b : { &keyBox, &scaleBox })
    {
        b->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*b);
    }
    keyAtt   = std::make_unique<ComboAtt> (proc.apvts, "key",   keyBox);
    scaleAtt = std::make_unique<ComboAtt> (proc.apvts, "scale", scaleBox);

    // Knobs
    setupKnob (speedKnob,    "speed", true);
    setupKnob (amountKnob,   "amount");
    setupKnob (humanizeKnob, "humanize");
    setupKnob (formantKnob,  "formant");
    setupKnob (vibratoKnob,  "vibrato");
    setupKnob (attackKnob,   "attack");
    setupKnob (releaseKnob,  "release");
    setupKnob (mixKnob,      "mix");

    // Output (arrastrar sobre el texto "0.0 dB")
    outputSlider.setSliderStyle (juce::Slider::LinearVertical);
    outputSlider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible (outputSlider);
    sliderAtts.push_back (std::make_unique<SliderAtt> (proc.apvts, "output", outputSlider));

    // Botones de modo
    for (auto* b : { &suaveBtn, &underBtn })
    {
        b->setColour (juce::TextButton::textColourOnId,  juce::Colour (0xffeaffea));
        b->setColour (juce::TextButton::textColourOffId, juce::Colour (0xff86e08f));
        addAndMakeVisible (*b);
    }
    suaveBtn.onClick = [this] { proc.applyPreset (0); };
    underBtn.onClick = [this] { proc.applyPreset (1); };

    if (auto* modeParam = proc.apvts.getParameter ("mode"))
    {
        modeAtt = std::make_unique<juce::ParameterAttachment> (*modeParam, [this] (float v)
        {
            const int idx = (int) std::lround (v);
            suaveBtn.setToggleState (idx == 0, juce::dontSendNotification);
            underBtn.setToggleState (idx == 1, juce::dontSendNotification);
            repaint();
        });
        modeAtt->sendInitialUpdate();
    }

    setResizable (true, true);
    setResizeLimits (768, 512, 1536, 1024);
    getConstrainer()->setFixedAspectRatio (1536.0 / 1024.0);
    setSize (1152, 768);

    startTimerHz (30);
}

NKLTuneEditor::~NKLTuneEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void NKLTuneEditor::setupKnob (juce::Slider& s, const char* paramId, bool big)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    s.setRotaryParameters (kStartAngle, kEndAngle, true);
    if (big) s.getProperties().set ("big", true);
    addAndMakeVisible (s);
    sliderAtts.push_back (std::make_unique<SliderAtt> (proc.apvts, paramId, s));
}

juce::Rectangle<int> NKLTuneEditor::nativeRect (float x, float y, float w, float h) const
{
    const float s = scaleFactor();
    return { juce::roundToInt (x * s), juce::roundToInt (y * s), juce::roundToInt (w * s), juce::roundToInt (h * s) };
}

void NKLTuneEditor::resized()
{
    if (bg.isValid())
        bgScaled = bg.rescaled (juce::jmax (1, getWidth()), juce::jmax (1, getHeight()),
                                juce::Graphics::highResamplingQuality);

    keyBox.setBounds   (nativeRect (358.0f, 430.0f, 182.0f, 65.0f));
    scaleBox.setBounds (nativeRect (358.0f, 563.0f, 182.0f, 64.0f));

    speedKnob.setBounds    (nativeRect (kBigCx - kBigSize * 0.5f, kBigCy - kBigSize * 0.5f, kBigSize, kBigSize));
    amountKnob.setBounds   (nativeRect (kPitchX - kTopSize * 0.5f, kTopY - kTopSize * 0.5f, kTopSize, kTopSize));
    humanizeKnob.setBounds (nativeRect (kHumanX - kTopSize * 0.5f, kTopY - kTopSize * 0.5f, kTopSize, kTopSize));

    juce::Slider* bottom[5] = { &formantKnob, &vibratoKnob, &attackKnob, &releaseKnob, &mixKnob };
    for (int i = 0; i < 5; ++i)
        bottom[i]->setBounds (nativeRect (kBotX[i] - kBotSize * 0.5f, kBotY - kBotSize * 0.5f, kBotSize, kBotSize));

    outputSlider.setBounds (nativeRect (1412.0f, 876.0f, 100.0f, 46.0f));

    suaveBtn.setBounds (nativeRect (400.0f, 936.0f, 150.0f, 44.0f));
    underBtn.setBounds (nativeRect (986.0f, 936.0f, 150.0f, 44.0f));
}

void NKLTuneEditor::drawValue (juce::Graphics& g, const juce::String& text, float cx, float cy,
                               float fontH, juce::Colour colour)
{
    const float s = scaleFactor();
    g.setFont (juce::Font (juce::FontOptions (fontH * s, juce::Font::bold)));
    const int w = juce::roundToInt (220.0f * s), h = juce::roundToInt (fontH * 1.4f * s);
    const juce::Rectangle<int> r (juce::roundToInt (cx * s) - w / 2, juce::roundToInt (cy * s) - h / 2, w, h);

    g.setColour (colour.withAlpha (0.22f));
    g.drawText (text, r.translated (0, 0).expanded (1), juce::Justification::centred);
    g.setColour (colour);
    g.drawText (text, r, juce::Justification::centred);
}

void NKLTuneEditor::paint (juce::Graphics& g)
{
    if (bgScaled.isValid()) g.drawImageAt (bgScaled, 0, 0);
    else                    g.fillAll (juce::Colour (0xff050a06));

    const float s = scaleFactor();
    auto v = [this] (juce::Slider& sl) { return (float) sl.getValue(); };

    // Valores
    drawValue (g, juce::String (juce::roundToInt (v (speedKnob))), kBigCx, kBigCy, 84.0f, kGreenText);

    drawValue (g, juce::String (juce::roundToInt (v (amountKnob)))   + "%", kPitchX, 620.0f, 36.0f, kGreenText);
    drawValue (g, juce::String (juce::roundToInt (v (humanizeKnob))) + "%", kHumanX, 620.0f, 36.0f, kGreenText);

    drawValue (g, juce::String (v (formantKnob), 2),                        kBotX[0], 881.0f, 34.0f, kGreenText);
    drawValue (g, juce::String (juce::roundToInt (v (vibratoKnob))) + "%",  kBotX[1], 881.0f, 34.0f, kGreenText);
    drawValue (g, juce::String (juce::roundToInt (v (attackKnob)))  + " ms", kBotX[2], 881.0f, 34.0f, kGreenText);
    drawValue (g, juce::String (juce::roundToInt (v (releaseKnob))) + " ms", kBotX[3], 881.0f, 34.0f, kGreenText);
    drawValue (g, juce::String (juce::roundToInt (v (mixKnob)))     + "%",  kBotX[4], 881.0f, 34.0f, kGreenText);

    drawValue (g, juce::String (v (outputSlider), 1) + " dB", 1459.0f, 900.0f, 30.0f, kGreenText);

    // Medidor de salida
    const int numSeg = 20;
    const float mx = 1475.0f, mw = 28.0f, my = 585.0f, mh = 249.0f;
    const float segH = mh / (float) numSeg;
    for (int i = 0; i < numSeg; ++i)
    {
        const float y = my + mh - (float) (i + 1) * segH;
        const juce::Rectangle<float> r (mx * s, (y + 1.5f) * s, mw * s, (segH - 3.0f) * s);
        const bool lit = meterLevel > ((float) i + 0.5f) / (float) numSeg;
        if (lit)
        {
            const float t = (float) i / (float) numSeg;
            g.setColour (juce::Colour (0xff1bd92a).interpolatedWith (juce::Colour (0xff8dff3a), t));
            g.fillRoundedRectangle (r, 1.5f * s);
        }
        else
        {
            g.setColour (juce::Colour (0xff0c2410));
            g.fillRoundedRectangle (r, 1.5f * s);
        }
    }
}

void NKLTuneEditor::timerCallback()
{
    // medidor
    const float peak = proc.outPeak.exchange (0.0f);
    float norm = 0.0f;
    if (peak > 1.0e-5f)
        norm = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (peak) + 60.0f) / 60.0f);
    meterLevel = juce::jmax (norm, meterLevel * 0.86f);
    repaint (nativeRect (1465.0f, 580.0f, 45.0f, 260.0f));

    // si cambio algun valor (por preset o por el host), repinta los numeros
    std::vector<float> now { (float) speedKnob.getValue(), (float) amountKnob.getValue(), (float) humanizeKnob.getValue(),
                             (float) formantKnob.getValue(), (float) vibratoKnob.getValue(), (float) attackKnob.getValue(),
                             (float) releaseKnob.getValue(), (float) mixKnob.getValue(), (float) outputSlider.getValue() };
    if (now != lastValues)
    {
        lastValues = std::move (now);
        repaint();
    }
}
