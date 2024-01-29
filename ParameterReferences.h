#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace moiraesoftware
{
    using namespace juce;
    using Parameter = AudioProcessorValueTreeState::Parameter;
    using Attributes = AudioProcessorValueTreeStateParameterAttributes;

    template <typename Param>
    static void add (AudioProcessorParameterGroup& group, std::unique_ptr<Param> param)
    {
        group.addChild (std::move (param));
    }

    template <typename Param>
    static void add (AudioProcessorValueTreeState::ParameterLayout& group, std::unique_ptr<Param> param)
    {
        group.add (std::move (param));
    }

    template <typename Param, typename Group, typename... Ts>
    static Param& addToLayout (Group& layout, Ts&&... ts)
    {
        auto param = new Param (std::forward<Ts> (ts)...);
        auto& ref = *param;
        add (layout, rawToUniquePtr (param));
        return ref;
    }

    static String getPanningTextForValue (float value)
    {
        if (value == 0.5f)
            return "< c >";

        if (value < 0.5f)
            return String (roundToInt ((0.5f - value) * 200.0f)) + "%L";

        return String (roundToInt ((value - 0.5f) * 200.0f)) + "%R";
    }

    static float getPanningValueForText (String strText)
    {
        strText = strText.trim().toLowerCase();

        if (strText == "center" || strText == "c" || strText == "< c >")
            return 0.5f;

        if (strText.endsWith ("%l"))
        {
            float percentage = strText.substring (0, strText.indexOf ("%")).getFloatValue();
            return (100.0f - percentage) / 100.0f * 0.5f;
        }

        if (strText.endsWith ("%r"))
        {
            float percentage = strText.substring (0, strText.indexOf ("%")).getFloatValue();
            return percentage / 100.0f * 0.5f + 0.5f;
        }

        return 0.5f;
    }

    static String valueToText (float x, int) { return { x, 2 }; }

    static float textToValue (const String& str) { return str.getFloatValue(); }

    static auto getBasicAttributes()
    {
        return Attributes().withStringFromValueFunction (valueToText).withValueFromStringFunction (textToValue);
    }

    static auto getDbAttributes()
    {
        return getBasicAttributes().withLabel ("dB");
    }

    static auto getMsAttributes()
    {
        return getBasicAttributes().withLabel ("ms");
    }

    static auto getHzAttributes()
    {
        return getBasicAttributes().withLabel ("Hz");
    }

    static auto getPercentageAttributes()
    {
        return getBasicAttributes().withLabel ("%");
    }

    static auto getRatioAttributes()
    {
        return getBasicAttributes().withLabel (":1");
    }

    static String valueToTextPan (float x, int)
    {
        return getPanningTextForValue ((x + 100.0f) / 200.0f);
    }

    static float textToValuePan (const String& str)
    {
        return getPanningValueForText (str) * 200.0f - 100.0f;
    }

    static auto getPanningAttributes()
    {
        return Attributes()
            .withStringFromValueFunction (valueToTextPan)
            .withValueFromStringFunction (textToValuePan);
    }

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