#include <iostream>
#include <string>
#include <memory>
#include <stdio.h>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}


using namespace std;

extern "C"{
    void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
        FILE *pFile;
        char szFilename[32];
        int
        y;
        // Open file
        sprintf(szFilename, "frame%d.ppm", iFrame);
        pFile=fopen(szFilename, "wb");
        if(pFile==NULL)
        return;
        fprintf(pFile, "P6\n%d %d\n255\n", width, height);
        // Write pixel data
        for(y=0; y<height; y++)
        fwrite(pFrame->data[1]+y*pFrame->linesize[2], 1, width, pFile);
        // Close file
        fclose(pFile);
    }
}

int main(int argc,char *argv[])
{
    string fileName;
    int videoStreamIndex = -1;
    AVFormatContext *p_avFormateCtx;
    AVCodecContext *p_avCodecCtx;
    AVCodecContext *p_avCodecCtxDecode;
    AVCodec *p_avCodec;

    fileName = argv[1];
    //1.register
    av_register_all();
    //2.init network
    avformat_network_init();
    //3.alloc avFormatCtx
    p_avFormateCtx = avformat_alloc_context();
    //4.open input
    if(avformat_open_input(&p_avFormateCtx,fileName.c_str(),NULL,NULL) != 0){
        cout << "can not open " << fileName << endl;
        //return -1;
    }
    //5.find stream info
    if(avformat_find_stream_info(p_avFormateCtx,NULL) != 0){
        cout << "can not find stream info" << endl;
        return -1;
    }
    //6.print file info
    av_dump_format(p_avFormateCtx,0,fileName.c_str(),0);
    //7.find video stream
    cout << "nb_streams : " << p_avFormateCtx->nb_streams << endl;
    for(int i = 0;i < p_avFormateCtx->nb_streams;i++){
        if(p_avFormateCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStreamIndex = i;
            break;
        }
    }
    cout << "videoStreamIndex : " << videoStreamIndex << endl;
    //8.find decoder
    p_avCodecCtx = p_avFormateCtx->streams[videoStreamIndex]->codec;
    p_avCodec = avcodec_find_decoder(p_avCodecCtx->codec_id);

    //8.open codec
    p_avCodecCtxDecode = avcodec_alloc_context3(p_avCodec);
    avcodec_copy_context(p_avCodecCtxDecode,p_avCodecCtx);

    if(avcodec_open2(p_avCodecCtxDecode,p_avCodec,NULL) < 0){
        cout << "avcodec_open2 failed" << endl;
        return -1;
    }

    //malloc some memery
    AVFrame *p_avFrame = avcodec_alloc_frame();
    int decodeFinish = 1;

    AVFrame *p_avFrameRGB24 = avcodec_alloc_frame();

    int byteNum = avpicture_get_size(AV_PIX_FMT_RGB24,p_avCodecCtx->width,p_avCodecCtx->height);
    uint8_t *buffer = (uint8_t *)av_malloc(byteNum * sizeof(uint8_t));
    avpicture_fill((AVPicture*)p_avFrameRGB24,buffer,AV_PIX_FMT_RGB24,p_avCodecCtx->width,p_avCodecCtx->height);

    SwsContext *p_swsContext;
    p_swsContext = sws_getCachedContext(NULL,
                                        p_avCodecCtx->width,
                                        p_avCodecCtx->height,
                                        p_avCodecCtx->pix_fmt,
                                        p_avCodecCtx->width,
                                        p_avCodecCtx->height,
                                        AV_PIX_FMT_RGB24,
                                        SWS_BICUBIC,
                                        NULL,
                                        NULL,
                                        NULL);
    cout << "timeBase.num : " << p_avFormateCtx->streams[videoStreamIndex]->time_base.num << endl;
    cout << "timeBase.den : " << p_avFormateCtx->streams[videoStreamIndex]->time_base.den << endl;
    //9.read frame
    AVPacket packet;
    int i = 0;
    while(av_read_frame(p_avFormateCtx,&packet) >= 0){
        //10.decode frame

        //cout << "pts : " << packet.pts << " ; "
        //     << "dts : " << packet.dts << " ; " << endl;
        //printf("dts : %d ; pts : %d\n",packet.dts,packet.pts);

        if(packet.stream_index == videoStreamIndex){
            avcodec_decode_video2(p_avCodecCtxDecode,p_avFrame,&decodeFinish,&packet);
            cout << "decodeFinish : " << decodeFinish << endl;
            if(decodeFinish != 0){
                //11.convert YUV420 to RGB24
                printf("pkt_dts: %d;pkt_pts: %d\n",p_avFrame->pkt_dts,p_avFrame->pkt_pts);
                printf("pts: %d\n",p_avFrame->pts);
                printf("pict_type : %d\n",p_avFrame->pict_type);

                sws_scale(p_swsContext,(uint8_t const * const *)p_avFrame->data,p_avFrame->linesize,0,p_avCodecCtx->height,p_avFrameRGB24->data,p_avFrameRGB24->linesize);
                if(i++ < 5)
                    SaveFrame(p_avFrame,p_avCodecCtx->width,p_avCodecCtx->height,i);
            }

        }
    }

    return 0;
}
