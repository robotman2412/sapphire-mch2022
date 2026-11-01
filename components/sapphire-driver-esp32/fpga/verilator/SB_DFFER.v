
// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

`default_nettype none

module SB_DFFER(
    input  wire D,
    output reg  Q,
    input  wire E,
    input  wire R,
    input  wire C
);
    initial begin
        Q = 0;
    end
    always @(posedge C, posedge R) begin
        if (R) begin
            Q <= 0;
        end else if (E) begin
            Q <= D;
        end
    end
endmodule
