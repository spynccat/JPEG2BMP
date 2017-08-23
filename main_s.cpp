#include <iostream>
#include <stdio.h>
#include<cstring>
extern "C"{
#include <jpeglib.h>
};
#pragma comment(lib,"jpeg.lib")
#pragma pack(2)        //两字节对齐，否则bmp_fileheader会占16Byte

using namespace std;

struct bmp_fileheader
{
    unsigned short    bfType;        //若不对齐，这个会占4Byte
    unsigned int    bfSize;
    unsigned short    bfReverved1;
    unsigned short    bfReverved2;
    unsigned int    bfOffBits;
};

struct bmp_infoheader
{
    unsigned int    biSize;
    unsigned int    biWidth;
    unsigned int    biHeight;
    unsigned short    biPlanes;
    unsigned short    biBitCount;
    unsigned int    biCompression;
    unsigned int    biSizeImage;
    unsigned int    biXPelsPerMeter;
    unsigned int    biYpelsPerMeter;
    unsigned int    biClrUsed;
    unsigned int    biClrImportant;
};

FILE *input_file;
FILE *output_file;
enum ERROR_CODE{PASS,ERROR_JPGREAD,ERROR_BMPWRITE};

int write_bmp_header(j_decompress_ptr cinfo)
{
    struct bmp_fileheader bfh;
    struct bmp_infoheader bih;

    unsigned long width;
    unsigned long height;
    unsigned short depth;
    unsigned long headersize;
    unsigned long filesize;

    width=cinfo->output_width;
    height=cinfo->output_height;
    depth=cinfo->output_components;

    if (depth==1)
    {
        headersize=14+40+256*4;
        filesize=headersize+width*height;
    }

    if (depth==3)
    {
        headersize=14+40;
        filesize=headersize+width*height*depth;
    }

    memset(&bfh,0,sizeof(struct bmp_fileheader));
    memset(&bih,0,sizeof(struct bmp_infoheader));
    
    //写入比较关键的几个bmp头参数
    bfh.bfType=0x4D42;
    bfh.bfSize=filesize;
    bfh.bfOffBits=headersize;

    bih.biSize=40;
    bih.biWidth=width;
    bih.biHeight=height;
    bih.biPlanes=1;
    bih.biBitCount=(unsigned short)depth*8;
    bih.biSizeImage=width*height*depth;

    cout<<"filesize:"<<filesize<<endl;
    cout<<"headersize:"<<headersize<<endl;
    cout<<"width:"<<width<<endl;
    cout<<"height:"<<height<<endl;
    cout<<"depth:"<<depth<<endl;

    if(fwrite(&bfh,sizeof(struct bmp_fileheader),1,output_file)!=1)	return ERROR_BMPWRITE;
    if(fwrite(&bih,sizeof(struct bmp_infoheader),1,output_file)!=1)	return ERROR_BMPWRITE;

    if (depth==1)        //灰度图像要添加调色板
    {
        unsigned char *platte;
        platte=new unsigned char[256*4];
        unsigned char j=0;
        for (int i=0;i<1024;i+=4)
        {
            platte[i]=j;
            platte[i+1]=j;
            platte[i+2]=j;
            platte[i+3]=0;
            j++;
        }
        if(fwrite(platte,sizeof(unsigned char)*1024,1,output_file)!=1)	return ERROR_BMPWRITE;
        delete[] platte;
    }
	
	return PASS;
}

int write_bmp_data(j_decompress_ptr cinfo,unsigned char *src_buff)
{
    unsigned char *dst_width_buff;
    unsigned char *point;

    unsigned long width;
    unsigned long height;
    unsigned short depth;

    width=cinfo->output_width;
    height=cinfo->output_height;
    depth=cinfo->output_components;

    dst_width_buff=new unsigned char[width*depth];
    memset(dst_width_buff,0,sizeof(unsigned char)*width*depth);

    point=src_buff+width*depth*(height-1);    //倒着写数据，bmp格式是倒的，jpg是正的
    for (unsigned long i=0;i<height;i++)
    {
        for (unsigned long j=0;j<width*depth;j+=depth)
        {
            if (depth==1)        //处理灰度图
            {
                dst_width_buff[j]=point[j];
            }

            if (depth==3)        //处理彩色图
            {
                dst_width_buff[j+2]=point[j+0];
                dst_width_buff[j+1]=point[j+1];
                dst_width_buff[j+0]=point[j+2];
            }
        }
        point-=width*depth;
        if(fwrite(dst_width_buff,sizeof(unsigned char)*width*depth,1,output_file)!=1)	return ERROR_BMPWRITE;    //一次写一行
    }
	return PASS;
}

int analyse_jpeg()
{
	int status=PASS;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    unsigned char *src_buff;
    unsigned char *point;

    cinfo.err=jpeg_std_error(&jerr);    //一下为libjpeg函数，具体参看相关文档
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo,input_file);
    jpeg_read_header(&cinfo,TRUE);
    jpeg_start_decompress(&cinfo);

    //cout<<"jpegfinish\n";

    unsigned long width=cinfo.output_width;
    unsigned long height=cinfo.output_height;
    unsigned short depth=cinfo.output_components;

    src_buff=new unsigned char[width*height*depth];
    memset(src_buff,0,sizeof(unsigned char)*width*height*depth);

    buffer=(*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo,JPOOL_IMAGE,width*depth,1);

    point=src_buff;
    while (cinfo.output_scanline<height)
    {
        jpeg_read_scanlines(&cinfo,buffer,1);    //读取一行jpg图像数据到buffer
        memcpy(point,*buffer,width*depth);    //将buffer中的数据逐行给src_buff
        point+=width*depth;            //一次改变一行
    }

    //cout<<"readfinish\n";

    if((status=write_bmp_header(&cinfo))!=PASS)	return status;            //写bmp文件头
    if((status=write_bmp_data(&cinfo,src_buff))!=PASS)	return status;    //写bmp像素数据

    //cout<"writefinish\n";

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    delete[] src_buff;
	delete[] dst_width_buff;

	return status;
}

int jpg2bmp(char *jn,char *bn)
{
	int status=PASS;
	
    input_file=fopen(jn,"rb");
    output_file=fopen(bn,"wb");
    if(input_file==NULL)
    {
		return ERROR_JPGREAD;
    }

    if((status=analyse_jpeg())!=PASS)	return status;

    fclose(input_file);
    fclose(output_file);

    //cout<<"good job."<<endl;
    //cin.get();
    return status;
}

int main()
{
	char *jn="lena.jpg";
	char *bn="lena.bmp";
	return jpg2bmp(jn,bn);
}
