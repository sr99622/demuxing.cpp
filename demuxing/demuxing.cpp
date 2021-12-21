/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 /**
  * @file
  * Demuxing and decoding example.
  *
  * Show how to use the libavformat and libavcodec API to demux and
  * decode audio and video data.
  * @example demuxing_decoding.c
  */

//  ffplay -f rawvideo -pix_fmt yuv420p -video_size 640x480 video
//  ffplay -f f32le -ac 1 -ar 44100 audio

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "FileReader.h"
#include "Decoder.h"
#include "CircularQueue.h"

void read(FileReader* reader, CircularQueue<AVPacket*>* video_pkt_q, CircularQueue<AVPacket*>* audio_pkt_q)
{
    while (AVPacket* pkt = reader->read_packet()) {
        if (pkt->stream_index == reader->video_stream_index) {
            video_pkt_q->push(pkt);
        }
        else if (pkt->stream_index == reader->audio_stream_index) {
            audio_pkt_q->push(pkt);
        }
    }

    video_pkt_q->flush();
    audio_pkt_q->flush();
}

void decode(Decoder* decoder, CircularQueue<AVPacket*>* pkt_q)
{
    int ret = 0;
    while (ret >= 0) {
        try {
            AVPacket* pkt = pkt_q->pop();
            ret = decoder->decode_packet(pkt);
            av_packet_unref(pkt);
        }
        catch (const QueueClosedException& e) {
            break;
        }
    }
    decoder->flush();
}

void write(RawFileWriter* writer, CircularQueue<Frame>* frame_q)
{
    Frame frame;
    while (frame_q->isOpen()) {
        try {
            frame_q->pop(frame);
            writer->write_frame(frame);
        }
        catch (const QueueClosedException& e) {
            break;
        }
    }
}

void show_hw_devices()
{
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
        std::cout << av_hwdevice_get_type_name(type) << std::endl;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage: demuxing <filename> <hardware decoder name> (*optional)\n"
            << "Available hardware decoders are listed below" << std::endl;
        show_hw_devices();
        return 0;
    }

    const char* hw_dec_name = "none";
    if (argc == 3)
        hw_dec_name = argv[2];

    AVHWDeviceType type = av_hwdevice_find_type_by_name(hw_dec_name);
    if (type != AV_HWDEVICE_TYPE_NONE)
        std::cout << "hw decoder: " << av_hwdevice_get_type_name(type) << std::endl;
    else
        std::cout << "using cpu decoder" << std::endl;

    const char* src_filename = argv[1];
    FileReader reader(src_filename);
    
    CircularQueue<AVPacket*> video_pkt_q(10);
    CircularQueue<AVPacket*> audio_pkt_q(10);
    CircularQueue<Frame> video_frame_q(10);
    CircularQueue<Frame> audio_frame_q(10);

    Decoder video_decoder(reader.fmt_ctx, reader.video_stream_index, &video_frame_q, type);
    Decoder audio_decoder(reader.fmt_ctx, reader.audio_stream_index, &audio_frame_q);

    RawFileWriter video_writer(video_decoder.dec_ctx);
    RawFileWriter audio_writer(audio_decoder.dec_ctx);

    std::thread read_file(read, &reader, &video_pkt_q, &audio_pkt_q);
    std::thread decode_video(decode, &video_decoder, &video_pkt_q);
    std::thread decode_audio(decode, &audio_decoder, &audio_pkt_q);
    std::thread write_video(write, &video_writer, &video_frame_q);
    std::thread write_audio(write, &audio_writer, &audio_frame_q);

    read_file.join();
    decode_video.join();
    decode_audio.join();
    write_video.join();
    write_audio.join();

    std::cout << "Demuxing succeeded." << std::endl;

    return 0;
}