/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
                                   int x, int y, int width, int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    // enable and disable
    auto enabled = slider.isEnabled();

    // draw the circles
    g.setColour(enabled ? Colour(97u, 18u, 167u) : Colours::darkgrey);
    g.fillEllipse(bounds);

    // draw the boundary
    g.setColour(enabled ? Colour(255u, 154u, 1u) : Colours::grey);
    g.drawEllipse(bounds, 1.f);

    if ( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider) )
    {
        auto center = bounds.getCentre();

        // define a path
        Path p;

        // draw the knob
        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5); // "subtract 1.5*TextHeight"
        
        //p.addRectangle(r);
        p.addRoundedRectangle(r, 2.f);

        // insure the relationship correct
        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        // Text(Display the slider's current value)
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        // TextBox
        // r.setSize(g.getCurrentFont().getStringWidth(text), rswl->getTextHeight());
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
}


void LookAndFeel::drawToggleButton(juce::Graphics& g,
                                   juce::ToggleButton& toggleButton,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown)
{
    using namespace juce;

    // Bypass Button
    if ( auto* pb = dynamic_cast<PowerButton*>(&toggleButton) )
    {
        Path powerButton;

        // start to draw
        auto bounds = toggleButton.getLocalBounds();
        // Auxiliary line
        // inside the red line, the click is effective
        //g.setColour(Colours::red);
        //g.drawRect(bounds);

        // use JUCE_LIVE_CONSTANT() to choose the suitable size
        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        float ang = 30.f;

        size -= 6;

        // draw a arc
        powerButton.addCentredArc(r.getCentreX(),
            r.getCentreY(),
            size * 0.5,
            size * 0.5,
            0.f,
            degreesToRadians(ang),
            degreesToRadians(360.f - ang),
            true);

        // draw a vertical line
        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());

        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

        // choose color
        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);

        g.setColour(color);
        g.strokePath(powerButton, pst);

        // draw ellipse
        g.drawEllipse(r, 2);

        // A little bug:
        // click the whole region and the button is still response
        // we need to set the hit test region
        // but we don't do that
    }

    // Analyaer Button
    // it would be cool to see a randomized path drawn inside of this button
    // that reflects the fact that it is an analyzer
    else if ( auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton) )
    {
        // choose color
        auto color = !toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
        g.setColour(color);

        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);
        
        // draw a random path
        // we don't need it to change every time we put our mouse on it
        // so we put the code in the AnalyzerButton struct

        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
    }
}


