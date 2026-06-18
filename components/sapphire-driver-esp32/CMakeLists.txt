idf_component_register(
  SRCS
    "src/sapphire_cmd.c"
    "src/sapphire_transport.c"
    "src/sapphire.c"
  INCLUDE_DIRS include
  REQUIRES driver spi-ice40 mch2022-rp2040 badge-bsp esp_timer pax-gfx
	EMBED_FILES "fpga/bitstream.bin"
)
