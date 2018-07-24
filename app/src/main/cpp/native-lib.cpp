#include <jni.h>
#include <string>
#include <android/log.h>
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testff",__VA_ARGS__)

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswsacle/swscale.h>
#include <libavcodec/jni.h>
}
#include<iostream>
using namespace std;

static double r2d(AVRational r)
{
    return r.num==0||r.den == 0 ? 0 :(double)r.num/(double)r.den;
}

//当前时间戳 clock
long long GetNowMs()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int sec = tv.tv_sec%360000;
    long long t = sec*1000+tv.tv_usec/1000;
    return t;
}
extern "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm,void *res)
{
    av_jni_set_java_vm(vm,0);
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jstring
JNICALL
Java_aplay_testffmpeg_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++ ";
    hello += avcodec_configuration();

    /* Initialize all */
    av_register_all();

    avformat_network_init();

    avcodec_register_all();

    /* Open the file */
    AVFormatContext *ic = NULL; // AVFormatContext -> unpack strcut for flv, mp4, rmvb, avi
    char path[] = "/sdcard/1080.mp4";

    int re = avformat_open_input(&ic, path, 0, 0);
    if(re != 0)
    {
        LOGW("avformat_open_input failed!:%s",av_err2str(re));
        return env->NewStringUTF(hello.c_str());
    }
    LOGW("avformat_open_input %s success!",path);

    /* Find stream information as Video and Audio */
    re = avformat_find_stream_info(ic,0);
    if(re != 0)
    {
        LOGW("avformat_find_stream_info failed!");
    }
    LOGW("duration = %lld nb_streams = %d",ic->duration,ic->nb_streams);

    int fps = 0;
    int videoStream = 0;
    int audioStream = 1;

    for(int i = 0; i < ic->nb_streams; i++)
    {
        AVStream *as = ic->streams[i];
        if(as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            LOGW("Video Stream...");
            videoStream = i;
            fps = r2d(as->avg_frame_rate);

            LOGW("fps = %d,width=%d height=%d codeid=%d pixformat=%d",fps,
                 as->codecpar->width,
                 as->codecpar->height,
                 as->codecpar->codec_id, // e.g. H264
                 as->codecpar->format //
            );
        }
        else if(as->codecpar->codec_type ==AVMEDIA_TYPE_AUDIO )
        {
            LOGW("Audio Stream...");
            audioStream = i;
            LOGW("sample_rate=%d channels=%d sample_format=%d",
                 as->codecpar->sample_rate,
                 as->codecpar->channels,
                 as->codecpar->format
            );
        }
    }

    // Get Audio Stream
    audioStream = av_find_best_stream(ic,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0); // return selected stream number
    LOGW("av_find_best_stream audioStream = %d",audioStream);
    //////////////////////////////////////////////////////////

    /* Find corresponding decoder */
    AVCodec *codec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);

    /* Hardware Decoding */
    codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if(!codec)
    {
        LOGW("avcodec_find failed!");
        return env->NewStringUTF(hello.c_str());
    }
    /* Decoder Initialization */
    AVCodecContext *vc = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(vc,ic->streams[videoStream]->codecpar);

    /* Multi-Threading decoding */
    vc->thread_count = 8;
    /* Open Decoder */
    re = avcodec_open2(vc,0,0);
    //vc->time_base = ic->streams[videoStream]->time_base;
    LOGW("vc timebase = %d/ %d",vc->time_base.num,vc->time_base.den);
    if(re != 0)
    {
        LOGW("avcodec_open2 video failed!");
        return env->NewStringUTF(hello.c_str());
    }

    //////////////////////////////////////////////////////////

    /* Soft Decoding */
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
    /* Hardware Decoding */
    //codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if(!acodec)
    {
        LOGW("avcodec_find failed!");
        return env->NewStringUTF(hello.c_str());
    }
    /* Decoder Initialization */
    AVCodecContext *ac = avcodec_alloc_context3(acodec);
    avcodec_parameters_to_context(ac,ic->streams[audioStream]->codecpar);
    ac->thread_count = 8;
    /* Open Decoder */
    re = avcodec_open2(ac,0,0);
    if(re != 0)
    {
        LOGW("avcodec_open2  audio failed!");
        return env->NewStringUTF(hello.c_str());
    }

    /* Read Frame Data */
    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    /*  */
    SwsContext* vctx = NULL;
    int outWidth = 1280;
    int outHeight = 720;

    long long start = GetNowMs();
    int frameCount = 0;
    char* rgb = new char[1920*1080*4];
    for(;;)
    {
        /* Record each per 3 seconds */
        if(GetNowMs() - start >= 3000)
        {
            LOGW("now decode fps is %d",frameCount/3);
            start = GetNowMs();
            frameCount = 0;
        }

        int re = av_read_frame(ic,pkt);
        if(re != 0)
        {
            LOGW("Read To End-of-File!");
            int pos = 20 * r2d(ic->streams[videoStream]->time_base);
            av_seek_frame(ic,videoStream,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME );
            continue;
        }

        AVCodecContext *cc = vc;
        if(pkt->stream_index == audioStream)
            cc=ac;

        /* Send to decode */
        re = avcodec_send_packet(cc,pkt);

        /* Presentation timestamp in AVStream->time_base units; */
        int p = pkt->pts;
        av_packet_unref(pkt);

        if(re != 0)
        {
            LOGW("avcodec_send_packet failed!");
            continue;
        }
        for(;;)
        {
            re = avcodec_receive_frame(cc,frame);
            if(re !=0)
            {
                break;
            }
            if(cc == vc)
            {
                frameCount++;
                vctx = sws_getCachedContext(vctx,
                                            frame->width,
                                            frame->height,
                                            (AVPixelFormat)frame->format,
                                            outWidth,
                                            outHeight,
                                            AV_PIX_FMT_RGBA,
                                            SWS_FAST_BILINEAR,
                                            0,
                                            0,
                                            0
                );
                if( !vctx )
                {
                    LOGW("sws_getCachedContext failed!");
                }
                else
                {
                    uint8_t* data[AV_NUM_DATA_POINTERS] = {0};
                    data[0] = (uint8_t*)rgb;
                    int lines[AV_NUM_DATA_POINTERS] = {0};
                    lines[0] = outWidth*4;
                    int h = sws_scale(
                        vctx,
                        (const uint8_t**)frame->data,
                        frame->linesize,
                        0,
                        frame->height,
                        data, 
                        lines
                    );
                    LOGW("sws_scale = %d", h);
                }
            }
        }
    }
    delete rgb;

    avformat_close_input(&ic);
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT jboolean JNICALL
Java_aplay_testffmpeg_MainActivity_Open(JNIEnv *env, jobject instance, jstring url_,
                                        jobject handle) {
    const char *url = env->GetStringUTFChars(url_, 0);

    // TODO
    FILE *fp = fopen(url,"rb");
    if(!fp)
    {
        LOGW("File %s open failed!",url);
    }
    else
    {
        LOGW("File %s open succes!",url);
        fclose(fp);
    }
    env->ReleaseStringUTFChars(url_, url);
    return true;
}
