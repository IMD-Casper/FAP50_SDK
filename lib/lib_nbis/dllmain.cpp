// dllmain.cpp : Defines the entry point for the DLL application.
#include "imd_framework.h"

#include "InterfaceImdFap50Method.h"
#include "lib_nbis.h"

extern "C" 
{
#include <wsq.h>
extern int write_raw_from_memsize(char*, unsigned char*, const int);

//#include "nfiq.h"
//extern int comp_nfiq(int*, float*, unsigned char*,
//	const int, const int, const int, const int, int*);
int comp_nfiq(int* onfiq, float* oconf, unsigned char* idata,
	const int iw, const int ih, const int id, const int ippi,
	int* optflag);
}

int debug = 0;

IMD_STD_API int get_nfiq_fap20_fap30(void* pSrc, int* score, int bitrate_, int ppi_, int width_, int height_)
{
	float conf = 0;
	BYTE* input_data = static_cast<BYTE*>(pSrc);
	int width = width_;
	int height = height_;
	int bitrate = bitrate_;
	int ppi = ppi_;
	int verbose = 0;

	int ret = comp_nfiq(score, &conf, input_data, width, height, bitrate, ppi, &verbose);
	return *score;
}


IMD_STD_API int get_nfiq(void* pSrc, int *score, SystemProperty* property)
{
	Mat* src = (Mat*)pSrc;

	float conf = 0;
	BYTE* input_data = src->data;
	int width = src->cols;
	int	height = src->rows;
	int bitrate = property->image_bit_per_pix;
	int ppi = property->image_pix_per_inch;
	int verbose = 0;
	int ret = 0;
	ret = comp_nfiq(score, &conf, input_data, width, height, bitrate, ppi, &verbose);

	return 0;
}


IMD_STD_API void save_wsq(void *pSrc, const char* file_path, SystemProperty* property)
{
	Mat* src = (Mat*)pSrc;

	unsigned char* odata = NULL;
	int olen = 0;
	BYTE* input_data = src->data;
	int width = src->cols;
	int	height = src->rows;
	float bitrate = property->wsq_bit_rate;
	int pix_per_inch = property->image_pix_per_inch;
	int bit_per_pix = property->image_bit_per_pix;
	char* comment_text = property->wsq_comment_text;
	int encode_ret = wsq_encode_mem(
		&odata, &olen, bitrate, input_data, width, height, bit_per_pix, pix_per_inch, comment_text);

	FILE* wsq_file = nullptr;

#ifdef _WIN32
	fopen_s(&wsq_file, file_path, "wb");
#else
	wsq_file = fopen(file_path, "wb");
#endif

	if (wsq_file)
	{
		fwrite(odata, 1, olen, wsq_file);
		fclose(wsq_file);
	}

	if (odata)
		free(odata);
}