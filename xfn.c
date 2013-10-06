/*
 * XFN - Xchat Friend Notify (plugin)
 * Copyright (C) 2013 Darcy Brás da Silva
 *
 * This file is part of XFN.
 *
 * XFN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * XFN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xchat-plugin.h"
#include "glinked_list.h"
#include <libnotify/notify.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>

/* You should find enough info about the following defines in
 * http://xchat.org/docs/plugin20.html 
 */
#define PNAME "XFN - XCHAT FRIEND NOTIFY"
#define PDESC \
"Generates GTK+ Notifications for a selected friend list"
#define PVERSION "1.0"

/* Don't deal with magic numbers. You are welcome */
#define XFN_OFF    0
#define XFN_ON     1
#define XFN_STATUS 2
#define XFN_NO_OP  3

#define SUCCESS    1

/*unix permission trick to get options in single int*/
#define XFN_MODE_ACTIVE 1
#define XFN_MODE_HIDDEN 2
#define XFN_MODE_NORMAL 4
#define XFN_MODE_ALWAYS (XFN_MODE_ACTIVE|XFN_MODE_NORMAL|XFN_MODE_HIDDEN)
#define XFN_MODE_UNKNOWN -1

/* Globals */
static xchat_plugin *ph; /* plugin handle */
/* This flag acts as standbye switch for the entire plugin */
static int xfn_status = XFN_ON;
/* This flag controls if the notification should display
 * the contents of the message sent to the user or not
 * xfn_dpm_status stands for xfn_display_message_status
 */
static int xfn_dpm_status = XFN_OFF;
/* This flag controls if the notifications should be displayed
 * only when the xchat window is active, 
 * normal, hidden or at all times. XFN_MODE_ALWAYS is default
 */
static int xfn_mode_status = XFN_MODE_ALWAYS;
/* This is the nicknames list for the current session */
static libglinked_list_t xfn_list;
/* custom functions */

/* This is an helper function to be used 
 * with dbsglinked functions
 */
static bool nickname_cmp(void *a, void *b)
{
	if(g_strcmp0(a,b) == 0)
		return TRUE;
	
	return FALSE;
}

/* This is an helper function that creates a copy
 * of str. the new allocated string needs to be freed
 */
static char *sstrdup(const char *str)
{
    size_t l = strlen(str);
    char *new;
    if( (new = malloc(l+1)) == NULL )
    {
        return NULL;
    }

    memcpy(new,str,l);
    new[l] = '\0';
	return new;
}

/* This function is responsible for displaying the names
 * stored in xfn_list
 */
static void print_name(void *data)
{
	xchat_printf(ph, " %s\n", (char*)data);
}

static int xfn_add_handler_cb(char *word[],
                              char *word_eol[],
                              void *userdata)
{
	if(word_eol[2][0] == 0)
	{
		xchat_print(ph,"* requires a second argument");
		xchat_command(ph,"HELP XFN_ADD");
		return XCHAT_EAT_ALL;
	}

	if( libglinked_find_node(&xfn_list, word[2], nickname_cmp) != NULL )
	{
		xchat_printf(ph,"* %s is already added", word[2]);
		return XCHAT_EAT_ALL;
	}
	
	char *dupped = sstrdup(word[2]);
	if(dupped != NULL)
	{
		libglinked_push_node(&xfn_list,
                             libglinked_create_node(&xfn_list,
                                                    dupped,
                                                    free));
		xchat_printf(ph,"* %s was added successfully", dupped);
	}
	else
	{
		xchat_printf(ph,"* wasn't able to insert %s", dupped);
	}

	return XCHAT_EAT_ALL;
}/* end xfn_add_handler_cb */

static int xfn_rm_handler_cb(char *word[],
                             char *word_eol[],
                             void *userdata)
{
	libglinked_node_t *node;

	if(word_eol[2][0] == 0)
	{
		xchat_print(ph,"* requires a second argument");
		xchat_command(ph,"HELP XFN_RM");
		return XCHAT_EAT_ALL;
	}

	
	node = libglinked_remove_node(&xfn_list,word[2],nickname_cmp);
	
	if(node != NULL)
	{
		libglinked_delete_node(&xfn_list, node);
	
		xchat_printf(ph,"* %s was removed successfully", word[2]);
	}
	else
	{
		xchat_printf(ph,
        "* could not remove %s. are you sure %s in in the list ?",
		word[2], word[2]);
	}

	return XCHAT_EAT_ALL;
}/* end xfn_rm_handler_cb */

static int xfn_list_handler_cb(char *word[],
                               char *word_eol[],
                               void *userdata)
{
	size_t num = libglinked_get_num_items(&xfn_list);
	if( num == 0 )
	{
		xchat_print(ph,"* xfn list is empty");
		return XCHAT_EAT_NONE;
	}
	
	xchat_print(ph,"* begining xfn list");
	libglinked_show_list(&xfn_list, print_name);
	xchat_print(ph,"* end xfn list");

	return XCHAT_EAT_NONE;
}/* end xfn_list_handler_cb */

