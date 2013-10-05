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
#include <libnotify/notify.h>
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

/* custom functions */

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
void xfn_mode_status_handler_cb(char *word[],
                                char *word_eol[],
                                void *userdata)
{

}/* end xfn_mode_status_handler_cb */

static int xfn_chmsg_handler_cb(char *word[],
                                char *word_eol[],
                                void *userdata)
{

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

	/* the main hook that traps channel messages */
	xchat_hook_command(ph,
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
	
	/* keep the user up to date */
	xchat_print(ph, "* XFN deinit successfully!\n");
	return SUCCESS;
}
