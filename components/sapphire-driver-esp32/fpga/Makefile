
makeflags += --silent

VERILOG = wrapper.v build/Top.v hdl/mch22sapphire/ice40_fastspi.v

all: bitstream.bin

build/Top.v: $(shell find -name '*.scala')
	sbt "runMain mch22sapphire.Generate"

bitstream.bin: $(VERILOG) pinout.pcf
	mkdir -p build
	yosys -p 'synth_ice40 -top wrapper -json build/project.json' $(VERILOG)
	nextpnr-ice40 --up5k --package sg48 --freq 33 --json build/project.json --pcf pinout.pcf --asc build/bitstream.asc
	icepack build/bitstream.asc bitstream.bin

sim:
	sbt "runMain mch22sapphire.FullSim"
