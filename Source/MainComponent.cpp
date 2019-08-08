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
SynthVoice::SynthVoice( int samplesPerBlockExpected )
    :bpFilter(dsp::IIR::Coefficients<float>::makeBandPass (getSampleRate(), 20000.0f, 0.0001f))
    {
        spec.sampleRate = getSampleRate();
        spec.maximumBlockSize = samplesPerBlockExpected;
        spec.numChannels = CHANNELS;
        samplesPerBlock = samplesPerBlockExpected;
        dsp::AudioBlock<float> block;
        

    
    }

void SynthVoice::timerCallback()
{
    if( attack < 1.0f ) attack += 0.01f;

}


bool SynthVoice::canPlaySound (SynthesiserSound* sound){
        return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity,
                            SynthesiserSound*, int /*currentPitchWheelPosition*/) {
    
    bpFilter.reset();
    
    tailOff = 0.0;
    attack = 0.0;
    startTimer(1);
    //level = velocity * 0.5;
    level = 0.5;
    isOn = true;
    
    frequency = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    
    updateFilter();
    bpFilter.prepare(spec);
    
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
        stopTimer();
        bpFilter.reset();
    }
}
void SynthVoice::pitchWheelMoved (int){}
void SynthVoice::controllerMoved (int, int){}

void SynthVoice::updateFilter(){

    if( qVal <= 0) qVal = 0.0001;
    
    *bpFilter.state = *dsp::IIR::Coefficients<float>::makeBandPass (getSampleRate(), frequency, qVal);

}
void SynthVoice::renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
    bufferBuffer = AudioBuffer<float>( outputBuffer.getNumChannels(), outputBuffer.getNumSamples());
    bufferBuffer.clear();
    
    if( isOn )
    {
        int index = numSamples;
        if (tailOff > 0.0) // with tail off
        {
            while (--index >= 0)
            {
  
                auto currentSample = level * tailOff * attack * (-0.25f + (0.5f * (float) random.nextFloat()));
                
                for (auto i = bufferBuffer.getNumChannels(); --i >= 0;){

                    bufferBuffer.addSample(i, startSample, currentSample);
                }
                ++startSample;
            
                tailOff *= 0.994;
            
                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    isOn = false;
                    stopTimer();
                    bpFilter.reset();
                    break;
                }
            }
        }
        else // without tail off (tail on)
        {
            while (--index >= 0)
            {
                
                auto currentSample = level * attack * (-0.25f + (0.5f * (float) random.nextFloat()));
                
                for (auto i = bufferBuffer.getNumChannels(); --i >= 0;){
                    bufferBuffer.addSample(i, startSample, currentSample);

                }
                ++startSample;
            }
        }
        dsp::AudioBlock<float> block(bufferBuffer);
        updateFilter();
        bpFilter.process(dsp::ProcessContextReplacing<float> (block));
        index = numSamples;
        while (--index >= 0){
            for( auto i = outputBuffer.getNumChannels(); --i >= 0;){
        
                //bufferBuffer.setSample(i, index, bufferBuffer.getSample(i, index) * static_cast<float> (MathConstants<double>::sqrt2));
                bufferBuffer.setSample(i, index, bufferBuffer.getSample(i, index) * (1 + qVal));
                outputBuffer.addSample(i, index, bufferBuffer.getSample(i, index));
            }
        }
    }
}


//==============================================================================
SynthAudioSource::SynthAudioSource (MidiKeyboardState& keyState)
    : keyboardState (keyState){}
    
    void SynthAudioSource::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
    {
        for (auto i = 0; i < POLYPHONY; ++i)
            synth.addVoice (new SynthVoice( samplesPerBlockExpected ));
        
        synth.addSound (new SynthSound());
        synth.setCurrentPlaybackSampleRate (sampleRate);
        midiCollector.reset (sampleRate);
    }
    
    void SynthAudioSource::releaseResources(){}
    
    void SynthAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
    {
        bufferToFill.clearActiveBufferRegion();

            MidiBuffer incomingMidi;
            midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples);
            keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
                                             bufferToFill.numSamples, true);

            synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
                               bufferToFill.startSample, bufferToFill.numSamples);

        
    }

    MidiMessageCollector* SynthAudioSource::getMidiCollector()
    {
        return &midiCollector;
    }


//==============================================================================
MainComponent::MainComponent()  :

    keyboardComponent(keyboardState, MidiKeyboardComponent::horizontalKeyboard),
    synthAudioSource(keyboardState),
    volume(0.0),
    previousSampleNumber(0)


