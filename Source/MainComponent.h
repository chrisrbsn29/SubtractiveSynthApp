/*
    File: MainComponent.h
    Date: December 26, 2018
    Author: Christopher Robinson
    Descrpition: This is a header file for the Subtractive Synthesiser Audio Application
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#define POLYPHONY 16

//global qVal, so SynthVoice can access and Slider in MainComponent can update
static double qVal = 0.0;


//==============================================================================
struct SynthSound   : public SynthesiserSound
{
public:
    SynthSound();
    
    bool appliesToNote    (int) override;
    bool appliesToChannel (int) override;
};

//==============================================================================
struct SynthVoice   : public SynthesiserVoice

{
public:
    SynthVoice();
    
    bool canPlaySound (SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;
    void pitchWheelMoved (int) override;
    void controllerMoved (int, int) override;
    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override;
    
private:
    double level = 0.0, tailOff = 0.0, attack = 0.0;
    double lastSample[2];
    bool isOn = false;
    Random random;
    IIRFilter bandPassFilter;

};

//==============================================================================
class SynthAudioSource   : public AudioSource
{
public:
    SynthAudioSource (MidiKeyboardState& keyState);
    
    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    
private:
    MidiKeyboardState& keyboardState;
    Synthesiser synth;

};

//==============================================================================
class MainComponent   : public AudioAppComponent,
                        public Slider::Listener,
                        private Timer
{
public:
    
    MainComponent();
    ~MainComponent();

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void sliderValueChanged (Slider* slider) override;
    void paint (Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    Slider volumeSlider;
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent keyboardComponent;
    SynthAudioSource synthAudioSource;
    float volume;
    Slider qValSlider;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
