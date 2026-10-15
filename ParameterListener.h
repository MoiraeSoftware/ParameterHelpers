#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace moiraesoftware
{

    struct ParameterListener : juce::AudioProcessorValueTreeState::Listener
    {
        explicit ParameterListener (std::atomic<bool>& needsUpdate) : updateNeeded (needsUpdate) {}

        void parameterChanged (const juce::String& parameterID, float newValue) override
        {
            updateNeeded.store (true);
        }

        std::atomic<bool>& updateNeeded;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterListener)
    };

    template <std::size_t N>
    class ParameterListenerManager
    {
    public:
        ParameterListenerManager (juce::AudioProcessorValueTreeState& state,
            const std::array<const juce::ParameterID*, N>& channelParameterIds,
            std::atomic<bool>& update)
            : apvts_ (state),
              parameterIds (channelParameterIds),
              listener (update)
        {
            for (const auto& param : parameterIds)
            {
                if (param)
                {
                    apvts_.addParameterListener (param->getParamID(), &listener);
                }
            }
        }

        ~ParameterListenerManager()
        {
            for (const auto param : parameterIds)
            {
                if (param)
                {
                    apvts_.removeParameterListener (param->getParamID(), &listener);
                }
            }
        }

    private:
        juce::AudioProcessorValueTreeState& apvts_;
        const std::array<const juce::ParameterID*, N>& parameterIds;
        ParameterListener listener;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterListenerManager)
    };
}