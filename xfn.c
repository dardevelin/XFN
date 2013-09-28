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

#define SUCCESS    1

/* Globals */
static xchat_plugin *ph; /* plugin handle */
/* This flag acts as standbye switch for the entire plugin */
static int xfn_status = XFN_ON;
/* This flag controls if the notification should display
 * the contents of the message sent to the user or not
 */
static int xfn_display_message = XFN_OFF;

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
	
	/* let the user know everything ran ok */
	xchat_print(ph, "XFN loaded_successfully!\n");
	return SUCCESS;

}/* end xchat_plugin_init */

/* house keeping is always a good idea */
int xchat_plugin_deinit()
{
	/* disable libnotify */
	notify_uninit();
	
	/* keep the user up to date */
	xchat_print(ph, "XFN deinit successfully!\n");
	return SUCCESS;
}