{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
    
    //add labels
    addAndMakeVisible (midiInputListLabel);
    midiInputListLabel.setText ("MIDI Input:", dontSendNotification);
    midiInputListLabel.attachToComponent (&midiInputList, true);
    addAndMakeVisible (midiInputList);
    
    
    addAndMakeVisible (qValLabel);
    qValLabel.setText ("q-Value:", dontSendNotification);
    qValLabel.attachToComponent (&qValSlider, true);
   
    addAndMakeVisible (volumeLabel);
    volumeLabel.setText ("Volume:", dontSendNotification);
    volumeLabel.attachToComponent (&volumeSlider, true);
    
    //add midi input combo box
    midiInputList.setTextWhenNoChoicesAvailable ("No MIDI Inputs Enabled");
    auto midiInputs = MidiInput::getDevices();
    midiInputList.addItemList (midiInputs, 1);
    midiInputList.onChange = [this] { setMidiInput (midiInputList.getSelectedItemIndex()); };
    // find the first enabled device and use that by default
    for (auto midiInput : midiInputs)
    {
        if (deviceManager.isMidiInputEnabled (midiInput))
        {
            setMidiInput (midiInputs.indexOf (midiInput));
            break;
        }
    }
    // if no enabled devices were found just use the first one in the list
    if (midiInputList.getSelectedId() == 0)
        setMidiInput (0);

    //add sliders and other components
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange (0.0, 1.0);
    volumeSlider.setSkewFactorFromMidPoint (0.5);
    volumeSlider.addListener (this);

    addAndMakeVisible(qValSlider);
    qValSlider.setRange (0.0001, 10.0);
    //qValSlider.setSkewFactorFromMidPoint (64.0);
    qValSlider.addListener (this);
    
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener (this);

    //startTimer (400);
    
    // specify the number of input and output channels that we want to open
    setAudioChannels (0, CHANNELS); 
    
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    startTimer (1);
    prevSampleRate = sampleRate;
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
        
        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample){
            
            //qVal or not?
            buffer[sample] = buffer[sample] * volume;
            //buffer[sample] = buffer[sample] * volume / (1.0f + pow(qValSlider.getValue(),1.2));
            if( buffer[sample] > 1.0f ) buffer[sample] = 1.0f;
            if( buffer[sample] < -1.0f ) buffer[sample] = -1.0f;

        }
    }

}

void MainComponent::releaseResources()
{
    synthAudioSource.releaseResources ();
}
void MainComponent::sliderValueChanged(Slider *slider){
    if( slider == &qValSlider ){
        qVal = pow(2, slider->getValue());
        //qVal = slider->getValue();
    }
    else if(slider == &volumeSlider){
        volume = slider->getValue();
    }
}

void MainComponent::timerCallback()
{
    
 
    auto currentTime = Time::getMillisecondCounterHiRes() * 0.001 - startTime;
    auto currentSampleNumber = (int) (currentTime * prevSampleRate);
    
    MidiBuffer::Iterator iterator (midiBuffer);
    MidiMessage message;
    int sampleNumber;
    
    while (iterator.getNextEvent (message, sampleNumber))
    {
        if (sampleNumber > currentSampleNumber)
            break;
        
       // message.setTimeStamp (sampleNumber / prevSampleRate);

    }
    

    midiBuffer.clear (previousSampleNumber, currentSampleNumber - previousSampleNumber); 
    previousSampleNumber = currentSampleNumber;
}

void MainComponent::setMidiInput (int index)
{
    auto list = MidiInput::getDevices();
    deviceManager.removeMidiInputCallback (list[lastInputIndex], this);
    
    auto newInput = list[index];
    
    if (! deviceManager.isMidiInputEnabled (newInput))
        deviceManager.setMidiInputEnabled (newInput, true);
    
    deviceManager.addMidiInputCallback (newInput, synthAudioSource.getMidiCollector());
    midiInputList.setSelectedId (index + 1, dontSendNotification);
    
    lastInputIndex = index;
}

void MainComponent::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    const ScopedValueSetter<bool> scopedInputFlag (isAddingFromMidiInput, true);
    keyboardState.processNextMidiEvent (message);
    //postMessageToList (message, source->getName());
}

void MainComponent::handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    if (! isAddingFromMidiInput)
    {
        MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, 1.0f));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        addMessageToBuffer (m);
    }
}

void MainComponent::handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    if (! isAddingFromMidiInput)
    {
        MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber, 1.0f));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        addMessageToBuffer (m);
    }
}

void MainComponent::comboBoxChanged (ComboBox* box)
{
    if (box == &midiInputList)
        setMidiInput (midiInputList.getSelectedItemIndex());
}

void MainComponent::addMessageToBuffer (const MidiMessage& message)
{
    auto timestamp = message.getTimeStamp();
    auto sampleNumber =  (int) (timestamp * prevSampleRate);
    midiBuffer.addEvent (message, sampleNumber);
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
    midiInputList.setBounds(100, 25, getWidth() - 150, 20);
    volumeSlider.setBounds (100, 70, getWidth() - 120, 20);
    qValSlider.setBounds (100, 100, getWidth() - 120, 20);
    keyboardComponent.setBounds (10, 140, getWidth() - 20, 120);

    
}

