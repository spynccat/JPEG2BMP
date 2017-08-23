# JPEG2BMP
FUNCTION:JPG2BMP
SOURCE CODE:main_s.c
LIBRARY USE:libjpeg
RUNNING:test_s


0.api:
int jpg2bmp(char *jpgname,char *bmpname)


input parameters:
jpgname:name of input jpg file.eg:*jpgname="lena.jpg"
bmpname:name of output bmp file.eg:*bmpname="lena.bmp"

output parameters:
return Error Code(see more in No.6)

1.function:decode jpeg image(lena.jpg),translate and output lena.bmp.
2.input:lena.jpg
3.output:lena.bmp
4.function:
(if not listed,see libjpeg document for more information)
int analyse_jpeg()								:analyse jpegfile,decompress jpegfile,write data to buffer(src_buff),using write_bmp_header(),write_bmp_data() to write bmp file
int write_bmp_header(j_decompress_ptr cinfo)						:write bmpfile header,using jpegfile info(cinfo),should used after decompress jpegfile
int write_bmp_data(j_decompress_ptr cinfo,unsigned char *src_buff)	:write bmpfile data,using jpegfile info(cinfo),write data from buffer(src_buff) to bmpfile

5.static vars:
FILE *input_file;										:pointer of input jpeg file
FILE *output_file;										:pointer of output bmp file
enum ERROR_CODE{PASS,ERROR_JPGREAD,ERROR_BMPWRITE};		:error code,see No.6 for more

6.Error Code:
PASS				:pass
ERROR_JPGREAD		:cannot open jpg file or cannot read from jpg file
ERROR_BMPWRITE		:cannot open bmp file or cannot write into it
