package mch22sapphire

// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

import spinal.core._
import spinal.lib._
import spinal.lib.bus.amba3.apb._
import sapphire._
import sapphire.mem._
import sapphire.phy.spi._
import sapphire.util._
import sapphire.scanout.Ili9341Scanout

case class Top(fastSpi: Boolean, enableDebug: Boolean = false) extends Component {
    val io = new Bundle {

        /** RAM chip select. */
        val ramChipSelect = out port Bool()

        /** Explicit SPI clock output that follows CPOL and CPHA rules. */
        val ramSclk = out port Bool()

        /** Data output pins. */
        val ramMosi = out port Bits(4 bits)

        /** Output enables. */
        val ramMosiEn = out port Bits(4 bits)

        /** Data input pins. */
        val ramMiso = in port Bits(8 bits)

        /** Interrupt output. */
        val irqOut = out port Bool()

        /** Active-high chip select input. */
        val slaveChipSelectN = in port Bool()

        /** Serial clock input. */
        val slaveSclk = in port Bool()

        /** Serial receive data. */
        val slaveMosi = in port Bool()

        /** Serial transmit data. */
        val slaveMiso = out port Bool()

        /** Active-high display reset. */
        val dispReset = out port Bool()

        /** Active-high display chip select. */
        val dispChipSelect = out port Bool()

        /** Display write strobe. */
        val dispStrobe = out port Bool()

        /** Display register select; 0: Command, 1: Data. */
        val dispRegSel = out port Bool()

        /** Display data or command byte. */
        val dispData = out port Bits(8 bits)
    }

    val gpu = SapphireGpu
        .Builder()
        .withRamSize(0x800000)
        .withDma()
        .withSerialInterface()
        .addScanout(Ili9341Scanout.builder)
        .build()

    val slaveChipSelect = Bool()

    /* ======== SPI slave ======== */
    val fastSpiSlave   = fastSpi generate Ice40FastSpi()
    val simpleSpiSlave = !fastSpi generate SimpleSpiSlave()
    val spiRxd         = Flow(Bits(8 bits))
    val spiTxd         = Vacuum(Bits(8 bits))
    if (fastSpi) {
        slaveChipSelect             := fastSpiSlave.io.chipSelectSync
        fastSpiSlave.io.chipSelectN := io.slaveChipSelectN
        fastSpiSlave.io.sclk        := io.slaveSclk
        fastSpiSlave.io.mosi        := io.slaveMosi
        io.slaveMiso                := fastSpiSlave.io.miso
        spiRxd << fastSpiSlave.io.rxd
        spiTxd >> fastSpiSlave.io.txd
    } else {
        slaveChipSelect              := RegNext(RegNext(!io.slaveChipSelectN))
        simpleSpiSlave.io.chipSelect := slaveChipSelect
        simpleSpiSlave.io.sclk       := RegNext(RegNext(io.slaveSclk))
        simpleSpiSlave.io.mosi       := RegNext(RegNext(io.slaveMosi))
        io.slaveMiso                 := simpleSpiSlave.io.miso
        spiRxd << simpleSpiSlave.io.rxd
        spiTxd >> simpleSpiSlave.io.txd
    }

    /* ======== QSPI PSRAM ======== */
    /** SPI/QIO master PHY. */
    val spiMaster = SpiMaster(SpiCfg.SPI_QIO)
    io.ramSclk        := spiMaster.io.sclk
    io.ramMosi        := spiMaster.io.mosi
    io.ramMosiEn      := spiMaster.io.mosiEn
    spiMaster.io.miso := io.ramMiso

    /** SPI memory controller. */
    val spiMemCtrl = SpiMemCtrl(gpu.cfg.vaddrBits bits)
    io.ramChipSelect     := spiMemCtrl.io.chipSelect
    spiMemCtrl.io.enable := True
    spiMaster.io.bus <> spiMemCtrl.io.spi

    // TODO: This currently uses SPI commands and QIO addr/data.
    // Setup latency could be reduced by initially enabling quad mode on the RAM.
    val settings = spiMemCtrl.io.settings

    // Enter SPI settings for command.
    settings.cmdSettings.fullDuplex := True
    settings.cmdSettings.log2Bits   := 0
    settings.cmdSettings.cpol       := False
    settings.cmdSettings.cpha       := False

    // Enter SPI settings for address / data.
    settings.dataSettings.fullDuplex := False
    settings.dataSettings.log2Bits   := 2
    settings.dataSettings.cpol       := False
    settings.dataSettings.cpha       := False

    // Enter SPI memory access parameters.
    settings.addrLen        := 3
    settings.writeCmd       := 0x38
    settings.readCmd        := 0xeb
    settings.preAddrCycles  := 0
    settings.preReadCycles  := 6
    settings.preWriteCycles := 0
    settings.csResetCycles  := 2

    /* ======== GPU interface ======== */
    gpu.io.dma <> spiMemCtrl.io.dma
    gpu.io.chipSelect := slaveChipSelect
    gpu.io.rxd << spiRxd
    gpu.io.txd >> spiTxd
    io.irqOut         := gpu.io.irqOut

    val scanout = gpu.io.scanoutIo(0).asInstanceOf[Ili9341Scanout.ExternalIO]
    io.dispChipSelect := scanout.chipSelect
    io.dispReset      := scanout.resetOut
    val dispSink = Ili9341Scanout.SlowSink(1, 1, 0)
    dispSink.io.in << scanout.dispStream
    io.dispStrobe := dispSink.io.strobe
    io.dispRegSel := dispSink.io.regSel
    io.dispData   := dispSink.io.data
}

object Generate extends App {
    Config.spinal.generateVerilog(Top(false))
}
