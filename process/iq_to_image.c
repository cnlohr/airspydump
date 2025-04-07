/*
MIT License

Copyright (c) 2024 Charles Lohr "CNLohr"

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

uint32_t HSVtoHEX( float hue, float sat, float value );

int main( int argc, char ** argv )
{
	if( argc != 3 )
	{
		fprintf( stderr, "Usage: iq_to_image [IQs] [IMAGE.png]\n" );
		return -1;
	}
	FILE * f = fopen( argv[1], "rb" );
	if( !f ) { fprintf( stderr, "Error: can't open %s\n", argv[1] ); exit( -59 ); }
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );
	len/=sizeof(uint32_t);
	int16_t * data_I = malloc( len * sizeof( int16_t ) );
	int16_t * data_Q = malloc( len * sizeof( int16_t ) );

	float * data_M = malloc( len * sizeof( float ) );

	if( len > 4096*4096 ) len = 4096*4096;

	int i;
	for( i = 0; i < len; i++ )
	{
		int r = fread( data_I + i , 1, sizeof( int16_t ), f );
		r = fread( data_Q + i , 1, sizeof( int16_t ), f );
	}



	int width = 4096;
	int height = len / width;
	printf( "%d/%d\n", width, height );

	uint32_t * outimage = malloc( sizeof( uint32_t ) * width * height );
	int y, x;
	float hueofs = 0.0;
	for( y = 0; y < height; y++ )
	{
		float linemin = 0;
		float linemax = .04;

		for( x = 0; x < width; x++ )
		{
			float I = data_I[x+y*width]/16384.0;
			float Q = data_Q[x+y*width]/16384.0;

			//I= sqrtf(I*I);
			//Q= sqrtf(Q*Q);
			//data_I[x+y*width] = I;
			//data_Q[x+y*width] = Q;
			data_M[x+y*width] = 
				//log( sqrtf( I*I + Q*Q ) );
				sqrtf( I*I + Q*Q );
		}

		int highestindex = 0;

		for( x = 0; x < width; x++ )
		{
			float M = data_M[x+y*width];
			if( M < linemin ) linemin = M;
			if( M > linemax ) { linemax = M; highestindex = x; }
		}

		for( x = 0; x < width; x++ )
		{
			float I = data_I[x+y*width]/16384.0;
			float Q = data_Q[x+y*width]/16384.0;
			float M = data_M[x+y*width];
			float Mm = ( M - linemin ) / (linemax - linemin);
			float hue = (((atan2( I, Q ) / 3.1415926)+1)/2.0);

			hueofs += .003;
			hue += hueofs;
			if( hueofs >= 1.0 ) hueofs -= 1.0;	
			outimage[x+y*width] = HSVtoHEX( hue, 1.0, Mm ) | 0xff000000; 
		//	if( x == highestindex && M > 1.0 ) printf( "%f %f %08x\n", M, hue, outimage[x+y*width] );

		}
	}
	stbi_write_png( argv[2], width, height, 4, outimage, width*4 );
}


uint32_t HSVtoHEX( float hue, float sat, float value )
{

	float pr = 0.0;
	float pg = 0.0;
	float pb = 0.0;

	short ora = 0.0;
	short og = 0.0;
	short ob = 0.0;

	float ro = fmod( hue * 6.0, 6.0 );

	float avg = 0.0;

	ro = fmod( ro + 6.0 + 1.0, 6.0 ); //Hue was 60* off...

	if( ro < 1.0 ) //yellow->red
	{
		pr = 1.0;
		pg = 1.0 - ro;
	} else if( ro < 2.0 )
	{
		pr = 1.0;
		pb = ro - 1.0;
	} else if( ro < 3.0 )
	{
		pr = 3.0 - ro;
		pb = 1.0;
	} else if( ro < 4.0 )
	{
		pb = 1.0;
		pg = ro - 3.0;
	} else if( ro < 5.0 )
	{
		pb = 5.0 - ro;
		pg = 1.0;
	} else
	{
		pg = 1.0;
		pr = ro - 5.0;
	}

	//Actually, above math is backwards, oops!
	pr *= value;
	pg *= value;
	pb *= value;

	avg += pr;
	avg += pg;
	avg += pb;

	pr = pr * sat + avg * (1.0-sat);
	pg = pg * sat + avg * (1.0-sat);
	pb = pb * sat + avg * (1.0-sat);

	ora = pr * 255;
	og = pb * 255;
	ob = pg * 255;

	if( ora < 0 ) ora = 0;
	if( ora > 255 ) ora = 255;
	if( og < 0 ) og = 0;
	if( og > 255 ) og = 255;
	if( ob < 0 ) ob = 0;
	if( ob > 255 ) ob = 255;

    // Pack bits in RGBA format
    return (ora << 0) | (og << 8) | (ob << 16);
}


