#pragma once

#include "melatonin_parameters/melatonin_parameters.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace moiraesoftware{
    using Attributes = juce::AudioProcessorValueTreeStateParameterAttributes;

    template <typename Param>
    static void add (juce::AudioProcessorParameterGroup& group, std::unique_ptr<Param> param) {
        group.addChild (std::move (param));
    }

    template <typename Param>
    static void add (juce::AudioProcessorValueTreeState::ParameterLayout& group, std::unique_ptr<Param> param) {
        group.add (std::move (param));
    }

    template <typename Param, typename Group, typename... Ts>
    static Param& addToLayout (Group& layout, Ts&&... ts) {
        auto   param = std::make_unique<Param> (std::forward<Ts> (ts)...);
        Param& ref   = *param;
        add (layout, std::move (param)); // Transfers ownership
        return ref;
    }

    static juce::String valueToText (float x, int decimalPlaces) {
        return { x, decimalPlaces };
    }

    static float textToValue (const juce::String& str) {
        auto trimmed = str.trim().toLowerCase();
        return str.getFloatValue();
    }

    static auto getBasicAttributes(int decimalPlaces = 2) {
        return Attributes()
            .withStringFromValueFunction (valueToText)
            .withValueFromStringFunction (textToValue);
    }

    static inline auto stringFromPanValue = [](float value, [[maybe_unused]] int maximumStringLength = 5) {
        float v = (value + 100.0f) / 200.0f;

        if (v == 0.5f)
            return juce::String ("< C >");

        const auto percentage = juce::roundToInt (std::abs (0.5f - v) * 200.0f);
        return v < 0.5f
                   ? "L " + juce::String (percentage) + " "
                   : " " + juce::String (percentage) + " R";
    };

    static inline auto panFromString = [](const juce::String& text) {
        auto strText = text.trim().toLowerCase();

        if (strText == "center" || strText == "c" || strText == "< c >")
            return 0.5f;

        if (strText.startsWithChar ('l')) {
            const float percentage = strText.substring (2).trim().getFloatValue();
            return 0.5f * (1.0f - percentage / 100.0f);
        }

        if (strText.endsWithChar ('r')) {
            const float percentage = strText.dropLastCharacters (1).trim().getFloatValue();
            return 0.5f * (1.0f + percentage / 100.0f);
        }

        return 0.5f;
    };

    static auto makeStringFromValueWithOffAt(float offValue, const juce::String& label) {
        return [offValue, label](float value, [[maybe_unused]] int maximumStringLength = 5) {
            if (juce::approximatelyEqual(value, offValue))
                return juce::String("OFF");
            return juce::String(value, 1) + label;
        };
    }

    static auto makeFromStringWithOffAt(float offValue, const juce::String& label) {
        return [offValue, label](const juce::String& text) {
            if (text.toLowerCase() == "off")
                return offValue;
            if (text.endsWith(label))
                return text.dropLastCharacters(2).getFloatValue();
            return text.getFloatValue();
        };
    }

    static auto makeStringFromValueWithFrequency() {
        return [](float value, [[maybe_unused]] int maximumStringLength = 5) {
            if (value >= 1000.0f) {
                // For kHz range, divide by 1000 and show 2 decimal places
                return juce::String(value / 1000.0f, 2) + "kHz";
            }
            // For Hz range, show 1 decimal place
            return juce::String(value, 1) + "Hz";
        };
    }

    static auto makeFromStringWithFrequency() {
        return [](const juce::String& text) {
            auto lowerText = text.toLowerCase();

            if (lowerText.endsWith("khz")) {
                // Handle kHz input - multiply by 1000
                return text.dropLastCharacters(3).getFloatValue() * 1000.0f;
            }
            if (lowerText.endsWith("hz")) {
                // Handle Hz input
                return text.dropLastCharacters(2).getFloatValue();
            }
            // If no unit specified, assume Hz
            return text.getFloatValue();
        };
    }

    static auto makeStringFromValueWithFrequencyWithOffAt(float offValue, bool noRoundingOnHz = false) {
        return [offValue, noRoundingOnHz](float value, [[maybe_unused]] int maximumStringLength = 5) {
            if (juce::approximatelyEqual(value, offValue))
                return juce::String("OFF");
            if (value >= 1000.0f) {
                // For kHz range, divide by 1000 and show 2 decimal places
                return juce::String(value / 1000.0f, 2) + "kHz";
            }
            // For Hz range, show 1 decimal place
            if  (noRoundingOnHz)
                return juce::String(value, 0) + "Hz";
            else return juce::String(value, 1) + "Hz";
        };
    }

    static auto makeFromStringWithFrequencyWithOffAt( float offValue) {
        return [offValue](const juce::String& text) {
            auto lowerText = text.toLowerCase();

            if (text.toLowerCase() == "off")
                return offValue;

            if (lowerText.endsWith("khz")) {
                // Handle kHz input - multiply by 1000
                return text.dropLastCharacters(3).getFloatValue() * 1000.0f;
            }
            if (lowerText.endsWith("hz")) {
                // Handle Hz input
                return text.dropLastCharacters(2).getFloatValue();
            }
            // If no unit specified, assume Hz
            return text.getFloatValue();
        };
    }

    static inline auto stringFromDBValue = [] (float value, [[maybe_unused]] int maximumStringLength = 5) {
        // only 1 decimal place for db values
        return juce::String (value, 1) + "dB";
    };

    static inline auto dBFromString = [] (const juce::String& text) {
        if (text.endsWith ("db"))
        {
            return text.dropLastCharacters (2).getFloatValue();
        }
        else
            return text.getFloatValue();
    };

    //Used like this
    //struct MainGroup {
    //public:
    //    explicit MainGroup (AudioProcessorParameterGroup& audioParams, const MainParameters& mainParams)
    //        : centerFreq (addToLayout<Parameter> (audioParams,
    //                                              mainParams.centerFreq,
    //                                              "Center Freq",
    //                                              NormalisableRange (20.0f, 20000.0f, 0.01f),
    //                                              8500.0f,
    //                                              getHzAttributes())),
    //          q (addToLayout<Parameter> (audioParams,
    //                                     mainParams.q,
    //                                     "Q",
    //                                     NormalisableRange (0.707f, 20.00f, 0.001f),
    //                                     9.898f,
    //                                     Attributes().withLabel("Q"))),
    //          gain (addToLayout<Parameter> (audioParams,
    //                                        mainParams.gain,
    //                                        "Gain",
    //                                        NormalisableRange (0.0f, 15.0f, 0.00025f),
    //                                        1.34100f,
    //                                        Attributes().withLabel("db"))),
    //
    //          bwAdj (addToLayout<Parameter> (audioParams,
    //                                         mainParams.bwAdj,
    //                                         "BW Adj",
    //                                         NormalisableRange (0.0f, 20.0f, 0.0005f),
    //                                         1.79850f,
    //                                         Attributes())),
    //
    //          mainParameters (mainParams) {}
    //    Parameter &centerFreq, &q, &gain, &bwAdj;
    //    const MainParameters& mainParameters;
    //};
    //
    //// This struct holds references to the raw parameters, so that we don't have to search
    //// the APVTS (involving string comparisons and map lookups) every time a parameter changes.
    //struct ParameterReferences {
    //public:
    //    explicit ParameterReferences (AudioProcessorValueTreeState::ParameterLayout& layout)
    //        : mainGroup (addToLayout<AudioProcessorParameterGroup> (
    //                         layout,
    //                         "main",
    //                         "Main",
    //                         "|"),
    //                     MainParameters::getMain()) {
    //    }
    //    MainGroup mainGroup;
    //};
}