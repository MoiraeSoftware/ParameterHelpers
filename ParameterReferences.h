#pragma once

#include "melatonin_parameters/melatonin_parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

namespace moiraesoftware {
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

    static inline auto stringFromPanValue = [] (float value, [[maybe_unused]] int maximumStringLength = 5) {
        float v = (value + 100.0f) / 200.0f;

        if (v == 0.5f)
            return juce::String ("< C >");

        const auto percentage = juce::roundToInt (std::abs (0.5f - v) * 200.0f);
        return v < 0.5f ? "L " + juce::String (percentage) + " " : " " + juce::String (percentage) + " R";
    };

    static inline auto panFromString = [] (const juce::String& text) {
        const auto strText = text.trim().toLowerCase();
        const int  len     = strText.length();

        // 1. Handle center/legacy cases
        if (strText == "center" || strText == "c" || strText == "<c>" || strText == "< c >" || strText == "0")
            return 0.0f;

        // 2. Shorthand for full left/right
        if (len == 1) {
            if (strText[0] == 'l')
                return -100.0f;
            if (strText[0] == 'r')
                return 100.0f;
        }

        // 3. Direction with number
        if (strText.startsWithChar ('l') && len > 1)
            return -juce::jlimit (0.0f, 100.0f, strText.substring (1).getFloatValue());

        if (strText.startsWithChar ('r') && len > 1)
            return juce::jlimit (0.0f, 100.0f, strText.substring (1).getFloatValue());

        if (strText.endsWithChar ('l') && len > 1)
            return -juce::jlimit (0.0f, 100.0f, strText.dropLastCharacters (1).getFloatValue());

        if (strText.endsWithChar ('r') && len > 1)
            return juce::jlimit (0.0f, 100.0f, strText.dropLastCharacters (1).getFloatValue());

        // 4. Number or % format
        bool isPercentage = strText.endsWithChar ('%');
        auto numberText   = isPercentage ? strText.dropLastCharacters (1) : strText;

        // Validate float format
        bool hasDigits = false;
        bool hasDot    = false;

        for (int i = 0; i < numberText.length(); ++i) {
            const auto c = numberText[i];

            if (i == 0 && (c == '-' || c == '+')) {
                continue;
            }
            if (juce::CharacterFunctions::isDigit (c)) {
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
            return juce::jlimit (-100.0f, 100.0f, numberText.getFloatValue());
        }

        return 0.0f;
    };

    static auto
        makeStringFromValueWithOffAt (float offValue, const juce::String& label, const juce::String& offText) {
        return [offValue, label, offText] (float value, [[maybe_unused]] int maximumStringLength = 5) {
            if (juce::approximatelyEqual (value, offValue))
                return juce::String (offText);
            return juce::String (value, 1) + label;
        };
    }

    static auto makeFromStringWithOffAt (float offValue, const juce::String& label, const juce::String& offText) {
        return [offValue, label, offText] (const juce::String& text) {
            if (text.compareIgnoreCase (offText) == 0)
                return offValue;
            if (text.endsWith (label))
                return text.dropLastCharacters (label.length()).getFloatValue();
            return text.getFloatValue();
        };
    }

    template <auto& ParamID, typename Range>
    constexpr auto makeDBParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout,
                                                           ParamID,
                                                           name,
                                                           range,
                                                           defaultVal,
                                                           juce::AudioParameterFloatAttributes()
                                                               .withStringFromValueFunction (stringFromDBValue)
                                                               .withValueFromStringFunction (dBFromString));
        };
    }

    namespace FrequencyUnit {
        struct Hz {}; // Tag for Hz-default mode
        struct kHz {}; // Tag for kHz-default mode
    }

    template <typename Unit = FrequencyUnit::Hz>
    static auto makeFromStringWithFrequency() {
        return [] (const juce::String& text) -> float {
            auto lowerText = text.toLowerCase().trim().removeCharacters (" ");

            if (lowerText.endsWith ("khz") || lowerText.endsWith ("k")) {
                int suffixLength = lowerText.endsWith ("khz") ? 3 : 1;
                return lowerText.dropLastCharacters (suffixLength).getFloatValue() * 1000.0f;
            } else if (lowerText.endsWith ("hz")) {
                return lowerText.dropLastCharacters (2).getFloatValue();
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
    static auto makeStringFromValueWithFrequency (int hzDecimalPlaces = 1, int khzDecimalPlaces = 2) {
        return [hzDecimalPlaces, khzDecimalPlaces] (float value,
                                                    [[maybe_unused]] int maximumStringLength = 0) -> juce::String {
            if constexpr (std::is_same_v<Unit, FrequencyUnit::kHz>) {
                if (value >= 1000.0f) {
                    return juce::String (value / 1000.0f, khzDecimalPlaces) + " kHz";
                }
            }
            return juce::String (value, hzDecimalPlaces) + " Hz";
        };
    }

    template <typename Unit = FrequencyUnit::Hz>
    static auto makeFromStringWithFrequencyWithOffAt (float offValue) {
        return [offValue] (const juce::String& text) -> float {
            auto lowerText = text.toLowerCase().trim();
            if (lowerText == "off")
                return offValue;
            return makeFromStringWithFrequency<Unit>() (text); // Reuse unit-aware parser
        };
    }

    // Ensure the lambda function matches the expected type
    template <typename Unit = FrequencyUnit::Hz>
    static auto makeStringFromValueWithFrequencyWithOffAt (float offValue,
                                                           int   hzDecimalPlaces  = 1,
                                                           int   khzDecimalPlaces = 2) {
        return
            [offValue, hzDecimalPlaces, khzDecimalPlaces] (float value, int /*maximumStringLength*/) -> juce::String {
                if (juce::approximatelyEqual (value, offValue)) {
                    return "OFF";
                }
                return makeStringFromValueWithFrequency<Unit> (hzDecimalPlaces, khzDecimalPlaces) (value);
            };
    }

    static inline auto stringFromDBValue = [] (float value, [[maybe_unused]] int maximumStringLength = 5) {
        // only 1 decimal place for db values
        return juce::String (value, 1) + "dB";
    };

    static inline auto dBFromString = [] (const juce::String& text) {
        if (text.endsWith ("db")) {
            return text.dropLastCharacters (2).getFloatValue();
        } else
            return text.getFloatValue();
    };

    static inline auto stringFromValue = [] (float value, [[maybe_unused]] int maximumStringLength = 5) {
        // only 1 decimal place for db values
        return juce::String (value, 1);
    };

    static inline auto valueFromString = [] (const juce::String& text) { return text.getFloatValue(); };

    static juce::NormalisableRange<float>
        logarithmicThenLinearRange (const float start, const float end, const float zeroPoint) {
        jassert (zeroPoint >= start && zeroPoint <= end);

        // For rotary knobs, we want 0dB at about 2 o'clock, which is roughly 0.7 of the rotation
        const float breakpointOnSlider = 0.7f;

        // Use better logarithmic scaling for audio with appropriate exponent
        const float exponent = 2.5f; // Reduced from 3.0f for more gradual curve

        auto range = juce::NormalisableRange<float> {
            start,
            end,
            // Convert normalized (0-1) to actual dB value
            [=] (const float start, const float end, const float normalizedValue) {
                if (normalizedValue < breakpointOnSlider) {
                    // First part: logarithmic from start to zeroPoint
                    const auto normalizedX = normalizedValue / breakpointOnSlider;
                    // Use more perceptually appropriate curve for audio levels
                    return start + (std::pow (normalizedX, 1.0f / exponent)) * (zeroPoint - start);
                } else {
                    // Second part: linear from zeroPoint to end
                    const auto normalizedX = (normalizedValue - breakpointOnSlider) / (1.0f - breakpointOnSlider);
                    return zeroPoint + normalizedX * (end - zeroPoint);
                }
            },
            // Convert actual dB value to normalized (0-1)
            [=] (const float start, const float end, const float unnormalizedValue) {
                if (unnormalizedValue < zeroPoint) {
                    // First part: logarithmic from start to zeroPoint
                    const float proportion = (unnormalizedValue - start) / (zeroPoint - start);
                    return breakpointOnSlider * std::pow (proportion, exponent);
                } else {
                    // Second part: linear from zeroPoint to end
                    return breakpointOnSlider
                           + (unnormalizedValue - zeroPoint) / (end - zeroPoint) * (1.0f - breakpointOnSlider);
                }
            },
            // Snap to appropriate increments based on value range
            [=] (const float start, const float end, const float unnormalizedValue) {
                // Very low range (below -40dB): 1.0dB increments
                if (unnormalizedValue < -40.0f)
                    return juce::jlimit (start, end, std::round (unnormalizedValue));

                // Low range (-40dB to -20dB): 0.5dB increments
                if (unnormalizedValue < -20.0f)
                    return juce::jlimit (start, end, std::round (unnormalizedValue * 2.0f) / 2.0f);

                // Critical range (-20dB to end): 0.1dB increments
                return juce::jlimit (start, end, std::round (unnormalizedValue * 10.0f) / 10.0f);
            }
        };

        // Use a very small interval for smooth dragging
        range.interval = 0.001f;

        return range;
    }

    // Zero-cost parameter factory templates - eliminates boilerplate!
    template <auto& ParamID, typename Range>
    constexpr auto makeStandardParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout,
                                                           ParamID,
                                                           name,
                                                           range,
                                                           defaultVal,
                                                           juce::AudioParameterFloatAttributes()
                                                               .withStringFromValueFunction (stringFromValue)
                                                               .withValueFromStringFunction (valueFromString));
        };
    }

    template <auto& ParamID, typename Range, typename Unit = FrequencyUnit::Hz>
    constexpr auto makeFrequencyParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (
                layout,
                ParamID,
                name,
                range,
                defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (makeStringFromValueWithFrequency<Unit>())
                    .withValueFromStringFunction (makeFromStringWithFrequency<Unit>()));
        };
    }

    // Percent parameter: internal value 0-1, displayed as "X.X%" (1 dp). Shows "OFF" at exactly 0.
    template <auto& ParamID, typename Range>
    constexpr auto makePercentParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromPercentValueWithDigits<1>)
                    .withValueFromStringFunction (percentValueFromString));
        };
    }

    template <auto& ParamID, typename Range, typename Unit = FrequencyUnit::Hz>
    constexpr auto makeFrequencyParamWithOff (const char* name, Range range, float defaultVal, float offValue) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (
                layout,
                ParamID,
                name,
                range,
                defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (makeStringFromValueWithFrequencyWithOffAt<Unit> (offValue, 0))
                    .withValueFromStringFunction (makeFromStringWithFrequencyWithOffAt<Unit> (offValue)));
        };
    }

    // ---- Unit-formatting helpers used by the factories below ----

    static inline auto stringFromMsValue = [] (float value, [[maybe_unused]] int maximumStringLength = 0) {
        const float absV = std::abs (value);
        if (absV >= 100.0f)  return juce::String (juce::roundToInt (value)) + " ms";
        if (absV >= 10.0f)   return juce::String (value, 1) + " ms";
        return juce::String (value, 2) + " ms";
    };

    static inline auto msValueFromString = [] (const juce::String& text) {
        auto t = text.toLowerCase().trim();
        if (t.endsWith ("ms")) return t.dropLastCharacters (2).trim().getFloatValue();
        return t.getFloatValue();
    };

    static inline auto stringFromRateHz = [] (float value, [[maybe_unused]] int maximumStringLength = 0) {
        if (value < 1.0f)   return juce::String (value, 3) + " Hz";
        if (value < 10.0f)  return juce::String (value, 2) + " Hz";
        return juce::String (value, 1) + " Hz";
    };

    static inline auto rateHzFromString = [] (const juce::String& text) {
        auto t = text.toLowerCase().trim();
        if (t.endsWith ("hz")) return t.dropLastCharacters (2).trim().getFloatValue();
        return t.getFloatValue();
    };

    static inline auto stringFromRatioValue = [] (float value, [[maybe_unused]] int maximumStringLength = 0) {
        return juce::String (value, 1) + ":1";
    };

    static inline auto ratioValueFromString = [] (const juce::String& text) {
        auto t = text.trim();
        if (t.endsWith (":1")) return t.dropLastCharacters (2).getFloatValue();
        return t.getFloatValue();
    };

    static inline auto stringFromSecondsValue = [] (float value, [[maybe_unused]] int maximumStringLength = 0) {
        return juce::String (value, 2) + " s";
    };

    static inline auto secondsValueFromString = [] (const juce::String& text) {
        auto t = text.toLowerCase().trim();
        if (t.endsWith ("s")) return t.dropLastCharacters (1).trim().getFloatValue();
        return t.getFloatValue();
    };

    static inline auto stringFromDegreesValue = [] (float value, [[maybe_unused]] int maximumStringLength = 0) {
        return juce::String (juce::roundToInt (value)) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0"));
    };

    static inline auto degreesValueFromString = [] (const juce::String& text) {
        auto t = text.trim();
        // Strip a trailing degree sign (UTF-8 0xc2 0xb0) or stray "deg".
        if (t.endsWith (juce::String (juce::CharPointer_UTF8 ("\xc2\xb0"))))
            t = t.dropLastCharacters (1);
        if (t.toLowerCase().endsWith ("deg"))
            t = t.dropLastCharacters (3);
        return t.getFloatValue();
    };

    static inline auto stringFromMultiplierValue = [] (float value, [[maybe_unused]] int maximumStringLength = 0) {
        return juce::String (value, 2) + "x";
    };

    static inline auto multiplierValueFromString = [] (const juce::String& text) {
        auto t = text.toLowerCase().trim();
        if (t.endsWith ("x")) return t.dropLastCharacters (1).getFloatValue();
        return t.getFloatValue();
    };

    static inline auto stringFromBitsValue = [] (float value, [[maybe_unused]] int maximumStringLength = 0) {
        return juce::String (value, 1) + " bits";
    };

    static inline auto bitsValueFromString = [] (const juce::String& text) {
        auto t = text.toLowerCase().trim();
        if (t.endsWith ("bits")) return t.dropLastCharacters (4).trim().getFloatValue();
        if (t.endsWith ("bit"))  return t.dropLastCharacters (3).trim().getFloatValue();
        return t.getFloatValue();
    };

    // ---- Unit factories ----

    template <auto& ParamID, typename Range>
    constexpr auto makeMsParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromMsValue)
                    .withValueFromStringFunction (msValueFromString));
        };
    }

    template <auto& ParamID, typename Range>
    constexpr auto makeRateParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromRateHz)
                    .withValueFromStringFunction (rateHzFromString));
        };
    }

    template <auto& ParamID, typename Range>
    constexpr auto makeRatioParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromRatioValue)
                    .withValueFromStringFunction (ratioValueFromString));
        };
    }

    template <auto& ParamID, typename Range>
    constexpr auto makeSecondsParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromSecondsValue)
                    .withValueFromStringFunction (secondsValueFromString));
        };
    }

    template <auto& ParamID, typename Range>
    constexpr auto makeDegreesParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromDegreesValue)
                    .withValueFromStringFunction (degreesValueFromString));
        };
    }

    template <auto& ParamID, typename Range>
    constexpr auto makeMultiplierParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromMultiplierValue)
                    .withValueFromStringFunction (multiplierValueFromString));
        };
    }

    template <auto& ParamID, typename Range>
    constexpr auto makeBitsParam (const char* name, Range range, float defaultVal) {
        return [=] (auto& layout) -> auto& {
            return addToLayout<juce::AudioParameterFloat> (layout, ParamID, name, range, defaultVal,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction (stringFromBitsValue)
                    .withValueFromStringFunction (bitsValueFromString));
        };
    }
}