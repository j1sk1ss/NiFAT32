/*
License:
    MIT License. See LICENSE file in project root.
    Copyright (c) 2025 Nikolay

Description:
    FAT 8.3 name and path conversion helpers.

Dependencies:
    - std/str.h - String helpers.
*/

#ifndef FATNAME_H_
#define FATNAME_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <std/str.h>

#define PATH_SPLITTER '/'

/*
Convert fatname 8.3 to default name and ext.
Example: NAME    EXT to name.ext
Params:
- `fatname` - Source fatname full name.
- `name` - Destination buffer where will be placed name.
*/
void nft32_fatname_to_name(const char* fatname, char* name);

/*
Convert default name and ext to fatname 8.3.
Example: name.ext to NAME    EXT
Params:
- `name` - Source full name.
- `fatname` - Destination buffer where will be placed fatname.
*/
void nft32_name_to_fatname(const char* name, char* fatname);

/*
Convert path to fatname 8.3.
Example: root/tdir/dir2/name.ext to ROOT/TDIR/DIR2/NAME    EXT
Params:
- `path` - Source path.
- `fatnames` - Destination buffer where will be placed converted path.
*/
void nft32_path_to_fatnames(const char* path, char* fatnames);

/*
Extract file name from path. 
Example: root/tdir/file.txt, name=file.txt
         Note: Same for 8.3 path
Params:
- `path` - Source path.
- `name` - Filename buffer for saving.
*/
void nft32_extract_name(const char* path, char* name);

/*
Unpack 8.3 name to name and extention.
Example: NAME    EXT, name=NAME, ext=EXT
Params:
- `name83` - 8.3 name.
- `name` - Buffer for name.
- `ext` - Buffer for extention.
*/
void unpack_83_name(const char* name83, char* name, char* ext);

#ifdef __cplusplus
}
#endif
#endif