

#pragma once

#ifdef _WIN32
	#define SOFTWARE_MOUSE_CURSOR
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
	#define SDL_MAIN_HANDLED
	#include "SDL.h"
#else
	#include "SDL2/SDL.h"
#endif

#include "interpreter.h"
#include "posixfilesystem.h"
#include "hal.h"
#include <queue>

typedef std::uint16_t Pixel;

static const SDL_PixelFormatEnum TextureFormat = SDL_PIXELFORMAT_RGB565;

static inline void expand_pixel(Pixel *destPixel, std::uint16_t srcWord, int srcBit) {
	*destPixel = -((srcWord & (1 << srcBit))==0);
}

struct options {
	std::string root_directory;
	std::string snapshot_name;
	bool three_buttons;
	int cycles_per_frame;
	int display_scale;
	bool vsync;
	Uint32 novsync_delay;
};


class VirtualMachine : public IHardwareAbstractionLayer {
public:
	VirtualMachine(struct options &vm_options) :
		vm_options(vm_options),
		fileSystem(vm_options.root_directory),
		interpreter(this, &fileSystem),
		window(0), renderer(0), texture(0),
		#ifdef SOFTWARE_MOUSE_CURSOR
		mouse_texture(0),
		#else
		cursor(0),
		#endif
		display_width(0), display_height(0),
		scheduled_semaphore(0), input_semaphore(0), scheduled_time(0),
		event_count(0), last_event_time(0),
		quit_signalled(false), texture_needs_update(false),
		image_name(vm_options.snapshot_name) {
	}
	
	~VirtualMachine() {
		#ifdef SOFTWARE_MOUSE_CURSOR
		if (mouse_texture)
			SDL_DestroyTexture(mouse_texture);
		#else
		if(cursor)
			SDL_FreeCursor(cursor);
		
		#endif
		if(texture)
			SDL_DestroyTexture(texture);
		if(renderer)
			SDL_DestroyRenderer(renderer);
		if(window)
			SDL_DestroyWindow(window);
	}
	
	void set_input_semaphore(int semaphore);
	
	// the number of seconds since 00:00 in the morning of January 1, 1901
	std::uint32_t get_smalltalk_epoch_time();
	
	// the number of milliseconds since the millisecond clock was
	// last reset or rolled over (a 32-bit unsigned number)
	
	std::uint32_t get_msclock();
	
	void check_scheduled_semaphore();
	
	// Schedule a semaphore to be signaled at a time. Only one outstanding
	// request may be scheduled at anytime. When called any outstanding
	// request will be replaced (or canceled if semaphore is 0).
	// Will signal immediate if scheduled time has passed.
	void signal_at(int semaphore, std::uint32_t msClockTime);
	
	SDL_Cursor *create_cursor(const Uint8 *cursor_bits);
	
	// Set the cursor image  (a 16 word form)
	void set_cursor_image(std::uint16_t *image);
	
	// Set the mouse cursor location
	void set_cursor_location(int x, int y);
	
	void get_cursor_location(int *x, int *y);
	
	void set_link_cursor(bool link);
	
	void initialize_texture();
	
	bool set_display_size(int width, int height);
	
	void update_texture();
	
	void display_changed(int x, int y, int width, int height);
	
	void error(const char *message);
	
	//Input queue
	bool next_input_word(std::uint16_t *word);
	
	// lifetime
	void signal_quit();
	
	const char *get_image_name();
	
	void set_image_name(const char *new_name);
	
	void exit_to_debugger();
	
	bool init();
	
	void queue_input_word(std::uint16_t word);
	
	void queue_input_word(std::uint16_t type, std::uint16_t parameter);
	
	void queue_input_time_words();
	
	void paste_clipboard();
	
	/*
	 decoded keyboard:
	 
	 A decoded keyboard consists of some independent keys and some “meta" keys (shift and escape)
	 that cannot be detected on their own, but that change the value of the other keys. The keys
	 on a decoded keyboard only indicate their down transition, not their up transition.
	 For a decoded keyboard, the full shifted and “controlled" ASCII should be used as a parameter
	 and successive type 3 and 4 words should be produced for each keystroke.
	 
	 undecoded keyboard:
	 
	 (independent keys with up/down detection)
	 On an undecoded keyboard, the standard keys produce parameters that are the ASCII code
	 of the character on the keytop without shift or control information (i.e., the key with “A”
	 on it produces the ASCII for  “a” and the key with “2” and “@“ on it produces the ASCII for “2”).
	 */
	void handle_keyboard_event(const SDL_KeyboardEvent &key);
	
	void handle_mouse_button_event(const SDL_MouseButtonEvent &mouse);
	
	void handle_mouse_movement_event(const SDL_MouseMotionEvent &motion);
	
	#ifdef SOFTWARE_MOUSE_CURSOR
	void update_mouse_cursor(const std::uint16_t* cursor_bits);
	#endif
	
	void render();
	
	void process_events();
	
	void run();
	
	options vm_options;
	
	PosixST80FileSystem fileSystem;
	
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	
	#ifdef SOFTWARE_MOUSE_CURSOR
	SDL_Texture *mouse_texture;
	#else
	SDL_Cursor *cursor;
	#endif
	
	Interpreter interpreter;
	std::queue<std::uint16_t> input_queue;
	std::uint32_t last_event_time;
	int event_count;
	
	int input_semaphore;
	bool quit_signalled;
	bool texture_needs_update;
	int display_width, display_height;
	
	SDL_Rect dirty_rect;
	
	int scheduled_semaphore;
	std::uint32_t scheduled_time;
	std::string image_name;
};
