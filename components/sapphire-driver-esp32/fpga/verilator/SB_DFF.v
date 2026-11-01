
// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

`default_nettype none

module SB_DFF(
    input  wire D,
    output reg  Q,
    input  wire C
);
    initial begin
        Q = 0;
    end
    always @(posedge C) begin
        Q <= D;
    end
endmodule
