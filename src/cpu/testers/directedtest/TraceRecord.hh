#ifndef TRACERECORD_HH
#define TRACERECORD_H

#include <string>

#include "base/logging.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "debug/DirectedTest.hh"
#include "mem/ruby/common/SubBlock.hh"
#include "sim/sim_exit.hh"
#include "sim/system.hh"

class TraceRecord
{
public:
    // Constructors
    TraceRecord(NodeID id, const Address &data_addr,
                const Address &pc_addr, std::string type, Cycles time);
    TraceRecord()
    {
        m_node_num = 0;
        m_time = Cycles(0);
        m_type = CacheRequestType_NULL;
    }

    // Destructor
    //  ~TraceRecord();

    // Public copy constructor and assignment operator
    TraceRecord(const TraceRecord &obj);
    TraceRecord &operator=(const TraceRecord &obj);

    // Public Methods
    bool node_less_then_eq(const TraceRecord &rec) const
    {
        return (this->m_time <= rec.m_time);
    }
    void print(ostream &out) const;
    void output(ostream &out) const;
    bool input(istream &in);

private:
    // Private Methods

    // Data Members (m_ prefix)
    // Int cpu port
    int m_cpu_idx;
    Cycles m_time;
    Addr m_data_address;
    Addr m_pc_address;
    Packet::Command cmd;
};

inline extern bool node_less_then_eq(const TraceRecord &n1,
                                     const TraceRecord &n2);

// Output operator declaration
ostream &operator<<(ostream &out, const TraceRecord &obj);

// ******************* Definitions *******************

inline extern bool node_less_then_eq(const TraceRecord &n1,
                                     const TraceRecord &n2)
{
    return n1.node_less_then_eq(n2);
}

// Output operator definition
extern inline ostream &operator<<(ostream &out,
                                  const TraceRecord &obj)
{
    obj.print(out);
    out << flush;
    return out;
}

#endif //TRACERECORD_HH
