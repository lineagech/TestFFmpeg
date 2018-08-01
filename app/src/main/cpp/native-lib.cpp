#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"testff",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_ERROR,"testff",__VA_ARGS__)

void PcmCall( SLAndroidSimpleBufferQueueItf bf, void* context)
{
    LOGD("PcmCall");
    static FILE* fp = NULL;
    static char* buf = NULL;
    if( !buf )
    {
        buf = new char[1024*1024];
    }
    if( !fp )
    {
        fp = fopen("/sdcard/test.pcm", "rb");
    }
    if( !fp ) return;
    if( feof(fp) == 0 )
    {
        int len = fread( buf, 1, 1024, fp);
        if( len > 0 )
        {
            (*bf)->Enqueue( bf, buf, len );
        }
    }
}

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavcodec/jni.h>
}
#include<iostream>
using namespace std;

static SLObjectItf engineObject;

static double r2d(AVRational r)
{
    return r.num==0||r.den == 0 ? 0 :(double)r.num/(double)r.den;
}

SLEngineItf createSL()
{
    SLresult re;
    SLEngineItf engineEngine;
    // Create Engine Object
    re = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert( re == SL_RESULT_SUCCESS );

    // Instantnitiate
    re = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert( re == SL_RESULT_SUCCESS );

    // Get Engine Interface
    re = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE/*SLInterfaceID struct*/, &engineEngine);
    assert( re == SL_RESULT_SUCCESS );

    return engineEngine;

}

void testSL()
{
    SLEngineItf eng = createSL();
    if( eng ) LOGD("createSL success!");
    else LOGD("createSL failed");

    SLObjectItf mix = NULL;
    SLresult re = 0;
    re = (*eng)->CreateOuputMix( eng, &mix, 0, 0, 0 );
    if( re != SL_RESULT_SUCCESS ) 
        LOGD("CreateOuputMix failed!");
    re = (*mix)->Realize( mix, SL_BOOLEAN_FALSE );
    if( re != SL_RESULT_SUCCESS ) 
        LOGD("(*mix)->Realize failed!");

    SLDataLocator_OuputMix outmix = {SL_DATALOCATOR_OUTPUTMIX, mix};// store to play
    SLDataSink audioSink = {&outmix, 0};

    /* Config Audio Info */
    SLDataLocator_AndroidSimpleBufferQueue queue
            = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10 };
    SLDataFormat_PCM pcm
            = { SL_DATAFORMAT_PCM,
                2,
                SL_SAMPLINGRATE_44_1,
                SL_PCMSAMPLEFORMAT_FIXED_16,
                SL_PCMSAMPLEFORMAT_FIXED_16,
                SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_BACK_RIGHT,
                SL_BYTEORDER_LITTLEENDIAN };

    SLDataSource ds = { &queue, &pcm };

    /* Create Player */
    SLObjectItf player = NULL;
    SLPlayItf iplayer = NULL;
    SLAndroidSimpleBufferQueueItf pcm_queue = NULL;
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE}; // Open Interface
    re = (*eng)->CreateAudioPlayer(eng, &player, &ds, &audioSink,
                                   sizeof(ids)/sizeof(SLInterfaceID),
                                   ids,
                                   req );
    if( re != SL_RESULT_SUCCESS )
    {
        LOGD("CreateAudioPlayer Failed!");
    } else{
        LOGD("CreateAudioPlayer success!");
    }
    (*player)->Realize(player, SL_BOOLEAN_FALSE);

    /* Get player interface */
    re = (*player)->GetInterface( player, SL_IID_PLAY, &iplayer );
    if( re != SL_RESULT_SUCCESS )
    {
        LOGD("GetInterface SL_IID_PLAY Failed!");
    } else{
        LOGD("GetInterface SL_IID_PLAY success!");
    }

    /* need to register first when createAudioPlayer */
    re = (*player)->GetInterface( player, SL_IID_BUFFERQUEUE, &pcm_queue );
    if( re != SL_RESULT_SUCCESS )
    {
        LOGD("GetInterface SL_IID_PLAY Failed!");
    } else{
        LOGD("GetInterface SL_IID_PLAY success!");
    }

    /* Set Callback function when queue is empty */
    (*pcm_queue)->RegisterCallback( pcm_queue, PcmCall, 0 );

    /* Set playing state */
    (*iplayer)->SetPlayState(iplayer, SL_PLAYSTATE_PLAYING);

    /* Enable callback */
    (*pcm_queue)->Enqueue(pcm_queue, "", 1);

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
extern "C"
JNIEXPORT void JNICALL
Java_aplay_testffmpeg_XPlay_Open(JNIEnv *env, jobject instance, jstring url_, jobject surface) {
    const char *path = env->GetStringUTFChars(url_, 0);

    // TODO
    /* Initialize all */
    av_register_all();

    avformat_network_init();

    avcodec_register_all();

    /* Open the file */
    AVFormatContext *ic = NULL; // AVFormatContext -> unpack strcut for flv, mp4, rmvb, avi
    //char path[] = "/sdcard/1080.mp4";

    int re = avformat_open_input(&ic, path, 0, 0);
    if(re != 0)
    {
        LOGW("avformat_open_input failed!:%s",av_err2str(re));
        return;
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
        return;
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
        return;
    }

    //////////////////////////////////////////////////////////

    /* Soft Decoding */
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
    /* Hardware Decoding */
    //codec = avcodec_find_decoder_by_name("h264_mediacodec");
    if(!acodec)
    {
        LOGW("avcodec_find failed!");
        return;
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
        return;
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


    SwrContext* actx = swr_alloc();
    actx = swr_alloc_set_opts(
            actx,
            av_get_default_channel_layout(2/*ac->channels*/),
            AV_SAMPLE_FMT_S16,
            ac->sample_rate,
            av_get_default_channel_layout(ac->channels),
            ac->sample_fmt,
            ac->sample_rate, 0, 0
    );
    re = swr_init(actx);
    if( re != 0 )
    {
        LOGW("swr_init failed!");
    } else {
        LOGW("swr_init success!");
    }

    char* rgb = new char[1920*1080*4];
    char* pcm = new char[48000*4*2];


    /*  */
    ANativeWindow* nwin = ANativeWindow_fromSurface(env, surface);
    int vWidth = vc->width;
    int vHeight = vc->height;
    ANativeWindow_setBuffersGeometry( nwin, outWidth, outWidth, WINDOW_FORMAT_RGBA_8888 );
    ANativeWindow_Buffer wbuf;

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
                    uint8_t* data[AV_NUM_DATA_POINTERS/*8*/] = {0};
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

                    if( h > 0 )
                    {
                        ANativeWindow_lock( nwin, &wbuf, 0 );
                        uint8_t* dst = (uint8_t*) wbuf.bits;
                        memcpy( dst, rgb, outWidth*outHeight*4 );
                        ANativeWindow_unlockAndPost( nwin );
                    }

                }
            }
            else // Audio
            {
                uint8_t* out[2] = {0};
                out[0] = (uint8_t*)pcm;
                // Audio Resample
                /*int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                                const uint8_t **in , int in_count); */
                int len = swr_convert(
                        actx,
                        out,
                        frame->nb_samples, /* how many samples, usually 1024 */
                        (const uint8_t**)frame->data,
                        frame->nb_samples
                );
                LOGW("swr_convert = %d", len);
            }
        }
    }
    delete rgb;
    delete pcm;

    avformat_close_input(&ic);

    env->ReleaseStringUTFChars(url_, path);
}