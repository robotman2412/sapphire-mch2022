package mch22sapphire

// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

import spinal.core._
import sapphire.phy.spi._
import sapphire.interface.cmd.CmdEngine
import sapphire.interface.cmd.DebugRegFile
import sapphire.SapphireCfg
import sapphire.mem.SpiMemCtrl

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
        val slaveChipSelect = in port Bool()

        /** Serial clock input. */
        val slaveSclk = in port Bool()

        /** Serial receive data. */
        val slaveMosi = in port Bool()

        /** Serial transmit data. */
        val slaveMiso = out port Bool()
    }

    /** Global sapphire configuration. */
    val cfg = SapphireCfg(ramSize = 0x800000)

    /** SPI/QIO master PYH. */
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
    val debugRegs = DebugRegFile(
        Seq(
            spiMemCtrl.addr.getBitsWidth bits,         // 0: buffered DMA address.
            spiMemCtrl.buffer.getBitsWidth bits,       // 1: command / address buffer.
            spiMemCtrl.state.asBits.getBitsWidth bits, // 2: FSM state (one-hot).
            30 bits,                                   // 3: DMA setup + stream ready signals.
            8 bits,                                    // 4: DMA write data payload.
            8 bits                                     // 5: DMA read data payload.
        )
    )
    debugRegs.io.regs(0) := spiMemCtrl.addr.asBits
    debugRegs.io.regs(1) := spiMemCtrl.buffer
    debugRegs.io.regs(2) := spiMemCtrl.state.asBits
    debugRegs.io.regs(3) := spiMemCtrl.io.dma.setup.asBits ##
        spiMemCtrl.io.dma.wdata.ready ## spiMemCtrl.io.dma.rdata.ready
    debugRegs.io.regs(4) := spiMemCtrl.io.dma.wdata.payload
    debugRegs.io.regs(5) := spiMemCtrl.io.dma.rdata.payload

    /** Command engine. */
    val cmdEngine = CmdEngine(cfg)
    cmdEngine.io.chipSelect := io.slaveChipSelect
    cmdEngine.io.irqIn      := 0
    io.irqOut               := cmdEngine.io.irqOut

    /** SPI slave PHY. */
    val spiSlave = SimpleSpiSlave()
    spiSlave.io.chipSelect := io.slaveChipSelect
    spiSlave.io.sclk       := io.slaveSclk
    spiSlave.io.mosi       := io.slaveMosi
    io.slaveMiso           := spiSlave.io.miso

    // Inter-component connections.
    spiMaster.io.bus <> spiMemCtrl.io.spi

    spiMemCtrl.io.dma <> cmdEngine.io.dma

    cmdEngine.io.debug <> debugRegs.io.debug

    cmdEngine.io.rxd << spiSlave.io.rxd
    cmdEngine.io.txd >> spiSlave.io.txd
}

object Generate extends App {
    Config.spinal.generateVerilog(Top())
}
