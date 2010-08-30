
/*
Copyright (C) 2007 ezQuake team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

$Id: ez_controls.c,v 1.78 2007/10/27 14:51:15 cokeman1982 Exp $
*/

#include "quakedef.h"
#include "keys.h"
#include "utils.h"
#include "common_draw.h"
#include "ez_controls.h"

// =========================================================================================
// Double Linked List
// =========================================================================================

//
// Add item to double linked list.
//
void EZ_double_linked_list_Add(ez_double_linked_list_t *list, void *payload)
{
	ez_dllist_node_t *item = (ez_dllist_node_t *)Q_malloc(sizeof(ez_dllist_node_t));
	memset(item, 0, sizeof(ez_dllist_node_t));

	item->payload = payload;
	item->next = NULL;

	// Add the item to the start of the list if it's empty
	// otherwise add it to the tail.
	if(!list->head)
	{
		list->head = item;
		item->next = NULL;
		item->previous = NULL;
	}
	else
	{
		item->previous = list->tail;
	}

	// Make sure the previous item knows who we are.
	if(item->previous)
	{
		item->previous->next = item;
	}

	list->tail = item;
	list->count++;
}

//
// Finds a given node based on the specified payload.
//
ez_dllist_node_t *EZ_double_linked_list_FindByPayload(ez_double_linked_list_t *list, void *payload)
{
	ez_dllist_node_t *iter = list->head;

	while(iter)
	{
		if(iter->payload == payload)
		{
			return iter;
		}

		iter = iter->next;
	}

	return NULL;
}

//
// Removes an item from a linked list by it's payload.
//
void *EZ_double_linked_list_RemoveByPayload(ez_double_linked_list_t *list, void *payload)
{
	ez_dllist_node_t *node = EZ_double_linked_list_FindByPayload(list, payload);
	return EZ_double_linked_list_Remove(list, node);
}

//
// Removes the first occurance of the item from double linked list and returns it's payload.
//
void *EZ_double_linked_list_Remove(ez_double_linked_list_t *list, ez_dllist_node_t *item)
{
	void *payload = item->payload;

	if (item->previous)
	{
		item->previous->next = item->next;
	}
	else
	{
		// We removed the first item, so make sure we still have a head.
		list->head = item->next;
	}

	if (item->next)
	{
		item->next->previous = item->previous;
	}
	else if (item->previous)
	{
		// We removed the last item, so make sure we have a tail.
		list->tail = item->previous;
	}

	list->count--;

	Q_free(item);

	return payload;
}

typedef void * PVOID;

//
// Double Linked List - Orders a list.
//
void EZ_double_linked_list_Order(ez_double_linked_list_t *list, PtFuncCompare compare_function)
{
	int i = 0;
	ez_dllist_node_t *iter = NULL;
	PVOID **items = (PVOID **)Q_calloc(list->count, sizeof(PVOID *));

	iter = list->head;

	while(iter)
	{
		items[i] = iter->payload;
		i++;

		iter = iter->next;
	}

	qsort(items, list->count, sizeof(PVOID *), compare_function);

	iter = list->head;

	for(i = 0; i < list->count; i++)
	{
		iter->payload = items[i];
		iter = iter->next;
	}

	Q_free(items);
}

// =========================================================================================
// Control Tree
// =========================================================================================

//
// Control tree -
// Sets the drawing bounds for a control and then calls the function
// recursivly on all it's children. These bounds are used to restrict the drawing
// of all children that should be contained within it's parent, and their children
// to within the bounds of the parent.
//
static void EZ_tree_SetDrawBounds(ez_control_t *control)
{
	ez_dllist_node_t *iter = NULL;
	ez_control_t *child = NULL;
	ez_control_t *p = control->parent;
	qbool contained = control->ext_flags & control_contained;

	// Calculate the controls bounds.
	int top		= control->absolute_y;
	int bottom	= control->absolute_y + control->height;
	int left	= control->absolute_x;
	int right	= control->absolute_x + control->width;

	int p_bound_top		= p ? p->bound_top		: 0;
	int p_bound_bottom	= p ? p->bound_bottom	: 0;
	int p_bound_left	= p ? p->bound_left		: 0;
	int p_bound_right	= p ? p->bound_right	: 0;

	// Change the bounds so that we don't draw over our parents resize handles.
	if (p)
	{
		if (p->ext_flags & control_resize_h)
		{
			p_bound_right	-= p->resize_handle_thickness;
			p_bound_left	+= p->resize_handle_thickness;
		}

		if (p->ext_flags & control_resize_v)
		{
			p_bound_bottom	-= p->resize_handle_thickness;
			p_bound_top		+= p->resize_handle_thickness;
		}
	}

	// If the control has a parent (and should be contained within it's parent), 
	// set the corresponding bound to the parents bound (ex. button), 
	// otherwise use the drawing area of the control as bounds (ex. windows).
	control->bound_top		= (p && contained && (top	 < p_bound_top))		? (p_bound_top)		: top;
	control->bound_bottom	= (p && contained && (bottom > p_bound_bottom))		? (p_bound_bottom)	: bottom;
	control->bound_left		= (p && contained && (left	 < p_bound_left))		? (p_bound_left)	: left;
	control->bound_right	= (p && contained && (right	 > p_bound_right))		? (p_bound_right)	: right;

	// Make sure that the left bounds isn't greater than the right bounds and so on.
	// This would lead to controls being visible in a few incorrect cases.
	clamp(control->bound_top, 0, max(0, control->bound_bottom));
	clamp(control->bound_bottom, control->bound_top, vid.conheight);
	clamp(control->bound_left, 0, max(0, control->bound_right));
	clamp(control->bound_right, control->bound_left, vid.conwidth);

	// Calculate the bounds for the children.
	for (iter = control->children.head; iter; iter = iter->next)
	{
		child = (ez_control_t *)iter->payload;

		// TODO : Probably should some better check for infinte loop here also.
		if (child == control)
		{
			Sys_Error("EZ_tree_SetDrawBounds(): Infinite loop, child is its own parent.\n");
		}

		EZ_tree_SetDrawBounds(child);
	}
}

//
// Control Tree - Draws a control tree.
//
static void EZ_tree_Draw(ez_tree_t *tree)
{
	ez_control_t *payload = NULL;
	ez_dllist_node_t *iter = tree->drawlist.head;

	while (iter)
	{
		payload = (ez_control_t *)iter->payload;

		// TODO : Remove this test stuff.
		/*
		Draw_AlphaRectangleRGB(
			payload->absolute_virtual_x, 
			payload->absolute_virtual_y, 
			payload->virtual_width, 
			payload->virtual_height, 
			1, false, RGBA_TO_COLOR(255, 0, 0, 125));
			*/

		// Don't draw the invisible controls.
		if (!(payload->ext_flags & control_visible) || (payload->int_flags & control_hidden_by_parent))
		{
			iter = iter->next;
			continue;
		}

		// TODO : Remove this test stuff.
		/*
		if (!strcmp(payload->name, "Close button"))
		{
			Draw_String(payload->absolute_virtual_x, payload->absolute_virtual_y - 10, 
				va("x: %i y: %i", payload->x, payload->y));
		}
		*/

		/*
		if (!strcmp(payload->name, "label"))
		{
			Draw_String(payload->absolute_virtual_x, payload->absolute_virtual_y - 10, 
				va("avx: %i avy: %i vx: %i vy %i", 
				payload->absolute_virtual_x, payload->absolute_virtual_y, payload->virtual_x, payload->virtual_y));
		}

		if (!strcasecmp(payload->name, "Child 1"))
		{
			Draw_String(payload->absolute_virtual_x, payload->absolute_virtual_y - 10, 
				va("vw: %i vh: %i w: %i h %i", 
				payload->virtual_width, payload->virtual_height, payload->width, payload->height));
		}
		*/

		/*
		if (!strcasecmp(payload->name, "button"))
		{
			Draw_AlphaLineRGB(payload->bound_left, 0, payload->bound_left, vid.conheight, 1, RGBA_TO_COLOR(255, 0, 0, 255));
			Draw_AlphaLineRGB(payload->bound_right, 0, payload->bound_right, vid.conheight, 1, RGBA_TO_COLOR(255, 0, 0, 255));
		}
		*/

		/*
		if (!strcasecmp(payload->name, "Vertical scrollbar"))
		{
			Draw_String(payload->absolute_virtual_x, payload->absolute_virtual_y - 10, 
				va("b: %i r: %i", payload->bottom_edge_gap, payload->right_edge_gap));
		}
		*/

		/*
		if (!strcmp(payload->name, "Close button") || !strcmp(payload->name, "Window titlebar"))
		{
			Draw_AlphaRectangleRGB(
				payload->absolute_virtual_x, 
				payload->absolute_virtual_y, 
				payload->virtual_width, 
				payload->virtual_height, 
				1, false, RGBA_TO_COLOR(0, 0, 255, 125));
		}
		*/
		
		// Bugfix: Make sure we don't even bother trying to draw something that is completly offscreen
		// it will cause a weird flickering bug because of glScissor.
		if ((payload->bound_left == payload->bound_right) || (payload->bound_top == payload->bound_bottom))
		{
			iter = iter->next;
			continue;
		}

		// Make sure that the control draws only within it's bounds.
		Draw_EnableScissor(payload->bound_left, payload->bound_right, payload->bound_top, payload->bound_bottom);

		// Raise the draw event for the control.
		CONTROL_RAISE_EVENT(NULL, payload, ez_control_t, OnDraw);

		// Reset the drawing bounds to the entire screen after the control has been drawn.
		Draw_DisableScissor();

		iter = iter->next;
	}
}

