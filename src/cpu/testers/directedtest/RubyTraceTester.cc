#include "cpu/testers/directedtest/RubyTraceTester.hh"

#include <string>

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/DirectedTest.hh"
#include "mem/ruby/common/SubBlock.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

RubyTraceTester::RubyTraceTester(const Params *p)
    : ClockedObject(p),
      directedStartEvent([this] { wakeup(); }, "Directed tick",
                         false, Event::CPU_Tick_Pri),
      m_trace_file(p->trace_file),
      m_requestorId(p->system->getRequestorId(this)),
      m_num_cpus(p->num_cpus)
{
    m_requests_completed = 0;

    // create the ports
    for (int i = 0; i < p->port_cpuPort_connection_count; ++i)
    {
        ports.push_back(new CpuPort(csprintf("%s-port%d", name(), i),
                                    this, i));
    }

    // add the check start event to the event queue
    schedule(directedStartEvent, 1);
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

void RubyTester::hitCallback(NodeID proc, SubBlock *data)
{
    // Mark that we made progress
    m_last_progress_vector[proc] = curCycle();

    DPRINTF(RubyTest, "completed request for proc: %d", proc);
    DPRINTFR(RubyTest, " addr: 0x%x, size: %d, data: ",
             data->getAddress(), data->getSize());
    for (int byte = 0; byte < data->getSize(); byte++)
    {
        DPRINTFR(RubyTest, "%d ", data->getByte(byte));
    }
    DPRINTFR(RubyTest, "\n");
    incrementCycleCompletions();

    if (m_requests_inflight == m_requests_completed)
    {
        schedule(directedStartEvent, curTick() + 1)
    }
}

void RubyTraceTester::wakeup()
{

    {
        exitSimLoop("Ruby DirectedTester completed");
    }

    // if file
    //     ended and m_requests == m
    //_requests_to_complete
    //
    // end it.

    //if (m_requests_completed < m_requests_to_complete)
    //     {
    //         if (!generator->initiate())
    //         {
    //         }
    //     }

    // if m_requests_inflight
    //     == m_requests_completed
    //             exit

    //         else
}

RubyTraceTester *
RubyTraceTesterParams::create()
{
    return new RubyTraceTester(this);
}

bool makerequest(const TraceRecord &entry)
{
    DPRINTF(DirectedTest, "initiating request\n");

    RequestPort *port = getCpuPort(entry.cpu_id);

    Request::Flags flags;

    // For simplicity, requests are assumed to be 1 byte-sized
    RequestPtr req = std::make_shared<Request>(entry.m_address, 1, flags,
                                               requestorId);

    Packet::Command cmd;
    cmd = trace.cmd;
    PacketPtr pkt = new Packet(req, cmd);
    pkt->allocate();

    if (port->sendTimingReq(pkt))
    {
        DPRINTF(DirectedTest, "initiating request - successful\n");
        m_status = SeriesRequestGeneratorStatus_Request_Pending;
        return true;
    }
    else
    {
        // If the packet did not issue, must delete
        // Note: No need to delete the data, the packet destructor
        // will delete it
        delete pkt;

        DPRINTF(DirectedTest, "failed to initiate -
        sequencer not ready\n");
        return false;
    }
}

void RubyTester::checkForDeadlock()
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
}
