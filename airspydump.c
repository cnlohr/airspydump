#include <libairspy/airspy.h>
#include <stdio.h>
#include <semaphore.h>

#define CAP_SAMPS 60000000
//#define CAP_SAMPS 60000
#define FREQ 1090000000
#define SAMPLERATE 6000000
#define IQsDat "IQs.dat"
#define LNA_GAIN 12   // Causes spurs above 10dB
#define MIXER_GAIN 14 // No seeming ill effects, up to 15dB (Though the noise floor is a tad higher)
#define VGA_GAIN 10   // Generally raises baseline noise above 10dB

int32_t IQs[CAP_SAMPS];

uint64_t serial = 0;
struct airspy_device * dev;
sem_t semrun;
int samples = 0;
int mins = 0;
int maxs = 0;

int callback(airspy_transfer* transfer)
{
	int i;
	uint32_t * sdata = (uint32_t*)transfer->samples;
	int sc = transfer->sample_count;
	for( i = 0; i < sc; i++ )
	{
		if( (samples % 1000000) == 0 ) { fprintf( stderr, "." ); fflush(stderr); }
		if( samples >= CAP_SAMPS )
		{
			sem_post( &semrun );
			break;
		}
		uint32_t iq = IQs[samples++] = sdata[i];
		int16_t iv = iq>>16;
		int16_t qv = iq;
		if( iv < mins ) mins = iv;
		if( iv > maxs ) maxs = iv;
		if( qv < mins ) mins = qv;
		if( qv > maxs ) maxs = qv;
	}
	return 0;
}


int main()
{
	int r;
	r = sem_init( &semrun, 0, 0 );
	if( r < 0 ) goto fail;
	r = airspy_init();
	if( r ) goto fail;
	fprintf( stderr, "Airspy Library Initted\n" );
	r = airspy_list_devices( &serial, 1 );
	if( r < 0 ) goto fail;
	fprintf( stderr, "Serial: %016x\n", serial );
	r = airspy_open_sn( &dev, serial );
	if( r < 0 ) goto fail;
	fprintf( stderr, "Device opened\n" );

	r = airspy_set_freq( dev, FREQ );
	if( r < 0 ) goto fail;
	fprintf( stderr, "Set tuner to %fMHz\n", FREQ / 1000000.0 );

	r = airspy_set_rf_bias( dev, 1 ); // Enable bias T
	if( r < 0 ) goto fail;

	r = airspy_set_lna_agc( dev, 0 );
	if( r < 0 ) goto fail;
	r = airspy_set_mixer_agc( dev, 0 );
	if( r < 0 ) goto fail;

	r = airspy_set_lna_gain( dev, LNA_GAIN );
	if( r < 0 ) goto fail;
	r = airspy_set_mixer_gain( dev, MIXER_GAIN );
	if( r < 0 ) goto fail;
	r = airspy_set_vga_gain( dev, VGA_GAIN );
	if( r < 0 ) goto fail;

	r = airspy_set_sample_type( dev, AIRSPY_SAMPLE_INT16_IQ );
	if( r < 0 ) goto fail;
	fprintf( stderr, "Sample Type Configured\n" );

	r = airspy_set_samplerate( dev, SAMPLERATE );
	if( r < 0 ) goto fail;
	fprintf( stderr, "Sample Rate Set\n" );

	r = airspy_start_rx( dev, callback, 0 );
	if( r < 0 ) goto fail;
	fprintf( stderr, "Capturing\n" );

	sem_wait( &semrun );
	r = airspy_stop_rx( dev );
	if( r < 0 ) goto fail;
	sem_close( &semrun );

	fprintf( stderr, "Complete; range = %d to %d\n", mins, maxs );

	FILE * fIQ = fopen( IQsDat, "wb" );
	r = fwrite( IQs, sizeof(IQs), 1, fIQ );
	if( r != 1 ) goto fail;
	fclose( fIQ );

	fprintf( stderr, "Wrote out to %s\n", IQsDat );

	return 0;
fail:
	fprintf( stderr, "Fail, with error code: %d\n", r );
	return 0;
}

