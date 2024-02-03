#include <stdio.h>
#include <stdlib.h>
#include "adflib.h"
#include <string.h>
#include <stdbool.h>

#define INPUT_MAX 500

/*  ADF MANAGER version 1.0 by Jake Aigner Feb, 2024
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

typedef struct configFile{
    uint16_t inputSize;
    uint16_t outputSize;
    char output_path[INPUT_MAX];
    char input_path[INPUT_MAX];
} configFile;

configFile settings;

bool strComp(char *input, char *comp) // compares strings
{
    bool same = true;
    for (int x = 0; x < strlen(comp); x++)
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
    printf(" |__V1.0___|\n");
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

void load_config()
{
    char file[INPUT_MAX];
    strcpy(file, getenv("HOME"));
    strncat(file, "/.adfman_config", 16);
    FILE *fp = fopen(file, "rb");
    if (!fp)
    {
        printf("Failed to read config file or no config file exists!\nrun 'config' to create file\n");
        return;
    }
    fread(&settings.inputSize, 2, 1, fp);
    fread(&settings.outputSize, 2, 1, fp);
    fread(&settings.input_path, 1, settings.inputSize, fp);
    fread(&settings.output_path, 1, settings.outputSize, fp);
    fclose(fp);
    settings.input_path[settings.inputSize] = '\0';
    settings.output_path[settings.outputSize] = '\0';
}

void config()
{
    settings.output_path[0] = '\0';
    settings.input_path[0] = '\0';
    printf("        Main path: ");
    fgets(settings.output_path, INPUT_MAX, stdin);
    printf("        ADF path: ");
    fgets(settings.input_path, INPUT_MAX, stdin);
    settings.outputSize = strlen(settings.output_path) - 1;
    settings.inputSize = strlen(settings.input_path) - 1;
    char file[INPUT_MAX];
    strcpy(file, getenv("HOME"));
    strncat(file, "/.adfman_config", 16);
    system("rm ~/.adfman_config");
    FILE *fp = fopen(file, "wb");
    if (!fp)
    {
        printf("        Error creating config file!\n");
        return;
    }
    fwrite(&settings.inputSize, 2, 1, fp);
    fwrite(&settings.outputSize, 2, 1, fp);
    fwrite(&settings.input_path, 1, settings.inputSize, fp);
    fwrite(&settings.output_path, 1, settings.outputSize, fp);
    fclose(fp);
}

void adf_path(char *path)
{
    settings.input_path[0] = '\0';
    strcpy(settings.input_path, path);
    settings.inputSize = strlen(settings.input_path) + 1;
}

void help() // lists commands
{
    printf("        List of Commands:\n");
    printf("            endshell :    quit.\n");
    printf("            help     :    command list.\n");
    printf("            clear    :    clear the screen.\n");
    printf("            new      :    create empty adf.      args: filename\n");
    printf("            open     :    open adf.              args: filename\n");
    printf("            cd       :    change directory.      args: dirname\n");
    printf("            mkdir    :    make new directory.    args: dirname\n");
    printf("            list     :    list contents of adf.  args: main, adf\n");
    printf("            push     :    add file to adf.       args: filename\n");
    printf("            pull     :    extract file from adf. args: filename\n");
    printf("            extract  :    extract entire adf. (limited to 2 subdirs)\n");
    printf("            delete   :    delete file from adf.  args: filename\n");
    printf("            config   :    edit config file.\n");
    printf("            adfpath  :    change adf file path. (doesn't affect config file) args: /path/to/files/\n");
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

void openADF(char *name)
{
    closeADF();
    char full[INPUT_MAX];
    full[0] = '\0';
    strcpy(full, settings.input_path);
    strncat(full, name, strlen(name));
    adf = adfMountDev(full, false);
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

void newADF(char *name)
{
    closeADF();
    char full[INPUT_MAX];
    strcpy(full, settings.input_path);
    strncat(full, name, strlen(name));
    adf = adfCreateDumpDevice(full, 80, 2, 11);
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

void push(char *filename)
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    char full[INPUT_MAX];
    strcpy(full, settings.output_path);
    strncat(full, filename, strlen(filename));
    FILE *fp = fopen(full, "rb");
    if (!fp)
    {
        printf("        Failed to read file!\n");
        return;
    }
    struct AdfFile *afp = adfFileOpen(vol, filename, 2); // 2 for write mode
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
    char comm[INPUT_MAX];
    strcpy(&comm[0], "mkdir ");
    strncat(&comm[0], &settings.output_path[0], settings.outputSize);
    strncat(&comm[0], "extract", 8);
    system(&comm[0]);
    char path[INPUT_MAX];
    strcpy(&path[0], &settings.output_path[0]);
    strncat(&path[0], "extract/", 9);
    pull_files(&path[0]);
    pull_subdirs(&path[0]);
}

void pull(char *filename)
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    char full[INPUT_MAX];
    strcpy(full, settings.output_path);
    strncat(full, filename, strlen(filename));
    FILE *fp = fopen(full, "wb");
    if (!fp)
    {
        printf("        Failed to open output file!\n");
        return;
    }
    struct AdfFile *afp = adfFileOpen(vol, filename, 1); // 1 for read mode
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

void delete(char *filename)
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    struct AdfList *list, *cell;
    struct AdfEntry *entry;
    cell = list = adfGetDirEnt(vol,vol->curDirPtr);
    SECTNUM sector;
    entry = (struct AdfEntry*)cell->content;
    sector = entry->parent;
    adfFreeDirList(list);
    adfRemoveEntry(vol, sector, filename);
}

void cd(char *dir)
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    RETCODE rc;
    if (strComp(dir, ".."))
    {
        rc = adfParentDir(vol);
        if (rc != RC_OK)
        {
            printf("        Couldn't get parent dir!\n");
        }
    } else {
        rc = adfChangeDir(vol, dir);
        if (rc != RC_OK)
        {
            printf("        Invalid directory!\n");
        }
    }
}

void makedir(char *dir)
{
    if (!adf_open)
    {
        printf("        No ADF open!\n");
        return;
    }
    RETCODE rc = adfCreateDir(vol, vol->curDirPtr, dir);
    if (rc != RC_OK)
    {
        printf("        Failed to create dir!\n");
    }
}

void list_main()
{
    char comm[INPUT_MAX];
    strcpy(&comm[0], "ls ");
    strncat(&comm[0], &settings.output_path[0], settings.outputSize-1);
    printf("        %s\n", settings.output_path);
    system(&comm[0]);
}

void list_adf()
{
    char comm[INPUT_MAX];
    strcpy(&comm[0], "ls ");
    strncat(&comm[0], &settings.input_path[0], settings.inputSize-1);
    printf("        %s\n", settings.input_path);
    system(&comm[0]);
}

void shell() // handles user input
{
    char input[INPUT_MAX];
    printf("? -> ");
    fgets(input, INPUT_MAX, stdin);
    char command[INPUT_MAX];
    char arg[INPUT_MAX];
    command[0] = '\0';
    arg[0] = '\0';
    char *token = strtok(input, " \n\t\r");
    strcpy(command, token);
    token = strtok(NULL, " \n\t\r");
    if (token != NULL)
    {
        strcpy(arg, token);
    }

    if (strComp(command, "endshell\0"))
    {
        closeADF();
        run = false;
    }
    else if (strComp(command, "help\0"))
    {
        help();
    }
    else if (strComp(command, "clear\0"))
    {
        system("clear");
        intro();
    }
    else if (strComp(command, "new\0"))
    {
        newADF(arg);
    }
    else if (strComp(command, "open\0"))
    {
        openADF(arg);
    }
    else if (strComp(command, "list\0"))
    {
        if (strComp(arg, "main"))
        {
            list_main();
        } else if (strComp(arg, "adf"))
        {
            list_adf();
        } else {
            list();
        }
    }
    else if (strComp(command, "push\0"))
    {
        push(arg);
    }
    else if (strComp(command, "pull\0"))
    {
        pull(arg);
    }
    else if (strComp(command, "delete\0"))
    {
        delete(arg);
    } else if (strComp(command, "cd\0"))
    {
        cd(arg);
    } else if (strComp(command, "mkdir\0"))
    {
        makedir(arg);
    } else if (strComp(command, "extract\0"))
    {
        pull_all();
    } else if (strComp(command, "config\0"))
    {
        config();
    } else if (strComp(command, "adfpath\0"))
    {
        adf_path(arg);
    }
}

int main()
{
    adfEnvInitDefault();
    intro();
    load_config();
    printf("Main: %s | ADF: %s\n", settings.output_path, settings.input_path);
    while (run)
    {
        shell();
    }
    adfEnvCleanUp();
    return 0;
}
