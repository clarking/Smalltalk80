

#include "vm.h"


void VirtualMachine::set_input_semaphore(int semaphore) {
	input_semaphore = semaphore;
}

std::uint32_t VirtualMachine::get_smalltalk_epoch_time() {
	// the number of seconds since 00:00 in the morning of January 1, 1901
	
	// Seconds between 1/1/1901 00:00 and 1/1/1970 00:00
	const std::uint32_t TIME_OFFSET = 2177452800;
	time_t unix_epoch_time = time(0);
	return (std::uint32_t) unix_epoch_time + TIME_OFFSET;
}

std::uint32_t VirtualMachine::get_msclock() {
	// the number of milliseconds since the millisecond clock was
	// last reset or rolled over (a 32-bit unsigned number)
	return SDL_GetTicks();
}

void VirtualMachine::check_scheduled_semaphore() {
	if (scheduled_semaphore && SDL_TICKS_PASSED(SDL_GetTicks(), scheduled_time)) {
		interpreter.asynchronousSignal(scheduled_semaphore);
		scheduled_semaphore = 0;
	}
}

void VirtualMachine::signal_at(int semaphore, std::uint32_t msClockTime) {
	
	// Schedule a semaphore to be signaled at a time. Only one outstanding
	// request may be scheduled at anytime. When called any outstanding
	// request will be replaced (or canceled if semaphore is 0).
	// Will signal immediate if scheduled time has passed.
	
	scheduled_semaphore = semaphore;
	scheduled_time = msClockTime;
	if (semaphore)
		check_scheduled_semaphore();        // Just in case the time passed
}

SDL_Cursor *VirtualMachine::create_cursor(const Uint8 *cursor_bits) {
	
	// Maps a nibble to a byte where each bit is repeated
	// e.g. 1010 -> 11001100
	
	static int expandedNibbleToByte[] = {
		0b00000000,  // 0000
		0b00000011,  // 0001
		0b00001100,  // 0010
		0b00001111,  // 0011
		0b00110000,  // 0100
		0b00110011,  // 0101
		0b00111100,  // 0110
		0b00111111,  // 0111
		0b11000000,  // 1000
		0b11000011,  // 1001
		0b11001100,  // 1010
		0b11001111,  // 1011
		0b11110000,  // 1100
		0b11110011,  // 1101
		0b11111100,  // 1110
		0b11111111   // 1111
		
	};
	
	SDL_Cursor *new_cursor = 0;
	
	if (vm_options.display_scale == 1)
		new_cursor = SDL_CreateCursor(
			cursor_bits,
			cursor_bits,
			16,
			16,
			0,
			0);
	
	else if (vm_options.display_scale == 2) {
		std::uint8_t image[128];
		std::uint8_t *src = (std::uint8_t *) cursor_bits;
		int dest = 0;
		
		// 4 bytes (32-bits) x 32 rows
		for (int h = 0; h < 16; h++) {
			image[dest] = expandedNibbleToByte[*(src) >> 4];
			image[dest + 1] = expandedNibbleToByte[*(src) & 0xf];
			image[dest + 2] = expandedNibbleToByte[*(src + 1) >> 4];
			image[dest + 3] = expandedNibbleToByte[*(src + 1) & 0xf];
			
			image[dest + 4] = image[dest];
			image[dest + 5] = image[dest + 1];
			image[dest + 6] = image[dest + 2];
			image[dest + 7] = image[dest + 3];
			dest += 8;
			src += 2;
		}
		
		new_cursor = SDL_CreateCursor(
			(const Uint8 *) image,
			(const Uint8 *) image,
			32,
			32,
			0,
			0);
	}
	
	if (!new_cursor)
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
		             "Create Cursor failed: %s", SDL_GetError());
	
	return new_cursor;
}

