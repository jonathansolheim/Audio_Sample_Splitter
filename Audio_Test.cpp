#include <stdlib.h>
#include <stdexcept>
#include <iostream>
//#include "elma.h"
#include <math.h>
#include <stdio.h>
#include "AudioFile.h"
#include <fstream>
#include <vector>
#include <iostream> 
#include <string>

//------------------------------------------------------------
// The minimum and maximum functions are good for
// getting an idea of what threshold value you may want to use
//------------------------------------------------------------
double get_max(AudioFile<double> audioFile){
    double max = -2;
    // Run through audio file
    for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++)
    {
        if (audioFile.samples[0][i]> max){
            max = audioFile.samples[0][i];
        }
    }
    return max;
}
double get_min(AudioFile<double> audioFile){
    double min = 2;
    // Run through audio file
    for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++)
    {
        if (audioFile.samples[0][i]< min){
            min = audioFile.samples[0][i];
        }
    }
    return min;
}

// Exports first half of audio file
void export_first_half( AudioFile<double> audioFile) {
    // Find half of number of samples in audio file
    int half_num_samps = audioFile.getNumSamplesPerChannel()/2;
    // Make buffer
    AudioFile<double>::AudioBuffer buffer;
    // Two channels
    buffer.resize (2);
    // Run through audio file
    for (int i = 0; i < half_num_samps; i++)
    {
        // audioFile.samples[channel][index]
        buffer[0].push_back(audioFile.samples[0][i]);
        buffer[1].push_back(audioFile.samples[1][i]);

    }

    AudioFile<double> output_file;
    // Give new audio file the properties of old audio file.
    output_file.setBitDepth (audioFile.getBitDepth());
    output_file.setSampleRate (audioFile.getSampleRate());
    output_file.setAudioBuffer (buffer);
    output_file.save ("Half_Size.wav");
}

// Splits and exports audio samples
// parameters:
// audioFile = the audiofile you want to split into samples as defined by AudioFile.h
// threshold = between 0 and 1, a new sample is triggered when audio surpasses threshold
// grace_time = after method starts to record a sample it won't be able to start another
//              until the grace time is up (in seconds). This prevents the method from
//              starting new recordings while the current sound is just "still being loud."
// export_files = whether or not you want to actually export files or just see how many 
//                would have been exported if you had. Good for trying out settings.
void split_and_export_samples(AudioFile<double> audioFile, double threshold, double grace_time, bool export_files){

    // Make buffer
    AudioFile<double>::AudioBuffer buffer;
    buffer.resize (2);
    bool recording = false;     // Is the buffer recording?
    int grace_sample_num = (int) (audioFile.getSampleRate()*grace_time);
    int grace_period = 0; // grace samples to be counted down
    int file_number = 1; 
    std::string file_name;
    AudioFile<double> output_file;
    output_file.setBitDepth (audioFile.getBitDepth());
    output_file.setSampleRate (audioFile.getSampleRate());
    // Run through audio file
    for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++)
    {

        if ((audioFile.samples[0][i]>threshold || audioFile.samples[1][i]>threshold)
            && grace_period <= 0 && !recording){
            recording = true;
            grace_period = grace_sample_num;
        } else if ((audioFile.samples[0][i]>threshold || audioFile.samples[1][i]>threshold)
            && grace_period <= 0 && recording){
                if(export_files){
                    file_name = "sample_" + std::to_string(file_number) + ".wav";
                    output_file.setAudioBuffer (buffer);
                    output_file.save (file_name);
                    buffer.clear();
                    buffer.resize (2);
                }
            file_number++;
            grace_period = grace_sample_num;
        } 

        if(recording){

        // audioFile.samples[channel][index]
        buffer[0].push_back(audioFile.samples[0][i]);
        buffer[1].push_back(audioFile.samples[1][i]);

        }
        grace_period--;
    }
    // Export last sample
    if(export_files){
        file_name = "sample_" + std::to_string(file_number) + ".wav";
        output_file.setAudioBuffer (buffer);
        output_file.save (file_name);
        buffer.clear();
        buffer.resize (2);
    }


    if (export_files){
        std::cout << "Exported " << (file_number) << " sample files." << std::endl;
    } else {
        std::cout << "Would have exported " << (file_number) << " sample files." << std::endl;
    }

}


// ==================== MAIN ======================
int main(){
std::string filename = "Drum_Samples_to_be_Split.wav";
std::cout << filename << std::endl;

AudioFile<double> audioFile;
audioFile.load (filename);
double max = get_max(audioFile);
double min = get_min(audioFile);
std::cout<<"Max: " <<max<<std::endl;
std::cout<<"Min: " <<min<<std::endl;

// For "Drum_Samples_to_be_Split.wav" the following settings work well
// threshold = .01, grace_time = 3
double threshold = .01; // From 0 to 1
double grace_time = 3; // in seconds
split_and_export_samples(audioFile,threshold,grace_time,true);


return 0;
}
