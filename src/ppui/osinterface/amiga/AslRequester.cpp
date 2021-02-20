/*
 *  ppui/osinterface/amiga/AslRequester.cpp
 *
 *  Copyright 2017-2021 Juha Niemimaki
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef __amigaos4__

// This is a common file requester for both load / save functions

#include "AslRequester.h"

#include "Dictionary.h"

#include <stdio.h>

#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/dos.h>

struct AslIFace *IAsl;
struct Library *AslBase;

static const int maxPath = 255;
static char defaultPath[maxPath];

static PPDictionary pathDictionary;

static void GetCurrentPath()
{
    if (strlen(defaultPath) == 0) {
        BPTR lock = IDOS->GetCurrentDir();
        int32 success = IDOS->NameFromLock(lock, defaultPath, sizeof(defaultPath));

        if (success) {
            //printf("Initialized to '%s'\n", defaultPath);
        } else {
            puts("Failed to get current dir name, use PROGDIR:");
            strncpy(defaultPath, "PROGDIR:", sizeof(defaultPath));
        }
    } else {
        //printf("Use known path '%s'\n", defaultPath);
    }
}

static PPSystemString GetFileNameFromRequester(struct FileRequester *req, CONST_STRPTR key)
{
    char buffer[maxPath];
    PPSystemString fileName = "";

    if (strlen(req->fr_Drawer) < sizeof(buffer)) {
        strncpy(buffer, req->fr_Drawer, sizeof(buffer));

        //printf("key %s, data %s\n", key, req->fr_Drawer);

        pathDictionary.store(key, req->fr_Drawer);

        int32 success = IDOS->AddPart(buffer, req->fr_File, sizeof(buffer));

        if (success == FALSE) {
            puts("Failed to construct path");
        } else {
            fileName = buffer;
        }

        //printf("%s\n", fileName.getStrBuffer());
    } else {
        printf("Path is too long (limit %ld chars)\n", sizeof(buffer));
    }

    return fileName;
}

struct Window* getNativeWindow(void);

static struct FileRequester *CreateRequester(CONST_STRPTR title, bool saveMode, CONST_STRPTR name)
{
    const char* path = NULL;

    PPDictionaryKey* key = pathDictionary.restore(title);
    if (key) {
         path = key->getStringValue().getStrBuffer();
         //printf("Restored path for key %s: %s\n", title, path);
    }

    if (path == NULL) {
        path = defaultPath;
    }

    struct FileRequester *req = (struct FileRequester *)IAsl->AllocAslRequestTags(
        ASL_FileRequest,
        ASLFR_Window, getNativeWindow(),
        ASLFR_TitleText, title,
        ASLFR_DoSaveMode, saveMode ? TRUE : FALSE,
        ASLFR_SleepWindow, TRUE,
        ASLFR_StayOnTop, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_InitialDrawer, path,
        ASLFR_InitialFile, IDOS->FilePart(name),
        TAG_DONE);

    return req;
}

static bool OpenAslLibrary()
{
    AslBase = IExec->OpenLibrary(AslName, 53);
    if (AslBase) {
         IAsl = (struct AslIFace *)IExec->GetInterface(AslBase, "main", 1, NULL);
         if (IAsl) {
             return true;
         }

         puts("Failed to get ASL interface");
     }

     printf("Failed to open %s\n", AslName);

     return false;
}

static void CloseAslLibrary()
{
    if (IAsl) {
        IExec->DropInterface((struct Interface *)IAsl);
        IAsl = NULL;
    }

    if (AslBase) {
        IExec->CloseLibrary(AslBase);
        AslBase = NULL;
    }
}

PPSystemString GetFileName(CONST_STRPTR title, bool saveMode, CONST_STRPTR name)
{
    PPSystemString fileName = "";

    if (OpenAslLibrary()) {
        GetCurrentPath();

        struct FileRequester *req = CreateRequester(title, saveMode, name);

        if (req) {
            BOOL result = IAsl->AslRequestTags(req, TAG_DONE);

            //printf("%d '%s' '%s'\n", b, r->fr_File, r->fr_Drawer);

            if (result != FALSE) {
                fileName = GetFileNameFromRequester(req, title);
            }

            IAsl->FreeAslRequest(req);
        } else {
            puts("Failed to allocate file requester");
        }
    }

    CloseAslLibrary();

    return fileName;
}

#endif

