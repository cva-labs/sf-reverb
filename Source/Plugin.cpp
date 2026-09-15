#include <juce_audio_utils/juce_audio_utils.h>
// Superformula Reverb - VST3 processor and user interface
// Copyright (C) 2026 CVA Labs
// SPDX-License-Identifier: AGPL-3.0-or-later
#include <BinaryData.h>
#include "Engine.h"

using namespace juce;
namespace Palette {
static const Colour blush   { 0xffb3b6b7 };
static const Colour cream   { 0xfff7f3e3 };
static const Colour rose    { 0xffaf9164 };
static const Colour terracotta { 0xff6f1a07 };
static const Colour berry   { 0xff1d3461 };

static void drawWoodGrain(Graphics& g, Rectangle<float> area) {
    g.saveState();
    g.reduceClipRegion(area.toNearestInt());
    for(int i=0;i<24;++i) {
        const float y=area.getY()+10.0f+i*31.0f;
        Path grain; grain.startNewSubPath(area.getX()-20.0f,y);
        for(float x=area.getX()-20.0f;x<=area.getRight()+20.0f;x+=18.0f)
            grain.lineTo(x,y+std::sin(x*0.030f+i*0.83f)*3.0f+std::sin(x*0.011f+i)*1.8f);
        g.setColour((i%3==0?terracotta:cream).withAlpha(i%3==0?0.12f:0.10f));
        g.strokePath(grain,PathStrokeType(i%3==0?1.4f:0.8f));
    }
    for(int i=0;i<5;++i) {
        const float cx=100.0f+i*224.0f, cy=75.0f+(i%3)*227.0f;
        g.setColour(terracotta.withAlpha(0.10f));
        g.drawEllipse(cx-35,cy-9,70,18,1.1f); g.drawEllipse(cx-23,cy-5,46,10,0.8f);
    }
    g.restoreState();
}

static void drawBrushedMetal(Graphics& g, Rectangle<float> area) {
    for(int y=int(area.getY())+10;y<int(area.getBottom())-8;y+=4) {
        const float inset=12.0f+std::sin(float(y)*0.17f)*3.0f;
        g.setColour((y%8==0?cream:berry).withAlpha(y%8==0?0.15f:0.045f));
        g.drawHorizontalLine(y,area.getX()+inset,area.getRight()-inset);
    }
}
}

