#include "fl_fasor~.h"

void ext_main(void *r)
{
	//create class
	fl_fasor_class = class_new("fl_fasor~", (method)fl_fasor_new, (method)fl_fasor_free, sizeof(t_fl_fasor), 0, A_GIMME, 0);

	//methods
	class_addmethod(fl_fasor_class, (method)fl_fasor_dsp64, "dsp64", A_CANT, 0);
	class_addmethod(fl_fasor_class, (method)fl_fasor_float, "float", A_FLOAT, 0);
	class_addmethod(fl_fasor_class, (method)fl_fasor_assist, "assist", A_CANT, 0);
	class_addmethod(fl_fasor_class, (method)fl_fasor_curva_list, "list", A_GIMME, 0);
	class_addmethod(fl_fasor_class, (method)fl_fasor_fadetime, "fadetime", A_GIMME, 0);
	class_addmethod(fl_fasor_class, (method)fl_fasor_fadetype, "fadetype", A_GIMME, 0);

	class_dspinit(fl_fasor_class);

	//attributes
	CLASS_ATTR_FLOAT(fl_fasor_class, "frequency", ATTR_SET_OPAQUE_USER, t_fl_fasor, a_frequency);
	CLASS_ATTR_LABEL(fl_fasor_class, "frequency", ATTR_SET_OPAQUE_USER, "Frequency (Hz)");
	CLASS_ATTR_ORDER(fl_fasor_class, "frequency", ATTR_SET_OPAQUE_USER, "1");
	CLASS_ATTR_FILTER_CLIP(fl_fasor_class, "frequency", MINIMUM_FREQUENCY, MAXIMUM_FREQUENCY);
	CLASS_ATTR_ACCESSORS(fl_fasor_class, "frequency", NULL, a_frequency_set);

	CLASS_ATTR_LONG(fl_fasor_class, "fadetype", ATTR_SET_OPAQUE_USER, t_fl_fasor, a_crossfade_type);
	CLASS_ATTR_LABEL(fl_fasor_class, "fadetype", ATTR_SET_OPAQUE_USER, "Crossfade type");
	CLASS_ATTR_ORDER(fl_fasor_class, "fadetype", ATTR_SET_OPAQUE_USER, "2");
	CLASS_ATTR_FILTER_CLIP(fl_fasor_class, "fadetype", NO_CROSSFADE, POWER_CROSSFADE);
	CLASS_ATTR_ENUMINDEX(fl_fasor_class, "fadetype", 0,
		"\"No Fade\" \"Linear\" \"Equal power\"");
	CLASS_ATTR_ACCESSORS(fl_fasor_class, "fadetype", NULL, a_crossfade_type_set);

	CLASS_ATTR_FLOAT(fl_fasor_class, "fadetime", ATTR_SET_OPAQUE_USER, t_fl_fasor, a_crossfade_time);
	CLASS_ATTR_LABEL(fl_fasor_class, "fadetime", ATTR_SET_OPAQUE_USER, "Crossfade time (ms)");
	CLASS_ATTR_ORDER(fl_fasor_class, "fadetime", ATTR_SET_OPAQUE_USER, "3");
	CLASS_ATTR_FILTER_CLIP(fl_fasor_class, "fadetype", MINIMUM_CROSSFADE, MAXIMUM_CROSSFADE);
	CLASS_ATTR_ACCESSORS(fl_fasor_class, "fadetime", NULL, a_crossfade_time_set);

	//register class
	class_register(CLASS_BOX, fl_fasor_class);
}

/* parsing arguments ------------------------------------------------------------------------------ */
void parse_float_arg(float *variable, float minimum_value, float default_value, float maximum_value, int arg_index, short argc, t_atom *argv)
{
	*variable = default_value;
	if (argc > arg_index) { *variable = atom_getfloatarg(arg_index, argc, argv); }
	*variable = min(maximum_value, max(minimum_value, *variable));
}
void parse_int_arg(long *variable, long minimum_value, long default_value, long maximum_value, int arg_index, short argc, t_atom *argv)
{
	*variable = default_value;
	if (argc > arg_index) { *variable = (long)atom_getintarg(arg_index, argc, argv); }
	*variable = min(maximum_value, max(minimum_value, *variable));

}
void parse_symbol_arg(t_symbol **variable, t_symbol *default_value, int arg_index, short argc, t_atom *argv)
{
	*variable = default_value;
	if (argc > arg_index) { *variable = atom_getsymarg(arg_index, argc, argv); }
}

