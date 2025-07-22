/* -*- coding: utf-8 -*- */

/** 
 * Basic example of using "high-level" miniaudio to play a file
 * using the engine.
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdio.h>  // getchar();
#include <iostream>


std::string format_str( const ma_format fmt )
{
  switch (fmt) {
    case ma_format_f32:
      return "f32 => [-1.0, 1.0]";
    case ma_format_s16:
      return "s16 => [-32768, 32768]";
    case ma_format_s24:
      return "s24 => [-8388608, 8388608]";
    case ma_format_s32:
      return "s32 => [-2147483648, 2147483648]";
    case ma_format_u8:
      return "u8: [0, 255]";
  }
  return "unknown";
}

int main(int argc, char *argv[])
{

  // test input file in arguments
  if (argc < 2) {
    std::cerr <<  "No input file." << std::endl;
    return -1;
  }

  // create engine
  ma_engine engine;
  ma_result result = ma_engine_init( NULL, &engine );
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to initialize MiniAudio engine" << std::endl;
    return -1;
  }
  // get info on device using ma_engine_get_device
  ma_device *device_ptr = ma_engine_get_device( &engine );
  if (device_ptr == NULL) {
    std::cerr << "Failed to get device from engine" << std::endl;
    return -1;
  }
  ma_device_info device_info;
  result = ma_device_get_info( device_ptr, ma_device_type_playback, &device_info);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to get device info" << std::endl;
    return -1;
  }
  std::cout << "__Device INFO ******************************" << std::endl;
  std::cout
    << "  name: " << device_info.name << std::endl
    << "  isdefault: " << device_info.isDefault << std::endl
    << "  nbDataFormat: " << device_info.nativeDataFormatCount << std::endl;

  for (unsigned int i=0; i < device_info.nativeDataFormatCount; ++i) {
   std::cout
     << "    format: " << format_str( device_info.nativeDataFormats[i].format ) << std::endl
     << "    channels:" << device_info.nativeDataFormats[i].channels << std::endl
     << "    sampleRate: " << device_info.nativeDataFormats[i].sampleRate << std::endl
     << "    flags: " << "TODO" << std::endl;
  }

  ma_sound sound;
  result = ma_sound_init_from_file(&engine, argv[1], 0, NULL, NULL, &sound);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to read sound" << std::endl;
    return -1;
  }

  ma_format snd_format;
  ma_uint32 snd_channels;
  ma_uint32 snd_rate;
  ma_channel snd_channel_map;
  size_t snd_channel_map_cap;
  result = ma_sound_get_data_format( &sound,
                                     &snd_format,
                                     &snd_channels,
                                     &snd_rate,
                                     &snd_channel_map,
                                     snd_channel_map_cap);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to read sound data" << std::endl;
    return -1;
  }
  std::cout << "__Sound DATA *******************************" << std::endl
            << "  format :" << format_str( snd_format ) << std::endl
            << "  channels : " << snd_channels << std::endl
            << "  sampleRate : " << snd_rate << std::endl;

  ma_uint64 snd_length_pcm;
  result = ma_sound_get_length_in_pcm_frames( &sound, &snd_length_pcm);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to read sound pcm length" << std::endl;
    return -1;
  }
  float snd_length_s;
  result = ma_sound_get_length_in_seconds( &sound, &snd_length_s);
  if (result != MA_SUCCESS) {
    std::cerr << "Failed to read sound length in seconds" << std::endl;
    return -1;
  }
  std::cout << "  length pcm: " << snd_length_pcm
            << " (" << static_cast<float>(snd_length_pcm) / static_cast<float>(snd_rate)
            << ")" << std::endl
            << "  length s : " << snd_length_s << std::endl;

  ma_sound_start(&sound);
  //ma_engine_play_sound( &engine, argv[1], NULL );

  std::cout << "Press ENTER to quit..." << std::endl;
  getchar();

  ma_sound_stop( &sound );
  ma_sound_uninit( &sound );
  ma_engine_uninit( &engine );
  
  return 0;
}

