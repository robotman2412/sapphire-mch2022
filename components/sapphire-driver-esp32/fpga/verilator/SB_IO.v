
// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

`default_nettype none

module SB_IO_OUTPUT(
    output wire PACKAGE_PIN,
    input  wire OUTPUT_ENABLE,
    input  wire D_OUT_0
);
    assign PACKAGE_PIN = OUTPUT_ENABLE ? D_OUT_0 : 'bz;
endmodule

module SB_IO_INPUT(
    input  wire PACKAGE_PIN,
    output wire D_IN_0
);
    assign D_IN_0 = PACKAGE_PIN;
endmodule
