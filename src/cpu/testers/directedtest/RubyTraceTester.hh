#ifndef __CPU_DIRECTEDTEST_RUBYTRACETESTER_HH__
#define __CPU_DIRECTEDTEST_RUBYTRACETESTER_HH__

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

class RubyTraceTester : public ClockedObject
{
public:
  class CpuPort : public RequestPort
  {
  private:
    RubyTraceTester *tester;

  public:
    CpuPort(const std::string &_name, RubyTraceTester *_tester,
            PortID _id)
        : RequestPort(_name, _tester, _id), tester(_tester)
    {
    }

  protected:
    virtual bool recvTimingResp(PacketPtr pkt);
    virtual void recvReqRetry()
    {
      panic("%s does not expect a retry\n", name());
    }
  };

  typedef RubyTraceTesterParams Params;
  RubyTraceTester(const Params *p);
  ~RubyTraceTester();

  Port &getPort(const std::string &if_name,
                PortID idx = InvalidPortID) override;

  RequestPort *getCpuPort(int idx);

  void init() override;

  void wakeup();

  void incrementCycleCompletions() { m_requests_completed++; }

  void printStats(std::ostream &out) const {}
  void clearStats() {}
  void printConfig(std::ostream &out) const {}

  void print(std::ostream &out) const;

protected:
  EventFunctionWrapper traceStartEvent;
  EventFunctionWrapper deadlockEvent;

private:
  void hitCallback(NodeID proc, Addr addr);
  bool makerequest(const TraceRecord &entry);

  void checkForDeadlock();

  // Private copy constructor and assignment operator
  RubyTraceTester(const RubyTraceTester &obj);
  RubyTraceTester &operator=(const RubyTraceTester &obj);

  uint64_t m_requests_completed;
  std::vector<RequestPort *> ports;
  std::string m_trace_file;
  uint64_t m_requests_inflight;
  std::ifstream m_file_descriptor;
  int m_num_cpus;
  int m_deadlock_threshold;
  RequestorID m_requestorId;

  std::vector<Cycles> m_last_progress_vector;
};

#endif // __CPU_DIRECTEDTEST_RUBYTRACETESTER_HH__