//
// Control Tree - Checks how long mouse buttons have been pressed.
//
static void EZ_tree_RaiseRepeatedMouseButtonEvents(ez_tree_t *tree)
{
	if (tree->focused_node && tree->focused_node->payload)
	{
		ez_control_t *control = (ez_control_t *)tree->focused_node->payload;

		// Notify controls that are listening to repeat mouse events 
		// if the control's set delay has been reached.
		if (control->ext_flags & control_listen_repeat_mouse)
		{
			int i;
			double now			= Sys_DoubleTime();
			mouse_state_t ms	= tree->prev_mouse_state;
			ms.button_up		= 0;
			
			// Go through all the mouse buttons and compare the time since
			// the mouse buttons where pressed with the controls delay time.
			for (i = 1; i <= 8; i++)
			{
				// Only repeat the mouse click if the button is already down
				// and the repeat isn't set to be all the time.
				if (ms.buttons[i]
				 &&	(control->mouse_repeat_delay > 0.0) && (tree->mouse_pressed_time[i] > 0.0)
				 && ((now - tree->mouse_pressed_time[i]) >= control->mouse_repeat_delay))
				{
					ms.button_down = i;
					CONTROL_RAISE_EVENT(NULL, control, ez_control_t, OnMouseEvent, &ms);
				}
			}
		}
	}
}

//
// Control Tree - Needs to be called every frame to keep the tree alive.
//
void EZ_tree_EventLoop(ez_tree_t *tree)
{
	if (!tree->root)
	{
		return;
	}

	// Check if it's time to raise any repeat mouse events for controls listening to those.
	EZ_tree_RaiseRepeatedMouseButtonEvents(tree);

	// Calculate the drawing bounds for all the controls in the control tree.
	EZ_tree_SetDrawBounds(tree->root);

	EZ_tree_Draw(tree);
}

//
// Control Tree - Dispatches a mouse event to a control tree.
//
qbool EZ_tree_MouseEvent(ez_tree_t *tree, mouse_state_t *ms)
{
	int mouse_handled = false;
	ez_control_t *control = NULL;
	ez_dllist_node_t *iter = NULL;

	if (!tree)
	{
		Sys_Error("EZ_tree_MouseEvent: NULL tree reference.\n");
	}

	// Save the time that the specified button was last pressed.
	if (ms->button_down || ms->button_up)
	{
		// Set the time for the pressed button.
		if (ms->button_down)
		{
			tree->mouse_pressed_time[ms->button_down] = Sys_DoubleTime();
		}
		else if (ms->button_up)
		{
			tree->mouse_pressed_time[ms->button_up] = 0.0;
		}
	}

	// Save the mouse state so that it can be used when raising
	// repeated mouse click events for controls that wants them.
	tree->prev_mouse_state = *ms;

	// Propagate the mouse event in the opposite order that we drew
	// the controls (Since they are drawn from back to front), so
	// that the foremost control gets it first.
	for (iter = tree->drawlist.tail; iter; iter = iter->previous)
	{
		control = (ez_control_t *)iter->payload;

		// Notify the control of the mouse event.
		CONTROL_RAISE_EVENT(&mouse_handled, control, ez_control_t, OnMouseEvent, ms);

		if (mouse_handled)
		{
			return mouse_handled;
		}
	}

	return mouse_handled;
}

//
// Control Tree - Moves the focus to the next control in the control tree.
//
void EZ_tree_ChangeFocus(ez_tree_t *tree, qbool next_control)
{
	qbool found = false;
	ez_dllist_node_t *node_iter = NULL;

	if(tree->focused_node)
	{
		node_iter = (next_control) ? tree->focused_node->next : tree->focused_node->previous;

		// Find the next control that can be focused.
		while(node_iter && !found)
		{
			// ez_control_t *ha = (ez_control_t *)node_iter->payload;
			found = EZ_control_SetFocusByNode((ez_control_t *)node_iter->payload, node_iter);
			node_iter = (next_control) ? node_iter->next : node_iter->previous;
		}
	}

	// We haven't found a focusable control yet,
	// or there was no focused control to start with.
	// So search for one from the start/end of the tab list
	// (depending on what direction we're searching in)
	if(!found || !tree->focused_node)
	{
		node_iter = (next_control) ? tree->tablist.head : tree->tablist.tail;

		// Find the next control that can be focused.
		while(node_iter && !found)
		{
			found = EZ_control_SetFocusByNode((ez_control_t *)node_iter->payload, node_iter);
			node_iter = (next_control) ? node_iter->next : node_iter->previous;
		}
	}

	// There is nothing to focus on.
	if(!found)
	{
		tree->focused_node = NULL;
	}
}

//
// Control Tree - Key event.
//
qbool EZ_tree_KeyEvent(ez_tree_t *tree, int key, int unichar, qbool down)
{
	int key_handled = false;
	ez_control_t *payload = NULL;
	ez_dllist_node_t *iter = NULL;

	if (!tree)
	{
		Sys_Error("EZ_tree_KeyEvent(): NULL control tree specified.\n");
	}

	if (tree->root && down)
	{
		switch (key)
		{
			case 'k' :
				key_handled = true;
				break;
			case K_TAB :
			{
				// Focus on the next focusable control (TAB)
				// or the previous (Shift + TAB)
				EZ_tree_ChangeFocus(tree, !isShiftDown());
				key_handled = true;
				break;
			}
		}
	}

	// Send key events to the focused control.
	if (tree->focused_node && tree->focused_node->payload)
	{
		CONTROL_RAISE_EVENT(&key_handled, (ez_control_t *)tree->focused_node->payload, ez_control_t, OnKeyEvent, key, unichar, down);
	}

	return key_handled;
}

//
// Control Tree - Finds any orphans and adds them to the root control.
//
void EZ_tree_UnOrphanizeChildren(ez_tree_t *tree)
{
	ez_control_t *payload = NULL;
	ez_dllist_node_t *iter = NULL;

	if(!tree)
	{
		assert(!"EZ_control_UnOrphanizeChildren: No control tree specified.\n");
		return;
	}

	iter = tree->drawlist.head;

	while(iter)
	{
		payload = (ez_control_t *)iter->payload;

		if(!payload->parent && payload != tree->root)
		{
			// The control is an orphan, and not the root control.
			EZ_double_linked_list_Add(&tree->root->children, payload);
		}
		else if(!tree->root)
		{
			// There was no root (should never happen).
			tree->root = payload;
		}

		iter = iter->next;
	}
}

//
// Control Tree - Destroys a tree. Will not free the memory for the tree.
//
void EZ_tree_Destroy(ez_tree_t *tree)
{
	ez_dllist_node_t *iter = NULL;

	if (!tree)
	{
		return;
	}

	EZ_control_Destroy(tree->root, true);

	memset(tree, 0, sizeof(ez_tree_t));
}

//
// Control Tree - Order function for the draw list based on the draw_order property.
//
static int EZ_tree_DrawOrderFunc(const void *val1, const void *val2)
{
	const ez_control_t **control1 = (const ez_control_t **)val1;
	const ez_control_t **control2 = (const ez_control_t **)val2;

	return ((*control1)->draw_order - (*control2)->draw_order);
}

//
// Control Tree - Orders the draw list based on the draw order property.
//
void EZ_tree_OrderDrawList(ez_tree_t *tree)
{
	EZ_double_linked_list_Order(&tree->drawlist, EZ_tree_DrawOrderFunc);
}

//
// Control Tree - Tree Order function for the tab list based on the tab_order property.
//
static int EZ_tree_TabOrderFunc(const void *val1, const void *val2)
{
	const ez_control_t **control1 = (const ez_control_t **)val1;
	const ez_control_t **control2 = (const ez_control_t **)val2;

	return ((*control1)->tab_order - (*control2)->tab_order);
}

//
// Control Tree - Orders the tab list based on the tab order property.
//
void EZ_tree_OrderTabList(ez_tree_t *tree)
{
	EZ_double_linked_list_Order(&tree->tablist, EZ_tree_TabOrderFunc);
}

// =========================================================================================
// Control
// =========================================================================================

//
// Control - Creates a new control and initializes it.
//
ez_control_t *EZ_control_Create(ez_tree_t *tree, ez_control_t *parent,
							  char *name, char *description,
							  int x, int y, int width, int height,
							  int flags)
{
	ez_control_t *control = NULL;

	// We have to have a tree to add the control to.
	if(!tree)
	{
		return NULL;
	}

	control = (ez_control_t *)Q_malloc(sizeof(ez_control_t));
	memset(control, 0, sizeof(ez_control_t));

	EZ_control_Init(control, tree, parent, name, description, x, y, width, height, flags);

	return control;
}

//
// Control - 
// Returns the screen position of the control. This will be different for a scrollable window
// since it's drawing position differs from the windows actual position on screen.
//
void EZ_control_GetDrawingPosition(ez_control_t *self, int *x, int *y)
{
	if (self->ext_flags & control_scrollable)
	{
		(*x) = self->absolute_virtual_x;
		(*y) = self->absolute_virtual_y;
	}
	else
	{
		(*x) = self->absolute_x;
		(*y) = self->absolute_y;
	}
}

