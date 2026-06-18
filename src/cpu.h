#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_initiator_socket.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "memory_map.h"

struct CPU : sc_core::sc_module {

    tlm_utils::simple_initiator_socket<CPU> init_socket;

    SC_CTOR(CPU) : init_socket("init_socket") { SC_THREAD(run); }

private:
    void send_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t* data, uint32_t len)
    {
        tlm::tlm_generic_payload trans;
        sc_core::sc_time delay = sc_core::SC_ZERO_TIME;

        trans.set_command(cmd);
        trans.set_address(addr);
        trans.set_data_ptr(data);
        trans.set_data_length(len);
        trans.set_streaming_width(len);
        trans.set_byte_enable_ptr(nullptr);
        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        init_socket->b_transport(trans, delay);
        wait(delay);  // advance simulation clock by bandwidth-annotated delay

        if (trans.get_response_status() != tlm::TLM_OK_RESPONSE)
            SC_REPORT_ERROR("CPU", ("transaction failed at 0x" + to_hex(addr)).c_str());
    }

    void run()
    {
        std::cout << "[" << sc_core::sc_time_stamp() << "] CPU: pipeline start\n";

        //load image from storage into local buffer
        std::vector<uint8_t> rgb_buf(RGB_SIZE);
        send_transaction(tlm::TLM_READ_COMMAND,
                 STORAGE_BASE + STORAGE_CMD_LOAD, rgb_buf.data(), RGB_SIZE);

        send_transaction(tlm::TLM_WRITE_COMMAND, INPUT_ADDR, rgb_buf.data(), RGB_SIZE);
        rgb_buf.clear();
        rgb_buf.shrink_to_fit();

        //configure accelerator — b_transport blocks until DMA+compute done
        AccelConfig cfg{ INPUT_ADDR, OUTPUT_ADDR, NUM_PIXELS };
        send_transaction(tlm::TLM_WRITE_COMMAND, ACCEL_BASE,
                 reinterpret_cast<uint8_t*>(&cfg), sizeof(AccelConfig));

        std::vector<uint8_t> gray_buf(GRAY_SIZE);
        send_transaction(tlm::TLM_READ_COMMAND, OUTPUT_ADDR, gray_buf.data(), GRAY_SIZE);

        //save result to storage
        send_transaction(tlm::TLM_WRITE_COMMAND,
                 STORAGE_BASE + STORAGE_CMD_SAVE, gray_buf.data(), GRAY_SIZE);

        std::cout << "[" << sc_core::sc_time_stamp() << "] CPU: pipeline done\n";
        sc_core::sc_stop();
    }

    static std::string to_hex(uint64_t v) {
        std::ostringstream os; os << std::hex << v; return os.str();
    }
};