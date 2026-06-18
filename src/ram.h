#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <vector>
#include <cstring>
#include "memory_map.h"

// 64 MB flat RAM with two independent target sockets:
//   cpu_port: connected to the system Bus
//   dma_port: connected directly to the Accelerator
struct RAM : sc_core::sc_module {

    tlm_utils::simple_target_socket<RAM> cpu_port;
    tlm_utils::simple_target_socket<RAM> dma_port;

    SC_CTOR(RAM)
        : cpu_port("cpu_port"), dma_port("dma_port"), mem_(RAM_SIZE, 0x00)
    {
        cpu_port.register_b_transport(this, &RAM::b_transport_cpu);
        dma_port.register_b_transport(this, &RAM::b_transport_dma);
    }

private:
    std::vector<uint8_t> mem_;

    void do_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        const uint64_t addr = trans.get_address();
        uint8_t* const ptr  = trans.get_data_ptr();
        const uint32_t len  = trans.get_data_length();

        if (addr + len > RAM_SIZE) {
            SC_REPORT_ERROR("RAM", "address out of range");
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        if (trans.get_command() == tlm::TLM_WRITE_COMMAND)
            std::memcpy(mem_.data() + addr, ptr, len);
        else
            std::memcpy(ptr, mem_.data() + addr, len);

        delay += sc_core::sc_time(static_cast<double>(len) * RAM_NS_PER_BYTE, sc_core::SC_NS);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    void b_transport_cpu(tlm::tlm_generic_payload& t, sc_core::sc_time& d) { do_transport(t, d); }
    void b_transport_dma(tlm::tlm_generic_payload& t, sc_core::sc_time& d) { do_transport(t, d); }
};
