MicroPython for ChibiOS
=======================
This directory contains the port of MicroPython to ChibiOS. It works with
all MCUs supported by ChibiOS. It is meant to be added to an existing ChibiOS
project as a submodule.

Most of the functionalities of the STM32 port are working in this port (REPL, float numbers, etc), except 
the modules to control the peripherals of the MCU. This is simply because it has no
interest for us to control the low level drivers with python.

With ChibiOS, the micropython port is compiled as a library (.a). It is done this way because the compilation
process of micropython is not simple and a lot of things can interfere with the compilation of ChibiOS, like
some variables name in the makefiles. We don't want to modify how micropython is compiled, nor the files used for this. Like this, we can follow the changes of micropython quite easily.

Python3 as well as GCC and arm-none-eabi-gcc are needed to compile the library. It works well on Linux and Mac. Windows needs an emulation of make and some gnutools to work. Better to avoid it :-)

Directories and files explanations
--------------------------------

Here is a non-exhaustive list of the important files and folders inside **Micropython_ChibiOS** :

- c_modules/ --Contains the C modules to include to micropython
- python_flash_code/py_flash.py --This file is the python script included in the flash of the MCU and executed a boot
- python_modules/ --Contains python's script that are included as precompiled modules for micropython
- manifest.py --Lists the python_modules to precompile and include to micropython
- micropython_chibios.mk --Makefile to include in the ChibiOS makefile
- mp_platform .h/.c --Platform dependent files. Most of the changes needed will probalby occur in these files
- mpconfigport_chibios.h --Enable or disable here functionalities of micropython
- python2cArray.py --Python script used to convert a python script to a C array

Modules
-------
Four ways have been implemented to add python code to micropython.
1. Sending a python script to the ram and keep it until the next reboot or reset of micropython
2. Placing a python script into the flash of the MCU which is compiled and executed at boot
3. Adding a precompiled python script to the code that can be executed at boot or imported as a module
4. Adding a module written in C which can be imported as a module 

### Python script sent to the ram

Usual commands of the micropython's REPL are supported. So, a simple CTRL-E while in the REPL will put the REPL in paste mode, which let you copy-paste python script to the ram which will be executed immediately after finishing with CTRL-D.

Keep in mind that sending for example 1k of code will usually use more than 1k of ram so pay attention on the amount of ram allocated for micropython in your code.

### Python script stored in the flash

Normally a board with micropython, when connected to the computer, would show a mass storage and a serial port.
We decided for simplicity to not do it this way. Instead we use a mimic of the paste mode to send code not to the ram, but directly to the flash of the MCU. 

A sector of the flash is allocated for it and an array called **py_flash_code** is put at the beginning of the sector. Then, when a code is sent through the REPL, the sector is erased and the python script is stored in the flash. Then micropython reboots and the code is compiled and executed.

Obviously, it means once the code has been copied correctly, it remains in the MCU until a new code is flashed.

To send a python code to the flash, use **CTRL-D** on the REPL. It will ask you if you want to send
a python script to the flash. Use **CTRL-E for yes** or **CTRL-D for no**. If no is chosen, it will perform the usual reset of micropython. When yes is chosen, the REPL will be put in paste mode but this time the code will be copied to the flash.
Use **CTRL-C to cancel** (this means no more code is present in the flash, regardless of what you sent) or
**CTRL-D to finish**.

To use the python script stored in flash functionality, you need to add
```
MP_PYTHON_FLASH_CODE = yes
```
to your makefile (see build instructions below for more details).

You also need to edit your .ld file to reserve a flash sector for the python code.

1. First you need to add the sector definition inside the ``MEMORY{}`` part. 
For example we want the sector at address 0x080C0000 to be called **py_flash_rw** :

```
 py_flash_rw : org = 0x080C0000, len = 268k
```

Don't forget to also reduce the length definition of the flash used for the usual code (usually called flash0 with ChibiOS) since we don't want the linker to put program code into our py_flash_rw sector.

2. Add the following lines after the ``MEMORY`` part to create an alias to **py_flash_rw** and the pointers used in the code to write the python script to the good address.

```
REGION_ALIAS("PY_FLASH", py_flash_rw);
_py_flash_rw_start = ORIGIN(py_flash_rw);
_py_flash_rw_end   = ORIGIN(py_flash_rw) + LENGTH(py_flash_rw);
```

3. Finally add the following line to the ``SECTIONS`` part (create it if not present in your .ld file)

```
.py_flash :
{
    *(.py_flash*);
} > PY_FLASH
```

This tells the linker to put at PY_FLASH (which is the alias of py_flash_rw) every declaration which explicitly asks to be stored in the .py_flash section.

Since we need to create the array in which is stored the python code, we can also fill it with a python script during the compilation. This is done with ``python3cArray.py`` that converts the python code ``py_flash.py`` to a C array of char. This means you can freely modify this file to have a default python script stored in flash each time the MCU is programmed.

### Python precompiled scripts

You can add a precompiled python script (called frozen module) to the code with the ``manifest.py`` file and the **python_modules** folder. The precompiled script is compiled on the computer during the compilation process and added to the code of micropython. This method uses more flash than a bare python script and cannot be changed at runtime (you need to recompile the whole library) but it has the advantage of using less ram. It's good to use it if you want to have a python module that won't change a lot, or if you can easily recompile the whole firmware.

For now, micropython tries to execute at launch a module called ``boot.py`` so if you want your script to be run
at launch, simply name it this way.
The others scripts can be imported as usual in micropython.

To use the frozen modules, you need to enable the functionality by adding

```
MP_FROZEN_PYTHON = yes
```

