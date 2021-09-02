set(COMPONENT_SRCS "main.c" "./app/etymology.c" "./audio/asrtask.c" "./network/mymqtt.c" "./app/storage.c" "./network/myble.c" "./periph/myuart.c" "audio/audio_tone_uri.c" "./ir_decoder/src/ir_ac_apply.c" "./ir_decoder/src/ir_ac_binary_parse.c" "./ir_decoder/src/ir_ac_build_frame.c" "./ir_decoder/src/ir_ac_parse_forbidden_info.c" "./ir_decoder/src/ir_decode.c" "./ir_decoder/src/ir_tv_control.c" "./ir_decoder/src/ir_utils.c" "./ir_decoder/src/ir_ac_control.c" "./ir_decoder/src/ir_ac_parse_frame_info.c" "./ir_decoder/src/ir_ac_parse_parameter.c" "./periph/ir.c" "./periph/general_gpio.c" "./network/mywifi.c" "./periph/myds18b20.c" "./app/clock.c" "./network/myhttp.c" "./audio/player.c")

set(COMPONENT_ADD_INCLUDEDIRS . ./periph ./network ./audio ./ir_decoder/src/include ./app)


register_component()