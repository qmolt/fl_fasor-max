#ifndef fl_fasor_h
#define fl_fasor_h

/* header files */
#include "ext.h"
#include "z_dsp.h"
#include "ext_obex.h"
#include <math.h>

/* global variables */
#define MINIMUM_FREQUENCY -20000.0
#define DEFAULT_FREQUENCY 1.0
#define MAXIMUM_FREQUENCY 20000.0

#define MINIMUM_TABLE_SIZE 4
#define DEFAULT_TABLE_SIZE 8192
#define MAXIMUM_TABLE_SIZE 1048576

#define MINIMUM_CROSSFADE 0.0
#define DEFAULT_CROSSFADE 200.0
#define MAXIMUM_CROSSFADE 10000.0

#define NO_CROSSFADE 0
#define LINEAR_CROSSFADE 1
#define POWER_CROSSFADE 2

#define N_PUNTOS_DEFAULT 2
#define MAX_PUNTOS_CURVA 20

#define CURVE_MIN -0.98
#define CURVE_MAX 0.98

/* object structure */
typedef struct _fl_fasor {
	t_pxobject obj;

	/* attributes -----------*/
	float a_frequency;
	long a_crossfade_type;
	float a_crossfade_time;
	//long a_curveform;
	/*-----------------------*/

	short frequency_connected;
	short fasor_connected;

	float frequency;
	long table_size;
	long curveform;
	
	long wavetable_bytes;
	float *wavetable;
	float *wavetable_old;

	double fs;

	long bytes_puntos_curva;
	float *puntos_curva;
	int n_puntos;

	float phase;
	float increment;
	short dirty;

	short crossfade_type;
	float crossfade_time;
	long crossfade_samples;
	long crossfade_countdown;
	short crossfade_in_progress;
	short just_turned_on;

	short switch_zz;
} t_fl_fasor;

/* arguments/inlets/outlets/vectors indexes */
enum ARGUMENTS { A_TABLE_SIZE, A_FREQUENCY };
enum INLETS { I_FREQUENCY, I_PHASE, I_LISTACURVA, NUM_INLETS };
enum OUTLETS { O_OUTPUT, NUM_OUTLETS };
//enum DSP { PERFORM, OBJECT, FREQUENCY, OUTPUT, VECTOR_SIZE, NEXT };

/* class pointer */
static t_class *fl_fasor_class;

/* function prototypes------------------------------------------------ */
	/* parsing arguments */
void parse_float_arg(float *variable, float minimum_value, float default_value, float maximum_value, int arg_index, short argc, t_atom *argv);
void parse_int_arg(long *variable, long minimum_value, long default_value, long maximum_value, int arg_index, short argc, t_atom *argv);
void parse_symbol_arg(t_symbol **variable, t_symbol *default_value, int arg_index, short argc, t_atom *argv);

	/* new, inlets */
void *fl_fasor_new(t_symbol *s, short argc, t_atom *argv);
void fl_fasor_float(t_fl_fasor *x, double farg);
void fl_fasor_curva_list(t_fl_fasor *x, t_symbol *msg, short argc, t_atom *argv);
void fl_fasor_assist(t_fl_fasor *x, void *b, long msg, long arg, char *dst);

	/*attributes*/
t_max_err a_frequency_set(t_fl_fasor *x, void *attr, long ac, t_atom *av);
t_max_err a_crossfade_type_set(t_fl_fasor *x, void *attr, long ac, t_atom *av);
t_max_err a_crossfade_time_set(t_fl_fasor *x, void *attr, long ac, t_atom *av);

	/* wavetable and crossfade */
void fl_fasor_build_wavetable(t_fl_fasor *x);
void fl_fasor_build_curveform(t_fl_fasor *x);
float parse_curve(float curva);
void fl_fasor_fadetime(t_fl_fasor *x, t_symbol *msg, short argc, t_atom *argv);
void fl_fasor_fadetype(t_fl_fasor *x, t_symbol *msg, short argc, t_atom *argv);

	/* memory stuff */
void fl_fasor_free(t_fl_fasor *x);

	/* audio stuff */
void fl_fasor_dsp64(t_fl_fasor *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void fl_fasor_perform64(t_fl_fasor *x, t_object *dsp64, double **inputs, long numinputs, double **outputs, long numoutputs, long vectorsize, long flags, void *userparams);

#endif /* fl_fasor_h */
