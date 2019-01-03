/*
 File: MainComponent.cpp
 Date: December 26, 2018
 Author: Christopher Robinson
 Descrpition: This file contains most of the code for the Subtractive Synthesiser Audio Application. 
    The Subtractive Synthesiser Audio Application takes in white noise and band-passes corresponding
    to what midi notes are received. The GUI contains a qValue slider, which controls the purity of
    the tone, and a volume slider. Additionally, there is a MidiKeyboardComponent, from which 
    the application receives its midi information.
 Version: 1.0.0
 */

#include "MainComponent.h"

//==============================================================================
SynthSound::SynthSound(){}

bool SynthSound::appliesToNote( int )    { return true; }
bool SynthSound::appliesToChannel( int ) { return true; }


//==============================================================================
SynthVoice::SynthVoice(){}

bool SynthVoice::canPlaySound (SynthesiserSound* sound){
        return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity,
                            SynthesiserSound*, int /*currentPitchWheelPosition*/) {
    
    tailOff = 0.0;
    attack = 0.0;
    level = velocity * 0.5;
    isOn = true;
    
    auto frequency = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    bandPassFilter.setCoefficients( IIRCoefficients::makeBandPass (getSampleRate(), frequency, qVal));
    
}
void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff){
    
    if (allowTailOff)
    {
        if (tailOff == 0.0)
            tailOff = 1.0;
    }
    else
    {
        clearCurrentNote();
        isOn = false;
    }
}
void SynthVoice::pitchWheelMoved (int){}
void SynthVoice::controllerMoved (int, int){}
void SynthVoice::renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
    
    if( isOn )
    {
        int index = numSamples;
        if (tailOff > 0.0) // with tail off
        {
            while (--index >= 0)
            {
                if( attack < 1.0 ) attack += 0.01;
                auto currentSample = (float) random.nextFloat() - 0.5f * level * tailOff;
                
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;){
                    if ( index == 0 ) currentSample = lastSample[i];
                    if ( index ==  numSamples - 1 ) lastSample[i] = currentSample;
                    outputBuffer.addSample (i, startSample, currentSample);
                    //bandPassFilter.processSamples(outputBuffer.getWritePointer(i), numSamples);
                }
                ++startSample;
            
                tailOff *= 0.99;
            
                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    isOn = false;
                    break;
                }
            }
        }
        else // without tail off
        {
            while (--index >= 0)
            {
                if( attack < 1.0 ) attack += 0.01;
                auto currentSample = (float) random.nextFloat() - 0.5f * level;
            
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;){
                    if ( index == 0 ) currentSample = lastSample[i];
                    if ( index ==  numSamples - 1 ) lastSample[i] = currentSample;
                    outputBuffer.addSample (i, startSample, currentSample);
                    //bandPassFilter.processSamples(outputBuffer.getWritePointer(i), numSamples);
                }
                ++startSample;
            }
        }
    }
    //bandpass filter all of the samples
    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
        bandPassFilter.processSamples(outputBuffer.getWritePointer(i), numSamples);
}


//==============================================================================
SynthAudioSource::SynthAudioSource (MidiKeyboardState& keyState)
    : keyboardState (keyState)
    {
        for (auto i = 0; i < POLYPHONY; ++i)
            synth.addVoice (new SynthVoice());
        
        synth.addSound (new SynthSound());
    }
    
    void SynthAudioSource::prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate)
    {
        synth.setCurrentPlaybackSampleRate (sampleRate);
    }
    
    void SynthAudioSource::releaseResources(){}
    
    void SynthAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
    {
       
        MidiBuffer incomingMidi;
        keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
                                             bufferToFill.numSamples, true);
        
        synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
                               bufferToFill.startSample, bufferToFill.numSamples);
        
        
        
    }


//==============================================================================
MainComponent::MainComponent()  :

    keyboardComponent(keyboardState, MidiKeyboardComponent::horizontalKeyboard),
    synthAudioSource(keyboardState),
    volume(0.0)


{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);

    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange (0.0, 1.0);
    volumeSlider.setSkewFactorFromMidPoint (0.5);
    volumeSlider.addListener (this);

    addAndMakeVisible(qValSlider);
    qValSlider.setRange (0.0, 127.0);
    qValSlider.setSkewFactorFromMidPoint (64.0);
    qValSlider.addListener (this);
    
    addAndMakeVisible(keyboardComponent);

    startTimer (400);
    
    // specify the number of input and output channels that we want to open
    setAudioChannels (0, 2);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    synthAudioSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    
    synthAudioSource.getNextAudioBlock (bufferToFill); //get midi data
    
    for (auto channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        // Get a pointer to the start sample in the buffer for this audio output channel
        auto* buffer = bufferToFill.buffer->getWritePointer (channel, bufferToFill.startSample);
        // Fill the required number of samples with noise between -0.125 and +0.125
        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample){
            
            //multiply by qVal because it gets hella quiet otherwise
            
            buffer[sample] = buffer[sample] * volume * qVal;
        }
    }

}

void MainComponent::releaseResources()
{
    synthAudioSource.releaseResources ();
}
void MainComponent::sliderValueChanged(Slider *slider){
    if( slider == &qValSlider ){
        qVal = slider->getValue();
    }
    else if(slider == &volumeSlider){
        volume = slider->getValue();
    }
}

void MainComponent::timerCallback()
{
    keyboardComponent.grabKeyboardFocus();
    stopTimer();
}



//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    volumeSlider.setBounds (10, 10, getWidth() - 20, 20);
    qValSlider.setBounds (10, 50, getWidth() - 20, 20);
    keyboardComponent.setBounds (10, 90, getWidth() - 20, 150);

    
}

