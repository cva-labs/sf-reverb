#include <juce_audio_utils/juce_audio_utils.h>
// Copyright (C) 2026 CVA Labs
// SPDX-License-Identifier: AGPL-3.0-or-later
#include <iostream>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    if(argc>1) {
        juce::VST3PluginFormat format;
        juce::OwnedArray<juce::PluginDescription> descriptions;
        format.findAllTypesForFile(descriptions,juce::String::fromUTF8(argv[1]));
        if(descriptions.isEmpty()) return 6;
        juce::String error;
        auto instance=format.createInstanceFromDescription(*descriptions[0],48000,512,error);
        if(!instance) { std::cerr<<error<<'\n'; return 7; }
        instance->prepareToPlay(48000,512);
        juce::AudioBuffer<float> audio(2,512); juce::MidiBuffer events;
        audio.clear(); audio.setSample(0,0,1); instance->processBlock(audio,events);
        for(int c=0;c<2;++c) for(int n=0;n<512;++n) if(!std::isfinite(audio.getSample(c,n))) return 8;
        instance->releaseResources();
        std::cout<<"PASS: built VST3 discovery, instantiation and processing\n";
    }
    std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());
    p->setPlayConfigDetails(2,2,48000,512); p->prepareToPlay(48000,512);
    auto params=p->getParameters();
    if(params.size()!=10) return 1;
    for(int i=0;i<params.size();++i) params[i]->setValueNotifyingHost(float(i+1)/11);
    juce::MemoryBlock saved; p->getStateInformation(saved);
    std::unique_ptr<juce::AudioProcessor> restored(createPluginFilter()); restored->setStateInformation(saved.getData(),int(saved.getSize()));
    for(int i=0;i<params.size();++i) if(std::abs(params[i]->getValue()-restored->getParameters()[i]->getValue())>0.001f) return 2;
    juce::AudioBuffer<float> buffer(2,512); juce::MidiBuffer midi;
    double energy=0;
    for(int b=0;b<500;++b) {
        buffer.clear(); if(b==20) { buffer.setSample(0,0,1); buffer.setSample(1,0,-1); }
        if(b%16==0) params[b%10]->setValueNotifyingHost(float(b%97)/97);
        p->processBlock(buffer,midi);
        for(int c=0;c<2;++c) for(int n=0;n<512;++n) { float v=buffer.getSample(c,n); if(!std::isfinite(v)||std::abs(v)>4) return 3; if(b>21) energy+=v*v; }
    }
    if(energy<0.0000001) return 4;
    // Render the real JUCE editor, with the same paint and layout used by VST3.
    std::unique_ptr<juce::AudioProcessor> visual(createPluginFilter());
    std::unique_ptr<juce::AudioProcessorEditor> editor(visual->createEditor());
    auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.0f);
    juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile("Superformula-Reverb-UI.png");
    output.deleteFile(); auto stream=output.createOutputStream();
    if(!stream || !juce::PNGImageFormat().writeImageToStream(image,*stream)) return 5;
    std::cout<<"PASS: parameter count, state recall, stereo anti-phase input, automation stability, editor render\n";
}
