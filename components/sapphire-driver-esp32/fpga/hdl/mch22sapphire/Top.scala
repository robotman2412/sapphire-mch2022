package mch22sapphire

// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

import sapphire._
import sapphire.phy.spi._
import sapphire.interface.cmd._
import sapphire.mem._
import spinal.core._
import spinal.lib._
import spinal.lib.bus.amba3.apb._
import sapphire.scanout.Ili9341Scanout
import spinal.lib.bus.misc.SizeMapping
import sapphire.dma.DmaMux

case class Top() extends Component {
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

    val enableDebug = false

    /** Global sapphire configuration. */
    val cfg = SapphireCfg(ramSize = 0x800000)

    /** APB control register bus. */
    val apb = Apb3(16, 32)

    /** SPI/QIO master PHY. */
    val spiMaster = SpiMaster(SpiCfg.SPI_QIO)
    io.ramSclk        := spiMaster.io.sclk
    io.ramMosi        := spiMaster.io.mosi
    io.ramMosiEn      := spiMaster.io.mosiEn
    spiMaster.io.miso := io.ramMiso

    /** SPI memory controller. */
    val spiMemCtrl = SpiMemCtrl(cfg.vaddrBits bits)
    io.ramChipSelect     := spiMemCtrl.io.chipSelect
    spiMemCtrl.io.enable := True

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

    /** Debug register file; aggregates debug taps and serves them to the
      * command engine over a debug bus.
      */
    val debugRegs = enableDebug generate DebugRegFile(
        Seq(
            spiMemCtrl.addr.getBitsWidth bits,         // 0: buffered DMA address.
            spiMemCtrl.buffer.getBitsWidth bits,       // 1: command / address buffer.
            spiMemCtrl.state.asBits.getBitsWidth bits, // 2: FSM state (one-hot).
            30 bits,                                   // 3: DMA setup + stream ready signals.
            8 bits,                                    // 4: DMA write data payload.
            8 bits,                                    // 5: DMA read data payload.
            10 bits                                    // 6: DMA error diagnosis.
        )
    )
    if (enableDebug) {
        debugRegs.io.regs(0) := spiMemCtrl.addr.asBits
        debugRegs.io.regs(1) := spiMemCtrl.buffer
        debugRegs.io.regs(2) := spiMemCtrl.state.asBits
        debugRegs.io.regs(3) := spiMemCtrl.io.dma.setup.asBits ##
            spiMemCtrl.io.dma.wdata.ready ## spiMemCtrl.io.dma.rdata.ready
        debugRegs.io.regs(4) := spiMemCtrl.io.dma.wdata.payload
        debugRegs.io.regs(5) := spiMemCtrl.io.dma.rdata.payload
        // 6: why is the DMA stream stalling? Captures the full DMA handshake, the
        // SPI engine status and the read-buffer occupancy. Latch on DMAERR to
        // freeze the state at the moment dma_error is raised.
        debugRegs.io.regs(6) := spiMemCtrl.readCap.asBits ##
            spiMemCtrl.isWrite ##
            spiMemCtrl.io.spi.action.ready ##
            spiMemCtrl.io.spi.action.valid ##
            spiMemCtrl.io.spi.busy ##
            spiMemCtrl.io.dma.wdata.ready ##
            spiMemCtrl.io.dma.wdata.valid ##
            spiMemCtrl.io.dma.rdata.ready ##
            spiMemCtrl.io.dma.rdata.valid
    }

    val slaveChipSelect = Bool()

    /** Command engine. */
    val cmdEngine = CmdEngine(cfg, enableDebug)
    cmdEngine.io.chipSelect := slaveChipSelect
    cmdEngine.io.irqIn      := 0
    io.irqOut               := cmdEngine.io.irqOut
    cmdEngine.io.apb >> apb

    /** SPI slave PHY. */
    val spiSlave = Ice40FastSpi()
    slaveChipSelect         := spiSlave.io.chipSelectSync
    spiSlave.io.chipSelectN := io.slaveChipSelectN
    spiSlave.io.sclk        := io.slaveSclk
    spiSlave.io.mosi        := io.slaveMosi
    io.slaveMiso            := spiSlave.io.miso

    /** ILI9341 scanout engine. */
    val scanout = Ili9341Scanout(cfg)
    val sink    = Ili9341Scanout.SlowSink(1, 1, 0)
    sink.io.in << scanout.io.dispStream
    io.dispStrobe     := sink.io.strobe
    io.dispRegSel     := sink.io.regSel
    io.dispData       := sink.io.data
    io.dispReset      := scanout.io.resetOut
    io.dispChipSelect := scanout.io.chipSelect

    // APB bus connections.
    val apbDecd   =
        Apb3Decoder(Apb3Config(16, 32), Seq(SizeMapping(0x0100, 0x100)))
    apbDecd.io.input << apb
    val apbRouter = new Apb3Router(Apb3Config(8, 32, 1))
    apbDecd.io.output >> apbRouter.io.input
    apbRouter.io.outputs(0) >> scanout.io.apb

    // Inter-component connections.
    spiMaster.io.bus <> spiMemCtrl.io.spi

    val dmaMux = DmaMux(2, cfg.vaddrBits)
    dmaMux.io.target <> spiMemCtrl.io.dma
    dmaMux.io.controllers(0) <> cmdEngine.io.dma
    dmaMux.io.controllers(1) <> scanout.io.dma

    if (enableDebug) cmdEngine.io.debug <> debugRegs.io.debug

    cmdEngine.io.rxd << spiSlave.io.rxd
    cmdEngine.io.txd >> spiSlave.io.txd
}

object Generate extends App {
    Config.spinal.generateVerilog(Top())
}
