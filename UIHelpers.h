#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace moiraesoftware
{
    template <typename Func, typename... Items>
    constexpr void forEach (Func&& func, Items&&... items)
    {
        (func (std::forward<Items> (items)), ...);
    }

    template <typename... Components>
    void addAllAndMakeVisible (Component& target, Components&... children)
    {
        forEach ([&] (Component& child) { target.addAndMakeVisible (child); }, children...);
    }

    template <typename... Processors>
    void prepareAll (const dsp::ProcessSpec& spec, Processors&... processors)
    {
        forEach ([&] (auto& proc) { proc.prepare (spec); }, processors...);
    }

    template <typename... Processors>
    void resetAll (Processors&... processors)
    {
        forEach ([] (auto& proc) { proc.reset(); }, processors...);
    }

    template <typename... Processors>
    void processAll (const dsp::ProcessContextReplacing<float>& context, Processors&... processors)
    {
        forEach ([&] (auto& proc) { proc.process (context); }, processors...);
    }

    enum class RadioButtonParameterType {
        IndexBased,
        ComponentIdBased
    };

    /*
To implement a new attachment type, create a new class which includes an instance of this class as a data member.
 * Your class should pass a function to the constructor of the ParameterAttachment, which will then be called on the
 * message thread when the parameter changes. You can use this function to update the state of the UI control.
 * Your class should also register as a listener of the UI control and respond to changes in the UI element by
 * calling either setValueAsCompleteGesture or beginGesture, setValueAsPartOfGesture and endGesture.
Make sure to call sendInitialUpdate at the end of your new attachment's constructor, so that the UI immediately
 reflects the state of the parameter.*/
    class RadioButtonParameterAttachment : private Button::Listener
    {
    public:
        //     Creates a connection between a plug-in parameter and some radio buttons.
        RadioButtonParameterAttachment (RangedAudioParameter& param,
            Array<Button*>& _buttons,
            int groupID,
            RadioButtonParameterType type = RadioButtonParameterType::IndexBased,
            UndoManager* um = nullptr) : storedParameter (param),
                                         attachment (param, [this] (float newValue) { setValue (newValue); }, um),
                                         radioButtonType (type)
        {
            for (int i = 0; i < _buttons.size(); ++i)
            {
                auto button = _buttons.getUnchecked (i);
                if (!buttons.contains (button))
                {
                    if (groupID > 0)
                    {
                        button->setRadioGroupId (groupID);
                    }
                    button->setClickingTogglesState (true);
                    buttons.add (button);
                    button->addListener (this);
                }
            }
            attachment.sendInitialUpdate();
        }

        ~RadioButtonParameterAttachment() override
        {
            for (int i = 0; i < buttons.size(); ++i)
            {
                Button* button = buttons.getUnchecked (i);
                button->removeListener (this);
            }
        }

        Button* getButton (int index) { return buttons.getUnchecked (index); }

        Array<Component::SafePointer<Button>> getButtons() { return buttons; }

        int numButtons() { return buttons.size(); }

        // place all buttons in a row with specified margin
        void setBounds (int x, int y, int width, int height, int margin)
        {
            for (int i = 0; i < buttons.size(); i++)
            {
                buttons.getUnchecked (i)->setBounds (x + margin * i, y, width, height);
            }
        }

        [[nodiscard]] RangedAudioParameter& getParam() const
        {
            return storedParameter;
        }

    private:
        void setValueUsingIndex()
        {
            const ScopedValueSetter<bool> svs (ignoreCallbacks, true);
            auto button = buttons[static_cast<int> (value)];
            button->setToggleState (true, sendNotification);
        }

        void buttonClickUseIndex (Button* b)
        {
            for (int i = 0; i < buttons.size(); i++)
            {
                if (b == buttons.getUnchecked (i) && b->getToggleState())
                {
                    //the value to set comes from the buttons index in the array 0-<no of buttons>
                    auto newValue = static_cast<float> (i);
                    attachment.setValueAsCompleteGesture (newValue);
                }
            }
        }

        void setValueUsingComponentId()
        {
            const ScopedValueSetter<bool> svs (ignoreCallbacks, true);
            {
                auto is_valueMatch = [this] (Component::SafePointer<Button> b) {
                    auto componentName = b->getName();
                    auto buttonComponentNameValue = componentName.getFloatValue();
                    auto match = value == buttonComponentNameValue;
                    return match;
                };

                auto matchedButton = std::find_if (buttons.begin(), buttons.end(), is_valueMatch);
                if (matchedButton != std::end (buttons))
                {
                    auto component = matchedButton->getComponent();
                    if (component)
                    {
                        component->setToggleState (true, sendNotification);
                    }
                }
                else
                {
                    //There is no match so toggle all buttons to off
                    std::for_each (buttons.begin(), buttons.end(), [this] (const Component::SafePointer<Button>& b) {
                        b->setToggleState (false, NotificationType::dontSendNotification);
                    });
                }
            }
        }

        void buttonClickUseComponentId (Button* b)
        {
            for (int i = 0; i < buttons.size(); i++)
            {
                if (b == buttons.getUnchecked (i) && b->getToggleState())
                {
                    //the value to set comes from the componentId for the button, yuck!  Alternatively we could use a tuple passed in with the
                    // button, the second value in the tuple could be an enum with a value which is then cast to float, or just the float value.
                    auto newValue = b->getName().getFloatValue();
                    auto existingValue = storedParameter.convertFrom0to1 (storedParameter.getValue());
                    if (newValue != existingValue)
                    {
                        attachment.setValueAsCompleteGesture (newValue);
                    }
                    else
                    {
                        //if this is setting the value to what it was then we need to reset it to a known default, so we use the default value
                        // for this.  We could assign a reset value if we ever need a default and reset.  This would onyl really be needed if
                        // the default was say 3, and when you re-clicked this radio button you wanted it to goto 0 or another value.  We can
                        // revisit this if needed...
                        auto defaultValue = storedParameter.getDefaultValue();
                        attachment.setValueAsCompleteGesture (defaultValue);
                    }
                }
            }
        }

        void setValue (float newValue)
        {
            value = newValue;

            switch (radioButtonType)
            {
                case RadioButtonParameterType::IndexBased:
                    setValueUsingIndex();
                    break;
                case RadioButtonParameterType::ComponentIdBased:
                    setValueUsingComponentId();
                    break;
            }
        }

        void buttonClicked (Button* b) override
        {
            if (ignoreCallbacks)
            {
                return;
            }
            switch (radioButtonType)
            {
                case RadioButtonParameterType::IndexBased:
                    buttonClickUseIndex (b);
                    break;
                case RadioButtonParameterType::ComponentIdBased:
                    buttonClickUseComponentId (b);
                    break;
            }
        }

        void buttonStateChanged (Button* b) override {}
        //        if (ignoreCallbacks) {
        //            return;
        //        }
        // state change occurs on mouse over and mouse down etc so we dont want to toggle in this callback
        //        for (int i = 0; i < buttons.size(); i++) {
        //            if (b == buttons.getUnchecked(i) && b->getToggleState()) {
        //                attachment.setValueAsCompleteGesture(static_cast<float>(i));
        //            }
        //        }
        //    }

        float value {};
        RangedAudioParameter& storedParameter;
        ParameterAttachment attachment;
        Array<Component::SafePointer<Button>> buttons;
        bool ignoreCallbacks = false;
        RadioButtonParameterType radioButtonType;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioButtonParameterAttachment)
    };

    class ComponentWithParamMenu : public Component
    {
    public:
        ComponentWithParamMenu (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn) : editor (editorIn),
                                                                                                 param (paramIn) {}

        void mouseUp (const MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown())
            {
                if (auto* c = editor.getHostContext())
                {
                    if (auto menuInfo = c->getContextMenuForParameter (&param))
                    {
                        menuInfo->getEquivalentPopupMenu().showMenuAsync (
                            PopupMenu::Options {}.withTargetComponent (this).withMousePosition());
                    }
                }
            }
        }

        [[nodiscard]] RangedAudioParameter& getParam() const
        {
            return param;
        }

    private:
        AudioProcessorEditor& editor;
        RangedAudioParameter& param;

    public:
        const RangedAudioParameter& getParameter()
        {
            return param;
        }
    };

    enum SuffixDisplay {
        OffOnMinimum,
        OffOnMaximum,
        Always,
        Never
    };

    class AttachedSlider : public ComponentWithParamMenu
    {
    public:
        AttachedSlider (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn, SuffixDisplay suffix = Always)
            : ComponentWithParamMenu (editorIn, paramIn),
              slider { Slider::RotaryVerticalDrag, Slider::TextBoxBelow },
              label ("", paramIn.name),
              attachment (paramIn, slider),
              suffixDisplay (suffix)
        {
            slider.addMouseListener (this, true);
            addAllAndMakeVisible (*this, slider, label);

            UpdateSuffix();

            label.attachToComponent (&slider, false);
            label.setJustificationType (Justification::centred);
        }

        void UpdateSuffix()
        {
            const auto value = slider.getValueObject();
            switch (suffixDisplay)
            {
                case OffOnMinimum:
                    if (value == slider.getMinimum())
                    {
                        ClearSuffix();
                    }
                    else
                    {
                        SetDefaultSuffix();
                    }
                    break;

                case OffOnMaximum:
                    if (value == slider.getMaximum())
                    {
                        ClearSuffix();
                    }
                    else
                    {
                        SetDefaultSuffix();
                    }
                    break;

                case Always:
                    SetDefaultSuffix();
                    break;

                case Never:
                    ClearSuffix();
                    break;
            }
        }

        void SetDefaultSuffix()
        {
            slider.setTextValueSuffix (" " + getParam().label);
        }

        void ClearSuffix()
        {
            slider.setTextValueSuffix ("");
        }

        void resized() override { slider.setBounds (getLocalBounds()); }

    public:
        Slider& getSlider()
        {
            return slider;
        }

        SliderParameterAttachment& getAttachment()
        {
            return attachment;
        }

    private:
        Slider slider;
        Label label;
        SliderParameterAttachment attachment;
        SuffixDisplay suffixDisplay;
    };

    class AttachedToggle : public ComponentWithParamMenu
    {
    public:
        AttachedToggle (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn)
            : ComponentWithParamMenu (editorIn, paramIn),
              toggleButton (paramIn.name),
              attachment (paramIn, toggleButton)
        {
            toggleButton.addMouseListener (this, true);
            addAndMakeVisible (toggleButton);
        }

        void resized() override { toggleButton.setBounds (getLocalBounds()); }

        ToggleButton& getToggle()
        {
            return toggleButton;
        }

        ButtonParameterAttachment& getAttachment()
        {
            return attachment;
        }

    private:
        ToggleButton toggleButton;
        ButtonParameterAttachment attachment;
    };

    class AttachedRadioButtons : public ComponentWithParamMenu
    {
    public:
        AttachedRadioButtons (AudioProcessorEditor& editorIn,
            RangedAudioParameter& paramIn,
            Array<Button*>& buttons,
            int groupId,
            RadioButtonParameterType radioType)
            : ComponentWithParamMenu (editorIn, paramIn),
              attachment (paramIn, buttons, groupId, radioType)
        {
            std::for_each (buttons.begin(), buttons.end(), [this] (Button* b) {
                b->addMouseListener (this, true);
                addAndMakeVisible (b);
            });
        }

        ~AttachedRadioButtons() override
        {
            auto buttons = attachment.getButtons();
            std::for_each (buttons.begin(), buttons.end(), [this] (Button* button) { button->removeMouseListener (this); });
        }

        //    void resized() override {
        //        auto buttons = attachment.getButtons();
        //        std::for_each(buttons.begin(), buttons.end(),
        //                      [this](Button *button) { button->setBounds(getLocalBounds()); });
        //    }

        Button* getButtonAtIndex (int i)
        {
            return attachment.getButton (i);
        }

        RadioButtonParameterAttachment& getAttachment()
        {
            return attachment;
        }

    private:
        RadioButtonParameterAttachment attachment;
    };

    class AttachedImageButton : public ComponentWithParamMenu
    {
    public:
        AttachedImageButton (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn)
            : ComponentWithParamMenu (editorIn, paramIn),
              param (paramIn),
              button (paramIn.name),
              attachment (paramIn, button)
        {
            button.addMouseListener (this, true);
            addAndMakeVisible (button);
        }

        void resized() override { button.setBounds (getLocalBounds()); }

        ImageButton& getButton()
        {
            return button;
        }

        ButtonParameterAttachment& getAttachment()
        {
            return attachment;
        }

        //    RangedAudioParameter &getParameter() {
        //        return param;
        //    }

    private:
        RangedAudioParameter& param;
        ImageButton button;
        ButtonParameterAttachment attachment;
    };

    class AttachedCombo : public ComponentWithParamMenu
    {
    public:
        AttachedCombo (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn)
            : ComponentWithParamMenu (editorIn, paramIn),
              combo (paramIn),
              label ("", paramIn.name),
              attachment (paramIn, combo)
        {
            combo.addMouseListener (this, true);
            combo.setJustificationType (Justification::centred);
            addAllAndMakeVisible (*this, combo, label);
            label.attachToComponent (&combo, false);
            label.setJustificationType (Justification::centred);
        }

        void resized() override
        {
            combo.setBounds (getLocalBounds());
            //.withSizeKeepingCentre(jmin(getWidth(), 150), 24));
        }

    private:
        struct ComboWithItems : public ComboBox
        {
            explicit ComboWithItems (RangedAudioParameter& param)
            {
                // Adding the list here in the constructor means that the combo
                // is already populated when we construct the attachment below
                addItemList (dynamic_cast<AudioParameterChoice&> (param).choices, 1);
            }
        };

        ComboWithItems combo;
        Label label;
        ComboBoxParameterAttachment attachment;

    public:
        ComboWithItems& getCombo()
        {
            return combo;
        }
    };

    struct GetTrackInfo
    {
        // Combo boxes need a lot of room
        Grid::TrackInfo operator() (AttachedCombo&) const { return 120_px; }

        // Toggles are a bit smaller
        Grid::TrackInfo operator() (AttachedToggle&) const { return 80_px; }

        // Sliders take up as much room as they can
        Grid::TrackInfo operator() (AttachedSlider&) const { return 1_fr; }
    };

    template <typename... Components>
    static void performLayout (const Rectangle<int>& bounds, Components&... components)
    {
        Grid grid;
        grid.alignContent = Grid::AlignContent::spaceAround;
        grid.autoColumns = Grid::TrackInfo (1_fr);
        grid.autoRows = Grid::TrackInfo (1_fr);
        grid.columnGap = Grid::Px (80);
        grid.rowGap = Grid::Px (80);
        grid.autoFlow = Grid::AutoFlow::column;
        grid.templateColumns = { 1_fr, 1_fr };
        grid.templateRows = { 1_fr, 1_fr };
        grid.items = { GridItem (components)... };
        grid.performLayout (bounds);
    }
}