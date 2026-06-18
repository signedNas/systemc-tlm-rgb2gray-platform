#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <fstream>
#include <string>
#include <iostream>
#include "memory_map.h"

// Models disk I/O as a memory-mapped peripheral.

struct PersistentStorage : sc_core::sc_module {

    tlm_utils::simple_target_socket<PersistentStorage> targ_socket;

    PersistentStorage(sc_core::sc_module_name name,
                      std::string input_file,
                      std::string output_file)
        : sc_core::sc_module(name)
        , targ_socket("targ_socket")
        , input_file_(std::move(input_file))
        , output_file_(std::move(output_file))
    {
        targ_socket.register_b_transport(this, &PersistentStorage::b_transport);
    }

private:
    std::string input_file_;
    std::string output_file_;

    void b_transport(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        // Address is already remapped to local offset by the Bus
        const uint64_t offset = trans.get_address();
        uint8_t* const ptr    = trans.get_data_ptr();
        const uint32_t len    = trans.get_data_length();

        if (trans.get_command() == tlm::TLM_READ_COMMAND && offset == STORAGE_CMD_LOAD) {
            std::ifstream file(input_file_, std::ios::binary);
            if (!file.is_open()) {
                SC_REPORT_FATAL("Storage",
                    ("cannot open: " + input_file_ + "  (run 'make image' first)").c_str());
                trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                return;
            }
            file.read(reinterpret_cast<char*>(ptr), len);
            std::cout << "[" << sc_core::sc_time_stamp() << "] Storage: read "
                      << file.gcount() << " B from '" << input_file_ << "'\n";
        }
        else if (trans.get_command() == tlm::TLM_WRITE_COMMAND && offset == STORAGE_CMD_SAVE) {
            std::ofstream file(output_file_, std::ios::binary);
            if (!file.is_open()) {
                SC_REPORT_FATAL("Storage", ("cannot create: " + output_file_).c_str());
                trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
                return;
            }
            file.write(reinterpret_cast<const char*>(ptr), len);
            std::cout << "[" << sc_core::sc_time_stamp() << "] Storage: wrote "
                      << len << " B to '" << output_file_ << "'\n";
        }
        else {
            SC_REPORT_WARNING("Storage", "unknown command/offset");
            trans.set_response_status(tlm::TLM_ADDRESS_ERROR_RESPONSE);
            return;
        }

        delay += sc_core::sc_time(static_cast<double>(len) * STORAGE_NS_PER_BYTE, sc_core::SC_NS);
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
    }
};