#ifdef SOFTWARE_MOUSE_CURSOR
void VirtualMachine::update_mouse_cursor(const std::uint16_t* cursor_bits)
	{
		int dest_pitch;
		std::uint8_t* pixels;
		
		int code = SDL_LockTexture(mouse_texture, 0, (void **)&pixels,  &dest_pitch);
		if (code < 0)
		{
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't LOCK MOUSE TEXTURE SDL: %s", SDL_GetError());
			return;
		}
		
		std::uint8_t* dest_row = pixels;
		std::uint16_t* source_pixel = (std::uint16_t *) cursor_bits;
		for(int h = 0; h < 16; h++)
		{
			std::uint16_t* dest_pixel = (Pixel *) dest_row;
			for(int b = 15; b >= 0; b--)
			{
				// Low order bit of a 5551 pixel is alpha
				*dest_pixel = (*source_pixel & (1 << b)) != 0;
				dest_pixel++;
			}
			source_pixel++;
			dest_row += dest_pitch;

		}
		SDL_UnlockTexture(mouse_texture);
	}
#endif

void VirtualMachine::set_cursor_image(std::uint16_t *image) {
	// Set the cursor image  (a 16 word form)
#ifdef SOFTWARE_MOUSE_CURSOR
	if (!mouse_texture)
		{
			mouse_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_STREAMING, 16, 16);
			SDL_SetTextureBlendMode(mouse_texture, SDL_BLENDMODE_BLEND);
		}
		update_mouse_cursor(image);
#else
	
	std::uint16_t cursor_bits[16];
	
	// SDL uses a MSB format, so swap the bytes
	for (int i = 0; i < 16; i++)
		cursor_bits[i] = ((image[i] & 0xff) << 8) | (image[i] >> 8);
	
	SDL_Cursor *old_cursor = cursor;
	cursor = create_cursor((const Uint8 *) cursor_bits);
	SDL_SetCursor(cursor);
	
	if (old_cursor)
		SDL_FreeCursor(old_cursor);

#endif
}

void VirtualMachine::set_cursor_location(int x, int y) {
	// Set the mouse cursor location
	SDL_WarpMouseInWindow(window, x * vm_options.display_scale, y * vm_options.display_scale);
}

void VirtualMachine::get_cursor_location(int *x, int *y) {
	SDL_GetMouseState(x, y);
	*x = *x / vm_options.display_scale;
	*y = *y / vm_options.display_scale;
}

void VirtualMachine::set_link_cursor(bool link) {
}

void VirtualMachine::initialize_texture() {
	std::uint8_t *dest_row;
	int dest_pitch;
	
	int code = SDL_LockTexture(texture, 0, (void **) &dest_row, &dest_pitch);
	if (code < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't LOCK SDL: %s", SDL_GetError());
		return;
	}
	
	// There may be many frames before Smalltalk renders, so initialize the screen texture
	// with something that looks like the Smalltalk desktop pattern
	for (int h = 0; h < display_height; h++) {
		Pixel *dest_pixel = (Pixel *) dest_row;
		for (int i = 0; i < display_width; i++)
			*dest_pixel++ = (h & 1) ^ (i & 1) ? 0 : ~0;
		dest_row += dest_pitch;
	}
	
	SDL_UnlockTexture(texture);
}

bool VirtualMachine::set_display_size(int width, int height) {
	
	if (display_width == width || display_height == height)
		return false;
	
	display_width = width;
	display_height = height;
	dirty_rect.x = 0;
	dirty_rect.y = 0;
	dirty_rect.w = width;
	dirty_rect.h = height;
	
	if (window) {
		SDL_SetWindowSize(
			window,
			vm_options.display_scale * display_width,
			vm_options.display_scale * display_height
		);
		
		SDL_DestroyTexture(texture);
	}
	else {
		window = SDL_CreateWindow(
			"Smalltalk-80",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			vm_options.display_scale * display_width,
			vm_options.display_scale * display_height,
			0
		);
		
		Uint32 flags = SDL_RENDERER_ACCELERATED;
		if (vm_options.vsync)
			flags = flags | SDL_RENDERER_PRESENTVSYNC;
		
		renderer = SDL_CreateRenderer(window, -1, flags);
	}
	
	texture = SDL_CreateTexture(
		renderer,
		TextureFormat,
		SDL_TEXTUREACCESS_STREAMING,
		display_width,
		display_height
	);
	
	initialize_texture();
	return true;
}

