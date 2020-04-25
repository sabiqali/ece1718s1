#ifndef AUX_FUNCTIONS_H
#define AUX_FUNCTIONS_H

#include <linux/input.h>

#define TOTAL_NO_OF_NOTES     20
#define SAMPLE_RATE                 8000
#define MAX_VOL                     0x7FFFFFFF
#define WSLC_MASK                   0xFF000000
#define N_EXPECTED_PARAMS           2
#define PARAM_KEYBOARD              1
#define ERR_INVALID_N_PARAMS        -1
#define ERR_INVALID_KBD             -2
#define SUCCESS                     0
#define NO_KBD_EVENT                -1
#define KEY_RELEASED                0
#define KEY_PRESSED                 1
#define INIT_FADING_INTENSITY       700000
#define MAX_REC_TIME                6000

#define S_1							0
#define S_2							1
#define S_3							2
#define S_4							3	
#define K_1							4
#define K_2							5
#define K_3							6
#define K_4							7
#define T1_1						8
#define T1_2						9
#define T1_3						10
#define T1_4						11	
#define T2_1						12
#define T2_2						13
#define T2_3						14
#define T2_4						15
#define T3_1						16
#define T3_2						17
#define T3_3						18
#define T3_4						19			
#define NO_KEY                      0xFFFF

#define VIDEO_X_RES                 320
#define VIDEO_Y_RES 				240
#define WHITE 						0xFFFF
#define GREEN 						0x0F00
#define COMMAND_STR_SIZE 			40
#define VIDEO_BYTES 				8                       // number of characters to read from /dev/video
#define KEY_BYTES                   2
#define HEX_BYTES                   6
#define STOPWATCH_BYTES 			9

#define REC_KEY                     1
#define PLAY_KEY                    2
#define REC_NEW_KEY					4
#define MERGE_KEY					8

#define HEX_CLEAR_MSG 				"CLR"
#define HEX_CLEAR_MSG_LEN 			4
#define LEDR_CLEAR_MSG 				"0"
#define LEDR_REC_MSG 				"1"
#define LEDR_PLAY_MSG 				"2"
#define LEDR_MSG_LEN 			    11	
#define STOPWATCH_DISP_MSG 			"disp"
#define STOPWATCH_DISP_MSG_LEN 		5	
#define STOPWATCH_NODISP_MSG 		"nodisp"
#define STOPWATCH_NODISP_MSG_LEN 	7
#define STOPWATCH_INIT_TIME_MSG 	"01:00:00"
#define STOPWATCH_INIT_TIME_MSG_LEN 9
#define STOPWATCH_RUN_MSG 			"run"		
#define STOPWATCH_RUN_MSG_LEN 		4
#define STOPWATCH_STOP_MSG 			"stop"
#define STOPWATCH_STOP_MSG_LEN 		5	

typedef struct recording_node {
	struct input_event ev;
	long ev_time;
	struct recording_node* next;
	} recording_node;


void clear_screen(void);
void plot_rect(int,int,int,int);
void highlight_rect(int,int,int,int);
void initial_plot();
void note_decay(int);
void delete_recording(void);
long time_in_hundreths(void);
int read_from_driver_FD(int driver_FD, char buffer[], int buffer_len);
void record_note(struct input_event kbd_event);
void set_rec_play(int key, int* recording, int* playing, int* overlay);
void control_ledr_hex(int key, int recording, int playing, int overlay);
int read_key(void);	
int find_abs_max(int array[], int size);
int set_processor_affinity(unsigned int core);
// Implements a quasi-exponential assympothic
// fade to prevent changes in the DC level.
void note_release_decay(int note);
void* audio_thread(void*);
void bit_mask_volume_mask(int key, int state);
void process_individual_key(int key_code, int state);
void update_notes_volume(struct input_event);
struct input_event read_keyboard(int fd);
int init_keyboard(char* dev_path);
void print_error(int err);
int parse_cmd_line(int argc, char** argv, int chord_vol_mask[]);
int current_WSLC(void);
void buffer_overflow_preventor(void);
void output_sample(int sample);
// Generaters the nth sample of a sin wave of a specific frequency (freq)
// where the sampling frequency is fs at a give volume
int get_next_sample_for_frequency(float freq, int fs, int sample, unsigned int volume);
int map_virtual(void);
void unmap_virtual(void);
int open_physical (int fd);
void close_physical (int fd);
void* map_physical(int fd, unsigned int base, unsigned int span);
int unmap_physical(void* virtual_base, unsigned int span);
void free_resources();

#endif
