/*
 * Copyright (C) 2013 Henner Zeller
 *
 * This file is part of GMediaRender.
 *
 * GMediaRender is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GMediaRender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMediaRender; if not, write to the Free Software 
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 *
 * -----------------
 *
 * Helpers for keeping track of server state variables. UPnP is about syncing
 * state between server and connected controllers and it does so by variables
 * (such as 'CurrentTrackDuration') that can be queried and whose changes
 * can be actively sent to parties that have registered for updates.
 * However, changes are not sent individually when a variable changes
 * but instead encapsulated in XML in a 'LastChange' variable, that contains
 * recent changes since the last update.
 *
 * These utility classes are here to help getting this done:
 *
 * variable_container - handling a bunch of variables containting NUL
 *   terminated strings, allowing C-callbacks to be called when content changes
 *   and differs from previous value.
 *
 * upnp_last_change_builder - a builder for the LastChange XML document
 *   containing name/value pairs of variables.
 *
 * upnp_last_change_collector - handling of the LastChange variable in UPnP.
 *   Hooks into the callback mechanism of the variable_container to assemble
 *   the LastChange variable to be sent over (using the last change builder).
 *
 */ 
#ifndef VARIABLE_CONTAINER_H
#define VARIABLE_CONTAINER_H

// TODO : remove this include - need to resolve cross-dependecies first
#include <upnp/ithread.h>
#include "upnp.h"

// -- VariableContainer
struct variable_container;
typedef struct variable_container variable_container_t;

// Create a new variable container. The variable_names need to be valid for the
// lifetime of this objec.
variable_container_t *VariableContainer_new(int variable_num,
					    struct var_meta *variable_meta,
					    const char **variable_init_values);
void VariableContainer_delete(variable_container_t *object);

// Get number of variables.
int VariableContainer_get_num_vars(variable_container_t *object);

// Get variable name/value. if OUT parameter 'name' is not NULL, returns
// name of variable for given number.
// Returns current value of variable or NULL if it does not exist.
// Returned value owned by variable container; on variable change, this value
// will be invalid.
const char *VariableContainer_get(variable_container_t *object, int var,
				  const char **name,
				  param_event *evented);

// Change content of variable with given number to NUL terminated content.
// Returns '1' if value actually changed and all callbacks were called,
// '0' if no change was detected.
int VariableContainer_change(variable_container_t *object,
			     int variable_num, const char *value);

// Callback handling. Whenever a variable changes, the callback is called.
// Be careful when changing variables in the original container as this will
// trigger recursive calls to the container.
typedef void (*variable_change_listener_t)(void *userdata,
					   int var_num, const char *var_name,
					   const char *old_value,
					   const char *new_value);
void VariableContainer_register_callback(variable_container_t *object,
					 variable_change_listener_t callback,
					 void *userdata);
void VariableContainer_lock(variable_container_t *object);
void VariableContainer_unlock(variable_container_t *object);

// -- UPnP LastChange Builder - builds a LastChange XML document from
// added name/value pairs.
struct upnp_last_change_builder;
typedef struct upnp_last_change_builder upnp_last_change_builder_t;

// Create a new last change builder. The pointer to the xml_namespace string
// must exist for the livetime of this object.
upnp_last_change_builder_t *UPnPLastChangeBuilder_new(const char *xml_namespace);
void UPnPLastChangeBuilder_delete(upnp_last_change_builder_t *builder);

void UPnPLastChangeBuilder_add(upnp_last_change_builder_t *builder,
			       const char *name, const char *value);
// Returns a newly allocated XML string that needs to be free()'d by the caller.
// Resets the document. If no changes have been added, NULL is returned.
char *UPnPLastChangeBuilder_to_xml(upnp_last_change_builder_t *builder);

// -- UPnP variable change collector
struct upnp_device;  // forward declare.
struct upnp_var_change_collector;
typedef struct upnp_var_change_collector upnp_var_change_collector_t;

// Create a new variable change collector that registers at the
// "variable_container" for changes in variables. It accumulates information
// about state variables changed during a transaction.
// After a transaction is finished, and update event is sent.
// If LastChange variable is present, it is the only variable sent within
// event. Otherwise, all modified variables which are defined as eventable
// are sent in the event.
upnp_var_change_collector_t *
UPnPVarChangeCollector_new(variable_container_t *variable_container,
		const char *event_xml_namespace,
		struct upnp_device *upnp_device,
		const char *service_id);

// If we know that there is at leats one change upcoming, we MUST
// 'start' a transaction and tell the collector to keep collecting until we
// 'finish'. This can be nested. 
void UPnPVarChangeCollector_start(upnp_var_change_collector_t *object);
void UPnPVarChangeCollector_finish(upnp_var_change_collector_t *object);
void variable_container_init(void);

// no delete yet. We leak that.
#endif  /* VARIABLE_CONTAINER_H */
