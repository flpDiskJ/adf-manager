#include <stdlib.h>
#include "adflib.h"
#include <string.h>
#include <stdbool.h>

#define INPUT_MAX 500

/*  ADF MAN 0.2 by Jake Aigner Jan, 2024
 *
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  ADFLib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

bool run = true;
bool adf_open = false;

struct AdfDevice *adf;
struct AdfVolume *vol;

bool strComp(char *input, char *comp, int offset) // compares strings
{
    bool same = true;
    for (int x = offset; x < strlen(comp); x++)
    {
        if (input[x] != comp[x])
        {
            same = false;
        }
    }
    return same;
}

void intro() // intro display
{
    printf("   #######\n");
    printf(" | ADF MAN |\n");
    printf(" |         |\n");
    printf(" |__V0.2___|\n");
    printf("      |\n");
    printf("    \\ | /\n");
    printf("     \\|/\n");
    printf("      |\n");
    printf("      |\n");
    printf("      ^\n");
    printf("     / \\\n");
    printf("    /   \\\n\n");
    printf("by Jake Aigner (2024)\n");
}

void help() // lists commands
{
    printf("        List of Commands:\n");
    printf("            endshell : quit\n");
    printf("            help : command list\n");
    printf("            clear : clear the screen\n");
    printf("            new : create empty adf\n");
    printf("            open : open adf\n");
    printf("            cd : change directory\n");
    printf("            mkdir : make new directory\n");
    printf("            list : list contents of adf\n");
    printf("            push : add file to adf\n");
    printf("            pull : extract file from adf\n");
    printf("            extract : extract entire adf (limited to 2 subdirs)\n");
    printf("            delete : delete file from adf\n");
}

void closeADF()
{
    if (adf_open)
    {
        adfUnMount(vol);
        adfUnMountDev(adf);
        adf_open = false;
    }
}

void openADF()
{
    closeADF();
    char name[INPUT_MAX];
    printf("Enter ADF Name: ");
    scanf("%s", &name);
    adf = adfMountDev(&name[0], false);
    if (!adf)
    {
        printf("        Failed to open ADF!\n");
    } else {
        vol = adfMount(adf, 0, false);
        if (!vol)
        {
            printf("        Failed to mount volume!\n");
        } else {
            adf_open = true;
        }
    }
}

void newADF()
{
    closeADF();
    char name[INPUT_MAX];
    printf("Enter ADF Name: ");
    scanf("%s", &name);
    adf = adfCreateDumpDevice(&name[0], 80, 2, 11);
    if (!adf)
    {
        printf("        Failed to create dump device!\n");
        return;
    }

    /* create the filesystem : OFS with DIRCACHE */
    RETCODE rc = adfCreateFlop(adf, "empty", FSMASK_DIRCACHE);
    if (rc!=RC_OK)
    {
        printf("        Failed to create disk!\n");
    }
    vol = adfMount(adf, 0, false);
    if (!vol)
    {
        printf("        Failed to mount volume!\n");
    }
    adf_open = true;
}

void list() // lists contents of adf
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    struct AdfList *list, *cell;
    struct AdfEntry *entry;

    /* saves the head of the list */
    cell = list = adfGetDirEnt(vol,vol->curDirPtr);

    /* while cell->next is NULL, the last cell */
    char ent_type;
    while(cell) {
        entry = (struct AdfEntry*)cell->content;
        if (entry->type == 2)
        {
            ent_type = 'D';
        } else {
            ent_type = 'F';
        }
        printf("        %-40s %-8d %c\n", entry->name, entry->sector, ent_type);
        cell = cell->next;
    }

    /* frees the list and the content */
    adfFreeDirList(list);
}

void push()
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    char filename[INPUT_MAX];
    printf("Filename: ");
    scanf("%s", &filename);
    FILE *fp = fopen(&filename[0], "rb");
    if (!fp)
    {
        printf("        Failed to read file!\n");
        return;
    }
    struct AdfFile *afp = adfFileOpen(vol, &filename[0], 2); // 2 for write mode
    if (!afp)
    {
        printf("        Failed to create file!\n");
        fclose(fp);
        return;
    }
    uint8_t byte;
    int byte_return = 0;
    while (1)
    {
        fread(&byte, 1, 1, fp);
        byte_return = adfFileWrite(afp, 1, &byte);
        if (byte_return != 1)
        {
            printf("        Disk Write Error.\n");
            break;
        }
        if (feof(fp))
        {
            break;
        }
    }
    adfFileClose(afp);
    fclose(fp);
}

void pull_files(char *path)
{
    FILE *fp = NULL;
    char file_path[INPUT_MAX];
    struct AdfList *list, *cell;
    struct AdfEntry *entry;
    struct AdfFile *afp;
    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    while(cell) {
        entry = (struct AdfEntry*)cell->content;
        if (entry->type == -3)
        {
            memset(&file_path[0], 0, INPUT_MAX);
            strcpy(&file_path[0], path);
            strncat(&file_path[0], entry->name, strlen(entry->name));
            printf("%s\n", file_path);
            fp = fopen(&file_path[0], "wb");
            if (!fp)
            {
                printf("        Failed to open output file! %s\n", file_path);
            } else {
                afp = adfFileOpen(vol, entry->name, 1); // 1 for read mode
                if (!afp)
                {
                    printf("        Failed to open file! %s\n", entry->name[0]);
                } else {
                    uint8_t byte;
                    int byte_return = 0;
                    while (!adfEndOfFile(afp))
                    {
                        byte_return = adfFileRead(afp, 1, &byte);
                        if (byte_return != 1)
                        {
                            printf("        File Read Error!\n");
                            break;
                        }
                        fwrite(&byte, 1, 1, fp);
                    }
                    adfFileClose(afp);
                }
            }
            fclose(fp);
        }
        cell = cell->next;
    }
    adfFreeDirList(list);
}

