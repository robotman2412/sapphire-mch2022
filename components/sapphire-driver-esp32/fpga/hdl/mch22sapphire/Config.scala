package mch22sapphire

// SPDX-License-Identifier: MIT
// SPDX-CopyRightText: 2025 Julian Scheffers <julian@scheffers.net>

import spinal.core._
import spinal.core.sim._

object Config {
    def spinal = SpinalConfig(
        targetDirectory = "build",
        defaultConfigForClockDomains = ClockDomainConfig(
            resetKind = BOOT,
            resetActiveLevel = HIGH
        ),
        onlyStdLogicVectorAtTopLevelIo = true
    )

    def sim =
        SimConfig.withConfig(spinal).withFstWave.setTestPath("$WORKSPACE/$TEST")
}
