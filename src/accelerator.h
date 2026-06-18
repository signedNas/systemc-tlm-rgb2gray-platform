#pragma once
#include <systemc>
#include <tlm>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstring>
#include "memory_map.h"


// b_transport on cfg_socket blocks until the full system concludes:
//  DMA read RGB -> convert -> DMA write grayscale
struct Accelerator : sc_core::sc_module {

    tlm_utils::simple_target_socket<Accelerator>    cfg_socket;
    tlm_utils::simple_initiator_socket<Accelerator> dma_socket;

    SC_CTOR(Accelerator) : cfg_socket("cfg_socket"), dma_socket("dma_socket") {
        cfg_socket.register_b_transport(this, &Accelerator::on_config);
    }

private:
    void do_dma(tlm::tlm_command cmd, uint64_t addr,
                uint8_t* data, uint32_t len, sc_core::sc_time& delay)
    {
        tlm::tlm_generic_payload trans;
        trans.set_command(cmd);
        trans.set_address(addr);
        trans.set_data_ptr(data);
        trans.set_data_length(len);
        trans.set_streaming_width(len);
        trans.set_byte_enable_ptr(nullptr);
        trans.set_dmi_allowed(false);
        trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        dma_socket->b_transport(trans, delay);

        if (trans.get_response_status() != tlm::TLM_OK_RESPONSE)
            SC_REPORT_ERROR("Accelerator", "DMA transaction failed");
    }

    void on_config(tlm::tlm_generic_payload& trans, sc_core::sc_time& delay)
    {
        if (trans.get_command() != tlm::TLM_WRITE_COMMAND ||
            trans.get_data_length() < sizeof(AccelConfig)) {
            trans.set_response_status(tlm::TLM_COMMAND_ERROR_RESPONSE);
            return;
        }

        AccelConfig cfg;
        std::memcpy(&cfg, trans.get_data_ptr(), sizeof(AccelConfig));

        std::cout << "[" << sc_core::sc_time_stamp() << "] Accel: config received"
                  << " in=0x"  << std::hex << cfg.input_addr
                  << " out=0x" << cfg.output_addr << std::dec
                  << " px="    << cfg.num_pixels << "\n";

        //DMA read full image from RAM
        const uint32_t rgb_bytes = cfg.num_pixels * 3;
        std::vector<uint8_t> rgb(rgb_bytes);
        std::cout << "[" << sc_core::sc_time_stamp() << "] Accel: DMA read  "
                  << rgb_bytes << " B\n";
        do_dma(tlm::TLM_READ_COMMAND, cfg.input_addr, rgb.data(), rgb_bytes, delay);

        //RGB to Grayscale  (BT.601 integer: Y = (77R + 150G + 29B) >> 8)
        std::cout << "[" << sc_core::sc_time_stamp() << "] Accel: processing "
                  << cfg.num_pixels << " pixels\n";
        std::vector<uint8_t> gray(cfg.num_pixels);
        for (uint32_t i = 0; i < cfg.num_pixels; ++i)
            gray[i] = static_cast<uint8_t>(
                (77u * rgb[i*3] + 150u * rgb[i*3+1] + 29u * rgb[i*3+2]) >> 8);

        delay += sc_core::sc_time(
            static_cast<double>(cfg.num_pixels) * ACCEL_NS_PER_PIXEL, sc_core::SC_NS);

        // DMA write grayscale result to RAM
        std::cout << "[" << sc_core::sc_time_stamp() << "] Accel: DMA write "
                  << cfg.num_pixels << " B\n";
        do_dma(tlm::TLM_WRITE_COMMAND, cfg.output_addr, gray.data(), cfg.num_pixels, delay);

        std::cout << "[" << sc_core::sc_time_stamp() << "] Accel: done\n";
        trans.set_response_status(tlm::TLM_OK_RESPONSE);
        // accumulated delay (DMA_read + compute + DMA_write) returned to CPU's wait()
    }
};