#include "Statistics.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <ios>
#include <map>
#include <ostream>
#include <string>

using namespace std;

vector<std::string> Statistics::mainArgs;
map<int64_t, int64_t> Statistics::integerHist;
map<string, struct allocStatsData> Statistics::allocationStats;

map<string, int64_t> Statistics::receiverTypes;
map<string, struct receiverStatData> Statistics::callStats;

int64_t Statistics::blockActivationWithContext = 0;
int64_t Statistics::blockActivationWithoutContext = 0;

void Statistics::Initialize() {}

void Statistics::OutputStatistics() {
    outputIntegerHistogram();
    outputAllocations();
    outputReceiverTypes();
    outputSendTypes();
    outputBlockStats();
}

void Statistics::outputSendTypes() {
    if (callStats.empty()) {
        return;
    }

    string file_name_send_types = argvFileName();
    file_name_send_types.append("_send_types.csv");
    fstream send_stat(file_name_send_types.c_str(), ios::out);
    send_stat << "#name, percentage_primitive_calls, no_primitive_calls, "
                 "no_non_primitive_calls"
              << '\n';
    for (auto& callStat : callStats) {
        send_stat << callStat.first << ", " << setiosflags(ios::fixed)
                  << setprecision(2)
                  << (double)(callStat.second.noPrimitiveCalls) /
                         (double)(callStat.second.noCalls)
                  << ", " << callStat.second.noPrimitiveCalls << ", "
                  << callStat.second.noCalls - callStat.second.noPrimitiveCalls
                  << '\n';
    }
}

void Statistics::outputReceiverTypes() {
    if (receiverTypes.empty()) {
        return;
    }

    string file_name_receivers = argvFileName();
    file_name_receivers.append("_receivers.csv");
    fstream receivers(file_name_receivers.c_str(), ios::out);
    for (auto& receiverType : receiverTypes) {
        receivers << receiverType.first << ",  " << receiverType.second << '\n';
    }
}

void Statistics::outputAllocations() {
    if (allocationStats.empty()) {
        return;
    }

    string const file_name_allocation = string("allocation_statistics.csv");
    fstream file_alloc_stats(file_name_allocation.c_str(), ios::out);
    map<string, struct allocStatsData>::iterator iter;
    for (iter = allocationStats.begin(); iter != allocationStats.end();
         iter++) {
        file_alloc_stats << iter->first << ", " << iter->second.noObjects
                         << ", " << iter->second.sizeObjects << '\n';
    }
}

void Statistics::outputIntegerHistogram() {
    if (integerHist.empty()) {
        return;
    }

    string file_name_hist = argvFileName();
    file_name_hist.append("_integer_histogram.csv");
    fstream hist_csv(file_name_hist.c_str(), ios::out);

    for (auto& it : integerHist) {
        hist_csv << it.first << ", " << it.second << '\n';
    }
}

void Statistics::outputBlockStats() {
    if (!BlockStatsEnabled) {
        return;
    }

    string file_name_block_stats = argvFileName();
    file_name_block_stats.append("_block_stats.csv");
    fstream block_stats(file_name_block_stats.c_str(), ios::out);

    block_stats << argvFileName() << ",with_context, " << blockActivationWithContext
                << '\n';
    block_stats << argvFileName() << ",without_context, "
                << blockActivationWithoutContext << '\n';
}