//
// Eventhandler - Creates a eventhandler.
//
ez_eventhandler_t *EZ_eventhandler_Create(void *event_func, int func_type, void *payload)
{
	ez_eventhandler_t *e	= Q_calloc(1, sizeof(ez_eventhandler_t));
	e->function_type		= func_type;
	e->payload				= payload;

	switch(func_type)
	{
		case EZ_CONTROL_HANDLER :
			e->function.normal = (ez_eventhandler_fp)event_func;
			break;
		case EZ_CONTROL_MOUSE_HANDLER :
			e->function.mouse = (ez_mouse_eventhandler_fp)event_func;
			break;
		case EZ_CONTROL_KEY_HANDLER :
			e->function.key = (ez_key_eventhandler_fp)event_func;
			break;
		case EZ_CONTROL_KEYSP_HANDLER :
			e->function.key_sp = (ez_keyspecific_eventhandler_fp)event_func;
			break;
		case EZ_CONTROL_DESTROY_HANDLER :
			e->function.destroy = (ez_destroy_eventhandler_fp)event_func;
			break;
		default :
			e->function.normal = NULL;
			break;
	}

	e->next	= NULL;

	return e;
}	

//
// Eventhandler - Goes through the list of events and removes the one with the specified function.
//
void EZ_eventhandler_Remove(ez_eventhandler_t *eventhandler, void *event_func, qbool all)
{
	ez_eventhandler_t *it = eventhandler;
	ez_eventhandler_t *prev = NULL;

	while (it)
	{
		if (all)
		{
			prev = prev->next;
			Q_free(it);
			it = prev;
		}
		else 
		{
			if (event_func && ((void *)it->function.normal == (void *)event_func))
			{
				if (prev)
				{
					prev->next = it->next;
				}

				Q_free(it);

				return;
			}

			prev = it;
			it = it->next;
		}
	}
}

//
// Eventhandler - Execute an event handler.
//
void EZ_eventhandler_Exec(ez_eventhandler_t *event_handler, ez_control_t *ctrl, ...)
{
	va_list argptr;
	int ft = event_handler->function_type;
	void *payload = event_handler->payload;
	ez_eventhandlerfunction_t *et = &event_handler->function;

	va_start(argptr, ctrl);

	// Pass on the proper amount of arguments, depending on the type of function.
	if (ft == EZ_CONTROL_HANDLER)
	{
		et->normal(ctrl, payload);
	}
	else if (ft == EZ_CONTROL_MOUSE_HANDLER)
	{
		et->mouse(ctrl, payload, va_arg(argptr, mouse_state_t *));	
	}
	else if (ft == EZ_CONTROL_KEY_HANDLER)
	{
		et->key(ctrl, payload, va_arg(argptr, int), va_arg(argptr, int), va_arg(argptr, qbool));
	}
	else if (ft == EZ_CONTROL_KEYSP_HANDLER)
	{
		et->key_sp(ctrl, payload, va_arg(argptr, int), va_arg(argptr, int));
	}
	else if (ft == EZ_CONTROL_DESTROY_HANDLER)	
	{
		et->destroy(ctrl, payload, va_arg(argptr, qbool));
	}

	va_end(argptr);
}

//
// Control - Initializes a control and adds it to the specified control tree.
//
void EZ_control_Init(ez_control_t *control, ez_tree_t *tree, ez_control_t *parent,
							  char *name, char *description,
							  int x, int y, int width, int height, 
							  ez_control_flags_t flags)
{
	static int order		= 0;

	control->CLASS_ID		= EZ_CONTROL_ID;

	control->control_tree	= tree;
	control->name			= name;
	control->description	= description;
	control->ext_flags		= flags | control_enabled | control_visible;
	control->anchor_flags	= anchor_none;

	// Default to containing a child within it's parent
	// if the parent is being contained by it's parent.
	if (parent && parent->ext_flags & control_contained)
	{
		control->ext_flags |= control_contained;
	}

	control->draw_order					= order++;

	control->resize_handle_thickness	= 5;
	control->width_max					= vid.conwidth;
	control->height_max					= vid.conheight;
	control->width_min					= 5;
	control->height_min					= 5;

	// Setup the default event functions, none of these should ever be NULL.
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseEvent, OnMouseEvent, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseClick, OnMouseClick, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseDown,	OnMouseDown, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseUp, OnMouseUp, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseUpOutside, OnMouseUpOutside, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseEnter, OnMouseEnter, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseLeave, OnMouseLeave, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMouseHover, OnMouseHover, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_Destroy, OnDestroy, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnDraw, OnDraw, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnGotFocus, OnGotFocus, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnKeyEvent, OnKeyEvent, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnKeyDown, OnKeyDown, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnKeyUp, OnKeyUp, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnLostFocus, OnLostFocus, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnLayoutChildren, OnLayoutChildren, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMove, OnMove, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnScroll, OnScroll, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnParentScroll, OnParentScroll, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnResize, OnResize, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnParentResize, OnParentResize, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnMinVirtualResize, OnMinVirtualResize, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnVirtualResize, OnVirtualResize, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnFlagsChanged, OnFlagsChanged, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnEventHandlerChanged, OnEventHandlerChanged, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnAnchorChanged, OnAnchorChanged, ez_control_t);
	CONTROL_REGISTER_EVENT(control, EZ_control_OnVisibilityChanged, OnVisibilityChanged, ez_control_t);

	// Add the control to the control tree.
	if(!tree->root)
	{
		// The first control will be the root.
		tree->root = control;
	}
	else if(!parent)
	{
		// No parent was given so make the control a root child.
		EZ_control_AddChild(tree->root, control);
	}
	else
	{
		// Add the control to the specified parent.
		EZ_control_AddChild(parent, control);
	}

	// Add the control to the draw and tab list.
	EZ_double_linked_list_Add(&tree->drawlist, (void *)control);
	EZ_double_linked_list_Add(&tree->tablist, (void *)control);

	// Order the lists.
	EZ_tree_OrderDrawList(tree);
	EZ_tree_OrderTabList(tree);

	// Position and size of the control.
	control->width					= width;
	control->height					= height;
	control->virtual_width			= width;
	control->virtual_height			= height;
	control->prev_width				= width;
	control->prev_height			= height;
	control->prev_virtual_width		= width;
	control->prev_virtual_height	= height;

	control->int_flags |= control_update_anchorgap;

	EZ_control_SetVirtualSize(control, width, height);
	EZ_control_SetMinVirtualSize(control, 1, 1);
	EZ_control_SetPosition(control, x, y);
	EZ_control_SetSize(control, width, height);

	// Set a default delay for raising new mouse click events.
	EZ_control_SetRepeatMouseClickDelay(control, 0.2);
}

//
// Control - Destroys a specified control.
//
int EZ_control_Destroy(ez_control_t *self, qbool destroy_children)
{
	ez_dllist_node_t *iter = NULL;
	ez_dllist_node_t *temp = NULL;

	// Nothing to destroy :(
	if (!self)
	{
		return 0;
	}

	if (!self->control_tree)
	{
		// Very bad!
		Sys_Error("EZ_control_Destroy(): tried to destroy control without a tree.\n");
		return 0;
	}

	// Cleanup any specifics this control may have.
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnDestroy, destroy_children);

	iter = self->children.head;

	// Destroy the children!
	while (iter)
	{
		if (destroy_children)
		{
			// Destroy the child!
			CONTROL_RAISE_EVENT(NULL, ((ez_control_t *)iter->payload), ez_control_t, OnDestroy, destroy_children);
		}
		else
		{
			// The child becomes an orphan :~/
			((ez_control_t *)iter->payload)->parent = NULL;
		}

		temp = iter;
		iter = iter->next;

		// Remove the child from the list.
		EZ_double_linked_list_Remove(&self->children, temp);
	}

	EZ_eventhandler_Remove(self->event_handlers.OnDestroy, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnDraw, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnEventHandlerChanged, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnFlagsChanged, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnGotFocus, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnKeyDown, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnKeyEvent, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnKeyUp, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnLayoutChildren, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnLostFocus, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMinVirtualResize, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseClick, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseDown, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseEnter, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseEvent, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseHover, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseLeave, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseUp, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMouseUpOutside, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnMove, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnParentResize, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnParentScroll, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnResize, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnResizeHandleThicknessChanged, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnScroll, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnVirtualResize, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnAnchorChanged, NULL, true);
	EZ_eventhandler_Remove(self->event_handlers.OnVisibilityChanged, NULL, true);

	Q_free(self);

	return 0;
}

