
makeflags += --silent



all: bitstream.bin

build/Top.v: $(shell find -name '*.scala')
	sbt "runMain mch22sapphire.Generate"

bitstream.bin: wrapper.v build/Top.v pinout.pcf
	mkdir -p build
	yosys -p 'synth_ice40 -top wrapper -json build/project.json' wrapper.v
	nextpnr-ice40 --up5k --package sg48 --json build/project.json --pcf pinout.pcf --asc build/bitstream.asc
	icepack build/bitstream.asc bitstream.bin

sim:
	sbt "runMain mch22sapphire.FullSim"