void pull_subdirs(char *p)
{
    char path[INPUT_MAX];
    char comm[INPUT_MAX];
    struct AdfList *list, *cell;
    struct AdfEntry *entry;
    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    RETCODE rc;
    while(cell) {
        entry = (struct AdfEntry*)cell->content;
        if (entry->type == 2)
        {
            rc = adfChangeDir(vol, entry->name);
            if (rc != RC_OK)
            {
                printf("        Invalid directory!\n");
            }

            memset(&path[0], 0, INPUT_MAX);
            strcpy(&path[0], p);
            strncat(&path[0], entry->name, strlen(entry->name));
            memset(&comm[0], 0, INPUT_MAX);
            strncat(&comm[0], "mkdir ", 7);
            strncat(&comm[0], path, strlen(path));
            system(&comm[0]);
            strncat(&path[0], "/", 2);
            pull_files(&path[0]);

            rc = adfParentDir(vol);
            if (rc != RC_OK)
            {
                printf("        Couldn't get parent dir!\n");
            }
        }
        cell = cell->next;
    }
    adfFreeDirList(list);
}

void pull_all()
{
    system("rm extract -r");
    system("mkdir extract");
    char path[INPUT_MAX];
    strcpy(&path[0], "extract/");
    pull_files(&path[0]);
    pull_subdirs(&path[0]);
}

void pull()
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    char filename[INPUT_MAX];
    printf("Filename: ");
    scanf("%s", &filename);
    FILE *fp = fopen(&filename[0], "wb");
    if (!fp)
    {
        printf("        Failed to open output file!\n");
        return;
    }
    struct AdfFile *afp = adfFileOpen(vol, &filename[0], 1); // 1 for read mode
    if (!afp)
    {
        printf("        Failed to open file!\n");
        fclose(fp);
        return;
    }
    uint8_t byte;
    int byte_return = 0;
    while (!adfEndOfFile(afp))
    {
        byte_return = adfFileRead(afp, 1, &byte);
        if (byte_return != 1)
        {
            printf("        File Read Error!\n");
            break;
        }
        fwrite(&byte, 1, 1, fp);
    }
    adfFileClose(afp);
    fclose(fp);
}

void delete()
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    char filename[INPUT_MAX];
    printf("Filename: ");
    scanf("%s", &filename);
    struct AdfList *list, *cell;
    struct AdfEntry *entry;
    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    SECTNUM sector;
    entry = (struct AdfEntry*)cell->content;
    sector = entry->parent;
    adfFreeDirList(list);
    adfRemoveEntry(vol, sector, &filename[0]);
}

void cd()
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    char name[INPUT_MAX];
    printf("Dir: ");
    scanf("%s", &name);
    RETCODE rc;
    if (strComp(&name[0], "..", 0))
    {
        rc = adfParentDir(vol);
        if (rc != RC_OK)
        {
            printf("        Couldn't get parent dir!\n");
        }
    } else {
        rc = adfChangeDir(vol, &name[0]);
        if (rc != RC_OK)
        {
            printf("        Invalid directory!\n");
        }
    }
}

void makedir()
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    char name[INPUT_MAX];
    printf("Dir: ");
    scanf("%s", &name);
    RETCODE rc = adfCreateDir(vol, vol->curDirPtr, &name[0]);
    if (rc != RC_OK)
    {
        printf("        Failed to create dir!\n");
    }
}

void shell() // handles user input
{
    char input[INPUT_MAX];
    printf("? -> ");
    scanf("%s", &input);

    if (strComp(&input[0], "endshell", 0))
    {
        closeADF();
        run = false;
    }
    else if (strComp(&input[0], "help", 0))
    {
        help();
    }
    else if (strComp(&input[0], "clear", 0))
    {
        system("clear");
        intro();
    }
    else if (strComp(&input[0], "new", 0))
    {
        newADF();
    }
    else if (strComp(&input[0], "open", 0))
    {
        openADF();
    }
    else if (strComp(&input[0], "list", 0))
    {
        list();
    }
    else if (strComp(&input[0], "push", 0))
    {
        push();
    }
    else if (strComp(&input[0], "pull", 0))
    {
        pull();
    }
    else if (strComp(&input[0], "delete", 0))
    {
        delete();
    } else if (strComp(&input[0], "cd", 0))
    {
        cd();
    } else if (strComp(&input[0], "mkdir", 0))
    {
        makedir();
    } else if (strComp(&input[0], "extract", 0))
    {
        pull_all();
    }
}

int main()
{
    adfEnvInitDefault();
    intro();
    while (run)
    {
        shell();
    }
    adfEnvCleanUp();
    return 0;
}
