#ifndef PANITENT_FILE_OPEN_H
#define PANITENT_FILE_OPEN_H

#include "precomp.h"

#include <initguid.h>
#include <shlwapi.h>
#include "crefptr.h"

#define INDEX_PNG 1

void FileOpenPng(LPWSTR pszPath);
crefptr_t* init_open_file_dialog();
crefptr_t* init_save_file_dialog();

#endif /* PANITENT_FILE_OPEN_H */
