# GBDK-2020 Workshop
Repository for the GBDK-2020 workshop conducted for the Retro Development Association at DigiPen.

# Installation

## SDK

This actual installation of the SDK itself is fairly straightforward. The repository is open-source: <https://github.com/gbdk-2020/gbdk-2020>. The latest release build can be downloaded from this page. The zip file can then be extracted to the desired location.

## Make

GBDK runs through LCC (Local C Compiler) which has to be built platform specific based on which OS it runs on. Therefore running it through WSL won’t work since executables don’t usually work the way we want them to on the subsystem. It is best to install make on your native environment to get the best setup. For this workshop we are going to directly use the new windows package manager so just open up the terminal and paste the following command.

```
winget install ezwinports.make
```

After restarting the terminal make sure that the installation was correct by checking the version.

## Files Location

This repository needs to be cloned and placed inside the 'examples\gb' path of the GBDK repo. This is not the ideal way to do this but for the sake of simplicity we shall keep it this way for now.

## Implementation Instructions

The instructions for implementing the snake game for the Game Boy is provided in the [instructions.md](instructions.md) file.
