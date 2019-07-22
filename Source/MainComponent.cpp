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
    : bandPassFilter(), bufferBuffer(CHANNELS, samplesPerBlockExpected)
    {
        spec.sampleRate = getSampleRate();
        spec.maximumBlockSize = samplesPerBlockExpected;
        spec.numChannels = CHANNELS;
        samplesPerBlock = samplesPerBlockExpected;
        dsp::AudioBlock<float> block;
        

    
    }


bool SynthVoice::canPlaySound (SynthesiserSound* sound){
        return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity,
                            SynthesiserSound*, int /*currentPitchWheelPosition*/) {
    
    stateVariableFilter.reset();
    stateVariableFilter.prepare(spec);
    stateVariableFilter.state->type = dsp::StateVariableFilter::Parameters<float>::Type::bandPass;
    
    tailOff = 0.0;
    attack = 0.0;
    level = velocity * 0.5;
    isOn = true;
    
    double frequency = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    //for( int i = 0; i < CHANNELS; i++)
        //bandPassFilters[i].setCoefficients(IIRCoefficients::makeBandPass (getSampleRate(), frequency, qVal));
    if (qVal <= 0) qVal = 0.0001;
    //bandPassFilter.setCoefficients(IIRCoefficients::makeBandPass (getSampleRate(), frequency, qVal));
    
    const double cyclesPerSample = frequency / getSampleRate(); // this is only for sine, delete after
    angleDelta = cyclesPerSample * 2.0 * double_Pi;

    
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
        stateVariableFilter.reset();
        //for( int i = 0; i < CHANNELS; i++)
            //bandPassFilters[i].makeInactive();
        //bandPassFilter.makeInactive();
    }
}
void SynthVoice::pitchWheelMoved (int){}
void SynthVoice::controllerMoved (int, int){}
void SynthVoice::updateFilter(){
    stateVariableFilter.state->setCutOffFrequency(getSampleRate(), qVal, static_cast<float> (1.0 / MathConstants<double>::sqrt2));
}
void SynthVoice::renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
    // New Version
    
    //block = AudioBlock(outputBuffer);
    
    if( isOn )
    {
        int index = numSamples;
        if (tailOff > 0.0) // with tail off
        {
            while (--index >= 0)
            {
                //TODO this is where you changed noise to sine (1/2)
                //auto currentSample = (float) random.nextFloat() * 0.5f - 0.25f * level * tailOff;
                auto currentSample = (float) std::sin (currentAngle) * level * tailOff;
                currentAngle += angleDelta;
                
                //for (auto i = bufferBuffer.getNumChannels(); --i >= 0;){ UNCOMMENT
                int32 check = outputBuffer.getNumChannels();
                int32 check2 = bufferBuffer.getNumChannels();
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;){

                    //bufferBuffer.addSample (i, startSample, currentSample); UNCOMMENT
                    outputBuffer.addSample(i, startSample, currentSample);//delete
                }
                ++startSample;
            
                tailOff *= 0.99;
            
                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    isOn = false;
                    bandPassFilter.makeInactive();
                    break;
                }
            }
        }
        else // without tail off (tail on)
        {
            while (--index >= 0)
            {
                //TODO only for sine, revert (2/2)
                //auto currentSample = (float) random.nextFloat() - 0.5f * level;
                auto currentSample = (float) std::sin (currentAngle) * level;
                currentAngle += angleDelta;
            
                //for (auto i = bufferBuffer.getNumChannels(); --i >= 0;){  UNCOMMENT

                    //bufferBuffer.addSample (i, startSample, currentSample); UNCOMMENT
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;){
                    outputBuffer.addSample(i, startSample, currentSample);//delete

                }
                ++startSample;
            }
        }
        //bandpass filter bufferBuffer
       // for (auto i = bufferBuffer.getNumChannels(); --i >= 0;) UNCOMMENT
            //bandPassFilters[i].processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
           // bandPassFilter.processSamples(bufferBuffer.getWritePointer(i), bufferBuffer.getNumSamples()); UNCOMMENT
        
        for (auto i = outputBuffer.getNumChannels(); --i >= 0;){ //DELETE WHOLE BLOCK
        bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
        bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
        bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
            bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
            bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
            bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
            bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
            bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
            bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
        }
        
        //add to og buffer
        /*while (--numSamples >= 0){ UNCOMMENT
            for (auto i = outputBuffer.getNumChannels(); --i >= 0;){
                outputBuffer.addSample (i, numSamples, bufferBuffer.getSample(i, numSamples));
            }
        }*/
        
    }// new version ends here
    

     /*
    // Old Version (only here in case I break everything)
    if( isOn )
    {
        int index = numSamples;
        if (tailOff > 0.0) // with tail off
        {
            while (--index >= 0)
            {
                //if( attack < 1.0 ) attack += 0.01;
                auto currentSample = (float) random.nextFloat() * 0.5f - 0.25f * level * tailOff;
                
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;){
                    //if ( index == 0 ) currentSample = lastSample[i];
                    //if ( index ==  numSamples - 1 ) lastSample[i] = currentSample;
                    outputBuffer.addSample (i, startSample, currentSample);
                }
                ++startSample;
                
                tailOff *= 0.99;
                
                if (tailOff <= 0.005)
                {
                    clearCurrentNote();
                    isOn = false;
                    bandPassFilter.makeInactive();
                    break;
                }
            }
        }
        else // without tail off
        {
            while (--index >= 0)
            {
                //if( attack < 1.0 ) attack += 0.01;
                auto currentSample = (float) random.nextFloat() - 0.5f * level;
                
                for (auto i = outputBuffer.getNumChannels(); --i >= 0;){
                    //if ( index == 0 ) currentSample = lastSample[i];
                    //if ( index ==  numSamples - 1 ) lastSample[i] = currentSample;
                    outputBuffer.addSample (i, startSample, currentSample);
                }
                ++startSample;
            }
        }
        //bandpass filter all of the samples
        for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
            //bandPassFilters[i].processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
            bandPassFilter.processSamples(outputBuffer.getWritePointer(i), outputBuffer.getNumSamples());
    } //old version ends here*/
    
}