/* this function enables and disables the plugin without
 * requiring the user to unload it */
static int xfn_status_handler_cb(char *word[],
                                 char *word_eol[],
                                 void *userdata)
{
	if(word_eol[2][0] == 0) 
	{
		xchat_print(ph,"* requires a second argument");
		xchat_command(ph,"HELP XFN");
		return XCHAT_EAT_ALL;
	}
	
	if( g_strcmp0(word[2],"STATUS") == 0 )
	{
		xchat_print(ph, xfn_status == XFN_ON ? "* xfn is on" : "* xfn is off");
		return XCHAT_EAT_ALL;
	}
	
	if( g_strcmp0(word[2], "ON") == 0 )
	{
		if( xfn_status == XFN_ON )
		{
			xchat_print(ph,"* xfn is already on");
			return XCHAT_EAT_ALL;
		}
		
		xfn_status = XFN_ON;
		xchat_print(ph,"* xfn is now on");
		return XCHAT_EAT_ALL;
	}
	
	if( g_strcmp0(word[2], "OFF") == 0)
	{
		if( xfn_status == XFN_OFF )
		{
			xchat_print(ph,"* xfn is already off");
			return XCHAT_EAT_ALL;
		}
		
		xfn_status = XFN_OFF;
		xchat_print(ph,"* xfn is now off");
		return XCHAT_EAT_ALL;
	}

	xchat_print(ph,"* invalid option. remember that xfn commands are case sensitive");
	xchat_command(ph,"HELP XFN");

	return XCHAT_EAT_ALL;
}/* end xfn_status_handler_cb */

/* this functions handles the behavior of
 * XFN_DPM ON | XFN_DPM OFF | XFN_DPM STATUS
 * these commands allow the user to select determine
 * if the content of the messages appear in 
 * the notification or not.
 */
static int xfn_dpm_status_handler_cb(char *word[],
                                     char *word_eol[],
                                     void *userdata)
{
	if(word_eol[2][0] == 0)
	{
		xchat_print(ph,"* requires a second argument");
		xchat_command(ph,"HELP XFN_DPM");
		return XCHAT_EAT_ALL;
	}
	
	if(g_strcmp0(word[2],"STATUS") == 0)
	{
		xchat_print(ph, xfn_dpm_status == XFN_ON ? "* xfn display message is on" : "* xfn display message is off");
		return XCHAT_EAT_ALL;
	}
	
	if( g_strcmp0(word[2],"ON") == 0 )
	{
		if(xfn_dpm_status == XFN_ON )
		{
			xchat_print(ph,"* xfn display message is already on");
			return XCHAT_EAT_ALL;
		}
		xfn_dpm_status = XFN_ON;
		xchat_print(ph,"* xfn display message is now on");
		return XCHAT_EAT_ALL;
	}
	
	if( g_strcmp0(word[2],"OFF") == 0 )
	{
		if( xfn_dpm_status == XFN_OFF )
		{
			xchat_print(ph,"* xfn display message is already off");
			return XCHAT_EAT_ALL;
		}
		xfn_dpm_status = XFN_OFF;
		xchat_print(ph,"* xfn display message is now off");
		return XCHAT_EAT_ALL;
	}
	
	xchat_print(ph,"* invalid option. remember that xfn commands are case sensitive");
	xchat_command(ph,"HELP XFN_DPM");
	
	return XCHAT_EAT_ALL;
}/* end xfn_dpm_status_handler_cb */

/* this functions controls in which window states
 * the notification should be displayed.
 * this functions responds to combinations of commands
 */
static int xfn_mode_status_handler_cb(char *word[],
                                       char *word_eol[],
                                       void *userdata)
{
	return XCHAT_EAT_NONE;
}/* end xfn_mode_status_handler_cb */

/* this function converts the xchat_get_info(ph,"win_status") 
 * results into XFN_MODE masks so it separates 
 * concerns on xfn_chmsg_handler_cb
 */
static int xfn_get_win_status_mask()
{
	const char *curr_status = xchat_get_info(ph,"win_status");

	if( g_strcmp0("active", curr_status) == 0)
	{
		return XFN_MODE_ACTIVE;
	}
	
	if( g_strcmp0("hidden", curr_status) == 0)
	{
		return XFN_MODE_HIDDEN;
	}
	
	if( g_strcmp0("normal", curr_status) == 0)
	{
		return XFN_MODE_NORMAL;
	}

	return XFN_MODE_UNKNOWN;
}/* end xfn_get_win_status_mask */

