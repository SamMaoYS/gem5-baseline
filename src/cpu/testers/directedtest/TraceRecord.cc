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
    m_cpu_idx = cpu_id;
    m_data_address = data_addr;
    m_pc_address = pc_addr;
    m_time = Cycles(0);

    // Don't differentiate between store misses and atomic requests in
    // the trace

    if (type == "LD")
    {
        m_cmd = MemCmd::ReadReq;
    }
    else if (type == "ST")
    {
        m_cmd = MemCmd::WriteReq;
    }
    else if (type == "FENCE")
    {
        m_cmd = MemCmd::MemFenceReq;
    }
    else
    {
        panic("Invalid operation type");
    }
}
// Public copy constructor and assignment operator
TraceRecord::TraceRecord(const TraceRecord &obj)
{
    *this = obj; // Call assignment operator
}

TraceRecord &TraceRecord::operator=(const TraceRecord &obj)
{
    m_cpu_idx = obj.m_cpu_idx;
    m_time = obj.m_time;
    m_data_address = obj.m_data_address;
    m_pc_address = obj.m_pc_address;
    m_cmd = obj.m_cmd;
    return *this;
}

bool TraceRecord::input(std::istream &in)
{
    in >> m_cpu_idx;
    in >> std::hex >> m_data_address;
    in >> m_pc_address;
    std::string type;
    if (!in.eof())
    {
        in >> type;
        if (type == "LD")
        {
            m_cmd = MemCmd::ReadReq;
        }
        else if (type == "ST")
        {
            m_cmd = MemCmd::WriteReq;
        }
        else if (type == "FENCE")
        {
            m_cmd = MemCmd::MemFenceReq;
        }

        // Ignore the rest of the line
        char c = '\0';
        while ((!in.eof()) && (c != '\n'))
        {
            in.get(c);
        }
        return true;
    }
    else
    {
        return false;
    }
}

void TraceRecord::print() const
{
    DPRINTF(DirectedTest, "[TraceRecord: %d %d %d %d]\n",
            m_cpu_idx, m_data_address, m_pc_address, m_cmd);
}
