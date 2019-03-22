#include "httplib/httplib.h"
#include "json/json.h"
#include <iostream>
#include <ctime>
#include <map>
#include <string>
#include <chrono>
#include "elma.h"
#include "client.h"
#include <stdlib.h>
#include <stdexcept>
#include <math.h>
#include <stdio.h>
#include "AudioFile.h"
#include "channel.h"
#include <fstream>
#include <vector>

using namespace std;
using namespace elma;

//! Takes audio data, splits it and exports it as several audio samples.
class SampleSplitter : public Process {

    public:
    //! The live mode instantiator.
    //! Use this instantiator if you wish to use the sample splitter to split an audio file being recorded live.
    //! The export attempt time is how often you would like the sample splitter to attempt to export a new sample in seconds.
    //! \param threshold The minimum amplitude that must be surpased to start recording a single sample.
    //! \param grace_time The minimum amount of time after one sample begins recording before another can start to be recorded.
    //! \param updates_per_export_attempt The number of updates between each attempt to export a sample.
    SampleSplitter(double threshold, double grace_time, double updates_per_export_attempt):Process("sample splitter") {
        upea = updates_per_export_attempt;
        th = threshold;
        gt = grace_time;
        backlog.resize (2);
        live = true;
    };
    //! The non-live mode instantiator
    //! Use this instantiator if you wish to use the sample splitter to split a pre recorded .wav file.
    //! \param filename The .wav file to be read.
    SampleSplitter(std::string filename):Process("sample splitter") 
    {
        audioFile.load (filename);
        live = false;
    };

    //! Listens to the manager for its bit depth and sample rate when initialized.
    void init() {
        watch("set bit depth", [this](Event& e) {
            bit_depth = e.value();
        });
        watch("set sample rate", [this](Event& e) {
            sample_rate = e.value();
        });
    }
    void start() {}

    //! Every update recieves the next json audio data packet through the left and right channels.
    //! Reads the json audio packet.
    //! Attempts to export a sample if the grace time and the user defined number updates since the last attempt has been surpassed.
    void update() {
        
        if ( channel("left audio").nonempty() && channel("right audio").nonempty() ) {

            read_data_packet(channel("left audio").latest(), channel("right audio").latest());
            
            if (backlog[0].size() > (int) (gt*sample_rate) && upea_counter > upea){
                attempt_live_export(th, gt);
                upea_counter = 0;
            }
            upea_counter++;
        }
    }
    void stop() {}
    
    //! \param threshold The minimum amplitude that must be surpased to start recording a single sample.
    void set_threshold(double threshold);

    //! \param grace_time The minimum amount of time after one sample begins recording before another can start to be recorded.
    void set_grace_time(double grace_time);

    // --------- non-live mode functions -------------------------------------------------

    //! For non-live mode use only.
    //! \return The number of samples in the user provided .wav file.
    double number_of_samples();

    //! For non-live mode use only.
    //! \return The maximum value in the user provided .wav file.
    double get_max();

    //! For non-live mode use only.
    //! \return The minimum value in the user provided .wav file.
    double get_min();

    //! For non-live mode use only.
    //! Splits and stores the user provided .wav file into samples.
    //! \param threshold The minimum amplitude that must be surpased to start recording a single sample.
    //! \param grace_time The minimum amount of time after one sample begins recording before another can start to be recorded.
    void split_samples(double threshold, double grace_time);

    //! For non-live mode use only.
    //! Should only be called after the .wav file has been split.
    //! Exports a stored sample of the user's choosing.
    //! \param sample_number The 1 based index of the sample the user wishes to export.
    //! \param file_name The title the user wishes name the .wav sample file export.
    void export_sample(double sample_number, std::string file_name);

    //! For non-live mode use only.
    //! Should only be called after the .wav file has been split.
    //! Exports all stored samples and names them sample_1, sample_2, etc.
    void export_all_samples();

    //! For non-live mode use only.
    //! Should only be called after the .wav file has been split.
    //! Exports all stored samples and names them according to user's input
    //! \param file_names The titles the user wishes name the .wav sample file exports.
    void export_all_samples(vector<std::string> file_names);

    //! For non-live mode use only.
    //! Can be called before the .wav file has been split.
    //! This function is the quickest and dirtiest approach.
    //! In exchange for the variability of seperate split and export functions, it provides simplicity.
    //! Splits, names and exports files sample_1, sample_2, etc.
    //! \param threshold The minimum amplitude that must be surpased to start recording a single sample.
    //! \param grace_time The minimum amount of time after one sample begins recording before another can start to be recorded.
    //! \param export_files A quick on/off switch to enable or disable exporting files. 
    void split_and_export_samples(double threshold, double grace_time, bool export_files);

