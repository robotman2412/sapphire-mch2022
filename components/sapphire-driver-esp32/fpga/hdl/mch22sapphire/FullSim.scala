package mch22sapphire

// Copyright © 2026, Julian Scheffers, see LICENSE for info

import sapphire._
import spinal.core._
import spinal.core.sim._
import spinal.lib._

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
        val spiHalfPeriod = 2

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

        // Run the DESC command.
        spiTrns(Seq(2))
        spiTrns(Seq.fill(32)(0))

        // Enable DMA interrupts.
        spiTrns(Seq(4, 0x03, 0x00, 0x00, 0x00))

        // Run a DMA write.
        spiTrns(Seq(10, 0xfe, 0xca, 0x00, 0x00))
        dut.clockDomain.waitSamplingWhere(dut.io.irqOut.toBoolean)
        spiTrns(Seq(11, 0xef, 0xbe, 0xad, 0xde))

        // Collect some extra samples before the sim ends.
        dut.clockDomain.waitSampling(40)
    }
}
