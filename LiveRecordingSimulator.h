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
#include <fstream>
#include <vector>
#include "channel.h"

using namespace std;
using namespace elma;

//! Reads .wav file and sends audio data packets in left and right channels, 1 json array per update.
//! Its purpose is to simulate an audio recording device, streaming live.
class LiveRecordingSimulator : public Process {

    public:
    
    //! Calls for the .wav file to be prepared for shipment as soon as it is instantiated.
    //! \param filename The .wav file to be read.
    //! \param buffer_size The size of the data packets to be sent.
    LiveRecordingSimulator(std::string filename, int buffer_size):Process("live recording simulator") 
    {
        audioFile.load (filename);
        bit_depth = audioFile.getBitDepth();
        sample_rate = audioFile.getSampleRate();
        bs = buffer_size;
        split_audio_into_packets();
    };

    void init() {}
    //! Tells the manager its bit depth and sample rate when initialized.
    void start() {
        emit(Event("set bit depth", bit_depth));
        emit(Event("set sample rate", sample_rate));
    }
    //! Every update sends the next json audio data packet through the left and right channels.
    void update() {
        if (i < left_data_packets.size()){
            channel("left audio").send(left_data_packets[i]);
            channel("right audio").send(right_data_packets[i]);
            i++;
        }
    }
    void stop() {}

    //! Splits the audio data from the .wav file into buffer sized packets.
    //! Converts the data packets to json and adds them to an array of json packets. 
    void split_audio_into_packets();

    private:
    //! Packet # update function uses to send next packet.
    int i = 0;
    AudioFile<double> audioFile;
    double bit_depth;
    double sample_rate;
    vector<json> left_data_packets;
    vector<json> right_data_packets;
    //! User defined buffer size.
    int bs;
};

void LiveRecordingSimulator::split_audio_into_packets(){

    int buffer_size_counter = 0;
    json left_buffer;
    json right_buffer;

    for (int i = 0; i < audioFile.getNumSamplesPerChannel(); i++){
        if(buffer_size_counter < bs){
            left_buffer.push_back(audioFile.samples[0][i]);
            right_buffer.push_back(audioFile.samples[1][i]);
            buffer_size_counter++;
        } else {
            left_data_packets.push_back(left_buffer);
            right_data_packets.push_back(right_buffer);
            left_buffer.clear();
            right_buffer.clear();
            buffer_size_counter = 0;
        }
    }

    // Add the last data packet
    left_data_packets.push_back(left_buffer);
    right_data_packets.push_back(right_buffer);
    left_buffer.clear();
    right_buffer.clear();
    buffer_size_counter = 0;

}