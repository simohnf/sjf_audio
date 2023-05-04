# sjf_audio


This folder contains a collection of header files comprising classes and functions used within my plugins. 
There are a variety of DSP and interface objects as well as some utility functions.

Uses the [Juce](https://juce.com/) framework.
sjf_fftConvo uses [Apple's Accelerate](https://developer.apple.com/documentation/accelerate) (so is currently MacOS only)
Some of the classes also use the [gcem compile time math library](https://github.com/kthohr/gcem)

...The organisation and some of the code needs some improvement and some of the files just need to be removed...

### OLD NOTES

                    SJF AUDIO
This is a collection of classes and functions used in my VST projects
As I often reuse the same classes in multiple projects I found that this was a convenient(ish) way of organising them
I have included only header files, rather than header and cpp files... sometimes this makes it easier, sometimes it is makes the files longer than they might otherwise be, but I just decided to work this way to keep everything uniform
I have attempted to keep as much of the code necessary for my vsts within these files as possible. This means the actual source for each VST is reliant on these files 
Many of the files still need additional work. e.g. some classes include overloaded functions I used when developing that may, or may not, be useful in future. Some of these will probably be removed in time.
Much of the code could probably do with some refactoring... particularly some of the older classes/functions



##TO DO                    
- refactor sampler... currently not the most logical way of doing things --> would be better to have a simpler basic sampler (with loading and playback etc), and then break out mangler specific code...
- further develop drummachine
- make some enums anytime I'm using switch cases

