
// Copyright (c) 2025-2026 Julian Scheffers <julian@scheffers.net>
// SPDX-License-Identifier: CERN-OHL-P-2.0

`default_nettype none

module SB_GB(
    input  wire USER_SIGNAL_TO_GLOBAL_BUFFER,
    output wire GLOBAL_BUFFER_OUTPUT
);
    assign GLOBAL_BUFFER_OUTPUT = USER_SIGNAL_TO_GLOBAL_BUFFER;
endmodule
