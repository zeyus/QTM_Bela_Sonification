// this is how many times to check packet latency
// if it's not defined, there will be no latency check

#include "latency_check.h"

// this is coupled with QTM, not ideal but it works for now
void checkLatency(CRTProtocol rtProtocol)
{
  using clock = std::chrono::system_clock;
  using ms = std::chrono::duration<double, std::milli>;

  std::array<float, CHECK_CMD_LATENCY> aLatency{};

  double minLatency = 1000.0;
  double maxLatency = 0.0;
  double avgLatency = 0.0;

  // allocate char to store version
  char *qtmVer = new char();

  for (int i = 0; i < CHECK_CMD_LATENCY; i++) {
    // get current time
    const auto before = clock::now();

    // request version from QTM
    rtProtocol.GetQTMVersion(qtmVer, 5000000U);

    // get the duration of the request / response
    const ms duration = clock::now() - before;

    // update min / max / avg
    minLatency = std::min(minLatency, duration.count());
    maxLatency = std::max(maxLatency, duration.count());
    avgLatency += duration.count();
    aLatency[i] = duration.count();
  }
  avgLatency /= CHECK_CMD_LATENCY;
  // print the version and duration
  printf("It took %3.5fms/%3.5fms/%3.5fms (min/max/avg) to send a command %d times and get a reply. %s",
         minLatency, maxLatency, avgLatency, CHECK_CMD_LATENCY, qtmVer);

  // write to file
  std::ofstream latency_log(gLatency_log_file);
  for (int i = 0; i < CHECK_CMD_LATENCY; i++) {
    latency_log << aLatency[i] << std::endl;
  }
  latency_log.close();
}