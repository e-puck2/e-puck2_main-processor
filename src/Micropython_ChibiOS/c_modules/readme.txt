To add a module to micropython written in C, you can use the folder example as model.
Be sure to respect the fodler structure.

1) Create a folder structure as follow inside c_modules:
		Micropython_ChibiOS/
		└── c_modules/
		    └──example/
		       ├──modexample.c
		       ├──modexample.h
		       └──micropython.mk

2)  Write your module
3)	Don't forget to add a DEFINE to mp_configport_chibios.h to enable your module.
	This define is the same as what you will write in the function
	"MP_REGISTER_MODULE(MP_QSTR_example, example_user_cmodule, MODULE_EXAMPLE_ENABLED);"
	at the end of your c file. Here we would add "#define MODULE_EXAMPLE_ENABLED (1)"

4)	The objects definitions for micropython can be found in "py/obj.h" the micropython 
5)	All the includes related to your project that micropthon cannot know have to be put 
	inside this preprocessor test.

	#ifdef CHIBIOS
	
	...your includes...

	#endif	/* CHIBIOS */

6)	The c module is processed by micropython makefile to generate the QSTR but is compiled
	by the makefile of ChibiOS.