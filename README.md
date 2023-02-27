# Smalltalk-80
![Screenshot](images/desktop.png)


This is a fork of Dan Banay's Smalltalk-80 "by the Bluebook" implementation adapted to cmake,
wih a couple of 'optimizations' to improve performance.

For more detailed information about the original project goto https://github.com/dbanay/Smalltalk

# Using Smalltalk
Smalltalk-80 uses a three button mouse labeled Red (the left mouse button), Yellow (the middle), and Blue (the right button). The Red button is for clicking and selection. The Yellow is for running commands like "doit" and the Blue for things like closing a window. If you have a three button mouse you can use the `-three` command line option to use the traditional mapping. If you do _not_ have a three button mouse, you can use the alternate (and default) mapping:

## Two button mouse mapping

| Button       | Mapping                                                             |                     
|--------------|---------------------------------------------------------------------|
| Left Button  | Maps to Red button (selection)                                      |
| Right Button | Maps to Yellow button (doit)                                        |
| Alt+Left	    | Maps to blue button (close). On the Macintosh, this is Command+Left |

_Note: Ctrl+Left Button can also be used as the "yellow button."_


# Building and running

* SDL 2.0.12 or later

### OS X / Linux   
Go into the project osx folder and type:

* Make

If you installed  SDL2 through a package manager, or built it yourself, so your system has  `sdl-config`  in your path you can simply do:

```bash 
$ make -f MakefileRT && ./Smalltalk -directory ../files
```

* CMake

for cmake in the project root you can do:
```bash 
$ mkdir build && cd buid && cmake .. && cmake -DRelease --build .
$ ./Smalltalk -d ../files  
```

## Windows
use the VisualStudio solution file provided in the windows directory. 


## Configuration
The behavior of the Smalltalk application can be customized through `#define` settings in the 'config.h' file.

### Object Memory 

used to set the garbage collection scheme to be used (all the approaches described in the Bluebook are available).

| Define              | Meaning                                                                                                                                                                                                                              |                     
|---------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `GC_MARK_SWEEP`     | Use Mark and Sweep garbage collection                                                                                                                                                                                                |
| `GC_REF_COUNT`      | Use the reference counting scheme                                                                                                                                                                                                    |
| `RUNTIME_CHECKING`  | Include runtime checks for memory accesses                                                                                                                                                                                           |                   |
| `RECURSIVE_MARKING` | The book describes a recursive marking algorithm that is simple,but consumes stack space. If this symbol is defined, that algorithm is used. If _not_ the more complicated, and clever, _pointer reversal_ approach is used instead. |

The  `GC_MARK_SWEEP` and `GC_REF_COUNT`  flags are **not** mutually exclusive. 

If both are defined (and they are), normal reference counting will be performed, with full garbage collection only when memory is exhausted or the object table becomes full. Smalltalk-80 generates a lot of cyclical references (e.g. a MethodContext references, through a temporary field, a BlockContext that back references the context through the _sender_ field). A Mark and Sweep only approach will result in many frequent collections as the object table will quickly fill with MethodContexts and other frequently allocated objects as the system runs. Some Smalltalk implementations recycle contexts for this reason.

### Interpreter 

 provided to allow the inclusion/exclusion of optional primitives as well as a handful of helpful debugging methods. 
 If an optional primitive is not implemented, a Smalltalk fall-back is executed instead. This can have a significant effect on performance.

| Define                          | Meaning                                                     |                    
|---------------------------------|-------------------------------------------------------------|
| `DEBUG`                         | Adds some additional methods for debugging support          |
| `IMPLEMENT_PRIMITIVE_NEXT`      | Implement the optional `primitiveNext` primitive            |
| `IMPLEMENT_PRIMITIVE_AT_END`    | Implement the optional  `primitiveAtEnd` primitive          |
| `IMPLEMENT_PRIMITIVE_NEXT_PUT`  | Implement the optional  `primitiveNextPut` primitive        |
| `IMPLEMENT_PRIMITIVE_SCANCHARS` | Implement the optional  `primitiveScanCharacters` primitive |

### Application 
When running under Windows I ran into two problems. First, the mouse cursor wouldn't reliably change if the left mouse button was being held down (e.g. when reframing a window). The second issue was that the mouse cursor was very small on high resolution displays, even when system scaling options were set to compensate for it. For these reasons, I added the option to have the app render the mouse cursor rather than the operating system. The `SOFTWARE_MOUSE_CURSOR` can be defined to do this. I set this conditionally if it's a Windows build. It works on the other platforms, but is unnecessary as they behave properly without it.

## Command line arguments

The Smalltalk application takes a number of arguments. The only required argument is `-dir` **path** which specifies the directory where the snapshot image can be found. Here is the complete list:

| Argument    | Meaning                                                                                                                                                                                                                                             | Default             |
|-------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------|
| -dir _path_ | Specifies the directory that contains the snapshot image, as well as the Smalltalk-80.sources and Smalltalk-80.changes files. Any other files in the directory will be accessible from Smalltalk-80 using the normal file/directory access methods. | **_required_**      |
| -three      | Use the three button mapping                                                                                                                                                                                                                        | _two button scheme_ |
| -image      | Name of the snapshot file to use.                                                                                                                                                                                                                   | **snapshot**.**im** |
| -cycles     | Number of VM instructions to run per update loop                                                                                                                                                                                                    | **1800**            |
| -vsync      | Turn on vertical sync for synchronizing the frame rate with the monitor refresh rate. This can eliminate screen tearing and other artifacts, at the cost of some input latency.                                                                     | _off_               |
| -delay _ms_ | if vsync is _not_ used a delay can be specified after presenting the next frame to the GPU. This is useful for lowering the CPU usage while still enjoying the benefits of not using vsync                                                          | **0**               |
| -scale      | Specifies the display scale to be used. Helpful for farsighted folks, or people running on very high resolution displays                                                                                                                            | 1_                  |
| -help       | Displays a help message                                                                                                                                                                                                                             |


## Extras