void VirtualMachine::update_texture() {
	int word_left = dirty_rect.x / 16;
	int word_right = (dirty_rect.x + dirty_rect.w - 1) / 16;
	int width_words = (display_width + 15) / 16;
	
	int src_index_row = word_left + (dirty_rect.y * width_words);
	int update_word_width = word_right - word_left + 1;
	
	// We transfer pixels in groups of WORDS from the display form,
	// so we need to set texture update rectangle so
	// the left and right edges are on a word boundary
	
	SDL_Rect update_rect;
	update_rect.x = word_left * 16;
	update_rect.y = dirty_rect.y;
	update_rect.w = update_word_width * 16;
	update_rect.h = dirty_rect.h;
	
	int bitmap = interpreter.getDisplayBits(display_width, display_height);
	if (bitmap == 0)
		return; // bail
	
	std::uint8_t *pixels;
	int dest_pitch;
	int code = SDL_LockTexture(texture, &update_rect, (void **) &pixels, &dest_pitch);
	if (code < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't LOCK SDL: %s", SDL_GetError());
		return;
	}
	
	std::uint8_t *dest_row = pixels;
	for (int h = 0; h < update_rect.h; h++) {
		Pixel *dest_pixel = (Pixel *) dest_row;
		int source_index = src_index_row;
		for (int i = 0; i < update_word_width; i++) {
			std::uint16_t source_pixel = interpreter.fetchWord_ofDislayBits(source_index, bitmap);
			expand_pixel(dest_pixel++, source_pixel, 15);
			expand_pixel(dest_pixel++, source_pixel, 14);
			expand_pixel(dest_pixel++, source_pixel, 13);
			expand_pixel(dest_pixel++, source_pixel, 12);
			expand_pixel(dest_pixel++, source_pixel, 11);
			expand_pixel(dest_pixel++, source_pixel, 10);
			expand_pixel(dest_pixel++, source_pixel, 9);
			expand_pixel(dest_pixel++, source_pixel, 8);
			expand_pixel(dest_pixel++, source_pixel, 7);
			expand_pixel(dest_pixel++, source_pixel, 6);
			expand_pixel(dest_pixel++, source_pixel, 5);
			expand_pixel(dest_pixel++, source_pixel, 4);
			expand_pixel(dest_pixel++, source_pixel, 3);
			expand_pixel(dest_pixel++, source_pixel, 2);
			expand_pixel(dest_pixel++, source_pixel, 1);
			expand_pixel(dest_pixel++, source_pixel, 0);
			source_index++;
		}
		
		dest_row += dest_pitch;
		src_index_row += width_words;
	}
	
	SDL_UnlockTexture(texture);
}

void VirtualMachine::display_changed(int x, int y, int width, int height) {
	texture_needs_update = true;

#ifdef DEBUG
	assert(x >= 0 && x < display_width);
		assert(y >= 0 && y < display_height);
		assert(x + width <= display_width);
		assert(y + height <= display_height);
#endif
	
	if (SDL_RectEmpty(&dirty_rect)) {
		dirty_rect.x = x;
		dirty_rect.y = y;
		dirty_rect.w = width;
		dirty_rect.h = height;
	}
	else {
		SDL_Rect update_rect{x, y, width, height};
		SDL_UnionRect(&dirty_rect, &update_rect, &dirty_rect);
	}
}

void VirtualMachine::error(const char *message) {
	std::cerr << message << std::endl;
	abort();
}

bool VirtualMachine::next_input_word(std::uint16_t *word) {
	//Input queue
	if (input_queue.size() == 0)
		return false;
	
	*word = input_queue.front();
	input_queue.pop();
	return true;
}

void VirtualMachine::signal_quit() {
	// lifetime
	quit_signalled = true;
}

const char *VirtualMachine::get_image_name() {
	return image_name.c_str();
}

void VirtualMachine::set_image_name(const char *new_name) {
	image_name = new_name;
}

void VirtualMachine::exit_to_debugger() {
	abort();
}

bool VirtualMachine::init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		return false;
	}

#ifdef SOFTWARE_MOUSE_CURSOR
	SDL_ShowCursor(0); // We show our own...
#endif
	
	texture_needs_update = false;
	quit_signalled = false;
	return interpreter.init();
}

void VirtualMachine::queue_input_word(std::uint16_t word) {
#ifdef DEBUG
	assert(input_semaphore);
#endif
	
	input_queue.push(word);
	interpreter.asynchronousSignal(input_semaphore);
}

