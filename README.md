# ParameterHelpers

A JUCE module (`parameter_helpers`, namespace `moiraesoftware`) for declaring `AudioProcessorValueTreeState` parameters and binding them to UI controls. Add it with `juce_add_module` after JUCE (`CMakeLists.txt` provides the `MoiraeSoftware::ParameterHelpers` alias) and include `parameter_helpers.h`. `ParameterReferences.h` includes `melatonin_parameters/melatonin_parameters.h`, so that module must also be on the include path.

**Declaring parameters** (`ParameterReferences.h`):

- `addToLayout<Param>(layout, args...)` constructs a parameter, adds it to a `ParameterLayout` or `AudioProcessorParameterGroup`, and returns a reference to it.
- The `make*Param<ID>(name, range, default)` factories return a callable that takes the layout and adds a float parameter with matching text conversion: `makeStandardParam`, `makeDBParam`, `makeFrequencyParam`, `makeFrequencyParamWithOff` (an extra `offValue` that displays as "OFF"), `makePercentParam`, `makeMsParam`, `makeRateParam`, `makeRatioParam`, `makeSecondsParam`, `makeDegreesParam`, `makeMultiplierParam`, `makeBitsParam`. `ID` is a reference to a `juce::ParameterID`:

```cpp
lowCut { moiraesoftware::makeFrequencyParamWithOff<ID::lowCut>(
    "LowCut", juce::NormalisableRange<float>(19.0f, 220.0f, 1.0f), 84.0f, 19.0f)(layout) }
```

**Listening** (`ParameterListener.h`): `ParameterListenerManager<N>` registers one listener on N parameter IDs and sets a shared `std::atomic<bool>` when any of them changes; it unregisters in its destructor.

**UI binding** (`UIHelpers.h`): each `Attached*` component owns its control and a JUCE parameter attachment, and derives from `ComponentWithParamMenu`, which shows the host's context menu for the parameter on right-click. `AttachedSlider` (suffix display via `SuffixDisplay` or a `SuffixStrategy`), `AttachedToggle`, `AttachedCombo`, `AttachedImageButton`, `AttachedRadioButtons` and `AttachedCycler<Component>` (steps a choice parameter forward and back through a custom component). `RadioButtonParameterAttachment` binds a group of buttons to a choice or int parameter, by button index or by component ID (`RadioButtonParameterType`).
