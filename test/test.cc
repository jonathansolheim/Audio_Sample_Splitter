#include "httplib/httplib.h"
#include "json/json.h"
#include <iostream>
#include <ctime>
#include <map>
#include <string>
#include <chrono>
#include "elma.h"
#include "client.h"
#include "SampleSplitter.h"
#include "LiveRecordingSimulator.h"
#include <vector>
#include "channel.h"

using namespace std::chrono;
using std::vector;
using namespace elma;

int main(){

double threshold = .1;
double grace_time = 3;
double upea = 30;

elma::Manager m;
Channel lft_a("left audio");
Channel rght_a("right audio");

// Testing Live Mode
// --------------------------------------------------------------------------

LiveRecordingSimulator rec("All_Drum_Samples.wav", 1024);
SampleSplitter ss(threshold, grace_time, upea);

std::cout << "Testing the sample splitter in live mode" <<std::endl;
std::cout << "Recording" <<std::endl;

m.schedule(rec, 23219_us)
.schedule(ss, 23219_us)
.add_channel(lft_a)
.add_channel(rght_a)
.init()
.start()
.run(26_s);

std::cout << "done" <<std::endl;
std::cout << std::endl;

// Testing Non-Live Mode
// --------------------------------------------------------------------------

std::cout << "Testing the sample splitter in non-live mode" <<std::endl;

SampleSplitter ss2("All_Drum_Samples.wav");

ss2.split_samples(threshold,grace_time);

vector<std::string> sample_names = {"kick.wav","snare.wav","hi_tom.wav","lo_tom.wav","hi_hat.wav","crash.wav","stop_recording_signal.wav"};
ss2.export_all_samples(sample_names);
std::cout << "done" <<std::endl;

return 0;
}