class CandyLookAndFeel final : public LookAndFeel_V4 {
public:
    CandyLookAndFeel() {
        setColour(Slider::textBoxTextColourId, Palette::berry);
        setColour(Slider::textBoxBackgroundColourId, Palette::cream);
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour(Label::textColourId, Palette::berry);
        setColour(ComboBox::backgroundColourId, Palette::cream);
        setColour(ComboBox::textColourId, Palette::berry);
        setColour(ComboBox::arrowColourId, Palette::berry);
        setColour(ComboBox::outlineColourId, Palette::terracotta);
        setColour(PopupMenu::backgroundColourId, Palette::cream);
        setColour(PopupMenu::textColourId, Palette::berry);
        setColour(PopupMenu::highlightedBackgroundColourId, Palette::rose);
        setColour(PopupMenu::highlightedTextColourId, Palette::berry);
    }
    Font getComboBoxFont(ComboBox&) override { return Font(FontOptions(15, Font::bold)); }
    void drawRotarySlider(Graphics& g,int x,int y,int w,int h,float pos,float start,float end,Slider&) override {
        auto side=float(jmin(w,h))-22.0f;
        auto knob=Rectangle<float>(float(x)+(w-side)*0.5f,float(y)+5.0f,side,side);

        // Layered translucent rims and reflections form the glass body.
        g.setColour(Palette::berry.withAlpha(0.80f)); g.fillEllipse(knob.expanded(5.0f));
        g.setColour(Palette::cream.withAlpha(0.72f)); g.fillEllipse(knob.expanded(2.5f));

        ColourGradient body(Palette::cream.withAlpha(0.82f),knob.getX()+knob.getWidth()*0.25f,knob.getY()+knob.getHeight()*0.18f,
                            Palette::berry.withAlpha(0.22f),knob.getRight(),knob.getBottom(),true);
        body.addColour(0.48,Palette::blush.withAlpha(0.43f));
        body.addColour(0.76,Palette::rose.withAlpha(0.30f));
        g.setGradientFill(body); g.fillEllipse(knob);

        g.setColour(Palette::cream.withAlpha(0.64f));
        g.fillEllipse(Rectangle<float>(knob.getWidth()*0.48f,knob.getHeight()*0.19f)
                          .withCentre({knob.getCentreX()-knob.getWidth()*0.12f,knob.getCentreY()-knob.getHeight()*0.25f}));
        g.setColour(Palette::cream.withAlpha(0.32f));
        g.drawEllipse(knob.reduced(knob.getWidth()*0.13f),3.0f);
        g.setColour(Palette::berry.withAlpha(0.34f));
        Path refraction;
        refraction.addCentredArc(knob.getCentreX(),knob.getCentreY()+2.0f,knob.getWidth()*0.39f,knob.getHeight()*0.34f,0.0f,1.95f,4.33f,true);
        g.strokePath(refraction,PathStrokeType(3.0f,PathStrokeType::curved,PathStrokeType::rounded));
        g.setColour(Palette::berry.withAlpha(0.72f)); g.drawEllipse(knob.reduced(1.0f),1.6f);

        float a=start+pos*(end-start),radius=knob.getWidth()*0.32f;
        Point<float> dot(knob.getCentreX()+std::sin(a)*radius,knob.getCentreY()-std::cos(a)*radius);
        g.setColour(Palette::berry); g.fillEllipse(Rectangle<float>(15,15).withCentre(dot));
        g.setColour(Palette::cream); g.fillEllipse(Rectangle<float>(7,7).withCentre(dot.translated(-1.5f,-1.5f)));
    }
    void drawLinearSlider(Graphics& g,int x,int y,int w,int h,float pos,float,float,Slider::SliderStyle,Slider&) override {
        auto track=Rectangle<float>(float(x)+8,float(y)+h*0.28f,float(w)-16,12).withCentre({float(x)+w*0.5f,float(y)+h*0.35f});
        g.setColour(Palette::blush); g.fillRoundedRectangle(track,6);
        auto active=track.withWidth(jmax(12.0f,pos-track.getX()));
        g.setColour(Palette::terracotta); g.fillRoundedRectangle(active,6);
        g.setColour(Palette::berry); g.fillEllipse(Rectangle<float>(20,20).withCentre({pos,track.getCentreY()}));
        g.setColour(Palette::cream); g.fillEllipse(Rectangle<float>(8,8).withCentre({pos,track.getCentreY()}));
    }
};
class SuperProcessor : public AudioProcessor {
public:
    AudioProcessorValueTreeState state;
    sf::Engine engine;
    static AudioProcessorValueTreeState::ParameterLayout layout() {
        AudioProcessorValueTreeState::ParameterLayout p;
        auto add=[&](const char* id,const char* name,float lo,float hi,float value,float interval=0.01f) {
            p.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},name,NormalisableRange<float>(lo,hi,interval),value));
        };
        add("m","Longitude lobes",0,12,6,2); add("vertical","Latitude lobes",0,12,4,2);
        add("n1","Contour N1",0.3f,6,1.2f); add("n2","Curvature N2",0.3f,6,3.8f); add("n3","Curvature N3",0.3f,6,3.8f);
        add("size","Room scale (m)",2,40,12,0.1f); add("decay","Decay multiplier",0.25f,3,1);
        add("mix","Wet mix",0,1,0.3f); add("pre","Pre-delay (ms)",0,200,15,1);
        StringArray names; for(auto& m:sf::materials) names.add(m.name);
        p.add(std::make_unique<AudioParameterChoice>(ParameterID{"material",1},"Bayin material",names,0));
        return p;
    }
    SuperProcessor():AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),state(*this,nullptr,"SuperformulaState",layout()) {}
    float value(const char* id) const { return state.getRawParameterValue(id)->load(); }
    sf::Shape shape() const { return {value("m"),value("n1"),value("n2"),value("n3"),value("vertical"),value("size")}; }
    const String getName() const override { return "Superformula Reverb"; }
    bool acceptsMidi() const override { return false; } bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 90; }
    int getNumPrograms() override { return 1; } int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {} const String getProgramName(int) override { return {}; } void changeProgramName(int,const String&) override {}
    void prepareToPlay(double sr,int) override { engine.prepare(float(sr)); }
    void releaseResources() override {}
    void reset() override { engine.prepare(float(getSampleRate()>0?getSampleRate():48000)); }
    bool isBusesLayoutSupported(const BusesLayout& b) const override {
        return b.getMainInputChannelSet()==b.getMainOutputChannelSet() && (b.getMainOutputChannelSet()==AudioChannelSet::stereo() || b.getMainOutputChannelSet()==AudioChannelSet::mono());
    }
    void processBlock(AudioBuffer<float>& b,MidiBuffer&) override {
        ScopedNoDenormals noDenormals;
        engine.configure(shape(),int(value("material")),value("decay"),value("mix"),value("pre"));
        auto* left=b.getWritePointer(0); auto* right=b.getNumChannels()>1?b.getWritePointer(1):nullptr;
        for(int i=0;i<b.getNumSamples();++i) { float l=left[i],r=right?right[i]:l; engine.process(l,r); left[i]=right?l:(l+r)*0.5f; if(right) right[i]=r; }
    }
    bool hasEditor() const override { return true; } AudioProcessorEditor* createEditor() override;
    void getStateInformation(MemoryBlock& block) override { auto xml=state.copyState().createXml(); copyXmlToBinary(*xml,block); }
    void setStateInformation(const void* data,int bytes) override { if(auto xml=getXmlFromBinary(data,bytes)) if(xml->hasTagName(state.state.getType())) state.replaceState(ValueTree::fromXml(*xml)); }
};

