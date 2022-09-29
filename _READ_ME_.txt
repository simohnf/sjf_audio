                    SJF AUDIO
This is a collection of classes and functions used in my VST projects
As I often reuse the same classes in multiple projects I found that this was a convenient(ish) way of organising them
I have included only header files, rather than header and cpp files... sometimes this makes it easier, sometimes it is makes the files longer than they might otherwise be, but I just decided to work this way to keep everything uniform
I have attempted to keep as much of the code necessary for my vsts within these files as possible. This means the actual source for each VST is reliant on these files (I've used #include to reference the relevant files, but the paths are, of course, based on the folder hierarchy in my computer)
Many of the files still need aditional work. e.g. some calasses include overloaded functions I used when developing that may, or may not, be useful in future. Some of these will probably be removed in time.
Much of the code could probably do with some refactoring... particularly some of the older classes/functions



TO DO
- refactor sampler... currently not the most logical way of doing things --> would be better to have a simpler basic sampler (with loading and playback etc), and then brekak out mangler specific code...
- further develop drummachine
- make some enums anytime I'm using switch cases
