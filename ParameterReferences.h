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
        const auto strText = text.trim().toLowerCase();
        const int len = strText.length();

        // 1. Handle center/legacy cases
        if (strText == "center" || strText == "c" || strText == "<c>" || strText == "< c >" || strText == "0")
            return 0.0f;

        // 2. Shorthand for full left/right
        if (len == 1) {
            if (strText[0] == 'l') return -100.0f;
            if (strText[0] == 'r') return 100.0f;
        }

        // 3. Direction with number
        if (strText.startsWithChar('l') && len > 1)
            return -juce::jlimit(0.0f, 100.0f, strText.substring(1).getFloatValue());

        if (strText.startsWithChar('r') && len > 1)
            return juce::jlimit(0.0f, 100.0f, strText.substring(1).getFloatValue());

        if (strText.endsWithChar('l') && len > 1)
            return -juce::jlimit(0.0f, 100.0f, strText.dropLastCharacters(1).getFloatValue());

        if (strText.endsWithChar('r') && len > 1)
            return juce::jlimit(0.0f, 100.0f, strText.dropLastCharacters(1).getFloatValue());

        // 4. Number or % format
        bool isPercentage = strText.endsWithChar('%');
        auto numberText = isPercentage ? strText.dropLastCharacters(1) : strText;

        // Validate float format
        bool hasDigits = false;
        bool hasDot = false;

        for (int i = 0; i < numberText.length(); ++i) {
            const auto c = numberText[i];

            if (i == 0 && (c == '-' || c == '+')) {
                continue;
            }
            if (juce::CharacterFunctions::isDigit(c)) {
                hasDigits = true;
                continue;
            }
            if (c == '.' && !hasDot) {
                hasDot = true;
                continue;
            }

            // Invalid character or multiple dots
            return 0.0f;
        }

        if (hasDigits) {
            return juce::jlimit(-100.0f, 100.0f, numberText.getFloatValue());
        }

        return 0.0f;
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


    namespace FrequencyUnit {
        struct Hz {};  // Tag for Hz-default mode
        struct kHz {}; // Tag for kHz-default mode
    }

    template <typename Unit = FrequencyUnit::Hz>
    static auto makeFromStringWithFrequency() {
        return [](const juce::String& text) -> float {
            auto lowerText = text.toLowerCase().trim().removeCharacters(" ");

            if (lowerText.endsWith("khz") || lowerText.endsWith("k")) {
                int suffixLength = lowerText.endsWith("khz") ? 3 : 1;
                return lowerText.dropLastCharacters(suffixLength).getFloatValue() * 1000.0f;
            }
            else if (lowerText.endsWith("hz")) {
                return lowerText.dropLastCharacters(2).getFloatValue();
            }
            // Default behavior based on unit tag
            if constexpr (std::is_same_v<Unit, FrequencyUnit::kHz>) {
                return lowerText.getFloatValue() * 1000.0f; // "8" → 8000
            } else {
                return lowerText.getFloatValue(); // "20" → 20
            }
        };
    }

    template <typename Unit = FrequencyUnit::Hz>
    static auto makeStringFromValueWithFrequency(int hzDecimalPlaces = 1, int khzDecimalPlaces = 2) {
        return [hzDecimalPlaces, khzDecimalPlaces](float value) -> juce::String {
            if constexpr (std::is_same_v<Unit, FrequencyUnit::kHz>) {
                if (value >= 1000.0f) {
                    return juce::String(value / 1000.0f, khzDecimalPlaces) + " kHz";
                }
            }
            return juce::String(value, hzDecimalPlaces) + " Hz";
        };
    }

    template <typename Unit = FrequencyUnit::Hz>
    static auto makeFromStringWithFrequencyWithOffAt(float offValue) {
        return [offValue](const juce::String& text) -> float {
            auto lowerText = text.toLowerCase().trim();
            if (lowerText == "off") return offValue;
            return makeFromStringWithFrequency<Unit>()(text); // Reuse unit-aware parser
        };
    }

    // Ensure the lambda function matches the expected type
    template <typename Unit = FrequencyUnit::Hz>
    static auto makeStringFromValueWithFrequencyWithOffAt(float offValue, int hzDecimalPlaces = 1, int khzDecimalPlaces = 2) {
        return [offValue, hzDecimalPlaces, khzDecimalPlaces](float value, int /*maximumStringLength*/) -> juce::String {
            if (juce::approximatelyEqual(value, offValue)) {
                return "OFF";
            }
            return makeStringFromValueWithFrequency<Unit>(hzDecimalPlaces, khzDecimalPlaces)(value);
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

    static inline auto stringFromValue = [] (float value, [[maybe_unused]] int maximumStringLength = 5) {
        // only 1 decimal place for db values
        return juce::String (value, 1);
    };

    static inline auto valueFromString = [] (const juce::String& text) {
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