to your makefile (see build instructions below for more details).

Now you can put your python scripts in the **python_modules** folder and edit the ``manifest.py`` to add them to the compilation. You can find examples of manifest.py in the STM32 port for the syntax.

### Python C module

Finally, it is possible to write micropython modules in C. They are then embedded in the micropython compiled library like the frozen modules, except they are written in C. This possibility lets the user write time critical functions and let the possibility to make a link between micropython and libraries and/or drivers written in C for the platform used. The modules are to be put in the **c_modules** folder.

Each C module has to be in its own folder to be correctly processed during the compilation. But nothing prevents you from putting multiple C modules inside the same folder, as long as this folder is inside **c_modules**, it's just less clean.

Here is an example of the structure folder we obtain with a C module called **example** :

```
Micropython_ChibiOS/
└── c_modules/
    └──example/
       ├──modexample.c
       ├──modexample.h
       └──micropython.mk
```

To add a C module to micropython:
1. Create the same folder structure as the example above inside **c_modules**
2. Write your C functions. A module **example** gives you an example of the way to write the functions
3. Add a DEFINE to ``mpconfigport_chibios.h`` to enable your module.
This define is the same as what you will write in the function
``MP_REGISTER_MODULE(MP_QSTR_example, example_user_cmodule, MODULE_EXAMPLE_ENABLED);``
at the end of your c file. Here we would add ``#define MODULE_EXAMPLE_ENABLED (1)``
4. The objects definition for micropython can be found in ``micropython/py/obj.h``
5. All the includes related to your project that micropython cannot know have to be put 
inside the following conditional preprocessor directives for each .c file of your C modules:

```
#ifdef CHIBIOS

...your includes...

#endif	/* CHIBIOS */
```

6. Edit the ``micropython.mk`` to add every .c file written. The C module is processed by micropython makefile to generate the QSTR but is compiled by the makefile of ChibiOS. Don't modify the name of this file. If it is named other than ``micropython.mk``, it won't be recognized for the QSTR generation.

Platform dependent files
------------------------

The main things to adapt to your configuration are the ``mp_platform.c/.h`` files and the ``mpconfigport_chibios.h`` file in order to make micropython work with your code.

- ``mp_platform.c`` contains the thread running micropython and the communication functions using the streams of ChibiOS for simplicity and modularity. You will also find some functions to write to the flash.

- ``mp_platform.h`` contains some defines used to configure micropython, like which serial port to use to communicate or the size of its virtual heap for example.

- ``mpconfigport_chibios.h`` is the file where one can enable or disable a functionality of micropython. Here most of the functionalities are enabled.

Build instructions
------------------

To use this port, you can add this git as a git submodule to your project. Then copy the folder 
**Micropython_ChibiOS** inside your project. We propose this because you will probably have to edit
some files inside this folder. This let you commit these changes without the need to fork this fork :-)

The **Micropython_ChibiOS** folder contains two makefiles :
1. ``Makefile`` : This is the makefile used to compile the micropython library. It is really similar to the makefiles findable in the different micropython ports.
2. ``micropython_chibios.mk`` : This makefile is intended to be included by the ChibiOS makefile. It lets ChibiOS know about all the .h files needed to use the micropython library and also lists some .c files to compile on the ChibiOS side. Finally, it triggers the compilation of micropython as a library.

Indeed, some files in the **Micropython_ChibiOS** directory are compiled in the micropython library and some are compiled along with ChibiOS. This is done to avoid cross dependencies. Each time a file uses ChibiOS functions, it needs to be compiled on the ChibiOS side, otherwise it is compiled in the micropython library.

Note : since the definitions of micropython are given to the ChibiOS makefile, micropython functions can be used by the user code without the need to compile them with micropython.

Here are some little things to add to your ChibiOS makefile to add this port :
- Add the specific variables below to your makefile before the CSRC definition
	- ``MPTOP`` 				: Path to the micropython folder (relative to the ChibiOS makefile)
	- ``MPTOP_CHIBIOS``			: Path to the Micropython_ChibiOS folder (relative to the ChibiOS makefile)
	- ``MPTOP_FOR_MP_MAKEFILE`` : Path to the micropython folder (relative to ``micropython_chibios.mk``)
	- ``MP_FROZEN_PYTHON``		: yes or no. Tells if you want to compile frozen python modules
	- ``MP_PYTHON_FLASH_CODE`` 	: yes or no. Tells if you want to include a python script into the flash
	- ``include $(MPTOP_CHIBIOS)/micropython_chibios.mk``
- Add the library path to the **ULIBS** variable near the end of the ChibiOS makefile
	- ``$(MPTOP_CHIBIOS)/libmicropython.a``

Note : **ALLCSRC** and **ALLINC** variables are used by the submakefiles to add sources and headers. Add them to **CSRC** and **INCDIR** respectively in your ChibiOS makefile if not already present.

For example with this folder structure :
```
Your_project/
├───micropython/
├───another_submodule/
└───src/
    ├──Micropython_ChibiOS/
    │  ├──micropython_chibios.mk
    │  └──...
    ├──makefile
    ├──main.c
    └──...
```
You would add these lines to your makefile :

```makefile
...


MPTOP = ../micropython
MPTOP_CHIBIOS = Micropython_ChibiOS
MPTOP_FOR_MP_MAKEFILE = ../../micropython
MP_FROZEN_PYTHON = yes
MP_PYTHON_FLASH_CODE = yes
include $(MPTOP_CHIBIOS)/micropython_chibios.mk

...


# List all user libraries here
ULIBS = $(MPTOP_CHIBIOS)/libmicropython.a

```