class RoomView : public Component {
    SuperProcessor& processor;
    float yaw=0.5f,pitch=0.35f;
    Point<float> last;
public:
    RoomView(SuperProcessor& p):processor(p) { setMouseCursor(MouseCursor::DraggingHandCursor); }
    void mouseDown(const MouseEvent& e) override { last=e.position; }
    void mouseDrag(const MouseEvent& e) override { auto d=e.position-last; yaw+=d.x*0.008f; pitch=std::clamp(pitch+d.y*0.008f,-1.3f,1.3f); last=e.position; repaint(); }
    void paint(Graphics& g) override {
        auto bounds=getLocalBounds().toFloat();
        g.setColour(Palette::cream); g.fillRoundedRectangle(bounds,24);
        auto accent=Colour(sf::materials[size_t(int(processor.value("material")))].colour);
        g.setColour(Palette::blush);
        for(int i=0;i<10;++i) { float y=bounds.getHeight()*0.52f+i*20; g.drawLine(20,y,bounds.getWidth()-20,y); }
        for(int i=-8;i<=8;++i) g.drawLine(bounds.getCentreX()+i*24.0f,bounds.getHeight()*0.52f,bounds.getCentreX()+i*60.0f,bounds.getHeight()-20);
        auto s=processor.shape();
        float maxRadius=0;
        for(int j=0;j<=32;++j) for(int i=0;i<=64;++i) { auto v=sf::point(-sf::pi+i*sf::pi/32,-sf::pi/2+j*sf::pi/32,s); maxRadius=std::max(maxRadius,std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z)); }
        float scale=std::min(bounds.getWidth(),bounds.getHeight())*0.37f/maxRadius;
        auto project=[&](sf::Vec v) {
            float x=v.x*std::cos(yaw)-v.y*std::sin(yaw), y=v.x*std::sin(yaw)+v.y*std::cos(yaw);
            float z=v.z*std::cos(pitch)-y*std::sin(pitch);
            return Point<float>(bounds.getCentreX()+x*scale,bounds.getCentreY()-z*scale+8);
        };
        for(int j=1;j<32;++j) {
            Path path;
            for(int i=0;i<=96;++i) { auto p=project(sf::point(-sf::pi+i*2*sf::pi/96,-sf::pi/2+j*sf::pi/32,s)); if(i==0) path.startNewSubPath(p); else path.lineTo(p); }
            g.setColour(accent.withAlpha(j%4==0?0.9f:0.38f)); g.strokePath(path,PathStrokeType(j%4==0?1.8f:0.9f));
        }
        for(int i=0;i<48;++i) {
            Path path;
            for(int j=0;j<=64;++j) { auto p=project(sf::point(-sf::pi+i*2*sf::pi/48,-sf::pi/2+j*sf::pi/64,s)); if(j==0) path.startNewSubPath(p); else path.lineTo(p); }
            g.setColour(accent.withAlpha(i%4==0?0.72f:0.3f)); g.strokePath(path,PathStrokeType(1.0f));
        }
        g.setFont(FontOptions(12,Font::bold)); g.setColour(Palette::berry); g.drawText("SUPERFORMULA ROOM",22,17,300,22,Justification::left);
        g.setColour(Palette::terracotta); g.drawText("DRAG TO ORBIT",22,getHeight()-36,200,20,Justification::left);
        g.drawText(String(s.size,1)+" m scale",getWidth()-160,getHeight()-36,138,20,Justification::right);
    }
};

