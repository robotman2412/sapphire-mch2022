package mch22sapphire

import sapphire.util._
import spinal.core._
import spinal.lib._

// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

object Ice40FastSpi {
    case class ice40_fastspi() extends BlackBox {
        val spi_miso = out port Bool()
        val spi_mosi = in port Bool()
        val spi_clk  = in port Bool()
        val spi_cs_n = in port Bool()

        val usr_mosi_data = out port Bits(8 bits)
        val usr_mosi_stb  = out port Bool()
        val usr_miso_data = in port Bits(8 bits)
        val usr_miso_ack  = out port Bool()

        val csn_state = out port Bool()
        val csn_rise  = out port Bool()
        val csn_fall  = out port Bool()

        val clk = in port Bool()
        val rst = in port Bool() // Not used

        addRTLPath("hdl/mch22sapphire/ice40_fastspi.v")
    }
}

case class Ice40FastSpi() extends Component {
    import Ice40FastSpi._

    val io = new Bundle {

        /** Received data flow. */
        val rxd = master port Flow(Bits(8 bits))

        /** Sent data vacuum. */
        val txd = slave port Vacuum(Bits(8 bits))

        /** Active-high chip select input. */
        val chipSelectN = in port Bool()

        /** Re-synchronized chip-select output. */
        val chipSelectSync = out port Bool()

        /** Serial clock input. */
        val sclk = in port Bool()

        /** Serial receive data. */
        val mosi = in port Bool()

        /** Serial transmit data. */
        val miso = out port Bool()
    }

    val ext = ice40_fastspi()

    io.miso      := ext.spi_miso
    ext.spi_mosi := io.mosi
    ext.spi_clk  := io.sclk
    ext.spi_cs_n := io.chipSelectN

    io.txd.peek       := False
    io.txd.ready      := ext.usr_miso_ack
    ext.usr_miso_data := io.txd.payload

    io.rxd.valid   := RegNext(ext.usr_mosi_stb).init(False)
    io.rxd.payload := RegNext(ext.usr_mosi_data)

    io.chipSelectSync := !ext.csn_state

    ext.clk := clockDomain.readClockWire
    ext.rst := False // Not used; dummy value.
}
