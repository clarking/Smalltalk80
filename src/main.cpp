//
//  main.cpp
//  Smalltalk-80
//
//  Created by Dan Banay on 2/20/20.
//  Copyright © 2020 Dan Banay. All rights reserved.
//
//  MIT License
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//

// If defined, the mouse cursor is rendered by the application rather than the system.
// System rendering is preferred, but on Windows, the cursor didn't always change
// if the left mouse button was being held down. In addition, I found that (again)
// on Windows, on high resolution displays the mouse cursor was tiny
// (it wasn't respecting the display scaling setting).
//

#include "vm.h"

static void help(const std::string exe_name){
	std::cout << "Smalltalk-80\n"
			  << "usage: \n  "
			  << exe_name << "' [-vsync,-delay,-cycles,-scale] -dir <root-directory> -image <image-file>\n"
			  << "  -dir    : Root directory.\n"
			  << "  -vsync  : Enable V-Sync\n"
			  << "  -delay  : Add a delay between frames (in ms)\n"
			  << "  -cycles : Cycles per frame (default:1800)\n"
			  << "  -scale  : Override default 1x scale\n"
			  << "  -three  : Enable three button mouse\n"
			  << "  -help   : Show this message\n";
	
	exit(0);
}

static bool process_args(int argc, const char *argv[], struct options &options) {
	if (argc < 3)
		return false;
	
	int arg = 1;
	while (arg < argc) {
		// directory option
		if (strcmp(argv[arg], "-dir") == 0 && arg + 1 < argc) {
			arg++;
			options.root_directory = argv[arg];
			
			// Remove trailing directory separator (if any) -- Unix and Windows
			if (options.root_directory[options.root_directory.size() - 1] == '/' ||
			    options.root_directory[options.root_directory.size() - 1] == '\\')
				options.root_directory.pop_back();
		}
		else if (strcmp(argv[arg], "-image") == 0 && arg + 1 < argc) {
			arg++;
			options.snapshot_name = argv[arg];
		}
		else if (strcmp(argv[arg], "-delay") == 0 && arg + 1 < argc) {
			arg++;
			int delay = atoi(argv[arg]);
			if (delay < 0)
				return false;
			options.novsync_delay = delay;
		}
		else if (strcmp(argv[arg], "-cycles") == 0 && arg + 1 < argc) {
			arg++;
			int cycles = atoi(argv[arg]);
			if (cycles <= 0)
				return false;
			options.cycles_per_frame = cycles;
		}
		else if (strcmp(argv[arg], "-scale") == 0 && arg + 1 < argc) {
			arg++;
			int scale = atoi(argv[arg]);
			if (scale < 0)
				return false;
			options.display_scale = scale;
		}
		else if (strcmp(argv[arg], "-vsync") == 0)
			options.vsync = true;
		else if (strcmp(argv[arg], "-three") == 0)
			options.three_buttons = true;
		else if (strcmp(argv[arg], "-help") == 0)
			help(argv[0]);
		else
			return false;
		arg++;
	}
	
	// Check for required args
	return options.root_directory.size() > 0;
}

int main(int argc, const char *argv[]) {
	
	options vm_options;
	
	vm_options.snapshot_name = "snapshot.im";
	vm_options.three_buttons = true;
	vm_options.vsync = false;
	vm_options.novsync_delay = 0;  // Try -delay 8 arg if your CPU is unhappy
	vm_options.cycles_per_frame = 1800;
	vm_options.display_scale = 1;
	
	if (!process_args(argc, argv, vm_options))
		help(argv[0]);
	
	VirtualMachine *vm = new VirtualMachine(vm_options);
	if (vm->init())
		vm->run();
	else
		std::cerr << "VM failed to initialize (invalid/missing directory or snapshot?)" << std::endl;
	
	delete vm;
	return 0;
}
