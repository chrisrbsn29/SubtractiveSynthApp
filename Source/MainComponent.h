/*
    File: MainComponent.h
    Date: December 26, 2018
    Author: Christopher Robinson
    Descrpition: This is a header file for the Subtractive Synthesiser Audio Application
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#define POLYPHONY 8
#define CHANNELS 2

//global qVal, so SynthVoice can access and Slider in MainComponent can update
static double qVal = 0.0;

//==============================================================================



//==============================================================================
struct SynthSound   : public SynthesiserSound
{
public:
    SynthSound();
    
    bool appliesToNote    (int) override;
    bool appliesToChannel (int) override;
};

//==============================================================================
struct SynthVoice   : public SynthesiserVoice,
                    private Timer

{
public:
    //SynthVoice();
    SynthVoice( int samplesPerBlockExpected );
    void timerCallback() override;
    bool canPlaySound (SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;
    void pitchWheelMoved (int) override;
    void controllerMoved (int, int) override;
    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override;
    void updateFilter();
    
    dsp::ProcessSpec spec;
    
private:
    double level = 0.0, tailOff = 0.0, attack = 0.0;
    double lastSample[2];
    bool isOn = false;
    Random random;
    double frequency;
    AudioSampleBuffer bufferBuffer;
    ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>> coefPtr;
    
    //juce::dsp::ProcessorDuplicator<dsp::StateVariableFilter::Filter<float>, dsp::StateVariableFilter::Parameters<float>> bpFilter;
    juce::dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> bpFilter;
    int samplesPerBlock;

};

//==============================================================================
class SynthAudioSource   : public AudioSource
{
public:
    SynthAudioSource (MidiKeyboardState& keyState);
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    MidiMessageCollector* getMidiCollector();
    
private:
    MidiKeyboardState& keyboardState;
    Synthesiser synth;
    MidiMessageCollector midiCollector;

};

//==============================================================================
class MainComponent   : public AudioAppComponent,
                        public Slider::Listener,
                        private Timer,
                        private ComboBox::Listener,
                        private MidiInputCallback,
                        private MidiKeyboardStateListener
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
    void setMidiInput (int index);
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void comboBoxChanged (ComboBox* box) override;
    void addMessageToBuffer (const MidiMessage& message);
    
    ComboBox midiInputList;
    AudioDeviceManager deviceManager;
    int lastInputIndex = 0;
    bool isAddingFromMidiInput = false;
    


private:
    void timerCallback() override;
    Slider volumeSlider;
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent keyboardComponent;
    SynthAudioSource synthAudioSource;
    float volume;
    Slider qValSlider;
    Label qValLabel;
    Label volumeLabel;
    Label midiInputListLabel;
    double prevSampleRate;
    MidiBuffer midiBuffer;
    double startTime;
    int previousSampleNumber;




    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