    //----------- live mode functions ----------------------------------------------------

    //! For live mode use only.
    //! Reads the json data from the audio channels and saves it to a backlog.
    //! \param left_data The left speaker audio data in json form.
    //! \param right_data The right speaker audio data in json form.
    void read_data_packet(json left_data, json right_data);

    //! For live mode use only.
    //! Attempts to export a sample from the backlog.
    //! If it does export a sample, the left over data is kept in the backlog to be used in the next sample.
    //! \param threshold The minimum amplitude that must be surpased to start recording a single sample.
    //! \param grace_time The minimum amount of time after one sample begins recording before another can start to be recorded.
    void attempt_live_export(double threshold, double grace_time);


    private:
    
    //! Is the sample splitter in live mode?
    bool live;

    double bit_depth;

    double sample_rate;

    //! Updates per export attempt.
    int upea;

    //! counts the number of updates since the last update attempt.
    int upea_counter = 0;

    //! Threshold for live mode.
    double th = 0;

    //! Grace time for live mode.
    double gt = 0;

    //! The saved audio data, waiting to be exported.
    AudioFile<double>::AudioBuffer backlog;

    AudioFile<double> audioFile;

    //! The list of samples waiting to be exported in non-live mode.
    vector<AudioFile<double>::AudioBuffer> sample_list;

    //! Keeps track of sample number for naming exports.
    int export_number = 1;
    
};



void SampleSplitter::set_threshold(double threshold){
    th = threshold;
}

void SampleSplitter::set_grace_time(double grace_time){
    gt = grace_time;
}

// --------- non-live mode functions -------------------------------------------------------------------
double SampleSplitter::number_of_samples(){
    if(live){
        std::cout << "Can't find number of samples in live mode." <<std::endl;
    } else{
        return sample_list.size();
    }
}

