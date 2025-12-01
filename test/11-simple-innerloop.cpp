/**
 * The idea is to play music from a file until the end of the loop is
 * met, then replay the loop (twice) and then keep going to the end.
 *
 * AD-HOC on 'ressources/Bashung — La Nuit Je Mens.wav'
 * as I can hard code loop position and music properties.
 *
 * Loop "enabled" between frames 200000 and 200000 + 2*48000 (2 seconds)
 * Hit Enter: disable loop
 * Hit Enter again: stop play
 *
 * To make better : display current time in seconds
 */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <string>
#include <iostream>

// *********************************************************************** GLOBAL
ma_result result;

// TODO length of loop > frameCount of data_callback
bool loop_enabled {true};
ma_uint64 loop_frame_start {200000};
ma_uint64 loop_frame_length {48000 * 2};  // 1 second ?

// data_callback read from data_source (i.e. ma_decoder)
// and copy to pOutput of device
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    // TOOD results
    ma_uint64 pCursor;
    ma_uint64 pFrameRead;
    ma_decoder_get_cursor_in_pcm_frames(pDecoder, &pCursor);
    // std::cout << "cursor at " << pCursor << std::endl;

    if (loop_enabled && ((pCursor + frameCount) > (loop_frame_start + loop_frame_length))) {
        // feed what is left of loop
        ma_decoder_read_pcm_frames( pDecoder, pOutput,
                                    (loop_frame_start+loop_frame_length-pCursor),
                                    &pFrameRead );
        // std::cout << "feed END " << pFrameRead << " from " << pCursor << std::endl;
        // then set pcm to loop_start
        ma_decoder_seek_to_pcm_frame( pDecoder, loop_frame_start );
    }
    else {
        ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &pFrameRead);
        // std::cout << "feed NOR " << pFrameRead << " from " << pCursor << std::endl;
    }

    std::cout << "  " << (pCursor / 48000) << " frames" << "\r";

    (void)pInput;
}

int main(int argc, char *argv[])
{

    std::string filename;

    if (argc < 2) {
        std::cerr << "No input file." << std::endl;
        return -1;
    }
    // filename = argv[1];
    filename = std::string(argv[1]);
    // open and read file as data_source
    ma_decoder decoder;
    std::cout << "__Opening: " << filename << std::endl;
    result = ma_decoder_init_file(filename.c_str(), NULL, &decoder);
    if (result != MA_SUCCESS) {
        std::cerr << "ERROR: could not open file" << std::endl;
        return -2;
    }

    // configure and initialize output/sink device with properties similar to
    // the decoded music
    // the device will use the given callbackfunction to be fed.
    ma_device_config device_config;
    ma_device device;

    device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format   = decoder.outputFormat;
    device_config.playback.channels = decoder.outputChannels;
    device_config.sampleRate        = decoder.outputSampleRate;
    device_config.dataCallback      = data_callback;
    device_config.pUserData         = &decoder;

    std::cout << "__Initialize playback device." << std::endl;
    if (ma_device_init(NULL, &device_config, &device) != MA_SUCCESS) {
        std::cerr << "ERROR: failed to initialize/open device" << std::endl;
        ma_decoder_uninit(&decoder);
        return -3;
    }

    // step 0: play the sound, without loop
    if (ma_device_start(&device) != MA_SUCCESS) {
        std::cerr << "ERROR: failed to start playback device" << std::endl;
        ma_device_uninit(&device);
        ma_decoder_uninit(&decoder);
        return -4;
    }

    std::cout << "Press ENTER to quit loop..." << std::endl;
    getchar();
    loop_enabled = false;

    std::cout << "Press ENTER to quit..." << std::endl;
    getchar();

    return 0;
}
