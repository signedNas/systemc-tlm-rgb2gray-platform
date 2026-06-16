#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <sstream>
#include <iomanip>
#include "memory_map.h"

// Address-based TLM router.
struct Bus : sc_core::sc_module {

    tlm_utils::simple_target_socket<Bus> targ_socket;

    tlm_utils::simple_initiator_socket<Bus> ram_socket;
    tlm_utils::simple_initiator_socket<Bus> accel_socket;
    tlm_utils::simple_initiator_socket<Bus> storage_socket;

    SC_CTOR(Bus)
        : targ_socket("targ_socket"), ram_socket("ram_socket")
        , accel_socket("accel_socket"), storage_socket("storage_socket")
    {
        targ_socket.register_b_transport(this, &Bus::b_transport);
    }

private:
    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        const uint64_t addr = trans.get_address();

        if (addr >= RAM_BASE && addr < RAM_BASE + RAM_SIZE) {
            ram_socket->b_transport(trans, delay);
        }
        else if (addr >= ACCEL_BASE && addr < ACCEL_BASE + ACCEL_SIZE) {
            trans.set_address(addr - ACCEL_BASE);
            accel_socket->b_transport(trans, delay);
            trans.set_address(addr);  // restore so the initiator sees the original address
        }
        else if (addr >= STORAGE_BASE && addr < STORAGE_BASE + STORAGE_SIZE) {
            trans.set_address(addr - STORAGE_BASE);
            storage_socket->b_transport(trans, delay);
            trans.set_address(addr);
        }
        else {
            SC_REPORT_WARNING("Bus", ("unmapped address: 0x" + to_hex(addr)).c_str());
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
        }
    }

    static std::string to_hex(uint64_t v) {
        std::ostringstream os;
        os << std::hex << std::setfill('0') << std::setw(8) << v;
        return os.str();
    }
};