double SampleSplitter::get_max(){
    if(live){
        std::cout << "Can't find max in live mode." <<std::endl;
        return 0;
    } else {
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
}
double SampleSplitter::get_min(){
    if(live){
        std::cout << "Can't find min in live mode." <<std::endl;
        return 0;
    } else {
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
}

void SampleSplitter::split_and_export_samples(double threshold, double grace_time, bool export_files){
    if(live){
        std::cout << "Can't manually split and export samples in live mode" << std::endl;
    } else {
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
}

void SampleSplitter::split_samples(double threshold, double grace_time){
    if(live){
        std::cout << "Can't manually split samples in live mode" << std::endl;
    } else {
        // Make buffer
        AudioFile<double>::AudioBuffer buffer;
        buffer.resize (2);
        bool recording = false;     // Is the buffer recording?
        int grace_sample_num = (int) (audioFile.getSampleRate()*grace_time);
        int grace_period = 0; // grace samples to be counted down
        int file_number = 1; 
        std::string file_name;
        // Run through audio file
        for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++)
        {

            if ((audioFile.samples[0][i]>threshold || audioFile.samples[1][i]>threshold)
                && grace_period <= 0 && !recording){
                recording = true;
                grace_period = grace_sample_num;
            } else if ((audioFile.samples[0][i]>threshold || audioFile.samples[1][i]>threshold)
                && grace_period <= 0 && recording){

                file_name = "sample_" + std::to_string(file_number) + ".wav";
                sample_list.push_back(buffer);
                buffer.clear();
                buffer.resize (2);

                file_number++;
                grace_period = grace_sample_num;
            } 

            if(recording){

            buffer[0].push_back(audioFile.samples[0][i]);
            buffer[1].push_back(audioFile.samples[1][i]);

            }
            grace_period--;
        }
        // Export last sample

        file_name = "sample_" + std::to_string(file_number) + ".wav";
        sample_list.push_back(buffer);
        buffer.clear();
        buffer.resize (2);

        std::cout << "Split " << (file_number) << " sample files." << std::endl;
    }


}

void SampleSplitter::export_sample(double sample_number, std::string file_name){
    if (live){
        std::cout << "Can't export a specific samples in live mode" << std::endl;
        std::cout << "No samples were exported" << std::endl;
    } else {
        if (sample_list.size() == 0){
            std::cout << "No samples to export" << std::endl;
            std::cout << "Did you split the original file into samples first?" << std::endl;

        } else if(sample_number > 0 && sample_number <= sample_list.size()){
            AudioFile<double> output_file;
            output_file.setBitDepth (audioFile.getBitDepth());
            output_file.setSampleRate (audioFile.getSampleRate());
            output_file.setAudioBuffer (sample_list.at(sample_number-1));
            output_file.save (file_name);
            std::cout << file_name << " was exported." << std::endl;
        } else{
            std::cout << "There are " << sample_list.size() << " samples." << std::endl;
            std::cout << "You asked for sample number " << sample_number << std::endl;
            std::cout << "No sample was exported" << std::endl;
        }
    }
}

void SampleSplitter::export_all_samples(){
    if(live){
        std::cout << "Can't export all samples in live mode" << std::endl;
        std::cout << "No samples were exported" << std::endl;
    } else {
        if (sample_list.size() == 0){
            std::cout << "No samples to export" << std::endl;
            std::cout << "Did you split the original file into samples first?" << std::endl;
            std::cout << "No samples were exported" << std::endl;
        } else{
            std::string file_name;
            for (int i = 1; i <= sample_list.size(); i++){
                file_name = "sample_" + std::to_string(i) + ".wav";
                export_sample(i,file_name);
            }
        }
    }
}

void SampleSplitter::export_all_samples(vector<std::string> file_names){
    if(live){
        std::cout << "Can't manually export all samples in live mode" << std::endl;
        std::cout << "No samples were exported" << std::endl;
    } else{
        if (sample_list.size() == 0){
            std::cout << "No samples to export" << std::endl;
            std::cout << "Did you split the original file into samples first?" << std::endl;
            std::cout << "No samples were exported" << std::endl;
        } else if(sample_list.size() != file_names.size()){
            std::cout << "The number of file names does not match the number of samples." << std::endl;
            std::cout << "No samples were exported" << std::endl;
        } else {
            for (int i = 1; i <= sample_list.size(); i++){
                export_sample(i,file_names.at(i-1));
            }
        }
    }
}

// --------- live mode functions ------------------------------------------------------------------------

void SampleSplitter::attempt_live_export(double threshold, double grace_time){
    if(live){
        // Make buffer
        AudioFile<double>::AudioBuffer buffer;
        buffer.resize (2);
        bool recording = false;     // Is the buffer recording?
        int grace_sample_num = (int) (sample_rate*grace_time);
        int grace_period = 0; // grace samples to be counted down
        int file_number = 1; 
        std::string file_name;
        AudioFile<double> output_file;
        output_file.setBitDepth (bit_depth);
        output_file.setSampleRate (sample_rate);
        // Run through backlog 
        for (int i = 0; i < backlog[0].size(); i++)
        {

            if ((backlog[0][i]>threshold || backlog[1][i]>threshold)
                && grace_period <= 0 && !recording){
                recording = true;
                grace_period = grace_sample_num;
            } else if ((backlog[0][i]>threshold || backlog[1][i]>threshold)
                && grace_period <= 0 && recording){

                file_name = "sample_" + std::to_string(export_number) + ".wav";
                output_file.setAudioBuffer (buffer);
                output_file.save (file_name);
                std::cout << "Exported " << file_name << std::endl;
                buffer.clear();
                buffer.resize (2);

                export_number++;
                grace_period = grace_sample_num;
            } 

            if(recording){

            buffer[0].push_back(backlog[0][i]);
            buffer[1].push_back(backlog[1][i]);

            }
            grace_period--;
        }

        // The last "sample" becomes the new backlog
        // I do this so that I don't export incomplete samples
        // The result is that the last sample won't export until another sample recording has been triggered
        // Thus to make sure your last sample exports make a loud noise to trigger the end of that sample.
        backlog.clear();
        backlog.resize (2);
        for(int i = 0; i < buffer[0].size(); i++){
            backlog[0].push_back(buffer[0][i]);
            backlog[1].push_back(buffer[1][i]);
        }
    } else {
        std::cout << "attempt_live_export is a live mode exclusive function" << std::endl;        
    }
}

void SampleSplitter::read_data_packet(json left_data, json right_data){
    if(live){
        for (int i = 0; i < left_data.size(); i++){
            backlog[0].push_back(left_data[i]);
            backlog[1].push_back(right_data[i]);
        }
    } else {
        std::cout << "read_data_packet is a live mode exclusive function" << std::endl;    
    }
}