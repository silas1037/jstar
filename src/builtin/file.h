#ifndef FILE_H
#define FILE_H

#include "native.h"

// interface File {

NATIVE(bl_File_readAll);
NATIVE(bl_File_readLine);
NATIVE(bl_File_close);
NATIVE(bl_File_size);
NATIVE(bl_File_seek);
NATIVE(bl_File_tell);
NATIVE(bl_File_rewind);

// } class File

// prototypes

NATIVE(bl_open);

#endif