/* max stuff: new, inlets, arguments ----------------------------------------------*/
void *fl_fasor_new(t_symbol *s, short argc, t_atom *argv)
{
	t_fl_fasor *x = (t_fl_fasor *)object_alloc(fl_fasor_class);

	inlet_new(x, "list");
	dsp_setup((t_pxobject *)x, 2);
	outlet_new((t_object *)x, "signal");
	x->obj.z_misc |= Z_NO_INPLACE;

	parse_int_arg(&x->table_size, MINIMUM_TABLE_SIZE, DEFAULT_TABLE_SIZE, MAXIMUM_TABLE_SIZE, A_TABLE_SIZE, argc, argv);
	parse_float_arg(&x->frequency, MINIMUM_FREQUENCY, DEFAULT_FREQUENCY, MAXIMUM_FREQUENCY, A_FREQUENCY, argc, argv);

	x->wavetable_bytes = x->table_size * sizeof(float);
	x->wavetable = (float *)new_memory(x->wavetable_bytes);
	x->wavetable_old = (float *)new_memory(x->wavetable_bytes);

	x->n_puntos = N_PUNTOS_DEFAULT;
	x->bytes_puntos_curva = MAX_PUNTOS_CURVA * 3 * sizeof(float);
	x->puntos_curva = (float *)sysmem_newptr(x->bytes_puntos_curva);
	if (x->puntos_curva == NULL) {
		object_error((t_object *)x, "no hay espacio de memoria para puntos ventana");
	}
	else {
		x->puntos_curva[0] = 0.0;
		x->puntos_curva[1] = 0.0;
		x->puntos_curva[2] = 0.0;
		x->puntos_curva[3] = 1.0;
		x->puntos_curva[4] = DEFAULT_TABLE_SIZE;
		x->puntos_curva[5] = 0.5;
	}

	x->fs = sys_getsr();

	x->phase = 0;
	x->increment = (float)(x->table_size / x->fs);
	x->dirty = 0;

	x->crossfade_type = LINEAR_CROSSFADE;
	x->crossfade_time = DEFAULT_CROSSFADE;
	x->crossfade_samples = (long)(x->crossfade_time * x->fs / 1000.0);
	x->crossfade_countdown = 0;
	x->crossfade_in_progress = 0;
	x->just_turned_on = 1;

	x->switch_zz = 1;

	x->a_frequency = x->frequency;
	x->a_crossfade_type = x->crossfade_type;
	x->a_crossfade_time = x->crossfade_time;
	attr_args_process(x, argc, argv);

	fl_fasor_build_wavetable(x);

	return x;
}

void fl_fasor_float(t_fl_fasor *x, double farg)
{
	long inlet = ((t_pxobject *)x)->z_in;
	switch (inlet) {
	case 0:
		farg = min(MAXIMUM_FREQUENCY, max(MINIMUM_FREQUENCY, fabs(farg)));
		x->a_frequency = x->frequency = (float)farg;
		object_attr_touch((t_object *)x, gensym("frequency"));
		break;
	case 1:
		farg = min(1.0, max(0.0, farg));
		x->phase = (float)(farg * x->table_size);
		break;
	}
}

void fl_fasor_curva_list(t_fl_fasor *x, t_symbol *s, short argc, t_atom *argv)
{
	if (argc % 3 != 0) {
		object_error((t_object *)x, "multiplo de 3");
		return;
	}

	if (argc > MAX_PUNTOS_CURVA * 3) {
		object_error((t_object *)x, "muchos puntos");
		return;
	}

	if (!x->dirty) {
		t_atom *aptr = argv;
		x->n_puntos = argc / 3;

		for (int i = 0; i < argc; i++, aptr++) {
			if (!((i + 1) % 3)) {
				x->puntos_curva[i] = parse_curve((float)atom_getfloat(aptr));
			}
			else {
				x->puntos_curva[i] = (float)atom_getfloat(aptr);
			}
		}

		fl_fasor_build_wavetable(x);
	}
}

void fl_fasor_assist(t_fl_fasor *x, void *b, long msg, long arg, char *dst)
{
	if (msg == ASSIST_INLET) {
		switch (arg) {
		case I_FREQUENCY: sprintf(dst, "(signal/float) Frequency");
			break;
		case I_PHASE: sprintf(dst, "(signal/float) Phase");
			break;
		case I_LISTACURVA: sprintf(dst, "(list) lista formato curva (y dx c)");
			break;
		}
	}
	else if (msg == ASSIST_OUTLET) {
		switch (arg) {
		case O_OUTPUT: sprintf(dst, "(signal) Output zero to one");
			break;
		}
	}
}

