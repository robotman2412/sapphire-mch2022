package mch22sapphire

// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

import spinal.core._
import spinal.core.sim._

object Config {
    def spinal = SpinalConfig(
        targetDirectory = "build",
        defaultConfigForClockDomains = ClockDomainConfig(
            resetKind = ASYNC,
            resetActiveLevel = LOW
        ),
        onlyStdLogicVectorAtTopLevelIo = true
    )

    def sim =
        SimConfig
            .withConfig(spinal)
            .addRtl("verilator/SB_DFF.v")
            .addRtl("verilator/SB_DFFER.v")
            .addRtl("verilator/SB_GB.v")
            .addRtl("verilator/SB_IO.v")
            .withFstWave
            .setTestPath("$WORKSPACE/$TEST")
}
