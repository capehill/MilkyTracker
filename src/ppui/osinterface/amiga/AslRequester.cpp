/*
 *  ppui/osinterface/amiga/AslRequester.cpp
 *
 *  Copyright 2017 Juha Niemimaki
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

#include <stdio.h>

#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/dos.h>

struct AslIFace *IAsl;

static const int maxPath = 255;

static char pathBuffer[maxPath];

static void GetCurrentPath()
{
    if (strlen(pathBuffer) == 0) {
        BPTR lock = IDOS->GetCurrentDir();
        int32 success = IDOS->NameFromLock(lock, pathBuffer, sizeof(pathBuffer));

        if (success) {
            //printf("Initialized to '%s'\n", pathBuffer);
        } else {
            puts("Failed to get current dir name, use PROGDIR:");
            strncpy(pathBuffer, "PROGDIR:", sizeof(pathBuffer));
        }
    } else {
        //printf("Use known path '%s'\n", pathBuffer);
    }
}

static PPSystemString GetFileNameFromRequester(struct FileRequester *req)
{
    char buffer[maxPath];
    PPSystemString fileName = "";

    if (strlen(req->fr_Drawer) < sizeof(buffer)) {

        strncpy(buffer, req->fr_Drawer, sizeof(buffer));
        strncpy(pathBuffer, req->fr_Drawer, sizeof(pathBuffer));

        int32 success = IDOS->AddPart(buffer, req->fr_File, sizeof(buffer));

        if (success == FALSE) {
            puts("Failed to construct path");
        } else {
            fileName = buffer;
        }

        //printf("%s\n", fileName.getStrBuffer());

    } else {
        printf("Path is too long (limit %ld)\n", sizeof(buffer));
    }

    return fileName;
}

struct Window* getNativeWindow(void);

static struct FileRequester *CreateRequester(CONST_STRPTR title, bool saveMode, CONST_STRPTR name)
{
    struct FileRequester *req = (struct FileRequester *)IAsl->AllocAslRequestTags(
        ASL_FileRequest,
        ASLFR_Window, getNativeWindow(),
        ASLFR_TitleText, title,
        //ASLFR_PositiveText, "Open file",
        ASLFR_DoSaveMode, saveMode ? TRUE : FALSE,
        ASLFR_SleepWindow, TRUE,
        ASLFR_StayOnTop, TRUE,
        ASLFR_RejectIcons, TRUE,
        ASLFR_InitialDrawer, pathBuffer,
        ASLFR_InitialFile, name,
        TAG_DONE);

    return req;
}

PPSystemString GetFileName(CONST_STRPTR title, bool saveMode, CONST_STRPTR name)
{
    PPSystemString fileName = "";

    struct Library *AslBase = IExec->OpenLibrary(AslName, 53);

    if (AslBase) {
        IAsl = (struct AslIFace *)IExec->GetInterface(AslBase, "main", 1, NULL);

        if (IAsl) {
            GetCurrentPath();

            struct FileRequester *req = CreateRequester(title, saveMode, name);

            if (req) {

                BOOL result = IAsl->AslRequestTags(req, TAG_DONE);

                //printf("%d '%s' '%s'\n", b, r->fr_File, r->fr_Drawer);

                if (result != FALSE) {
                    fileName = GetFileNameFromRequester(req);
                }

                IAsl->FreeAslRequest(req);
            } else {
                puts("Failed to allocate file requester");
            }

            IExec->DropInterface((struct Interface *)IAsl);
        } else {
            puts("Failed to get ASL interface");
        }

        IExec->CloseLibrary(AslBase);
    } else {
        printf("Failed to open %s\n", AslName);
    }

    return fileName;
}

#endif

