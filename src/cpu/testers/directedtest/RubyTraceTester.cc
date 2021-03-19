
#include "cpu/testers/directedtest/RubyTraceTester.hh"
#include "cpu/testers/directedtest/TraceRecord.hh"
#include "base/logging.hh"
#include "debug/DirectedTest.hh"
#include "base/trace.hh"
#include "mem/ruby/common/SubBlock.hh"
#include "sim/sim_exit.hh"
#include "params/RubyTraceTester.hh"
#include "sim/system.hh"

RubyTraceTester::RubyTraceTester(const Params *p)
    : ClockedObject(p),
      traceStartEvent([this] { wakeup(); }, "Directed tick",
                      false, Event::CPU_Tick_Pri),
      deadlockEvent([this] { checkForDeadlock(); }, name(),
                    false, Event::CPU_Tick_Pri),

      m_trace_file(p->trace_file),
      m_num_cpus(p->num_cpus),
      m_deadlock_threshold(p->deadlock_threshold),
      m_requestorId(p->system->getRequestorId(this))
{
    m_requests_completed = 0;
    // create the ports
    for (int i = 0; i < p->port_cpuPort_connection_count; ++i)
    {
        ports.push_back(new CpuPort(csprintf("%s-port%d", name(), i),
                                    this, i));
    }

    // add the check start event to the event queue
    schedule(traceStartEvent, 1);
    schedule(deadlockEvent, 1);
}

RubyTraceTester::~RubyTraceTester()
{
    for (int i = 0; i < ports.size(); i++)
        delete ports[i];
}

void RubyTraceTester::init()
{

    m_last_progress_vector.resize(m_num_cpus);
    for (int i = 0; i < m_last_progress_vector.size(); i++)
    {
        m_last_progress_vector[i] = Cycles(0);
    }

    assert(ports.size() > 0);

    m_file_descriptor.open(m_trace_file.c_str());
    if (m_file_descriptor.fail())
    {
        panic("Error: error opening file" + m_trace_file);
        return;
    }
    DPRINTF(DirectedTest, "Request trace enabled to output file");
}

Port &
RubyTraceTester::getPort(const std::string &if_name, PortID idx)
{
    if (if_name != "cpuPort")
    {
        // pass it along to our super class
        return ClockedObject::getPort(if_name, idx);
    }
    else
    {
        if (idx >= static_cast<int>(ports.size()))
        {
            panic("RubyTraceTester::getPort: unknown index %d\n", idx);
        }

        return *ports[idx];
    }
}

bool RubyTraceTester::CpuPort::recvTimingResp(PacketPtr pkt)
{
    tester->hitCallback(id, pkt->getAddr());

    //
    // Now that the tester has completed, delete the packet, then return
    //
    delete pkt;
    return true;
}

RequestPort *
RubyTraceTester::getCpuPort(int idx)
{
    assert(idx >= 0 && idx < ports.size());

    return ports[idx];
}

void RubyTraceTester::hitCallback(NodeID proc, Addr addr)
{
    // Mark that we made progress
    m_last_progress_vector[proc] = curCycle();

    DPRINTF(DirectedTest, "completed request for proc: %d", proc);
    DPRINTFR(DirectedTest, "\n");
    incrementCycleCompletions();

    if (m_requests_inflight == m_requests_completed)
    {
        schedule(traceStartEvent, curTick() + 1);
    }
}

void RubyTraceTester::wakeup()
{
    TraceRecord tr_entry;
    while (tr_entry.input(m_file_descriptor))
    {
        if (makerequest(tr_entry))
        {
            m_requests_inflight++;
        }
        else
        {
            break;
        }
    }
    // If all inflight requests are complete there is no reason for
    // makerequest to reject request.
    if (m_requests_completed == m_requests_inflight)
    {
        m_file_descriptor.close();
        exitSimLoop("Ruby TraceTester completed \
        with %d requests",
                    m_requests_inflight);
    }
}

bool RubyTraceTester::makerequest(const TraceRecord &entry)
{
    DPRINTF(DirectedTest, "initiating request\n");

    RequestPort *port = getCpuPort(entry.m_cpu_idx);

    Request::Flags flags;

    // For simplicity, requests are assumed to be 1 byte-sized
    RequestPtr req = std::make_shared<Request>(entry.m_data_address, 1, flags,
                                               m_requestorId);

    Packet::Command cmd;
    cmd = entry.m_cmd;
    PacketPtr pkt = new Packet(req, cmd);
    pkt->allocate();

    if (port->sendTimingReq(pkt))
    {
        DPRINTF(DirectedTest, "initiating request - successful\n");
        return true;
    }
    else
    {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt;

        DPRINTF(DirectedTest, "failed to initiate - sequencer not ready\n");
        return false;
    }
}

void RubyTraceTester::checkForDeadlock()
{
    int size = m_last_progress_vector.size();
    Cycles current_time = curCycle();
    for (int processor = 0; processor < size; processor++)
    {
        if ((current_time - m_last_progress_vector[processor]) >
            m_deadlock_threshold)
        {
            panic("Deadlock detected: current_time: %d last_progress_time: %d "
                  "difference:  %d processor: %d\n",
                  current_time, m_last_progress_vector[processor],
                  current_time - m_last_progress_vector[processor], processor);
        }
    }

    schedule(deadlockEvent, 1000);
}

RubyTraceTester *RubyTraceTesterParams::create()
{
    return new RubyTraceTester(this);
}