//
// Control - Sets the OnDestroy event handler.
//
void EZ_control_AddOnDestroy(ez_control_t *self, ez_destroy_eventhandler_fp OnDestroy, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_DESTROY_HANDLER, OnDestroy, ez_control_t, OnDestroy, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnFlagsChanged event handler.
//
void EZ_control_AddOnFlagsChanged(ez_control_t *self, ez_eventhandler_fp OnFlagsChanged, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnFlagsChanged, ez_control_t, OnFlagsChanged, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnLayoutChildren event handler.
//
void EZ_control_AddOnLayoutChildren(ez_control_t *self, ez_eventhandler_fp OnLayoutChildren, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnLayoutChildren, ez_control_t, OnLayoutChildren, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMove event handler.
//
void EZ_control_AddOnMove(ez_control_t *self, ez_eventhandler_fp OnMove, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnMove, ez_control_t, OnMove, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnScroll event handler.
//
void EZ_control_AddOnScroll(ez_control_t *self, ez_eventhandler_fp OnScroll, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnScroll, ez_control_t, OnScroll, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnResize event handler.
//
void EZ_control_AddOnResize(ez_control_t *self, ez_eventhandler_fp OnResize, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnResize, ez_control_t, OnResize, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnParentResize event handler.
//
void EZ_control_AddOnParentResize(ez_control_t *self, ez_eventhandler_fp OnParentResize, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnParentResize, ez_control_t, OnParentResize, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMinVirtualResize event handler.
//
void EZ_control_AddOnMinVirtualResize(ez_control_t *self, ez_eventhandler_fp OnMinVirtualResize, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnMinVirtualResize, ez_control_t, OnMinVirtualResize, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnVirtualResize event handler.
//
void EZ_control_AddOnVirtualResize(ez_control_t *self, ez_eventhandler_fp OnVirtualResize, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnVirtualResize, ez_control_t, OnVirtualResize, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnKeyEvent event handler.
//
void EZ_control_AddOnKeyEvent(ez_control_t *self, ez_key_eventhandler_fp OnKeyEvent, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_KEY_HANDLER, OnKeyEvent, ez_control_t, OnKeyEvent, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnLostFocus event handler.
//
void EZ_control_AddOnLostFocus(ez_control_t *self, ez_eventhandler_fp OnLostFocus, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnLostFocus, ez_control_t, OnLostFocus, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnGotFocus event handler.
//
void EZ_control_AddOnGotFocus(ez_control_t *self, ez_eventhandler_fp OnGotFocus, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnGotFocus, ez_control_t, OnGotFocus, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseHover event handler.
//
void EZ_control_AddOnMouseHover(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseHover, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseHover, ez_control_t, OnMouseHover, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseLeave event handler.
//
void EZ_control_AddOnMouseLeave(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseLeave, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseLeave, ez_control_t, OnMouseLeave, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseEnter event handler.
//
void EZ_control_AddOnMouseEnter(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseEnter, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseEnter, ez_control_t, OnMouseEnter, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseClick event handler.
//
void EZ_control_AddOnMouseClick(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseClick, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseClick, ez_control_t, OnMouseClick, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseUp event handler.
//
void EZ_control_AddOnMouseUp(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseUp, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseUp, ez_control_t, OnMouseUp, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseUpOutside event handler.
//
void EZ_control_AddOnMouseUpOutside(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseUpOutside, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseUpOutside, ez_control_t, OnMouseUpOutside, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseDown event handler.
//
void EZ_control_AddOnMouseDown(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseDown, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseDown, ez_control_t, OnMouseDown, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnMouseEvent event handler.
//
void EZ_control_AddOnMouseEvent(ez_control_t *self, ez_mouse_eventhandler_fp OnMouseEvent, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_MOUSE_HANDLER, OnMouseEvent, ez_control_t, OnMouseEvent, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnDraw event handler.
//
void EZ_control_AddOnDraw(ez_control_t *self, ez_eventhandler_fp OnDraw, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnDraw, ez_control_t, OnDraw, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Set the event handler for the OnEventHandlerChanged event.
//
void EZ_control_AddOnEventHandlerChanged(ez_control_t *self, ez_eventhandler_fp OnEventHandlerChanged, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnEventHandlerChanged, ez_control_t, OnEventHandlerChanged, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Sets the OnResizeHandleThicknessChanged event handler.
//
void EZ_control_AddOnResizeHandleThicknessChanged(ez_control_t *self, ez_eventhandler_fp OnResizeHandleThicknessChanged, void *payload)
{
	CONTROL_ADD_EVENTHANDLER(self, EZ_CONTROL_HANDLER, OnResizeHandleThicknessChanged, ez_control_t, OnResizeHandleThicknessChanged, payload);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnEventHandlerChanged);
}

//
// Control - Focuses on a control.
//
qbool EZ_control_SetFocus(ez_control_t *self)
{
	return EZ_control_SetFocusByNode(self, EZ_double_linked_list_FindByPayload(&self->control_tree->tablist, self));
}

//
// Control - Focuses on a control associated with a specified node from the tab list.
//
qbool EZ_control_SetFocusByNode(ez_control_t *self, ez_dllist_node_t *node)
{
	ez_tree_t *tree = NULL;

	if(!self || !node->payload)
	{
		Sys_Error("EZ_control_SetFocus(): Cannot focus on a NULL control.\n");
	}

	// The nodes payload and the control must be the same.
	if(self != node->payload)
	{
		return false;
	}

	// We can't focus on this control.
	if(!(self->ext_flags & control_focusable) || !(self->ext_flags & control_enabled))
	{
		return false;
	}

	tree = self->control_tree;

	// Steal the focus from the currently focused control.
	if(tree->focused_node)
	{
		ez_control_t *payload = NULL;

		if(!tree->focused_node->payload)
		{
			Sys_Error("EZ_control_SetFocus(): Focused node has a NULL payload.\n");
		}

		payload = (ez_control_t *)tree->focused_node->payload;

		// We're trying to focus on the already focused node.
		if(payload == self)
		{
			return true;
		}

		// Remove the focus flag and set the focus node to NULL.
		payload->int_flags &= ~control_focused;
		tree->focused_node = NULL;

		// Reset all manipulation flags.
		payload->int_flags &= ~(control_clicked | control_moving | control_resizing_left | control_resizing_right | control_resizing_bottom | control_resizing_top);

		// Raise event for losing the focus.
		CONTROL_RAISE_EVENT(NULL, payload, ez_control_t, OnLostFocus);
	}

	// Set the new focus.
	self->int_flags |= control_focused;
	tree->focused_node = node;

	// Raise event for getting focus.
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnGotFocus);

	return true;
}

//
// Control - Set the background image for the control.
//
void EZ_control_SetBackgroundImage(ez_control_t *self, const char *background_path)
{
	self->background = background_path ? Draw_CachePicSafe(background_path, false, true) : NULL;
}

//
// Control - Sets whetever the control is contained within the bounds of it's parent or not, or is allowed to draw outside it.
//
void EZ_control_SetContained(ez_control_t *self, qbool contained)
{
	SET_FLAG(self->ext_flags, control_contained, contained);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is enabled or not.
//
void EZ_control_SetEnabled(ez_control_t *self, qbool enabled)
{
	// TODO : Make sure input isn't allowed if a control isn't enabled.
	SET_FLAG(self->ext_flags, control_enabled, enabled);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is movable.
//
void EZ_control_SetMovable(ez_control_t *self, qbool movable)
{
	SET_FLAG(self->ext_flags, control_movable, movable);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is focusable.
//
void EZ_control_SetFocusable(ez_control_t *self, qbool focusable)
{
	SET_FLAG(self->ext_flags, control_focusable, focusable);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is resizeable horizontally by the user.
//
void EZ_control_SetResizeableHorizontally(ez_control_t *self, qbool resize_horizontally)
{
	SET_FLAG(self->ext_flags, control_resize_h, resize_horizontally);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is resizeable both horizontally and vertically by the user.
//
void EZ_control_SetResizeableBoth(ez_control_t *self, qbool resize)
{
	SET_FLAG(self->ext_flags, (control_resize_h | control_resize_v), resize);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is resizeable at all, not just by the user.
//
void EZ_control_SetResizeable(ez_control_t *self, qbool resizeable)
{
	// TODO : Is it confusing having this resizeable?
	SET_FLAG(self->ext_flags, control_resizeable, resizeable);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is resizeable vertically by the user.
//
void EZ_control_SetResizeableVertically(ez_control_t *self, qbool resize_vertically)
{
	SET_FLAG(self->ext_flags, control_resize_v, resize_vertically);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is visible or not.
//
void EZ_control_SetVisible(ez_control_t *self, qbool visible)
{
	SET_FLAG(self->ext_flags, control_visible, visible);

	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnVisibilityChanged);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control is scrollable or not.
//
void EZ_control_SetScrollable(ez_control_t *self, qbool scrollable)
{
	SET_FLAG(self->ext_flags, control_scrollable, scrollable);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets if the control should move it's parent when it moves itself.
//
void EZ_control_SetMovesParent(ez_control_t *self, qbool moves_parent)
{
	SET_FLAG(self->ext_flags, control_move_parent, moves_parent);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets whetever the control should care about mouse input or not.
//
void EZ_control_SetIgnoreMouse(ez_control_t *self, qbool ignore_mouse)
{
	SET_FLAG(self->ext_flags, control_ignore_mouse, ignore_mouse);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Listen to repeated mouse click events when holding down a mouse button. 
//           The delay between events is set using EZ_control_SetRepeatMouseClickDelay(...)
//
void EZ_control_SetListenToRepeatedMouseClicks(ez_control_t *self, qbool listen_repeat)
{
	SET_FLAG(self->ext_flags, control_listen_repeat_mouse, listen_repeat);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets the amount of time to wait between each new mouse click event
//           when holding down the mouse over a control.
//
void EZ_control_SetRepeatMouseClickDelay(ez_control_t *self, double delay)
{
	self->mouse_repeat_delay = delay;
}

//
// Control - Sets the external flags of the control.
//
void EZ_control_SetFlags(ez_control_t *self, ez_control_flags_t flags)
{
	self->ext_flags = flags;
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnFlagsChanged);
}

//
// Control - Sets the external flags of the control.
//
ez_control_flags_t EZ_control_GetFlags(ez_control_t *self)
{
	return self->ext_flags;
}

//
// Control - Gets the anchor flags.
//
ez_anchor_t EZ_control_GetAnchor(ez_control_t *self)
{
	return self->anchor_flags;
}

//
// Control - Sets the tab order of a control.
//
void EZ_control_SetTabOrder(ez_control_t *self, int tab_order)
{
	self->tab_order = tab_order;
	EZ_tree_OrderTabList(self->control_tree);
}

//
// Control - Updates the anchor gaps between the controls edges and it's parent 
//           (the distance to maintain from the parent when anchored to opposit edges).
//
static void EZ_control_UpdateAnchorGap(ez_control_t *self)
{
	if (!(self->int_flags & control_update_anchorgap))
	{
		return;
	}
	else if (self->parent)
	{
		qbool anchor_viewport	= self->ext_flags & control_anchor_viewport;
		int p_height			= anchor_viewport ? self->parent->height : self->parent->virtual_height;
		int p_width				= anchor_viewport ? self->parent->width  : self->parent->virtual_width;
		
		// Anchored to both top and bottom edges.
		if ((self->anchor_flags & (anchor_bottom | anchor_top)) == (anchor_bottom | anchor_top))
		{
			self->top_edge_gap		= self->y;
			self->bottom_edge_gap	= p_height - (self->y + self->height);
		}

		// Anchored to both left and right edges.
		if ((self->anchor_flags & (anchor_left | anchor_right)) == (anchor_left | anchor_right))
		{
			self->left_edge_gap		= self->x;
			self->right_edge_gap	= p_width - (self->x + self->width);
		}
	}
}

//
// Control - Sets the anchoring of the control to it's parent.
//
void EZ_control_SetAnchor(ez_control_t *self, ez_anchor_t anchor_flags)
{
	self->anchor_flags = anchor_flags;

	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnAnchorChanged);
}

//
// Control - Sets the tab order of a control.
//
void EZ_control_SetDrawOrder(ez_control_t *self, int draw_order, qbool update_children)
{
	// TODO : Is this the best way to change the draw order for the children?
	if (update_children && self->children.count > 0)
	{
		ez_dllist_node_t *it	= self->children.head;
		ez_control_t *child		= (ez_control_t *)it->payload;
		int draw_order_delta	= draw_order - self->draw_order;

		while (it)
		{
			child = (ez_control_t *)it->payload;
			EZ_control_SetDrawOrder(child, (draw_order + draw_order_delta), update_children);
			it = it->next;
		}
	}

	self->draw_order = draw_order;

	// TODO : Force teh user to do this explicitly? Because it will be run a lot of times if there's many children. 
	EZ_tree_OrderDrawList(self->control_tree);
}

//
// Control - Sets the size of a control.
//
void EZ_control_SetSize(ez_control_t *self, int width, int height)
{
	self->prev_width = self->width;
	self->prev_height = self->height;

	self->width  = width;
	self->height = height;

	clamp(self->height, self->height_min, self->height_max);
	clamp(self->width, self->width_min, self->width_max);

	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnResize);
}

//
// Control - Set the thickness of the resize handles (if any).
//
void EZ_control_SetResizeHandleThickness(ez_control_t *self, int thickness)
{
	self->resize_handle_thickness = thickness;
}

//
// Control - Set the max size for the control.
//
void EZ_control_SetMaxSize(ez_control_t *self, int max_width, int max_height)
{
	self->width_max = max(0, max_width);
	self->height_max = max(0, max_height);

	// Do we need to change the size of the control to fit?
	if ((self->width > self->width_max) || (self->height > self->height_max))
	{
		EZ_control_SetSize(self, min(self->width, self->width_max), min(self->height, self->height_max));
	}
}

//
// Control - Set the min size for the control.
//
void EZ_control_SetMinSize(ez_control_t *self, int min_width, int min_height)
{
	self->width_max = max(0, min_width);
	self->height_max = max(0, min_height);

	// Do we need to change the size of the control to fit?
	if ((self->width < self->width_min) || (self->height < self->height_min))
	{
		EZ_control_SetSize(self, max(self->width, self->width_min), max(self->height, self->height_min));
	}
}

//
// Control - Set color of a control.
//
void EZ_control_SetBackgroundColor(ez_control_t *self, byte r, byte g, byte b, byte alpha)
{
	self->background_color[0] = r;
	self->background_color[1] = g;
	self->background_color[2] = b;
	self->background_color[3] = alpha;
}

//
// Control - Sets the position of a control, relative to it's parent.
//
void EZ_control_SetPosition(ez_control_t *self, int x, int y)
{
	// Set the new relative position.
	self->x = x;
	self->y = y;

	// Raise the event that we have moved.
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnMove);
}

//
// Control - Sets the part of the control that should be shown if it's scrollable.
//
void EZ_control_SetScrollPosition(ez_control_t *self, int scroll_x, int scroll_y)
{
	// Only scroll scrollable controls.
	if (!(self->ext_flags & control_scrollable))
	{
		return;
	}

	// Don't allow scrolling outside the scroll area.
	if ((scroll_x > (self->virtual_width - self->width)) 
	 || (scroll_y > (self->virtual_height - self->height))
	 || (scroll_x < 0)
	 || (scroll_y < 0))
	{
		return;
	}

	self->virtual_x = scroll_x;
	self->virtual_y = scroll_y;
 
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnScroll);
}

//
// Control - Convenient function for changing the scroll position by a specified amount.
//
void EZ_control_SetScrollChange(ez_control_t *self, int delta_scroll_x, int delta_scroll_y)
{
	EZ_control_SetScrollPosition(self, (self->virtual_x + delta_scroll_x), (self->virtual_y + delta_scroll_y));
}

//
// Control - Sets the virtual size of the control (this area can be scrolled around in).
//
void EZ_control_SetVirtualSize(ez_control_t *self, int virtual_width, int virtual_height)
{
	self->prev_virtual_width = self->virtual_width;
	self->prev_virtual_height = self->virtual_height;

	// Set the new virtual size of the control (cannot be smaller than the current size of the control).
	self->virtual_width = max(virtual_width, self->virtual_width_min);
	self->virtual_height = max(virtual_height, self->virtual_height_min);

	self->virtual_width = max(self->virtual_width, self->width + self->virtual_x);
	self->virtual_height = max(self->virtual_height, self->height + self->virtual_y);

	// Raise the event that we have changed the virtual size of the control.
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnVirtualResize);
}

//
// Control - Set the min virtual size for the control, the control size is not allowed to be larger than this.
//
void EZ_control_SetMinVirtualSize(ez_control_t *self, int min_virtual_width, int min_virtual_height)
{
	self->virtual_width_min = max(0, min_virtual_width);
	self->virtual_height_min = max(0, min_virtual_height);

	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnMinVirtualResize);
}

//
// Control - Returns true if this control is the root control.
//
qbool EZ_control_IsRoot(ez_control_t *self)
{
	return (self->parent == NULL);
}

//
// Control - Adds a child to the control.
//
void EZ_control_AddChild(ez_control_t *self, ez_control_t *child)
{
	// Remove the control from it's current parent.
	if (child->parent)
	{
		EZ_double_linked_list_RemoveByPayload(&child->parent->children, child);
	}
 
	// Set the new parent of the child.
	child->parent = self;

	// Add the child to the new parents list of children.
	EZ_double_linked_list_Add(&self->children, child);
}

//
// Control - Remove a child from the control. Returns a reference to the child that was removed.
//
ez_control_t *EZ_control_RemoveChild(ez_control_t *self, ez_control_t *child)
{
	return (ez_control_t *)EZ_double_linked_list_RemoveByPayload(&self->children, child);
}

//
// Control - The anchoring for the control changed.
//
int EZ_control_OnAnchorChanged(ez_control_t *self)
{
	// Calculate the gaps to the parents edges (used for anchoring, see OnParentResize for detailed explination).
	EZ_control_UpdateAnchorGap(self);

	// Make sure the control is repositioned correctly.
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnMove);
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnParentResize);

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnAnchorChanged);
	return 0;
}

//
// Control - The control got focus.
//
int EZ_control_OnGotFocus(ez_control_t *self)
{
	if(!(self->int_flags & control_focused))
	{
		return 0;
	}

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnGotFocus);

	return 0;
}

//
// Control - The control lost focus.
//
int EZ_control_OnLostFocus(ez_control_t *self)
{
	if (self->int_flags & control_focused)
	{
		return 0;
	}

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnLostFocus);

	return 0;
}

//
// Control - The control was resized.
//
int EZ_control_OnResize(ez_control_t *self)
{
	// Calculate how much the width has changed so that we can
	// compensate by scrolling.
	int scroll_change_x = (self->prev_width - self->width);
	int scroll_change_y = (self->prev_height - self->height);

	ez_control_t *payload = NULL;
	ez_dllist_node_t *iter = self->children.head;

	// Calculate the gaps to the parents edges (used for anchoring, see OnParentResize for detailed explination).
	EZ_control_UpdateAnchorGap(self);

	// Make sure the virtual size never is smaller than the normal size of the control
	// and that it's never smaller than the virtual max size.
	EZ_control_SetVirtualSize(self, self->width, self->height);

	// First scroll the window so that we use the current virtual space fully
	// before expanding it. That is, only start expanding when the:
	// REAL_SIZE > VIRTUAL_MIN_SIZE
	if (scroll_change_x < 0)
	{
		EZ_control_SetScrollChange(self, scroll_change_x, 0);
	}

	if (scroll_change_y < 0)
	{
		EZ_control_SetScrollChange(self, 0, scroll_change_y);
	}

	// Tell the children we've resized.
	while(iter)
	{
		payload = (ez_control_t *)iter->payload;
		CONTROL_RAISE_EVENT(NULL, payload, ez_control_t, OnParentResize);
		iter = iter->next;
	}

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnResize);

	return 0;
}

//
// Control - The controls parent was resized.
//
int EZ_control_OnParentResize(ez_control_t *self)
{
	if (self->parent && (self->ext_flags & control_resizeable))
	{
		ez_control_t *p			= self->parent;
		int x					= self->x;
		int y					= self->y;
		int new_width			= self->width;
		int new_height			= self->height;
		qbool anchor_viewport	= (self->ext_flags & control_anchor_viewport);
		int parent_prev_width	= anchor_viewport ? self->parent->prev_width	: self->parent->prev_virtual_width;
		int parent_prev_height	= anchor_viewport ? self->parent->prev_height	: self->parent->prev_virtual_height;
		int parent_width		= anchor_viewport ? self->parent->width			: self->parent->virtual_width; 
		int parent_height		= anchor_viewport ? self->parent->height		: self->parent->virtual_height;

		if ((self->anchor_flags & (anchor_left | anchor_right)) == (anchor_left | anchor_right))
		{			
			// Set the new width so that the right side of the control is
			// still the same distance from the right side of the parent.
			new_width = parent_width - (x + self->right_edge_gap);
		}

		if ((self->anchor_flags & (anchor_top | anchor_bottom)) == (anchor_top | anchor_bottom))
		{
			new_height = parent_height - (y + self->bottom_edge_gap);
		}

		// Set the new size if it changed.
		if ((self->width != new_width) || (self->height != new_height))
		{
			// When the control is anchored to two opposit edges we want to stretch it
			// when it's parent is resized. If the parent becomes either
			// smaller or larger than the controls max/min size when doing this, it will
			// make us stop resizing the child. And since we want the child to

			// We need special behaviour when resizing a control that is anchored to two
			// opposit edges of it's parent (it should be stretched). A child control is placed
			// inside of a parent, and it's given a size and anchoring points. When resizing the
			// parent, we want to maintain the distance between the parents edges and the childs edges,
			// that is, if the childs right edge was 10 pixels from it's parents right edge before
			// we resized the parent, we want it to be the same afterwards also. We achieve this by
			// resizing the child (stretching it) to maintain the same gap. 
			//
			// This all works fine if the child is allowed to have any size (even negative), since it will
			// always maintain the same gap between the edges. The problem arises when you introduce the 
			// possibility for a control to have a min/max size, in this case we need to stop updating 
			// the gap when the min/max size has been reached, and remember the last used gap size until
			// the parent goes back to a size that let's its child keep the correct edge gap without 
			// violating its size bounds again. 
			//
			// We do this by only updating the gap size either when setting new anchoring points,
			// or when someone explicitly changes the size of the child control. Therefore any resize of the child
			// control that is triggered by its parent being resized doesn't produce a new gap size.
			self->int_flags &= ~control_update_anchorgap;
			
			EZ_control_SetSize(self, new_width, new_height);

			self->int_flags |= control_update_anchorgap;
		}
	}

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnParentResize);

	return 0;
}

//
// Control - The minimum virtual size has changed for the control.
//
int EZ_control_OnMinVirtualResize(ez_control_t *self)
{
	self->virtual_width		= max(self->virtual_width_min, self->virtual_width);
	self->virtual_height	= max(self->virtual_height_min, self->virtual_height);

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnMinVirtualResize);

	return 0;
}

//
// Control - Hide a control if its parent is hidden. We don't unset control_visible for this
//			because we want to let any child inside of a control to decide if it should be
//			visible or not on it's own, if we just set control_visible here we'd overwrite
//			the childs visibility.
//
static void EZ_control_SetHiddenByParent(ez_control_t *self, qbool hidden)
{
	ez_dllist_node_t *iter	= self->children.head;
	ez_control_t *child		= NULL;

	SET_FLAG(self->int_flags, control_hidden_by_parent, hidden);

	// Show or hide our children also.
	while(iter)
	{
		child = (ez_control_t *)iter->payload;
		EZ_control_SetHiddenByParent(child, hidden);
		iter = iter->next;
	}
}

//
// Control - Visibility changed.
//
int EZ_control_OnVisibilityChanged(ez_control_t *self)
{
	// Hide any children.
	EZ_control_SetHiddenByParent(self, !(self->ext_flags & control_visible));

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnVisibilityChanged);
	return 0;
}

//
// Label - The flags for the control changed.
//
int EZ_control_OnFlagsChanged(ez_control_t *self)
{
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnFlagsChanged);
	return 0;
}

//
// Control - OnResizeHandleThicknessChanged event.
//
int EZ_control_OnResizeHandleThicknessChanged(ez_control_t *self)
{
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnResizeHandleThicknessChanged);
	return 0;
}

//
// Label - The virtual size of the control has changed.
//
int EZ_control_OnVirtualResize(ez_control_t *self)
{
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnVirtualResize);

	return 0;
}

//
// Control - The control was moved.
//
int EZ_control_OnMove(ez_control_t *self)
{	
	ez_control_t *child		= NULL;
	ez_dllist_node_t *iter	= self->children.head;
	qbool anchor_viewport	= (self->ext_flags & control_anchor_viewport);
	int parent_x			= 0;
	int parent_y			= 0;

	self->prev_absolute_x = self->absolute_x;
	self->prev_absolute_y = self->absolute_y;

	// Get the position of the area we're anchoring to. Normal behaviour is to anchor to the virtual area.
	if (self->parent)
	{
		parent_x = anchor_viewport ? self->parent->absolute_x : self->parent->absolute_virtual_x;
		parent_y = anchor_viewport ? self->parent->absolute_y : self->parent->absolute_virtual_y;
	}

	// Update the absolute screen position based on the parents position.
	self->absolute_x = (self->parent ? parent_x : 0) + self->x;
	self->absolute_y = (self->parent ? parent_y : 0) + self->y;

	if (self->parent)
	{
		// If the control has a parent, position it in relation to it
		// and the way it's anchored to it.
		int parent_prev_width	= anchor_viewport ? self->parent->prev_width	: self->parent->prev_virtual_width;
		int parent_prev_height	= anchor_viewport ? self->parent->prev_height	: self->parent->prev_virtual_height;
		int parent_width		= anchor_viewport ? self->parent->width			: self->parent->virtual_width; 
		int parent_height		= anchor_viewport ? self->parent->height		: self->parent->virtual_height;

		// If we're anchored to both the left and right part of the parent we position
		// based on the parents left pos 
		// (We will stretch to the right if the control is resizable and if the parent is resized).
		if ((self->anchor_flags & anchor_left) && !(self->anchor_flags & anchor_right))
		{
			self->absolute_x = parent_x + self->x;
		}
		else if ((self->anchor_flags & anchor_right) && !(self->anchor_flags & anchor_left))
		{
			self->absolute_x = parent_x + parent_width + (self->x - self->width);
		}

		if ((self->anchor_flags & anchor_top) && !(self->anchor_flags & anchor_bottom))
		{
			self->absolute_y = parent_y + self->y;
		}
		else if ((self->anchor_flags & anchor_bottom) && !(self->anchor_flags & anchor_top))
		{
			self->absolute_y = parent_y + parent_height + (self->y - self->height);
		}
	}
	else
	{
		// No parent, position based on screen coordinates.
		self->absolute_x = self->x;
		self->absolute_y = self->y;
	}

	// We need to move the virtual area of the control also.
	CONTROL_RAISE_EVENT(NULL, self, ez_control_t, OnScroll);

	// Tell the children we've moved.
	while(iter)
	{
		child = (ez_control_t *)iter->payload;
		CONTROL_RAISE_EVENT(NULL, child, ez_control_t, OnMove);
		iter = iter->next;
	}

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnMove);

	return 0;
}

//
// Control - On scroll event.
//
int EZ_control_OnScroll(ez_control_t *self)
{
	ez_control_t *child = NULL;
	ez_dllist_node_t *iter = self->children.head;

	self->absolute_virtual_x = self->absolute_x - self->virtual_x;
	self->absolute_virtual_y = self->absolute_y - self->virtual_y;

	// Tell the children we've scrolled. And make them recalculate their
	// absolute position by telling them they've moved.
	while(iter)
	{
		child = (ez_control_t *)iter->payload;
		CONTROL_RAISE_EVENT(NULL, child, ez_control_t, OnMove);
		CONTROL_RAISE_EVENT(NULL, child, ez_control_t, OnParentScroll);
		iter = iter->next;
	}

	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnScroll);

	return 0;
}

//
// Control - On parent scroll event.
//
int EZ_control_OnParentScroll(ez_control_t *self)
{
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnParentScroll);
	return 0;
}

//
// Control - Layouts children.
//
int EZ_control_OnLayoutChildren(ez_control_t *self)
{
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnLayoutChildren);
	return 0;
}

//
// Control - Event for when a new event handler is set for an event.
//
int EZ_control_OnEventHandlerChanged(ez_control_t *self)
{
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnEventHandlerChanged);
	return 0;
}

//
// Control - Draws the control.
//
int EZ_control_OnDraw(ez_control_t *self)
{
	int x, y;
	EZ_control_GetDrawingPosition(self, &x, &y);

	if (self->background_color[3] > 0)
	{
		Draw_AlphaRectangleRGB(self->absolute_x, self->absolute_y, self->width, self->height, 1, true, RGBAVECT_TO_COLOR(self->background_color));
	}

	// TODO : Remove this test stuff.
	/*
	Draw_String(x, y, va("%s%s%s%s",
			((self->int_flags & control_moving) ? "M" : " "),
			((self->int_flags & control_focused) ? "F" : " "),
			((self->int_flags & control_clicked) ? "C" : " "),
			((self->int_flags & control_resizing_left) ? "R" : " ")
			));
	*/

	// Draw control specifics.
	CONTROL_EVENT_HANDLER_CALL(NULL, self, ez_control_t, OnDraw);

	return 0;
}

//
// Control - Key down event.
//
int EZ_control_OnKeyDown(ez_control_t *self, int key, int unichar)
{
	int key_handled = false;

	CONTROL_EVENT_HANDLER_CALL(&key_handled, self, ez_control_t, OnKeyDown, key, unichar);

	return key_handled;
}

//
// Control - Key up event.
//
int EZ_control_OnKeyUp(ez_control_t *self, int key, int unichar)
{
	int key_handled = false;

	CONTROL_EVENT_HANDLER_CALL(&key_handled, self, ez_control_t, OnKeyUp, key, unichar);

	return key_handled;
}

//
// Control - Key event.
//
int EZ_control_OnKeyEvent(ez_control_t *self, int key, int unichar, qbool down)
{
	int key_handled			= false;
	ez_control_t *payload	= NULL;
	ez_dllist_node_t *iter	= self->children.head;

	if (down)
	{
		CONTROL_RAISE_EVENT(&key_handled, self, ez_control_t, OnKeyDown, key, unichar);
	}
	else
	{
		CONTROL_RAISE_EVENT(&key_handled, self, ez_control_t, OnKeyUp, key, unichar);
	}

	if (key_handled)
	{
		return true;
	}

	CONTROL_EVENT_HANDLER_CALL(&key_handled, self, ez_control_t, OnKeyEvent, key, unichar, down);

	return key_handled;
}

typedef enum
{
	RESIZE_LEFT		= (1 << 0),
	RESIZE_RIGHT	= (1 << 1),
	RESIZE_UP		= (1 << 2),
	RESIZE_DOWN		= (1 << 3)
} resize_direction_t;

//
// Control - Resizes the control by moving the left corner.
//
static void EZ_control_ResizeByDirection(ez_control_t *self, mouse_state_t *ms, int delta_x, int delta_y, resize_direction_t direction)
{
	int width				= self->width;
	int height				= self->height;
	int x					= self->x;
	int y					= self->y;
	qbool resizing_width	= (direction & RESIZE_LEFT) || (direction & RESIZE_RIGHT);
	qbool resizing_height	= (direction & RESIZE_UP) || (direction & RESIZE_DOWN);

	if (resizing_width)
	{
		// Set the new width based on how much the mouse has moved
		// keeping it within the allowed bounds.
		width = self->width + ((direction & RESIZE_LEFT) ? delta_x : -delta_x);
		clamp(width, self->width_min, self->width_max);

		// Move the control to counteract the resizing when resizing to the left.
		x = self->x + ((direction & RESIZE_LEFT) ? (self->width - width) : 0);
	}

	if (resizing_height)
	{
		height = self->height + ((direction & RESIZE_UP) ? delta_y : -delta_y);
		clamp(height, self->height_min, self->height_max);
		y = self->y + ((direction & RESIZE_UP) ? (self->height - height) : 0);
	}

	// If the child should be contained within it's parent the
	// mouse should stay within the parent also when resizing the control.
	if (ms && CONTROL_IS_CONTAINED(self))
	{
		if (resizing_width && !POINT_X_IN_CONTROL_DRAWBOUNDS(self->parent, ms->x))
		{
			ms->x = ms->x_old;
			x = self->x;
			width = self->width;
		}

		if (resizing_height && !POINT_Y_IN_CONTROL_DRAWBOUNDS(self->parent, ms->y))
		{
			ms->y = ms->y_old;
			y = self->y;
			height = self->height;
		}
	}

	EZ_control_SetSize(self, width, height);
	EZ_control_SetPosition(self, x, y);
}

//
// Control -
// The initial mouse event is handled by this, and then raises more specialized event handlers
// based on the new mouse state.
// 
// NOTICE! When extending this event you need to make sure that you need to tell the framework
// that you've handled all mouse events that happened within the controls bounds in your own
// implementation by returning true whenever the mouse is inside the control.
// This can easily be done with the following macro: MOUSE_INSIDE_CONTROL(self, ms); 
// If this is not done, all mouse events will "fall through" to controls below.
//
int EZ_control_OnMouseEvent(ez_control_t *self, mouse_state_t *ms)
{
	mouse_state_t *old_ms			= &self->prev_mouse_state;
	qbool mouse_inside				= false;
	qbool prev_mouse_inside			= false;
	qbool mouse_inside_parent		= false;
	qbool prev_mouse_inside_parent	= false;
	qbool is_contained				= CONTROL_IS_CONTAINED(self);
	int mouse_handled_tmp			= false;
	int mouse_handled				= false;
	int mouse_delta_x				= 0;
	int mouse_delta_y				= 0;

	if (!ms)
	{
		Sys_Error("EZ_control_OnMouseEvent(): mouse_state_t NULL\n");
	}

	// Ignore the mouse?
	if (self->ext_flags & control_ignore_mouse)
	{
		return false;
	}

	mouse_delta_x = Q_rint(ms->x_old - ms->x);
	mouse_delta_y = Q_rint(ms->y_old - ms->y);

	mouse_inside		= POINT_IN_CONTROL_DRAWBOUNDS(self, ms->x, ms->y);
	prev_mouse_inside	= POINT_IN_CONTROL_DRAWBOUNDS(self, ms->x_old, ms->y_old);

	// Raise more specific events.
	if (mouse_inside)
	{
		if (!prev_mouse_inside)
		{
			// Were we inside of the control last time? Otherwise we've just entered it.
			CONTROL_RAISE_EVENT(&mouse_handled_tmp, self, ez_control_t, OnMouseEnter, ms);
		}
		else
		{
			// We're hovering the control.
			CONTROL_RAISE_EVENT(&mouse_handled_tmp, self, ez_control_t, OnMouseHover, ms);
		}

		mouse_handled = (mouse_handled || mouse_handled_tmp);

		if (ms->button_down)
		{
			// Mouse down.
			CONTROL_RAISE_EVENT(&mouse_handled_tmp, self, ez_control_t, OnMouseDown, ms);
			mouse_handled = (mouse_handled || mouse_handled_tmp);
		}

		if (ms->button_up)
		{
			// Mouse up.
			CONTROL_RAISE_EVENT(&mouse_handled_tmp, self, ez_control_t, OnMouseUp, ms);
			mouse_handled = (mouse_handled || mouse_handled_tmp);
		}
	}
	else if (prev_mouse_inside)
	{
		// Mouse leave.
		CONTROL_RAISE_EVENT(&mouse_handled_tmp, self, ez_control_t, OnMouseLeave, ms);
		mouse_handled = (mouse_handled || mouse_handled_tmp);
	}

	// Send a mouse up event to controls if the mouse is outside also.
	if (ms->button_up && (ms->button_up != old_ms->button_up))
	{
		// Mouse up.
		CONTROL_RAISE_EVENT(&mouse_handled_tmp, self, ez_control_t, OnMouseUpOutside, ms);
		mouse_handled = (mouse_handled || mouse_handled_tmp);
	}

	// Make sure we remove the click flag always when releasing the button
	// not just when we're hovering above the control.
	if (ms->button_up && (ms->button_up != old_ms->button_up))
	{
		self->int_flags &= ~control_clicked;
	}

	// TODO : Move these to new methods.

	if (!mouse_handled)
	{
		// Check for moving and resizing.
		if ((self->int_flags & control_resizing_left)
		 || (self->int_flags & control_resizing_right)
		 || (self->int_flags & control_resizing_top)
		 || (self->int_flags & control_resizing_bottom))
		{
			// These can be combined to grab the corners for resizing.

			// Resize by width.
			if (self->int_flags & control_resizing_left)
			{
				EZ_control_ResizeByDirection(self, ms, mouse_delta_x, mouse_delta_y, RESIZE_LEFT);
				mouse_handled = true;
			}
			else if (self->int_flags & control_resizing_right)
			{
				EZ_control_ResizeByDirection(self, ms, mouse_delta_x, mouse_delta_y, RESIZE_RIGHT);
				mouse_handled = true;
			}

			// Resize by height.
			if (self->int_flags & control_resizing_top)
			{
				EZ_control_ResizeByDirection(self, ms, mouse_delta_x, mouse_delta_y, RESIZE_UP);
				mouse_handled = true;
			}
			else if (self->int_flags & control_resizing_bottom)
			{
				EZ_control_ResizeByDirection(self, ms, mouse_delta_x, mouse_delta_y, RESIZE_DOWN);
				mouse_handled = true;
			}
		}
		else if (self->int_flags & control_moving)
		{
			// Root control will be moved relative to the screen,
			// others relative to their parent.
			int m_delta_x = Q_rint(ms->x - ms->x_old);
			int m_delta_y = Q_rint(ms->y - ms->y_old);
			int x = self->x + m_delta_x;
			int y = self->y + m_delta_y;

			// Should the control be contained within it's parent?
			// Then don't allow the mouse to move outside the parent
			// while moving the control.
			if (CONTROL_IS_CONTAINED(self))
			{
				if (!POINT_X_IN_CONTROL_DRAWBOUNDS(self->parent, ms->x))
				{
					ms->x = ms->x_old;
					x = self->x;
					mouse_handled = true;
				}

				if (!POINT_Y_IN_CONTROL_DRAWBOUNDS(self->parent, ms->y))
				{
					ms->y = ms->y_old;
					y = self->y;
					mouse_handled = true;
				}
			}

			if (self->parent && (self->ext_flags & control_move_parent))
			{
				// Move the parent instead of just the control, the parent will in turn move the control.
				// TODO : Do we need to keep the mouse inside the parents parent here also?
				EZ_control_SetPosition(self->parent, (self->parent->x + m_delta_x), (self->parent->y + m_delta_y));
			}
			else
			{
				EZ_control_SetPosition(self, x, y);
			}

			mouse_handled = true;
		}
	}

	// Let any event handler run if the mouse event wasn't handled.
	CONTROL_EVENT_HANDLER_CALL(&mouse_handled_tmp, self, ez_control_t, OnMouseEvent, ms);
	mouse_handled = (mouse_handled || mouse_handled_tmp);

	// Save the mouse state for the next time we check.
	self->prev_mouse_state = *ms;

	return mouse_handled;
}

//
// Control - The mouse was pressed and then released within the bounds of the control.
//
int EZ_control_OnMouseClick(ez_control_t *self, mouse_state_t *mouse_state)
{
	int mouse_handled = false;
	CONTROL_EVENT_HANDLER_CALL(&mouse_handled, self, ez_control_t, OnMouseClick, mouse_state);
	return mouse_handled;
}

//
// Control - The mouse entered the controls bounds.
//
int EZ_control_OnMouseEnter(ez_control_t *self, mouse_state_t *mouse_state)
{
	int mouse_handled = false;
	self->int_flags |= control_mouse_over;
	CONTROL_EVENT_HANDLER_CALL(&mouse_handled, self, ez_control_t, OnMouseEnter, mouse_state);
	return mouse_handled;
}

//
// Control - The mouse left the controls bounds.
//
int EZ_control_OnMouseLeave(ez_control_t *self, mouse_state_t *mouse_state)
{
	int mouse_handled = false;

	// Stop moving since the mouse is outside the control.
	self->int_flags &= ~(/*control_moving |*/ control_mouse_over);

	CONTROL_EVENT_HANDLER_CALL(&mouse_handled, self, ez_control_t, OnMouseLeave, mouse_state);
	return mouse_handled;
}

//
// Control - A mouse button was released either inside or outside of the control.
//
int EZ_control_OnMouseUpOutside(ez_control_t *self, mouse_state_t *ms)
{
	int mouse_handled_tmp	= false;
	int mouse_handled		= false;

	// Stop moving / resizing.
	self->int_flags &= ~(control_moving | control_resizing_left | control_resizing_right | control_resizing_top | control_resizing_bottom);

	// Call event handler.
	CONTROL_EVENT_HANDLER_CALL(&mouse_handled_tmp, self, ez_control_t, OnMouseUpOutside, ms);
	mouse_handled = (mouse_handled || mouse_handled_tmp);

	return mouse_handled;
}

//
// Control - A mouse button was released within the bounds of the control.
//
int EZ_control_OnMouseUp(ez_control_t *self, mouse_state_t *mouse_state)
{
	int mouse_handled_tmp	= false;
	int mouse_handled		= false;

	// Stop moving / resizing.
	self->int_flags &= ~(control_moving | control_resizing_left | control_resizing_right | control_resizing_top | control_resizing_bottom);

	// Raise a click event.
	if (self->int_flags & control_clicked)
	{
		self->int_flags &= ~control_clicked;
		CONTROL_RAISE_EVENT(&mouse_handled_tmp, self, ez_control_t, OnMouseClick, mouse_state);
		mouse_handled = (mouse_handled || mouse_handled_tmp);
	}

	// Call event handler.
	CONTROL_EVENT_HANDLER_CALL(&mouse_handled_tmp, self, ez_control_t, OnMouseUp, mouse_state);
	mouse_handled = (mouse_handled || mouse_handled_tmp);

	return mouse_handled;
}

//
// Control - A mouse button was pressed within the bounds of the control.
//
int EZ_control_OnMouseDown(ez_control_t *self, mouse_state_t *ms)
{
	int mouse_handled_tmp	= false;
	int mouse_handled		= false;

	if (!(self->ext_flags & control_enabled))
	{
		return false;
	}

	// Make sure the current control is focused.
	if (self->control_tree->focused_node)
	{
		// If the focused node isn't this one, refocus.
		if (self->control_tree->focused_node->payload != self)
		{
			mouse_handled = EZ_control_SetFocus(self);
		}
	}
	else
	{
		// If there's no focused node, focus on this one.
		mouse_handled = EZ_control_SetFocus(self);
	}

	if (self->int_flags & control_focused)
	{
		// Check if the mouse is at the edges of the control, and turn on the correct reszie mode if it is.
		if (((self->ext_flags & control_resize_h) || (self->ext_flags & control_resize_v)) && (ms->button_down == 1))
		{
			if (self->ext_flags & control_resize_h)
			{
				// Left side of the control.
				if (POINT_IN_RECTANGLE(ms->x, ms->y,
					self->absolute_x, self->absolute_y,
					self->resize_handle_thickness, self->height))
				{
					self->int_flags |= control_resizing_left;
					mouse_handled = true;
				}

				// Right side of the control.
				if (POINT_IN_RECTANGLE(ms->x, ms->y,
					self->absolute_x + self->width - self->resize_handle_thickness, self->absolute_y,
					self->resize_handle_thickness, self->height))
				{
					self->int_flags |= control_resizing_right;
					mouse_handled = true;
				}
			}

			if (self->ext_flags & control_resize_v)
			{
				// Top of the control.
				if (POINT_IN_RECTANGLE(ms->x, ms->y,
					self->absolute_x, self->absolute_y,
					self->width, self->resize_handle_thickness))
				{
					self->int_flags |= control_resizing_top;
					mouse_handled = true;
				}

				// Bottom of the control.
				if (POINT_IN_RECTANGLE(ms->x, ms->y,
					self->absolute_x, self->absolute_y + self->height - self->resize_handle_thickness,
					self->width, self->resize_handle_thickness))
				{
					self->int_flags |= control_resizing_bottom;
					mouse_handled = true;
				}
			}
		}

		// The control is being moved.
		if ((self->ext_flags & control_movable) && (ms->button_down == 1))
		{
			self->int_flags |= control_moving;
			mouse_handled = true;
		}

		if (ms->button_down)
		{
			self->int_flags |= control_clicked;
			mouse_handled = true;
		}
	}

	/*
	TODO : Fix OnMouseClick
	if(!mouse_handled)
	{
		CONTROL_EVENT_HANDLER_CALL(&mouse_handled, self, OnMouseClick, mouse_state);
	}
	*/

	CONTROL_EVENT_HANDLER_CALL(&mouse_handled_tmp, self, ez_control_t, OnMouseDown, ms);
	mouse_handled = (mouse_handled || mouse_handled_tmp);

	return mouse_handled;
}

/*
// TODO : Add support for mouse wheel.
//
// Control - The mouse wheel was triggered within the bounds of the control.
//
int EZ_control_OnMouseWheel(ez_control_t *self, mouse_state_t *mouse_state)
{
	int wheel_handled = false;
	CONTROL_EVENT_HANDLER_CALL(&wheel_handled, self, ez_control_t, OnMouseWheel, mouse_state);
	return wheel_handled;
}
*/

//
// Control - The mouse is hovering within the bounds of the control.
//
int EZ_control_OnMouseHover(ez_control_t *self, mouse_state_t *mouse_state)
{
	int mouse_handled = false;

	if (!mouse_handled)
	{
		CONTROL_EVENT_HANDLER_CALL(&mouse_handled, self, ez_control_t, OnMouseHover, mouse_state);
	}

	return mouse_handled;
}

// TODO : Add a checkbox control.
// TODO : Add a radiobox control.
// TODO : Add a window control.