void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderbounds();

    /************************ Auxiliary line ***************************/

    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);

    // They still overlap
    // let's change the function getSliderbounds()
    /*******************************************************************/

    // draw the rotary slider 
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);

    /*******************************************************************/
    // add the labels that will show the minimum and maximum values
    // that our parameters can have.
    // the minimum value would be drawn right outside of that seven o'clock position on the slider
    // and the maximum value to be drawn outside of the five o'clock position

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for ( int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        // Remaps a value from a source range to a target range.
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
    /*******************************************************************/
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderbounds() const
{
    //return getLocalBounds();
    // change the function getSliderbounds()

    // understand each step 
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    // min{width, height}

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    // square
    r.setCentre(bounds.getCentreX(), 0);
    // changes the position of the rectangle's centre (leaving its size unchanged).
    r.setY(2); 
    // pos.y = newY;

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    // return juce::String(getValue()); // Display the slider's current value
    // we want better getDisplayString() function!

    // choice: lowCutSlope & highCutSlope
    if ( auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
        return choiceParam->getCurrentChoiceName();
    //else
    //return juce::String(getValue());

    juce::String str;
    // if the value is more than 1000, use "k" to represent it. 
    bool addK = false;
    if ( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();

        if ( val > 999.f )
        {
            val /= 1000.f;
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
        // String(float floatValue, int numberOfDecimalPlaces, bool useScientificNotation = false);
        // Creates a string representing this floating-point number.
    }

    else
    {
        jassertfalse; // this shouldn't happen!
    }

    // if the suffix is not empty, then append it to the str;(if "k" exists, add it too;)
    if ( suffix.isNotEmpty() )
    {
        str << " ";
        if( addK ) str << "k";
        str << suffix;
    }

    return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : 
audioProcessor(p),
//leftChannelFifo(&audioProcessor.leftChannelFifo)
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    // Constructor
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    
    /* 48000 / 2048 = 23hz one bin */
    //leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
    //monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    // move to the PathProducer Constructor

    // when first open the GUI, the curve should work
    updateChain();

    // start timer
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    // Destructor
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}


// move the code from timerCallback() to process()
// and call process() in timerCallback()
// notice "left" represents the general situation
void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    // while there are buffers to pull
    // if we can pull a buffer
    // we are going to send it to the fft data generator
    // the first thing we need is a temp buffer to pull into
    juce::AudioBuffer<float> tempIncomingBuffer;

    while ( leftChannelFifo->getNumCompleteBuffersAvailable() )
    // there are more than zero buffers available to be pulled
    {
        // let try to pull one
        if ( leftChannelFifo->getAudioBuffer(tempIncomingBuffer) )
        {
            // if we can pull one of these
            // we are going to send it to the fft data generator
            // but we must make sure that they stay in the same order and the blocks being sent to fft data generator is the right size
            // so let's create a mono buffer first

            auto size = tempIncomingBuffer.getNumSamples();

            // FloatVectorOperations::copy(float* dest, const float* src, int num)
            //               |            original data                 |  | new data |
            //               ............................................  ............ 
            //               0                                          n  <-- size -->
            // after the first copy
            // |              original data                 |              | new data |
            // ............  ................................              ............
            // <-- size -->  0                              n - size       <-- size -->
            // after the second copy
            // |  no need |  |    part of original data     |  | new data |
            // ............  ................................  ............
            // <-- size -->  0                                            n
 
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);

            // start sending mono buffers to the generator
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);

        }
    }


    /*
    if there are FFT data buffers to pull
        if we can pull a buffer
            generate a path
    */
    //const auto fftBounds = getAnalysisArea().toFloat();
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();

    /* 
    48000 / 2048 = 23hz  <- this is the bin width
    */
    //const auto binWidth = audioProcessor.getSampleRate() / (double)fftSize;
    const auto binWidth = sampleRate / (double)fftSize;

    // if we have more than zero fft blocks available 
    while ( leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
    {
        std::vector<float> fftData;
        // let try to pull one
        if ( leftChannelFFTDataGenerator.getFFTData(fftData) ) 
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }


    /*
    while there are paths that can be pull 
        pull as many as we can
            display the most recent path
    */

    while ( pathProducer.getNumPathsAvailable() )
    {
        pathProducer.getPath(leftChannelFFTPath);
    }

}


// familiar timer !
void ResponseCurveComponent::timerCallback()
{
    /***************************************************************************/
    // call our process function
    if ( shouldshowFFTAnalysis )
    {
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();

        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

    /***************************************************************************/


    // if the parameter has changed
    // we need to set the flag back to false
    // and update the coefficients and repaint
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //DBG( "params changed" );
        // update the monochain
        updateChain();
        // signal a repaint
        // the repaint function has already implemented
        //repaint();
    }

    // we need to repaint all the time
    // comment the repaint function above
    repaint();
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // 
    //g.setColour (juce::Colours::white);
    //g.setFont (15.0f);
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);

    /****************************** my code here *******************************/

    using namespace juce;

    g.fillAll(Colours::black);

    /* draw grid background */
    g.drawImage(background, getLocalBounds().toFloat());

    /*auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);*/
    // auto responseArea = getLocalBounds(); // Returns the component's bounds, relative to its own origin.

    // upgrade!
    // the responseArea should be a lot smaller, so we can add some labels
    // auto responseArea = getRenderArea();
    // upgrade!
    // even smaller
    auto responseArea = getAnalysisArea();

    auto w = responseArea.getWidth(); // w : int 

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;   // magnitude

    mags.resize(w);

    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak>())
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if ( !monoChain.isBypassed<ChainPositions::LowCut>() )
        {
            if (!lowcut.isBypassed<0>())
                mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowcut.isBypassed<1>())
                mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowcut.isBypassed<2>())
                mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!lowcut.isBypassed<3>())
                mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if ( !monoChain.isBypassed<ChainPositions::HighCut>() )
        {
            if (!highcut.isBypassed<0>())
                mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highcut.isBypassed<1>())
                mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highcut.isBypassed<2>())
                mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (!highcut.isBypassed<3>())
                mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve; // Juce::Path

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    // draw FFTcurve before we draw our rendered area
    if ( shouldshowFFTAnalysis )
    {
        // left Channel
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colours::skyblue);
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
        // Right Channel
        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colours::lightyellow);
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }

    // draw
    g.setColour(Colours::orange);
    // g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    // the Rect of RenderArea should be orange
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    // draw
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));



    /***************************************************************************/
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

    Graphics g(background);

    // Horizontal axis value(frequency)
    Array<float> freqs
    {
        20, /*30, 40*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    // position located
    Array<float> xs;
    for ( auto f : freqs )
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }

    g.setColour(Colours::dimgrey);
    //for ( auto f : freqs )
    for ( auto x : xs )
    {
        // auto normX = mapFromLog10(f, 20.f, 20000.f);

        // g.drawVerticalLine(getWidth() * normX, 0.f, getHeight());
        g.drawVerticalLine(x, top, bottom);// draw the line
    }

    // Vertical axis value(gain)
    Array<float> gain
    {
        -24, -12, 0, 12, 24
    };

    for ( auto gDb : gain )
    {
        // auto y = jmap(gDb, -24.f, 24.f, float(getHeight()), 0.f);
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        // g.drawHorizontalLine(y, 0, getWidth());
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);//dimgrey
        g.drawHorizontalLine(y, left, right);// draw the line
    }

    // g.drawRect(getRenderArea());
    // g.drawRect(getAnalysisArea());

    // draw the labels
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    for ( int i = 0; i < freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if ( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK)
            str << "k";
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));

        String str;
        if ( gDb > 0 ) str << "+";
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);// the labels are on the right side
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);

        g.drawFittedText(str, r, juce::Justification::centred, 1);

        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();

    //bounds.reduce(10, // JUCE_LIVE_CONSTANT(5), 
    //              8   // JUCE_LIVE_CONSTANT(5)
    //              );

    // more customized
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

responseCurveComponent(audioProcessor),
peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),

lowCutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowCutBypassButton),
peakBypassButtonAttachment(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),
highCutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highCutBypassButton),
analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    // initialize the min/max labels outside the slider
    peakFreqSlider.labels.add({ 0.f, "20Hz" });
    peakFreqSlider.labels.add({ 1.f, "20kHz" });

    peakGainSlider.labels.add({ 0.f, "-24dB" });
    peakGainSlider.labels.add({ 1.f, "+24dB" });

    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });

    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });

    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });

    lowCutSlopeSlider.labels.add({ 0.f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });

    highCutSlopeSlider.labels.add({ 0.f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });

    // Constructor
    for ( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
    }

    // assign lnf to the three bands
    peakBypassButton.setLookAndFeel(&lnf);
    lowCutBypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);
    analyzerEnabledButton.setLookAndFeel(&lnf);

    // enable and disable
    // after bypass the slider can't move many more 
    auto safePtr = juce::Component::SafePointer<SimpleEQAudioProcessorEditor>(this);
    peakBypassButton.onClick = [safePtr]()
    {
        if ( auto* comp = safePtr.getComponent() )
        {
            auto bypassed = comp->peakBypassButton.getToggleState();
            comp->peakFreqSlider.setEnabled( !bypassed );
            comp->peakGainSlider.setEnabled( !bypassed );
            comp->peakQualitySlider.setEnabled( !bypassed );
        }
    };

    lowCutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->lowCutBypassButton.getToggleState();
            comp->lowCutFreqSlider.setEnabled(!bypassed);
            comp->lowCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    highCutBypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->highCutBypassButton.getToggleState();
            comp->highCutFreqSlider.setEnabled(!bypassed);
            comp->highCutSlopeSlider.setEnabled(!bypassed);
        }
    };

    analyzerEnabledButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto enabled = comp->analyzerEnabledButton.getToggleState();
            comp->responseCurveComponent.toggleAnalysisEnablement(enabled);
        }
    };

    // A little bug:
    // when you turn off the bypass button and close the SimpleEQ
    // and you reopen it, you will find that the sliders don't turn grey as we expected
    // and the sliders can still use as normal
    // maybe call this bug "initial problem"
    // we should record the state of the buttons
    // and coordinate these with the enablement


    // control the window size
    setSize (600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    peakBypassButton.setLookAndFeel(nullptr);
    lowCutBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    // 
    //g.setColour (juce::Colours::white);
    //g.setFont (15.0f);
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);

    /****************************** my code here *******************************/
    
    using namespace juce;

    g.fillAll(Colours::black);

    /***************************************************************************/
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    // my code here
    // adjust positions of subcomponents
    
    auto bounds = getLocalBounds();

    // set the analyzer enabled button
    auto analyzerEnabledArea = bounds.removeFromTop(25);
    analyzerEnabledArea.setWidth(100);
    analyzerEnabledArea.setX(5);
    analyzerEnabledArea.removeFromTop(2);

    analyzerEnabledButton.setBounds(analyzerEnabledArea);

    bounds.removeFromTop(5);

    float hRatio = 25.f / 100.f; //JUCE_LIVE_CONSTANT(33) / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);

    responseCurveComponent.setBounds(responseArea);
    // Changes the component's position and size.

    // a little narrow, create some empty space
    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

    // Bypass Button Position
    lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    highCutBypassButton.setBounds(highCutArea.removeFromTop(25));
    peakBypassButton.setBounds(bounds.removeFromTop(25));

    // Slider Position
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,

        &lowCutBypassButton, 
        &peakBypassButton, 
        &highCutBypassButton, 
        &analyzerEnabledButton
    };
}