class SuperEditor : public AudioProcessorEditor, private Timer {
    SuperProcessor& processor;
    CandyLookAndFeel look;
    RoomView room;
    Image logo=ImageCache::getFromMemory(BinaryData::cvalabslogo_png,BinaryData::cvalabslogo_pngSize);
    std::array<Slider,9> sliders;
    std::array<Label,9> labels;
    std::array<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>,9> attachments;
    ComboBox material;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> materialAttachment;
    Label materialLabel;
    TextButton licenseButton { "AGPLv3" };
    void timerCallback() override { room.repaint(); }
public:
    SuperEditor(SuperProcessor& p):AudioProcessorEditor(p),processor(p),room(p) {
        setLookAndFeel(&look); addAndMakeVisible(room);
        const char* ids[]={"m","vertical","n1","n2","n3","size","decay","pre","mix"};
        const char* names[]={"LONGITUDE / M","LATITUDE / M","CONTOUR / N1","CURVE / N2","CURVE / N3","SCALE / METRES","DECAY / MULTIPLIER","PRE-DELAY / MS","DRY / WET"};
        for(size_t i=0;i<9;++i) {
            auto& s=sliders[i]; s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(Slider::TextBoxBelow,false,95,23); addAndMakeVisible(s);
            labels[i].setText(names[i],dontSendNotification); labels[i].setFont(FontOptions(10.5f,Font::bold)); labels[i].setJustificationType(Justification::centred); labels[i].setColour(Label::textColourId,Palette::berry); addAndMakeVisible(labels[i]);
            attachments[i]=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s);
        }
        sliders[8].textFromValueFunction=[](double v){return String(roundToInt(v*100))+" %";};
        sliders[8].valueFromTextFunction=[](const String& s){return s.getDoubleValue()/100.0;};
        sliders[8].updateText();
        for(size_t i=6;i<9;++i) sliders[i].setSliderStyle(Slider::LinearHorizontal);
        for(size_t i=0;i<sf::materials.size();++i) material.addItem(sf::materials[i].name,int(i)+1);
        addAndMakeVisible(material);
        materialLabel.setText("BAYIN / RESONATING MATERIAL",dontSendNotification); materialLabel.setFont(FontOptions(11,Font::bold)); materialLabel.setColour(Label::textColourId,Palette::berry); addAndMakeVisible(materialLabel);
        materialAttachment=std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"material",material);
        licenseButton.setColour(TextButton::buttonColourId,Palette::cream);
        licenseButton.setColour(TextButton::buttonOnColourId,Palette::rose);
        licenseButton.setColour(TextButton::textColourOffId,Palette::berry);
        licenseButton.setColour(TextButton::textColourOnId,Palette::berry);
        licenseButton.onClick=[] {
            AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,"Superformula Reverb — License",
                "Copyright © 2026 CVA Labs\n\nLicensed under GNU AGPLv3 or later. This software comes with no warranty. You may redistribute and modify it under the license terms.\n\nLicense and source code:\nhttps://github.com/cva-labs/sf-reverb");
        };
        addAndMakeVisible(licenseButton);
        setSize(1040,720); startTimerHz(24);
    }
    ~SuperEditor() override { stopTimer(); setLookAndFeel(nullptr); }
    void paint(Graphics& g) override {
        g.fillAll(Palette::rose);
        Palette::drawWoodGrain(g,getLocalBounds().toFloat());
        g.setColour(Palette::blush); g.fillRoundedRectangle(18,14,1004,90,28);
        Palette::drawBrushedMetal(g,{18,14,1004,90});
        if(logo.isValid()) { g.setColour(Palette::berry); g.drawImageWithin(logo,34,25,198,60,RectanglePlacement::centred,true); }
        g.setColour(Palette::berry); g.setFont(FontOptions(32,Font::bold)); g.drawText("Superformula Reverb v1.0",262,31,710,42,Justification::left);
        g.setColour(Palette::blush); g.fillRoundedRectangle(644,118,378,446,28); g.fillRoundedRectangle(18,570,1004,118,28);
        Palette::drawBrushedMetal(g,{644,118,378,446});
        Palette::drawBrushedMetal(g,{18,570,1004,118});
    }
    void resized() override {
        room.setBounds(18,118,612,446);
        for(int i=0;i<6;++i) { int x=659+(i%3)*118,y=139+(i/3)*144; labels[size_t(i)].setBounds(x,y,115,20); sliders[size_t(i)].setBounds(x,y+22,115,113); }
        materialLabel.setBounds(672,443,315,22); material.setBounds(672,475,322,44);
        licenseButton.setBounds(918,42,76,25);
        for(int i=6;i<9;++i) { int x=64+(i-6)*330; labels[size_t(i)].setBounds(x,588,240,18); sliders[size_t(i)].setBounds(x,610,240,68); }
    }
};
AudioProcessorEditor* SuperProcessor::createEditor() { return new SuperEditor(*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SuperProcessor; }
