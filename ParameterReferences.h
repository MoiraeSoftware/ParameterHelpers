#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "melatonin_parameters/melatonin_parameters.h"

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
        return [hzDecimalPlaces, khzDecimalPlaces] (float value) -> juce::String {
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
}