Audio Sample Splitter
===

The audio sample splitter takes in audio data and exports .wav files of singular instances of a musical instrument, such as a single note on a piano or a single hit of a snare drum. The seperated audio files can then be assigned to midi instruments. Live instruments are often recorded in one long .wav file where, for example, each drum in a set would be played by itself and allowed to ring out before the next is played. Typically an audio engineer would use a DAW to manually go through the .wav file (which is sometimes an hour or more) to split and export each sample manually.

The audio sample splitter operates in two distinct modes; live and non-live. In non-live mode the user simply inputs the .wav file upon instantiation, splits it and exports all or some of the samples. In live mode, The sample splitter operates in conjunction with [Elma](http://klavinslab.org/elma) to export samples as the audio data is being fed in.

The source code for this project is available [on github](https://github.com/jonathansolheim/Audio_Sample_Splitter).

Installation
---

    git clone https://github.com/jonathansolheim/Audio_Sample_Splitter.git
    cd elma_project
    docker run -v $PWD:/source -it cppenv-http bash
    make
    make docs

Testing
---
The test uses an included parent audio file of distinct synth drum sounds and seperates them using non-live and live mode.
To run tests, do
```bash
test/bin/test
```

Architecture
---
I designed the sample splitter [Elma](http://klavinslab.org/elma) process by first creating the non-live version. Using Adam Stark's [AudioFile library](https://github.com/adamstark/AudioFile), I defined the SampleSplitter class to require the user to name a file to be split into samples. This file is loaded on instantiation and the user can then split and export the samples by calling the appropriate functions. Whenever splitting, the user is required to input a threshold and a grace period. The split function works by looping through the audio data and recording to a buffer only if the data surpases the user defined threshold. The function will not detect another "threshold surpassed" until the user defined grace period is up. If the grace period is too short the function will read the same instrument instance as multiple. After the grace period is up, another recording will not start until the threshold has been passed once again. When this happens the previous recording is terminated and exported and the cycle continues. At the end of the audio file loop, the remaining data in the buffer is exported as the final sample. Exporting the remaining data is exclusive to non-live mode.

In live mode more care must be taken by the user to ensure the sample splitter works properly. When instantiating in live-mode, the user provides the threshold and gracetime upfront, though they can be changed during runtime with their corresponding set functions. The user is also required to input the number of updates between each export attempt. Attempting to export a sample every update would be computationally costly and nobody wants samples that are milliseconds long anyway. The right number of updates between each export attempt depends on the update speed which in turn depends on the sample rate and buffer size of the recording source.

To simulate a recording device I created a live recording simulator [Elma](http://klavinslab.org/elma) process. The user provides an audio file and a buffer size upon instantiation. The live recording simulator then splits up the audio file into data packets and sends them as json values through left and right audio channels. The frequency at which this happens depends on the user input but in reality it would be dependent on the sample rate and the buffer size of the recording device. However, the user must be sure to update the live recording simulator and the sample splitter at the same rate to ensure the sample splitter recieves every update.

Once the sample splitter has recieved the json data, it converts it back into audio data and adds it to a backlog. Every user defined number of updates, the sample splitter attempts to split and export what it has collected in its backlog. If it finds a sample to export it does so. It only knows to export a file if its threshold is passed, the grace period is passed and another threshold is passed. The onset of another sample lets the sample splitter know that the current sample has been completed. The left over data is kept in the backlog so that when more data arrives that sample can be exported as well.

Results
---
All tests run successfully. For the provided example file, I found the optimal threshold and grace time through trial and error to be .1 and 3 respectively. I used a buffer size of 1024 because that is a common size in applications where latency is not an issue (like exporting .wav files). The provided audio file uses a sample rate of 44100Hz, and so to simulate a live recording I sent 1024 samples every 23219us. I chose the number of updates between each export attempt to be 30 because that roughly translates to 2 export attempts per second. I had to run the sample splitter for slightly longer than the sound file to ensure all files exported. Some latency is to be expected when attempting to read, split up and write audio files as fast as you recieve them.

The testing of the non-live mode was more straight forward. I instantiated the sample splitter with the provided sound file, split and exported its contents. There are several ways to export, but I only included one in the test for the sake of your file clean up. However, they have all been tested successfully. The user has the option to split and export all in one function or simply split. Once a file is split, the user can export a single sample or all the samples. The function to export all samples is overwritten to allow the user to provide names for the files if they so choose. If not, the samples will be named sample_1, sample_2 etc. Since the live recording test exports sample_1, sample_2..., I chose to demonstrate the file naming function of non-live mode.

Acknowledgements
---
Thank you to Dr. Klavins, Justin Vrana and Henry Lu for teaching me the ways of C++ and a myriad of auxiliary programs.

References
---
I used the following libraries to build the sample splitter:
[Adam Stark's AudioFile library](https://github.com/adamstark/AudioFile)
[Dr. Klavin's Elma](http://klavinslab.org/elma)
