/*
 * @Author: your name
 * @Date: 2020-10-22 10:00:03
 * @LastEditTime: 2021-09-02 11:37:09
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\audio\player.h
 */
#ifndef _PLAYER_H
#define _PLAYER_H
#include "main.h"
#include "audio_tone_uri.h"


#define WOZAI_MP3 (play_flash(TONE_TYPE_WOZAI))
#define WIFIDISC_MP3 (play_flash(TONE_TYPE_WIFI_DISC))
#define WIFICON_MP3 (play_flash(TONE_TYPE_WIFI_CON))
#define BLECON_MP3 (play_flash(TONE_TYPE_BLE_CONN_SUCCE))
#define BLEDISC_MP3 (play_flash(TONE_TYPE_BLE_DISCONN))
#define SETTMR_MP3 (play_flash(TONE_TYPE_SET_TIMER))
#define AC_CLOSE_MP3 (play_flash(TONE_TYPE_AIR_CLOSE))
#define AC_SUCCESS_MP3 (play_flash(TONE_TYPE_AIR_SET_OK))
#define BLEOPEN_MP3 (play_flash(TONE_TYPE_BLE_OPEN))
#define BLECLOSE_MP3 (play_flash(TONE_TYPE_BLE_CLOSE))
#define WHAT_MP3 (play_flash(TONE_TYPE_WHATAREYOUTAKING))
#define NO_MP3 (play_flash(TONE_TYPE_NO))



int player_init();
int play_flash(const tone_type_t index);
int speech_sync(char *t);
void player_testTask(void *arg);

#endif
