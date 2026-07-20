```
███████╗     ██╗███████╗    █████╗ ██╗   ██╗██████╗ ██╗ ██████╗
██╔════╝     ██║██╔════╝   ██╔══██╗██║   ██║██╔══██╗██║██╔═══██╗
███████╗     ██║█████╗     ███████║██║   ██║██║  ██║██║██║   ██║
╚════██║██   ██║██╔══╝     ██╔══██║██║   ██║██║  ██║██║██║   ██║
███████║╚█████╔╝██║███████╗██║  ██║╚██████╔╝██████╔╝██║╚██████╔╝
╚══════╝ ╚════╝ ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝
```

A high-performance C++20 DSP utility library designed specifically for the **JUCE** framework. This library provides compile-time abstractions to eliminate allocation overhead, structure hierarchical plugin parameters, and maximize compiler vectorization via automated fast-path branching.

---

## Key Architectural Concepts

*   **Fast-Path Evaluation:** State changes are evaluated once per *block*. If parameters are stationary, the engine routes processing to a completely static lane, allowing the compiler to auto-vectorize loops.
*   **Zero Runtime-Allocation Chains:** Sequential processing chains are unrolled at compile time using C++ variadic templates, completely eliminating runtime overhead or virtual function table penalties.
*   **Strict Parameter Namespacing:** Hierarchical parameter tree building eliminates ID collisions across complex multi-module setups while preserving clean automation paths in the host DAW.

---

## Component Overview

| Component                 | Purpose | Technical Highlights                                                                                                                                                                                                                                                          |
|:--------------------------| :--- |:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **`ParameterFactory`**    | Strict hierarchical parameter generation | Extends `juce::AudioProcessorParameterGroup` to safely map and isolate nested sub-module automation strings.                                                                                                                                                                  |
| **`AudioParametersBase`** | State management & divergence engine | Decouples the audio thread from host parameter updates. Enables DSP fast-pathing via block-rate divergence checks, bypassing redundant calculations when values are static. Automatically handles parameter mapping and linear smoothing for artifact-free float transitions. |
| **`ProcessorSequence`**   | Compile-time serial processing chain | Binds an arbitrary series of DSP blocks into a single cache-friendly `std::tuple`. Unrolls all lifecycle steps at compile time.                                                                                                                                               |
| **`OversamplingWrapper`** | Dynamic runtime oversampling | Wraps any DSP processor to add dynamic, automated upsampling/downsampling channels mid-stream based on parameter targets.                                                                                                                                                     |
| **`ChunkedWrapper`**      | Bounded block segmentation layout | Subdivides arbitrary host processing audio buffers into manageable chunks matching a strict maximum length constraint. Ideal for stabilizing feedback loops, maintaining small-cache consistency, or optimizing SIMD loops.                                                   |

---

## Quick Start Example

Below is a brief look at how these architectural pieces interconnect inside a standard JUCE processor.

### 1. Define Sub-Modules & Parameters
```cpp
struct DistortionModule
{
    struct Parameters : sjf::helpers::AudioParametersBase
    {
        FloatState drive;

        std::unique_ptr<ParameterFactory> createParameters(const juce::String& id, const juce::String& name) override {
            auto factory = ParameterFactory::create(id, name);
            // Tracks drive and maps 0-100% smoothly
            createTrackedParameter(*factory, drive, "Drive", "Drive Amount", {0.0f, 1.0f}, 0.5f);
            return factory;
        }
    } parameters;

    void prepare(const juce::dsp::ProcessSpec& spec) {}
    void reset() {}
    
    template <typename Context>
    void process(const Context& context) {
        // Implement fast-path optimized or smoothed path using parameters.drive.currentValue
    }
    
    std::unique_ptr<ParameterFactory> createParameters(const juce::String& id, const juce::String& name)
    {
        return parameters.createParameters(id, name);
    }
};
```

### 2. Compose the Master Pipeline
   Using ProcessorSequence and OversamplingWrapper, you can cleanly chain individual components together while auto-generating a complex nested parameter tree.
   
```c++
using namespace sjf::helpers;

// A sequence chaining an oversampled distortion module into a standard delay
using MyDSPChain = ProcessorSequence<
    OversamplingWrapper<DistortionModule>,
    Delay
>;

class MyAudioProcessor : public juce::AudioProcessor
{
    MyDSPChain dspChain;
    std::unique_ptr<ParameterFactory> rootParameters;

public:
    MyAudioProcessor() 
    {
        // Recursively structures the hierarchy for the host DAW:
        // Main Group -> Distortion (with 16x Oversampling controls) -> Delay Group
        rootParameters = dspChain.createParameters("MainID", "MyPlugin",
            processor_sequence::NestedConfig(
                processor_sequence::SubFactoryConfig{"Dist", "Distortion"}
            ),
            processor_sequence::SubFactoryConfig{"Delay", "Echo Delay"}
        );
        
        // Pass the structural layout to APVTS or AudioProcessor
        addParameterGroup(std::move(rootParameters));
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getTotalNumOutputChannels() };
        dspChain.prepare(spec);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        
    if ( auto playHead = getPlayHead())
    {
        auto positionInfo = playHead->getPosition();
        if (positionInfo.hasValue())
            processor.setPositionInfo(positionInfo);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    processor.process(context);
    }
};
```

---

### Integration Requirements
Language Standard: C++20 or higher.

Framework Dependency: JUCE 7+.

Compiler Target: Highly compatible with optimization flags (-O3, /O2, -ffast-math) to take full advantage of static-loop unrolling.