static int xfn_chmsg_handler_cb(char *word[],
                                void *userdata)
{
	NotifyNotification *notify;
	GError *error = NULL;
	char buffer[128];
	
	const int curr_mode = xfn_get_win_status_mask();
	
	/* check if the plugin is not in standbye mode */
	if(xfn_status != XFN_ON)
		return XCHAT_EAT_NONE;
	
	/* check if we got the current mode properly */
	if( curr_mode == XFN_MODE_UNKNOWN )
		return XCHAT_EAT_NONE;
	
	/* check if the current mode is allowed */
	if( (xfn_mode_status & curr_mode ) == 0 )
		return XCHAT_EAT_NONE;
	
	/* check if the nickname is in the notify list */
	if( NULL == libglinked_find_node(&xfn_list, word[1], nickname_cmp) )
	{
		/* could not find the nickname, don't display the notification */
		return XCHAT_EAT_NONE;
	}
	
	/* check if we are going to display the content of
	 * the message or not
	 */
	if( xfn_dpm_status == XFN_ON )
	{
		/* show the message */
		snprintf(buffer,128,"said: %s", word[2]);
		notify = notify_notification_new(word[1],
                                         buffer,
                                         NULL/*icon*/);
	}
	else
	{
		snprintf(buffer,128,"from %s", word[1]);
		notify = notify_notification_new("XFN MSG:",
                                         buffer,
                                         NULL/*icon*/);
	}
	
	notify_notification_show(notify, &error);

	return XCHAT_EAT_NONE;
	
}/* end xfn_chmsg_handler_cb */

/* end of custom functions */

/* let xchat know about us */
void xchat_plugin_get_info(char **name, char **desc, 
                           char **version, void **reserved)
{
	*name = PNAME;
	*desc = PDESC;
	*version = PVERSION;
}/* end xchat_plugin_get_info */

/* This is were we start */
int xchat_plugin_init(xchat_plugin *plugin_handle,
                      char **plugin_name,
                      char **plugin_desc,
                      char **plugin_version,
                      char *arg)
{
	/* init libnotify, clean in deinit */
	notify_init("XchatFriendNotify_plugin");
	
	/* init dbsglinked, so xfn_list can be used */
	libglinked_init_list(&xfn_list, NULL, NULL);

	/* save this in order to use xchat_* functions */
	ph = plugin_handle;
	
	/* tell xchat our info */
	*plugin_name    = PNAME;
	*plugin_desc    = PDESC;
	*plugin_version = PVERSION;

	/* adding commands that control the plugin behavior */
	xchat_hook_command(ph,
                       "XFN",
                       XCHAT_PRI_NORM,
                       xfn_status_handler_cb,
                       "* usage:/XFN ON/OFF/STATUS\n**(turns the plugin on and off without unloading it)",
                       NULL);
	
	xchat_hook_command(ph,
                       "XFN_DPM",
                       XCHAT_PRI_NORM,
                       xfn_dpm_status_handler_cb,
                       "* usage:/XFN_DPM ON/OFF/STATUS\n**(enables and disables the messages content on notifications)",
                       NULL);
	
	xchat_hook_command(ph,
                       "XFN_MODE",
                       XCHAT_PRI_NORM,
                       xfn_mode_status_handler_cb,
                       "* usage:/XFN_MODE [ACTIVE/NORMAL/HIDDEN/]/ALWAYS\n"
                       "** this command allows you to combine the multiple forms or simple use always for full combination\n"
                       "**example: /XFN_MODE HIDDEN NORMAL",
                       NULL);

	xchat_hook_command(ph,
                       "XFN_ADD",
                       XCHAT_PRI_NORM,
                       xfn_add_handler_cb,
                       "* usage:/XFN_ADD nickname",
                       NULL);

	xchat_hook_command(ph,
                       "XFN_RM",
                       XCHAT_PRI_NORM,
                       xfn_rm_handler_cb,
                       "* usage:/XFN_RM nickname",
                       NULL);

	xchat_hook_command(ph,
                       "XFN_LIST",
                       XCHAT_PRI_NORM,
                       xfn_list_handler_cb,
                       "* usage:/XFN_LIST",
                       NULL);

	/* the main hook that traps channel messages */
	xchat_hook_print(ph,
                     "Channel Message",
                     XCHAT_PRI_NORM,
                     xfn_chmsg_handler_cb,
                     NULL);
	
	/* let the user know everything ran ok */
	xchat_print(ph, "* XFN loaded successfully!\n");
	return SUCCESS;

}/* end xchat_plugin_init */

/* house keeping is always a good idea */
int xchat_plugin_deinit()
{
	/* disable libnotify */
	notify_uninit();
	/* release allocated data by dbslibglinked */
	libglinked_delete_list(&xfn_list);
	/* keep the user up to date */
	xchat_print(ph, "* XFN deinit successfully!\n");
	return SUCCESS;
}