void VirtualMachine::queue_input_word(std::uint16_t type, std::uint16_t parameter) {
	queue_input_word(((type & 0xf) << 12) | (parameter & 0xfff));
}

void VirtualMachine::queue_input_time_words() {
	std::uint32_t delta_time;
	std::uint32_t now = get_msclock();
	
	if (event_count++ == 0)
		delta_time = 0;
	else
		delta_time = now - last_event_time;
	
	if (delta_time <= 4095) // can fit in 12 bits
		queue_input_word(0, delta_time);
	else {
		std::uint32_t abs_time = get_smalltalk_epoch_time();
		// too large, use type 5 with absolute time
		queue_input_word(5, 0); // parameter is ignored
		queue_input_word((abs_time >> 16) & 0xffff); // high word first
		queue_input_word(abs_time & 0xffff);  // low word next
	}
	
	last_event_time = now;
}

void VirtualMachine::paste_clipboard() {
	if (SDL_HasClipboardText()) {
		queue_input_time_words();
		for (const char *text = SDL_GetClipboardText(); *text; text++) {
			int ch = *text;
			if (ch == '\n')
				ch = '\r';
			queue_input_word(3, ch);
			queue_input_word(4, ch);
		}
	}
}

void VirtualMachine::handle_keyboard_event(const SDL_KeyboardEvent &key) {
	
	
	std::uint16_t type = key.type == SDL_KEYDOWN ? 3 : 4;
	std::uint16_t param = 0;
	/*
	 left shift 136 right shift 137 control 138 alpha-lock 139
	 backspace 8 tab 9 line feed 10 return 13 escape 27 space 32 delete 127
	 */
	
	//printf("%d:%d\n",key.keysym.scancode,key.keysym.mod );
	
	if ( key.keysym.mod == KMOD_LCTRL && key.keysym.scancode == SDL_SCANCODE_V ) {
		printf("win+v\n");
		if (type == 3)
			paste_clipboard();
		return;
	}
	
	// My initial plan was to use go unencoded for everything, but
	// when I pressed shift 6, I saw a ~ appear!. It turns out the
	// ALTO keyboard has ~ above 6!
	// https://www.flickr.com/photos/walkingsf/31415192416
	
	switch (key.keysym.scancode) {
		case SDL_SCANCODE_LCTRL:
		case SDL_SCANCODE_RCTRL:
			param = 138;
			break;
		case SDL_SCANCODE_CAPSLOCK:
			param = 139;
			break;
		case SDL_SCANCODE_DELETE:
			param = 127;
			break;
		default:
			if (key.keysym.sym > 127)
				return;  // Must be ascii
			param = key.keysym.sym & 0x7f;
			break;
	}
	
	if (param < 128) {
		if (type == 3) {
			if (key.keysym.mod & (KMOD_LGUI | KMOD_RGUI))
				return; // Ignore
			
			if (key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
				param = shift_map[param];
			
			/*
			 For a decoded keyboard, the full shifted and “controlled" ASCII should be
			 used as a parameter and successive type 3 and 4 words should be produced for each keystroke.
			 */
			queue_input_time_words();
			queue_input_word(3, param);
			queue_input_word(4, param);
		}
	}
	else {
		// send undecoded
		queue_input_time_words();
		queue_input_word(type, param);
	}
	
}

void VirtualMachine::handle_mouse_button_event(const SDL_MouseButtonEvent &mouse) {
	
	// The bluebook got these wrong!
	const int RedButton = 130;      // select
	const int YellowButton = 129;   // doit etc.
	const int BlueButton = 128;     // frame, close
	
	unsigned mods;
	int smalltalk_button = 0;
	int button_index;
	
	static unsigned button_down_mods[3] = {0}; // modifier state at button down
	
	switch (mouse.button) {
		case SDL_BUTTON_LEFT:
			smalltalk_button = RedButton;
			button_index = 0;
			break;
		case SDL_BUTTON_MIDDLE:
			smalltalk_button = BlueButton;
			button_index = 1;
			break;
		case SDL_BUTTON_RIGHT:
			smalltalk_button = YellowButton;
			button_index = 2;
			break;
		default:
			return;
	}
	
	if (mouse.type == SDL_MOUSEBUTTONDOWN) {
		// Save mod state when the button went down
		// when the button is released we will use these rather than active one
		mods = SDL_GetModState();
		button_down_mods[button_index] = mods;
	}
	else
		mods = button_down_mods[button_index];
	
	if (vm_options.three_buttons) {
		// Real 3 button mouse
		switch (mouse.button) {
			case SDL_BUTTON_LEFT:
				smalltalk_button = RedButton;
				break;
			case SDL_BUTTON_MIDDLE:
				smalltalk_button = YellowButton;
				break;
			case SDL_BUTTON_RIGHT:
				smalltalk_button = BlueButton;
				break;
			default:
				return;
		}
	}
	
	else {
#ifdef __APPLE__
		const Uint32 BlueFlags = KMOD_RGUI|KMOD_LGUI;
#else
		const Uint32 BlueFlags = KMOD_RALT | KMOD_LALT;
#endif
		
		// Left                   = Red
		// Right/Ctrl+Left        = Yellow
		// Alt+Left (win+linux)   = Blue
		// Command+Left(mac)      = Blue
		
		switch (mouse.button) {
			case SDL_BUTTON_LEFT:
				if (mods & BlueFlags)
					smalltalk_button = BlueButton;
				else if (mods & (KMOD_RCTRL | KMOD_LCTRL))
					smalltalk_button = YellowButton;
				else
					smalltalk_button = RedButton;
				break;
			case SDL_BUTTON_RIGHT:
				smalltalk_button = YellowButton;
				break;
			default:
				return;  // Don't care about this button
		}
	}
	
	if (mouse.type == SDL_MOUSEBUTTONDOWN) {
		queue_input_time_words();
		queue_input_word(3, smalltalk_button);
	}
	else if (mouse.type == SDL_MOUSEBUTTONUP) {
		button_down_mods[button_index] = 0;
		queue_input_time_words();
		queue_input_word(4, smalltalk_button);
	}
}

void VirtualMachine::handle_mouse_movement_event(const SDL_MouseMotionEvent &motion) {
	queue_input_time_words();
	queue_input_word(1, (std::uint16_t) motion.x);
	queue_input_time_words();
	queue_input_word(1, (std::uint16_t) motion.y);
}

void VirtualMachine::render() {
	
	if (renderer) {
		if (texture_needs_update) {
			update_texture();
			texture_needs_update = false;
		}
		
		if (texture)
			SDL_RenderCopy(renderer, texture, NULL, NULL);

#ifdef SOFTWARE_MOUSE_CURSOR
		if (mouse_texture)
			{
				static SDL_Rect mouse_src_rect{0,0,16,16};
				int mouseX, mouseY;
				SDL_GetMouseState(&mouseX, &mouseY);

				SDL_Rect dst = {mouseX, mouseY, 16*vm_options.display_scale, 16*vm_options.display_scale};
				SDL_RenderCopy(renderer, mouse_texture, &mouse_src_rect, &dst);
			}
#endif
		
		SDL_RenderPresent(renderer);
		dirty_rect.x = 0;
		dirty_rect.y = 0;
		dirty_rect.w = 0;
		dirty_rect.h = 0;
	}
}

void VirtualMachine::process_events() {
	SDL_Event event;
	
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			quit_signalled = true;
			break;
		}
		if (input_semaphore) {
			if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN)
				handle_keyboard_event(event.key);
			else if (event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEBUTTONDOWN)
				handle_mouse_button_event(event.button);
			else if (event.type == SDL_MOUSEMOTION)
				handle_mouse_movement_event(event.motion);
		}
	}
}

void VirtualMachine::run() {
	
	while (!quit_signalled) {
		
		process_events();
		check_scheduled_semaphore();
		interpreter.checkLowMemoryConditions();
		
		for (int i = 0; i < vm_options.cycles_per_frame && !quit_signalled; i++)
			interpreter.cycle();
		
		render();
		
		if (!vm_options.vsync && vm_options.novsync_delay > 0)
			SDL_Delay(vm_options.novsync_delay); // Don't kill CPU
	}
}