t_max_err a_frequency_set(t_fl_fasor *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		x->a_frequency = (float)atom_getfloat(av);
		x->frequency = x->a_frequency;
	}
	return MAX_ERR_NONE;
}

t_max_err a_crossfade_type_set(t_fl_fasor *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		x->a_crossfade_type = (long)atom_getlong(av);
		x->crossfade_type = (short)x->a_crossfade_type;
	}
	return MAX_ERR_NONE;
}

t_max_err a_crossfade_time_set(t_fl_fasor *x, void *attr, long ac, t_atom *av)
{
	if (ac && av) {
		x->a_crossfade_time = (float)atom_getfloat(av);
		x->crossfade_time = x->a_crossfade_time;
	}
	fl_fasor_fadetime(x, NULL, (short)ac, av);
	return MAX_ERR_NONE;
}

/* The object-specific methods ************************************************/
void fl_fasor_build_wavetable(t_fl_fasor *x)
{
	long table_size = x->table_size;
	float *wavetable = x->wavetable;
	float *wavetable_old = x->wavetable_old;

	if (x->crossfade_in_progress) { return; }

	for (int ii = 0; ii < table_size; ii++) {
		wavetable_old[ii] = wavetable[ii];
	}
	x->dirty = 1;

	/* Initialize crossfade and wavetable */
	if (x->crossfade_type != NO_CROSSFADE && !x->just_turned_on) {
		x->crossfade_countdown = x->crossfade_samples;
		x->crossfade_in_progress = 1;
	}
	else {
		x->crossfade_countdown = 0;
		x->crossfade_in_progress = 0;
	}

	/* Initialize (clear) wavetable with DC component */
	for (int ii = 0; ii < table_size; ii++) {
		wavetable[ii] = 0.5;
	}

	/* Build the wavetable */
	fl_fasor_build_curveform(x);

	/* //Normalize wavetable to a peak value of 1.0 
	float max = 0.0;
	for (int ii = 0; ii < table_size; ii++) {
		if (max < fabs(wavetable[ii])) {
			max = fabs(wavetable[ii]);
		}
	}
	if (max != 0.0) {
		float rescale = 1.0 / max;
		for (int ii = 0; ii < table_size; ii++) {
			wavetable[ii] *= rescale;
		}
	}*/

	x->dirty = 0;
	x->just_turned_on = 0;
}

void fl_fasor_build_curveform(t_fl_fasor *x) 
{
	long table_size = x->table_size;
	float *wavetable = x->wavetable;
	float *puntos_curva = x->puntos_curva;
	int n = x->n_puntos;

	float accum = 0;
	int dom = 0;
	for (int i = 0; i < n; i++) {	//accum dominio function
		accum += puntos_curva[i * 3 + 1];
	}
	dom = (int)accum;
	
	float x_i = 0.0;
	int j = 0;
	int k = 0;
	int segmento = -1;
	float y_i = 0.0;
	float y_f = 0.0;
	float curva = 0.5;

	for (int i = 0, j = 0; i < table_size; i++, j++) {	//k:puntos i:samps_table j:samps_segm 
		if (j > segmento) {
			y_f = puntos_curva[k * 3];
			segmento = (int)(puntos_curva[k * 3 + 1] * table_size / dom);
			curva = puntos_curva[k * 3 + 2];
			y_i = puntos_curva[((n + k - 1) % n) * 3];
			j = 0;
			k++;
		}
		
		x_i = (j / (float)max(segmento,1));
		x->wavetable[i] = ((float)pow(x_i, curva))*(y_f - y_i) + y_i;
	}
}

float parse_curve(float curva) 
{	
	curva = (float)min(CURVE_MAX, max(CURVE_MIN, curva));
	if (curva > 0.0) { return (float)(1.0 / (1.0 - curva)); }
	else { return (float)(curva + 1.0); }
}

void fl_fasor_fadetime(t_fl_fasor *x, t_symbol *msg, short argc, t_atom *argv)
{
	if (argc > 1) { return; }
	if (atom_gettype(argv) != A_FLOAT) { return; }
	float crossfade_ms = (float)atom_getfloat(argv);	
	x->a_crossfade_time = x->crossfade_time = (float)min(MAXIMUM_CROSSFADE, max(MINIMUM_CROSSFADE, crossfade_ms));
	x->crossfade_samples = (long)(x->crossfade_time * x->fs / 1000.0);
	object_attr_touch((t_object *)x, gensym("fadetime"));
}

