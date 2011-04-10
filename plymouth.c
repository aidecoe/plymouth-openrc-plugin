/* plymouth.c - Plymouth plugin for the OpenRC init system
 *
 * Copyright (C) 2011  Amadeusz Żołnowski <aidecoe@gentoo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <einfo.h>
#include <rc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#ifdef DEBUG
#    define DBGSTR(x) fprintf(dbg, "  " #x "=\"%s\"", x)
#else
#    define DBGSTR(x)
#endif


int ply_update(const char* hook, const char* name)
{
    char buffer[100];
    sprintf(buffer, "plymouth update --status=%s-%s", hook, name);
    if(system(buffer) != 0) {
        ewarn("Failed on sending status update to Plymouth daemon!");
        return 1;
    }
    return 0;
}


int ply_text(const char* hook, const char* name)
{
    char buffer[100];
    sprintf(buffer, "plymouth message --text=\"%s %s\"", hook, name);
    if(system(buffer) != 0) {
        ewarn("Failed on sending message to Plymouth daemon!");
        return 1;
    }
    return 0;
}


int ply_start()
{
    if(system("plymouth --ping") != 0) {
        if(system("plymouthd") != 0) {
            eerror("Starting Plymouth daemon failed!");
            return 1;
        }
    }

    if(system("plymouth --sysinit") != 0) {
        eerror("plymouth --sysinit failed!");
        return 1;
    }

    if(system("plymouth --show-splash") != 0) {
        eerror("Couldn't show Plymouth splash!");
        return 1;
    }

    return 0;
}


int ply_stop()
{
    if(system("plymouth --quit") != 0) {
        eerror("Error when quiting Plymouth daemon.");
        return 1;
    }
    return 0;
}


int rc_plugin_hook(RC_HOOK hook, const char *name)
{
    int retval = 0;
    char* runlevel = rc_runlevel_get();
    const char* bootlevel = getenv("RC_BOOTLEVEL");
    const char* defaultlevel = getenv("RC_DEFAULTLEVEL");

#ifdef DEBUG
    FILE* dbg;

    if(strcmp(runlevel, RC_LEVEL_SHUTDOWN) == 0)
        dbg = fopen("/plymouth.log", "a");
    else
        dbg = fopen("/dev/.initramfs/plymouth.log", "a");

    fprintf(dbg, "hook=%d", hook);
#endif

    /* Don't do anything if we're not booting or shutting down. */
    if(!(rc_runlevel_starting() || rc_runlevel_stopping())) {
        if(!(hook == RC_HOOK_RUNLEVEL_STOP_IN ||
                    hook == RC_HOOK_RUNLEVEL_STOP_OUT ||
                    hook == RC_HOOK_RUNLEVEL_START_IN ||
                    hook == RC_HOOK_RUNLEVEL_START_OUT))
            goto exit;
    }

    /* It's assumed in ply_* functions that 'name' is not longer than 50 chars.
     */
    if(strlen(name) > 50)
        goto exit;

    DBGSTR(name);
    DBGSTR(runlevel);

    switch(hook) {
    case RC_HOOK_RUNLEVEL_STOP_IN:
        retval |= ply_update("RUNLEVEL_STOP_IN", name);

        /* Start the Plymouth daemon and show splash when system is being shut
         * down. */
        if(strcmp(name, RC_LEVEL_SHUTDOWN) == 0) {
            ply_start();
        }
        break;

    case RC_HOOK_RUNLEVEL_STOP_OUT:
        retval |= ply_update("RUNLEVEL_STOP_OUT", name);
        break;

    case RC_HOOK_RUNLEVEL_START_IN:
        /* Start the Plymouth daemon and show splash when entering the boot
         * runlevel. Required /proc and /sys should already be mounted in
         * sysinit runlevel. */
        if(strcmp(name, bootlevel) == 0) {
            if((retval |= ply_start()) != 0)
                goto exit;
        }

        retval |= ply_update("RUNLEVEL_START_IN", name);
        break;

    case RC_HOOK_RUNLEVEL_START_OUT:
        retval |= ply_update("RUNLEVEL_START_OUT", name);

        /* Stop the Plymouth daemon right after default runlevel is started. */
        if(strcmp(name, defaultlevel) == 0) {
            retval |= ply_stop();
        }
        break;

    case RC_HOOK_SERVICE_START_NOW:
        retval |= ply_text("Starting service", name);
        break;

    case RC_HOOK_SERVICE_START_OUT:
        break;

    case RC_HOOK_SERVICE_START_DONE:
        retval |= ply_update("SERVICE_START_DONE", name);
        break;

    case RC_HOOK_SERVICE_STOP_NOW:
        retval |= ply_text("Stopping service", name);
        break;

    case RC_HOOK_SERVICE_STOP_DONE:
        retval |= ply_update("SERVICE_STOP_DONE", name);
        break;

    case RC_HOOK_ABORT:
        break;

    default:
        break;
    }

exit:    
#ifdef DEBUG
    fwrite("\n", 1, 1, dbg);
    fclose(dbg);
#endif
    free(runlevel);
    return retval;
}
