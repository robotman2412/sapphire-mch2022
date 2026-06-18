package mch22sapphire

// Copyright © 2026, Julian Scheffers, see LICENSE for info

import sapphire._
import spinal.core._
import spinal.core.sim._
import spinal.lib._
import sapphire.scanout.scanoutRegs

object FullSim extends App {
    Config.sim.compile(Top()).doSim(this.getClass.getSimpleName) { dut =>
        dut.clockDomain.forkStimulus(period = 10)
        dut.io.slaveSclk #= false
        dut.io.slaveChipSelect #= false

        // Dilly-dally a bit before sending commands.
        dut.clockDomain.waitSampling(10)

        // Half-period of the emulated SPI clock, in system clock cycles. The
        // real SPI clock is far slower than the FPGA clock, so this must be
        // comfortably larger than the input synchronizer depth for the slave
        // to settle each bit before it is sampled.
        val spiHalfPeriod = 4

        /// Perform one byte of SPI transaction.
        def spiByte(byte: Int): Int = {
            var rxd = 0
            for (bit <- 0 until 8) {
                // Master drives MOSI while the clock is low (CPHA=0)...
                dut.io.slaveMosi #= ((byte << bit) & 0x80) != 0
                dut.clockDomain.waitSampling(spiHalfPeriod)
                // ...and samples MISO on the rising edge.
                dut.io.slaveSclk #= true
                rxd = (rxd << 1) | dut.io.slaveMiso.toBoolean.toInt
                dut.clockDomain.waitSampling(spiHalfPeriod)
                dut.io.slaveSclk #= false
            }
            return rxd
        }

        /// Perform a full SPI transaction.
        def spiTrns(data: Seq[Int]): Seq[Int] = {
            dut.clockDomain.waitSampling(spiHalfPeriod)
            dut.io.slaveChipSelect #= true
            dut.clockDomain.waitSampling(spiHalfPeriod)

            val res = for (byte <- data) yield {
                spiByte(byte)
            }

            dut.clockDomain.waitSampling(spiHalfPeriod)
            dut.io.slaveChipSelect #= false
            dut.clockDomain.waitSampling(spiHalfPeriod)

            return res
        }

        // Read an I/O register.
        def ioRead(addr: Int): Int = {
            spiTrns(Seq(15, addr & 0xff, (addr >> 8) & 0xff))
            val resp = spiTrns(Seq.fill(4)(0))
            resp(0) | (resp(1) << 8) | (resp(2) << 16) | (resp(3) << 24)
        }

        // Write an I/O register.
        def ioWrite(addr: Int, value: Int) = {
            spiTrns(
                Seq(
                    16,
                    addr & 0xff,
                    (addr >> 8) & 0xff,
                    value & 0xff,
                    (value >> 8) & 0xff,
                    (value >> 16) & 0xff,
                    (value >> 24) & 0xff
                )
            )
        }

        // Run the DESC command.
        spiTrns(Seq(2))
        spiTrns(Seq.fill(32)(0))

        // Enable DMA interrupts.
        spiTrns(Seq(4, 0x01, 0x00, 0x00, 0x00))

        // Run a DMA write.
        spiTrns(Seq(10, 0xfe, 0xca, 0x00, 0x00))
        dut.clockDomain.waitSamplingWhere(dut.io.irqOut.toBoolean)
        spiTrns(Seq(11, 0xef, 0xbe, 0xad, 0xde))

        // Run a DMA read.
        spiTrns(Seq(8, 0xfe, 0xca, 0x00, 0x00))
        dut.clockDomain.waitSamplingWhere(dut.io.irqOut.toBoolean)
        spiTrns(Seq(9))
        spiTrns(Seq(0, 0, 0, 0))
        spiTrns(Seq(14))

        // Test the scanout engine.
        val caps = ioRead(0x0100 + scanoutRegs.caps)
        println("Scanout caps: 0x%08x".format(caps))
        assert(
            caps == (scanout.caps.isSerial | scanout.caps.reset | scanout.caps.commands)
        )
        ioWrite(0x0100 + scanoutRegs.control, scanout.control.reset)
        ioWrite(0x0100 + scanoutRegs.control, 0)
        ioWrite(0x0100 + scanoutRegs.serialData, 0xcc)
        ioWrite(0x0100 + scanoutRegs.control, scanout.control.regsel)
        ioWrite(0x0100 + scanoutRegs.serialData, 0xca)
        ioWrite(0x0100 + scanoutRegs.serialData, 0xfe)

        // Collect some extra samples before the sim ends.
        dut.clockDomain.waitSampling(40)
    }
}
