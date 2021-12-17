#include "FileReader.h"

FileReader::FileReader(const char* filename)
{
    src_filename = filename;
    try {
        av.ck(avformat_open_input(&fmt_ctx, filename, NULL, NULL), CmdTag::AOI);
        av.ck(avformat_find_stream_info(fmt_ctx, NULL), CmdTag::AFSI);
        video_stream_idx = open_video_context();
        audio_stream_idx = open_audio_context();
    }
    catch (const AVException& e) {
        std::cout << "FileReader constructor exception: " << e.what() << std::endl;
    }
}

FileReader::~FileReader()
{
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&audio_dec_ctx);
}

AVPacket* FileReader::read_packet()
{
    AVPacket* pkt = av_packet_alloc();

    try {
        av.ck(av_read_frame(fmt_ctx, pkt), CmdTag::ARF);
    }
    catch (const AVException& e) {
        std::cout << "FileReader::read_packet exception: " << e.what() << std::endl;
        return NULL;
    }
    return pkt;
}

int FileReader::open_codec_context(AVCodecContext** dec_ctx, AVMediaType type, AVStream** stream)
{
    int stream_index;
    const AVCodec* dec = NULL;

    try {
        av.ck(stream_index = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0), std::string("av_find_best_stream could not find ") + av_get_media_type_string(type));
        *stream = fmt_ctx->streams[stream_index];
        av.ck(dec = avcodec_find_decoder((*stream)->codecpar->codec_id), std::string("avcodec_find_decoder could not find ") + avcodec_get_name((*stream)->codecpar->codec_id));
        av.ck(*dec_ctx = avcodec_alloc_context3(dec), CmdTag::AAC3);
        av.ck(avcodec_parameters_to_context(*dec_ctx, (*stream)->codecpar), CmdTag::APTC);
        av.ck(avcodec_open2(*dec_ctx, dec, NULL), CmdTag::AO2);
    }
    catch (const AVException& e) {
        std::cout << "FileReader::open_codec_context exception: " << e.what() << std::endl;
        return -1;
    }

    return stream_index;
}

int FileReader::open_video_context()
{
    return open_codec_context(&video_dec_ctx, AVMEDIA_TYPE_VIDEO, &video_stream);
}


int FileReader::open_audio_context()
{
    return open_codec_context(&audio_dec_ctx, AVMEDIA_TYPE_AUDIO, &audio_stream);
}
