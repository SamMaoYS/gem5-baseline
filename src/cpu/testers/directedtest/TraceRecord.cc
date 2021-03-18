#include "cpu/testers/directedtest/TraceRecord.hh"

#include <string>

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/DirectedTest.hh"
#include "mem/ruby/common/SubBlock.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

TraceRecord::TraceRecord(int cpu_id, const Addr &data_addr,
                         const Addr &pc_addr,
                         std::string type)
{
    m_node_num = id;
    m_data_address = data_addr;
    m_pc_address = pc_addr;
    m_time = Cycles(0);

    // Don't differentiate between store misses and atomic requests in
    // the trace

    if (type == "LD")
    {
        cmd = MemCmd::ReadReq;
    }
    else if (type == "ST")
    {
        cmd = MemCmd::WriteReq;
    }
    else if (type == "FENCE")
    {
        cmd = MemCmd::MemFenceReq
    }
    else
    {
        panic("Invalid operation type")
    }
}
// Public copy constructor and assignment operator
TraceRecord::TraceRecord(const TraceRecord &obj)
{
    *this = obj; // Call assignment operator
}

TraceRecord &TraceRecord::operator=(const TraceRecord &obj)
{
    m_node_num = obj.m_node_num;
    m_time = obj.m_time;
    m_data_address = obj.m_data_address;
    m_pc_address = obj.m_pc_address;
    m_cmd = obj.cmd;
    return *this;
}
