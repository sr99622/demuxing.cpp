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

static const char* src_filename = NULL;

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

void decode_video(Decoder* video_decoder, CircularQueue<AVPacket*>* video_pkt_q)
{
    bool decoding = true;
    while (decoding) {
        try {
            AVPacket* pkt = video_pkt_q->pop();
            AVFrame* frame = video_decoder->decode_packet(pkt);
            if (frame) {
                video_decoder->writer->write_frame(frame);
                av_frame_free(&frame);
            }
            av_packet_unref(pkt);
        }
        catch (const QueueClosedException& e) {
            std::cout << "decode_video exception: " << e.what() << std::endl;
            decoding = false;
        }
    }
    video_decoder->flush();
}

void decode_audio(Decoder* audio_decoder, CircularQueue<AVPacket*>* audio_pkt_q)
{
    bool decoding = true;
    while (decoding) {
        try {
            AVPacket* pkt = audio_pkt_q->pop();
            AVFrame* frame = audio_decoder->decode_packet(pkt);
            if (frame) {
                audio_decoder->writer->write_frame(frame);
                av_frame_free(&frame);
            }
            av_packet_unref(pkt);
        }
        catch (const QueueClosedException& e) {
            std::cout << "decode_audio exception: " << e.what() << std::endl;
            decoding = false;
        }
    }
    audio_decoder->flush();
}



int main(int argc, char** argv)
{
    int ret = 0;

    src_filename = "../../data/test.mp4";

    FileReader reader(src_filename);
    CircularQueue<AVPacket*> video_pkt_q(10);
    CircularQueue<AVPacket*> audio_pkt_q(10);

    Decoder video_decoder(reader.fmt_ctx, reader.video_stream_index);
    Decoder audio_decoder(reader.fmt_ctx, reader.audio_stream_index);

    std::thread read_file(read, &reader, &video_pkt_q, &audio_pkt_q);
    std::thread get_video(decode_video, &video_decoder, &video_pkt_q);
    std::thread get_audio(decode_audio, &audio_decoder, &audio_pkt_q);

    read_file.join();
    get_video.join();
    get_audio.join();

    printf("Demuxing succeeded.\n");

    return ret < 0;
}