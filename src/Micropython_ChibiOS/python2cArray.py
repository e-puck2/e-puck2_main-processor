# Reads in a text file, and performs the necessary escapes so that it
# can be #included as a static string like:
#    static const char string_from_textfile[] =
#    #include "build/textfile.h"
#    ;
# This script simply prints the escaped string straight to stdout

# Use with python3 python3cArray.py file_input.py > file_output.c

from __future__ import print_function

import sys

# Can either be set explicitly, or left blank to auto-detect
# Except auto-detect doesn't work because the file has been passed
# through Python text processing, which makes all EOL a \n
line_end = '\\r\\n'

beginning_file = """#include "py_flash.h"

const __attribute__((section(".py_flash"))) char py_flash_code[] = {"""

end_file = """};"""

if __name__ == "__main__":
    filename = sys.argv[1]

    print(beginning_file)
    for line in open(filename, 'r').readlines():
        if not line_end:
            for ending in ('\r\n', '\r', '\n'):
                if line.endswith(ending):
                    line_end = ending.replace('\r', '\\r').replace('\n', '\\n')
                    break
            if not line_end:
                raise Exception("Couldn't auto-detect line-ending of %s" % filename)
        line = line.rstrip('\r\n')
        line = line.replace('\\', '\\\\')
        line = line.replace('"', '\\"')
        print('"%s%s"' % (line, line_end))
        
    print(end_file)