void fl_fasor_fadetype(t_fl_fasor *x, t_symbol *msg, short argc, t_atom *argv)
{
	if (argc > 1) { return; }
	if (atom_gettype(argv) != A_LONG) { return; }
	float crossfade_type = (short)atom_getfloat(argv);
	x->a_crossfade_type = x->crossfade_type = (short)min(POWER_CROSSFADE, max(NO_CROSSFADE, crossfade_type));
	object_attr_touch((t_object *)x, gensym("fadetype"));
}

/* memory stuff --------------------------------------------------------------------- */
void *new_memory(long nbytes)
{
	t_ptr pointer = sysmem_newptr(nbytes);
	if (pointer == NULL) {
		object_error(NULL, "Cannot allocate memory for this object");
		return NULL;
	}
	return pointer;
}

void free_memory(void *ptr, long nbytes)
{
	sysmem_freeptr(ptr);
}

void fl_fasor_free(t_fl_fasor *x)
{
	/* Remove the object from the DSP chain */
	dsp_free((t_pxobject *)x);

	/* Free allocated dynamic memory */
	free_memory(x->wavetable, x->wavetable_bytes);
	free_memory(x->wavetable_old, x->wavetable_bytes);
}

/* audio stuff -------------------------------------------------------------------------------*/
void fl_fasor_dsp64(t_fl_fasor *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	x->frequency_connected = count[0];	/* store signal connection states of inlets */
	x->fasor_connected = count[1];

	if (x->fs != samplerate) { /* adjust to changes in the sampling rate */
		x->fs = samplerate;

		x->increment = (float)(x->table_size / x->fs);
		x->crossfade_samples = (long)(x->crossfade_time * x->fs / 1000.0);
	}
	x->phase = 0;
	//x->just_turned_on = 1;

	object_method(dsp64, gensym("dsp_add64"), x, fl_fasor_perform64, 0, NULL);	/* attach the object to the DSP chain */
}

void fl_fasor_perform64(t_fl_fasor *x, t_object *dsp64, double **inputs, long numinputs, double **outputs, long numoutputs,
	long vectorsize, long flags, void *userparams)
{
	/* Copy signal pointers */
	t_double *freq_signal = inputs[0];
	t_double *fasor_signal = inputs[1];
	t_double *output_zo = outputs[0];
	/* Copy the signal vector size */
	long n = vectorsize;

	/* Load state variables */
	float frequency = x->frequency;
	long table_size = x->table_size;

	float *wavetable = x->wavetable;
	float *wavetable_old = x->wavetable_old;

	float phase = x->phase;
	float increment = x->increment;

	short crossfade_type = x->crossfade_type;
	long crossfade_samples = x->crossfade_samples;
	long crossfade_countdown = x->crossfade_countdown;

	/* Perform the DSP loop */
	float sample_increment;
	long iphase;

	float interp;
	float samp1;
	float samp2;

	float old_sample;
	float new_sample;
	float out_sample;
	float fraction;

	while (n--)
	{
		//leer vectores audio si están conectados
		if (x->frequency_connected) { sample_increment = increment * (float)*freq_signal++; }
		else { sample_increment = increment * frequency; }

		if (x->fasor_connected) { phase = table_size * (float)*fasor_signal++; }

		//parse phase
		while (phase >= table_size) { phase -= table_size; }
		while (phase < 0) { phase += table_size; }

		//interpolar samps tablas old y new
		iphase = (long)floor(phase);
		interp = phase - iphase;

		samp1 = wavetable_old[(iphase + 0)];
		samp2 = wavetable_old[(iphase + 1) % table_size];
		old_sample = samp1 + interp * (samp2 - samp1);

		samp1 = wavetable[(iphase + 0)];
		samp2 = wavetable[(iphase + 1) % table_size];
		new_sample = samp1 + interp * (samp2 - samp1);

		if (x->dirty) {		//creating new table
			out_sample = old_sample;
		}
		else if (crossfade_countdown > 0) {		//fade between tables
			fraction = (float)crossfade_countdown / (float)crossfade_samples;

			if (crossfade_type == POWER_CROSSFADE) {
				fraction *= (float)PIOVERTWO;
				out_sample = (float)(sin(fraction) * old_sample + cos(fraction) * new_sample);
			}
			else if (crossfade_type == LINEAR_CROSSFADE) {
				out_sample = fraction * old_sample + (1 - fraction) * new_sample;
			}
			else {
				out_sample = old_sample;
			}
			crossfade_countdown--;
		}
		else {		//just play
			out_sample = new_sample;
		}

		*output_zo++ = out_sample;

		phase += sample_increment;
	}

	if (crossfade_countdown <= 0) {
		x->crossfade_in_progress = 0;
	}

	/* Update state variables */
	x->phase = phase;
	x->crossfade_countdown = crossfade_countdown;
}