//==============================================================================
SynthAudioSource::SynthAudioSource (MidiKeyboardState& keyState)
    : keyboardState (keyState)
    {
        /*for (auto i = 0; i < POLYPHONY; ++i)
            synth.addVoice (new SynthVoice( samplesPerBlockExpected ));
        
        synth.addSound (new SynthSound());*/
    }
    
    void SynthAudioSource::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
    {
        for (auto i = 0; i < POLYPHONY; ++i)
            synth.addVoice (new SynthVoice( samplesPerBlockExpected ));
        
        synth.addSound (new SynthSound());
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
    qValSlider.setRange (0.0, 1000.0);
    //qValSlider.setSkewFactorFromMidPoint (64.0);
    qValSlider.addListener (this);
    
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener (this);

    startTimer (400);
    
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
            
            //multiply by qVal because it gets hella quiet otherwise
            
            //buffer[sample] = buffer[sample] * volume * qVal; qval or no?
            buffer[sample] = buffer[sample] * volume;
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

void MainComponent::setMidiInput (int index)
{
    auto list = MidiInput::getDevices();
    deviceManager.removeMidiInputCallback (list[lastInputIndex], this);
    auto newInput = list[index];
    if (! deviceManager.isMidiInputEnabled (newInput))
        deviceManager.setMidiInputEnabled (newInput, true);
    deviceManager.addMidiInputCallback (newInput, this);
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
        //MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
        //m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        //addMessageToBuffer (m);
    }
}

void MainComponent::handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity)
{
    if (! isAddingFromMidiInput)
    {
        //MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, velocity));
        //m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        //addMessageToBuffer (m);
    }
}

void MainComponent::comboBoxChanged (ComboBox* box)
{
    if (box == &midiInputList)
        setMidiInput (midiInputList.getSelectedItemIndex());
}

void MainComponent::addMessageToBuffer (const MidiMessage& message)
{
    //auto timestamp = message.getTimeStamp();
    //auto sampleNumber =  (int) (timestamp * 44100.0);  //todo: fix sample rate magic num
    //midiBuffer.addEvent (message, sampleNumber);
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

