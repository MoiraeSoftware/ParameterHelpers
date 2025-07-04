#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace moiraesoftware {

    inline juce::Rectangle<int>
        extractTileByNumber (const juce::Image& mosaic, int tileWidth, int tileHeight, int n) {
        int numColumns = mosaic.getWidth() / tileWidth;

        // Calculate row and column for the image n (0-based index n)
        int row    = n / numColumns;
        int column = n % numColumns;

        // Calculate the coordinates of the desired tile
        int x = column * tileWidth;
        int y = row * tileHeight;

        // Return the desired tile from the mosaic
        return { x, y, tileWidth, tileHeight };
    }

    enum class RadioButtonParameterType { IndexBased, ComponentIdBased };

    /*
To implement a new attachment type, create a new class which includes an instance of this class as a data member.
 * Your class should pass a function to the constructor of the ParameterAttachment, which will then be called on the
 * message thread when the parameter changes. You can use this function to update the state of the UI control.
 * Your class should also register as a listener of the UI control and respond to changes in the UI element by
 * calling either setValueAsCompleteGesture or beginGesture, setValueAsPartOfGesture and endGesture.
Make sure to call sendInitialUpdate at the end of your new attachment's constructor, so that the UI immediately
 reflects the state of the parameter.*/
    class RadioButtonParameterAttachment : juce::Button::Listener {
    public:
        //     Creates a connection between a plug-in parameter and some radio buttons.
        RadioButtonParameterAttachment (juce::RangedAudioParameter&       param,
                                        const juce::Array<juce::Button*>& _buttons,
                                        const int                         groupID,
                                        juce::UndoManager*                undoManager,
                                        const RadioButtonParameterType type = RadioButtonParameterType::IndexBased) :
            storedParameter (param),
            attachment (
                param,
                [this] (const float newValue) { setValue (newValue); },
                undoManager),
            radioButtonType (type) {
            for (int i = 0; i < _buttons.size(); ++i) {
                auto button = _buttons.getUnchecked (i);
                if (!buttons.contains (button)) {
                    if (groupID > 0) {
                        button->setRadioGroupId (groupID);
                    }
                    button->setClickingTogglesState (true);
                    buttons.add (button);
                    button->addListener (this);
                }
            }
            attachment.sendInitialUpdate();
        }

        ~RadioButtonParameterAttachment() override {
            for (int i = 0; i < buttons.size(); ++i) {
                juce::Button* button = buttons.getUnchecked (i);
                button->removeListener (this);
            }
        }

        juce::Button* getButton (const int index) const { return buttons.getUnchecked (index); }

        juce::Array<juce::Component::SafePointer<juce::Button>> getButtons() { return buttons; }

        int numButtons() const { return buttons.size(); }

        // place all buttons in a row with specified margin
        void setBounds (const int x, const int y, const int width, const int height, const int margin) const {
            for (int i = 0; i < buttons.size(); i++) {
                buttons.getUnchecked (i)->setBounds (x + margin * i, y, width, height);
            }
        }

        [[nodiscard]] juce::RangedAudioParameter& getParam() const { return storedParameter; }

    private:
        void setValueUsingIndex() {
            const juce::ScopedValueSetter<bool> svs (ignoreCallbacks, true);
            auto                                button = buttons[static_cast<int> (value)];
            button->setToggleState (true, juce::sendNotification);
        }

        void buttonClickUseIndex (juce::Button* b) {
            for (int i = 0; i < buttons.size(); i++) {
                if (b == buttons.getUnchecked (i) && b->getToggleState()) {
                    //the value to set comes from the buttons index in the array 0-<no of buttons>
                    const auto newValue = static_cast<float> (i);
                    attachment.setValueAsCompleteGesture (newValue);
                }
            }
        }

        void setValueUsingComponentId() {
            const juce::ScopedValueSetter<bool> svs (ignoreCallbacks, true);
            {
                auto is_valueMatch = [this] (juce::Component::SafePointer<juce::Button> b) {
                    const auto componentName            = b->getName();
                    const auto buttonComponentNameValue = componentName.getFloatValue();
                    const auto match                    = value == buttonComponentNameValue;
                    return match;
                };

                auto matchedButton = std::find_if (buttons.begin(), buttons.end(), is_valueMatch);
                if (matchedButton != std::end (buttons)) {
                    auto component = matchedButton->getComponent();
                    if (component) {
                        component->setToggleState (true, juce::sendNotification);
                    }
                } else {
                    //There is no match so toggle all buttons to off
                    std::ranges::for_each (buttons, [this] (const juce::Component::SafePointer<juce::Button>& b) {
                        b->setToggleState (false, juce::NotificationType::dontSendNotification);
                    });
                }
            }
        }

        void buttonClickUseComponentId (juce::Button* b) {
            for (int i = 0; i < buttons.size(); i++) {
                if (b == buttons.getUnchecked (i) && b->getToggleState()) {
                    //the value to set comes from the componentId for the button, yuck!  Alternatively we could use a tuple passed in with the
                    // button, the second value in the tuple could be an enum with a value which is then cast to float, or just the float value.
                    const auto newValue      = b->getName().getFloatValue();
                    auto       existingValue = storedParameter.convertFrom0to1 (storedParameter.getValue());
                    if (newValue != existingValue) {
                        attachment.setValueAsCompleteGesture (newValue);
                    } else {
                        //if this is setting the value to what it was then we need to reset it to a known default, so we use the default value
                        // for this.  We could assign a reset value if we ever need a default and reset.  This would onyl really be needed if
                        // the default was say 3, and when you re-clicked this radio button you wanted it to goto 0 or another value.  We can
                        // revisit this if needed...
                        const auto defaultValue = storedParameter.getDefaultValue();
                        attachment.setValueAsCompleteGesture (defaultValue);
                    }
                }
            }
        }

        void setValue (float newValue) {
            value = newValue;

            switch (radioButtonType) {
                case RadioButtonParameterType::IndexBased:
                    setValueUsingIndex();
                    break;
                case RadioButtonParameterType::ComponentIdBased:
                    setValueUsingComponentId();
                    break;
            }
        }

        void buttonClicked (juce::Button* b) override {
            if (ignoreCallbacks) {
                return;
            }
            switch (radioButtonType) {
                case RadioButtonParameterType::IndexBased:
                    buttonClickUseIndex (b);
                    break;
                case RadioButtonParameterType::ComponentIdBased:
                    buttonClickUseComponentId (b);
                    break;
            }
        }

        void buttonStateChanged (juce::Button* b) override {}
        //        if (ignoreCallbacks) {
        //            return;
        //        }
        // state change occurs on mouse over and mouse down etc. So we don't want to toggle in this callback
        //        for (int i = 0; i < buttons.size(); i++) {
        //            if (b == buttons.getUnchecked(i) && b->getToggleState()) {
        //                attachment.setValueAsCompleteGesture(static_cast<float>(i));
        //            }
        //        }
        //    }

        float                                                   value {};
        juce::RangedAudioParameter&                             storedParameter;
        juce::ParameterAttachment                               attachment;
        juce::Array<juce::Component::SafePointer<juce::Button>> buttons;
        bool                                                    ignoreCallbacks = false;
        RadioButtonParameterType                                radioButtonType;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioButtonParameterAttachment)
    };

    class ComponentWithParamMenu : public juce::Component {
    public:
        ComponentWithParamMenu (juce::AudioProcessorEditor& editorIn, juce::RangedAudioParameter& paramIn) :
            editor (editorIn), param (paramIn) {}

        void mouseUp (const juce::MouseEvent& e) override {
            if (e.mods.isRightButtonDown()) {
                if (const auto* c = editor.getHostContext()) {
                    if (const auto menuInfo = c->getContextMenuForParameter (&param)) {
                        menuInfo->getEquivalentPopupMenu().showMenuAsync (
                            juce::PopupMenu::Options {}.withTargetComponent (this).withMousePosition());
                    }
                }
            }
        }

        [[nodiscard]] juce::RangedAudioParameter& getParam() const { return param; }

    private:
        juce::AudioProcessorEditor& editor;
        juce::RangedAudioParameter& param;
    };

    enum SuffixDisplay { OffOnMinimum, OffOnMaximum, Always, Never, Zero };

    class AttachedSlider : public ComponentWithParamMenu {
    public:
        AttachedSlider (juce::AudioProcessorEditor& editorIn,
                        juce::RangedAudioParameter& paramIn,
                        juce::UndoManager*          undoManager,
                        const SuffixDisplay         suffix = Always,
                        juce::Slider::SliderStyle   style  = juce::Slider::RotaryVerticalDrag) :
            ComponentWithParamMenu (editorIn, paramIn),
            slider { juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow },
            label ("", paramIn.name),
            attachment (paramIn, slider, undoManager),
            suffixDisplay (suffix) {
            slider.addMouseListener (this, true);

            addAndMakeVisible(slider);
            addAndMakeVisible(label);

            UpdateSuffix();

            label.attachToComponent (&slider, false);
            label.setJustificationType (juce::Justification::centred);
        }

        void UpdateSuffix() {
            const auto value  = slider.getValueObject();
            const bool isMin  = value == slider.getMinimum();
            const bool isMax  = value == slider.getMaximum();
            const bool isZero = juce::approximatelyEqual (static_cast<float> (value.getValue()), 0.0f);

            if ((suffixDisplay == OffOnMinimum && isMin) || (suffixDisplay == OffOnMaximum && isMax)
                || (suffixDisplay == Never) || (suffixDisplay == Zero && isZero)) {
                ClearSuffix();
            } else if (suffixDisplay != Never) {
                SetDefaultSuffix();
            }
        }

        void SetDefaultSuffix() { slider.setTextValueSuffix (" " + getParam().label); }

        void ClearSuffix() { slider.setTextValueSuffix (""); }

        void resized() override { slider.setBounds (getLocalBounds()); }

        juce::Slider& getSlider() { return slider; }

        juce::SliderParameterAttachment& getAttachment() { return attachment; }

    private:
        juce::Slider                    slider;
        juce::Label                     label;
        juce::SliderParameterAttachment attachment;
        SuffixDisplay                   suffixDisplay;
    };

    class AttachedToggle : public ComponentWithParamMenu {
    public:
        AttachedToggle (juce::AudioProcessorEditor& editorIn, juce::RangedAudioParameter& paramIn) :
            ComponentWithParamMenu (editorIn, paramIn),
            toggleButton (paramIn.name),
            attachment (paramIn, toggleButton) {
            toggleButton.addMouseListener (this, true);
            addAndMakeVisible (toggleButton);
        }

        void resized() override { toggleButton.setBounds (getLocalBounds()); }

        juce::ToggleButton& getToggle() { return toggleButton; }

        juce::ButtonParameterAttachment& getAttachment() { return attachment; }

    private:
        juce::ToggleButton              toggleButton;
        juce::ButtonParameterAttachment attachment;
    };

    class AttachedRadioButtons : public ComponentWithParamMenu {
    public:
        AttachedRadioButtons (juce::AudioProcessorEditor&    editorIn,
                              juce::RangedAudioParameter&    paramIn,
                              juce::Array<juce::Button*>&    buttons,
                              const int                      groupId,
                              juce::UndoManager*             um,
                              const RadioButtonParameterType radioType) :
            ComponentWithParamMenu (editorIn, paramIn), attachment (paramIn, buttons, groupId, um, radioType) {
            std::ranges::for_each (buttons, [this] (juce::Button* b) {
                b->addMouseListener (this, true);
                addAndMakeVisible (b);
            });
        }

        ~AttachedRadioButtons() override {
            auto buttons = attachment.getButtons();
            std::ranges::for_each (buttons, [this] (juce::Button* button) { button->removeMouseListener (this); });
        }

#if DEBUG
        void paint (juce::Graphics& g) override {
            auto buttons = attachment.getButtons();
            std::for_each (buttons.begin(), buttons.end(), [this, &g] (juce::Button* button) {
                g.setColour (juce::Colours::yellowgreen);
                g.drawRect (button->getBounds(), 1);
            });
        }
#endif

        //        void resized() override {
        //            auto buttons = attachment.getButtons();
        //            auto lb = getLocalBounds();
        //            std::for_each (
        //                buttons.begin(), buttons.end(), [this, &lb] (Button* button) {
        //                    button->setBounds (lb);
        //                });
        //        }

        juce::Button* getButtonAtIndex (const int i) const { return attachment.getButton (i); }

        RadioButtonParameterAttachment& getAttachment() { return attachment; }

    private:
        RadioButtonParameterAttachment attachment;
    };

    template <typename T = juce::ImageButton, typename = std::enable_if_t<std::is_base_of_v<juce::ImageButton, T>>>
    class AttachedImageButton : public ComponentWithParamMenu {
    public:
        AttachedImageButton (juce::AudioProcessorEditor& editorIn,
                             juce::RangedAudioParameter& paramIn,
                             juce::UndoManager*          undoManager) :
            ComponentWithParamMenu (editorIn, paramIn),
            param (paramIn),
            button (paramIn.name),
            attachment (paramIn, button, undoManager) {
            button.addMouseListener (this, true);
            addAndMakeVisible (button);
        }

        void resized() override { button.setBounds (getLocalBounds()); }

        T& getButton() { return button; }

        juce::ButtonParameterAttachment& getAttachment() { return attachment; }

        //    RangedAudioParameter &getParameter() {
        //        return param;
        //    }

    private:
        juce::RangedAudioParameter&     param;
        T                               button;
        juce::ButtonParameterAttachment attachment;
    };

    class AttachedCombo : public ComponentWithParamMenu {
    public:
        AttachedCombo (juce::AudioProcessorEditor& editorIn,
                       juce::RangedAudioParameter& paramIn,
                       juce::UndoManager*          undoManager) :
            ComponentWithParamMenu (editorIn, paramIn),
            combo (paramIn),
            label ("", paramIn.name),
            attachment (paramIn, combo, undoManager) {
            combo.addMouseListener (this, true);
            combo.setJustificationType (juce::Justification::centred);
            addAndMakeVisible(combo);
            addAndMakeVisible (label);

            label.attachToComponent (&combo, false);
            label.setJustificationType (juce::Justification::centred);
        }

        void resized() override {
            combo.setBounds (getLocalBounds());
            //.withSizeKeepingCentre(jmin(getWidth(), 150), 24));
        }

    private:
        struct ComboWithItems final : public juce::ComboBox {
            explicit ComboWithItems (juce::RangedAudioParameter& param) {
                // Adding the list here in the constructor means that the combo
                // is already populated when we construct the attachment below
                addItemList (dynamic_cast<juce::AudioParameterChoice&> (param).choices, 1);
            }
        };

        ComboWithItems                    combo;
        juce::Label                       label;
        juce::ComboBoxParameterAttachment attachment;

    public:
        ComboWithItems&                    getCombo() { return combo; }
        juce::ComboBoxParameterAttachment& getAttachment() { return attachment; }
    };

    using Px = juce::Grid::Px;
    using Fr = juce::Grid::Fr;

    struct GetTrackInfo {
        // Combo boxes need a lot of room
        juce::Grid::TrackInfo operator() (AttachedCombo&) const { return Px (120); }

        // Toggles are a bit smaller
        juce::Grid::TrackInfo operator() (AttachedToggle&) const { return Px (80); }

        // Sliders take up as much room as they can
        juce::Grid::TrackInfo operator() (AttachedSlider&) const { return Fr (1); }
    };

    template <typename... Components>
    static void performLayout (const juce::Rectangle<int>& bounds, Components&... components) {
        juce::Grid grid;
        grid.alignContent    = juce::Grid::AlignContent::spaceAround;
        grid.autoColumns     = juce::Grid::TrackInfo (Fr (1));
        grid.autoRows        = juce::Grid::TrackInfo (Fr (1));
        grid.columnGap       = juce::Grid::Px (80);
        grid.rowGap          = juce::Grid::Px (80);
        grid.autoFlow        = juce::Grid::AutoFlow::column;
        grid.templateColumns = { Fr (1), Fr (1) };
        grid.templateRows    = { Fr (1), Fr (1) };
        grid.items           = { GridItem (components)... };
        grid.performLayout (bounds);
    }

}