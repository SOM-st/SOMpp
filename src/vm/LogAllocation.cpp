#ifdef GENERATE_ALLOCATION_STATISTICS
  #include "LogAllocation.h"

  #include <fstream>
  #include <map>
  #include <string>
using std::map;
map<std::string, struct alloc_data> allocationStats;
#endif

void InitializeAllocationLog() {  // NOLINT(misc-use-internal-linkage)
#ifdef GENERATE_ALLOCATION_STATISTICS
    allocationStats["VMArray"] = {0, 0};
#endif
}

void OutputAllocationLogFile() {  // NOLINT(misc-use-internal-linkage)
#ifdef GENERATE_ALLOCATION_STATISTICS
    std::string file_name_allocation = std::string("allocation_statistics.csv");
    std::fstream file_alloc_stats(file_name_allocation.c_str(), std::ios::out);
    map<std::string, struct alloc_data>::iterator iter;
    for (iter = allocationStats.begin(); iter != allocationStats.end();
         iter++) {
        file_alloc_stats << iter->first << ", " << iter->second.noObjects
                         << ", " << iter->second.sizeObjects << std::endl;
    }
#endif
}
