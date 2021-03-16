set(COMPONENT_SRCS "example_asr_main.c" "audio_tone_uri.c" "./ir_decoder/src/ir_ac_apply.c" "./ir_decoder/src/ir_ac_binary_parse.c" "./ir_decoder/src/ir_ac_build_frame.c" "./ir_decoder/src/ir_ac_parse_forbidden_info.c" "./ir_decoder/src/ir_decode.c" "./ir_decoder/src/ir_tv_control.c" "./ir_decoder/src/ir_utils.c" "./ir_decoder/src/ir_ac_control.c" "./ir_decoder/src/ir_ac_parse_frame_info.c" "./ir_decoder/src/ir_ac_parse_parameter.c" "./periph/irTask.c" "./periph/buttonTask.c" "./periph/mynvs.c" "./periph/mywifi.c" "./periph/myds18b20.c" "./periph/clock.c" "./network/myhttp.c" "./audio/player.c")

set(COMPONENT_ADD_INCLUDEDIRS . ./periph ./network ./audio ./ir_decoder/src/include)


register_component()