/*********************************************************************
*
* Copyright (C) 2004,2008,  Simon Kagstrom
*
* Filename:      menu.c
* Author:        Simon Kagstrom <simon.kagstrom@gmail.com>
* Description:   Code for menus (originally for Mophun)
*
* $Id$
*
********************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "menu.hh"
#include "utils.hh"
#include "menutexts.h"

#define IS_SUBMENU(p_msg) ( (p_msg)[0] == '^' )
#define IS_TEXT(p_msg) ( (p_msg)[0] == '#' || (p_msg)[0] == ' ' )
#define IS_MARKER(p_msg) ( (p_msg)[0] == '@' )


static submenu_t *find_submenu(menu_t *p_menu, int index)
{
	int i;

	for (i=0; i<p_menu->n_submenus; i++)
	{
		if (p_menu->p_submenus[i].index == index)
			return &p_menu->p_submenus[i];
	}

	return NULL;
}

void menu_print_font(SDL_Surface *screen, int r, int g, int b,
		int x, int y, const char *msg)
{
#define _MAX_STRING 64
	SDL_Surface *font_surf;
	SDL_Rect dst = {x, y,  0, 0};
	SDL_Color color = {r, g, b};
	char buf[255];
	unsigned int i;

	memset(buf, 0, sizeof(buf));
	strncpy(buf, msg, 254);
	if (buf[0] != '|' && buf[0] != '^' && buf[0] != '.'
		&& buf[0] != '-' && buf[0] != ' ' && !strstr(buf, "  \""))
	{
		if (strlen(buf)>_MAX_STRING)
		{
			buf[_MAX_STRING-3] = '.';
			buf[_MAX_STRING-2] = '.';
			buf[_MAX_STRING-1] = '.';
			buf[_MAX_STRING] = '\0';
		}
	}
	/* Fixup multi-menu option look */
	for (i = 0; i < strlen(buf) ; i++)
	{
		if (buf[i] == '^' || buf[i] == '|')
			buf[i] = ' ';
	}

	font_surf = TTF_RenderText_Blended(menu_font, buf,
			color);
	if (!font_surf)
	{
		fprintf(stderr, "%s\n", TTF_GetError());
		exit(1);
	}

	SDL_BlitSurface(font_surf, NULL, screen, &dst);
	SDL_FreeSurface(font_surf);
}


static void menu_draw(SDL_Surface *screen, menu_t *p_menu, int sel)
{
	int font_height = TTF_FontHeight(p_menu->p_font);
	int line_height = (font_height + font_height / 4);
	int x_start = p_menu->x1;
	int y_start = p_menu->y1 + line_height;
	SDL_Rect r;
	int entries_visible = (p_menu->y2 - p_menu->y1) / line_height - 2;

	int i, y;
	char pTemp[256];

	if ( p_menu->n_entries * line_height > p_menu->y2 )
		y_start = p_menu->y1 + line_height;

	if (p_menu->cur_sel - p_menu->start_entry_visible > entries_visible)
	{
		while (p_menu->cur_sel - p_menu->start_entry_visible > entries_visible)
		{
			p_menu->start_entry_visible ++;
			if (p_menu->start_entry_visible > p_menu->n_entries)
			{
				p_menu->start_entry_visible = 0;
				break;
			}
		}
	}
	else if ( p_menu->cur_sel < p_menu->start_entry_visible )
		p_menu->start_entry_visible = p_menu->cur_sel;

	if (strlen(p_menu->title))
	{
		r.x = p_menu->x1;
		r.y = p_menu->y1;
		r.w = p_menu->x2 - p_menu->x1;
		r.h = line_height-1;
		if (sel < 0)
			SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0x40, 0x00, 0x00));
		else
			SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0x00, 0x00, 0xff));
		menu_print_font(screen, 0,0,0, p_menu->x1, p_menu->y1, p_menu->title);
	}

	for (i = p_menu->start_entry_visible; i <= p_menu->start_entry_visible + entries_visible; i++)
	{
		const char *msg = p_menu->pp_msgs[i];

		if (i >= p_menu->n_entries)
			break;
		if (IS_MARKER(msg))
			p_menu->cur_sel = atoi(&msg[1]);
		else
		{
			y = (i - p_menu->start_entry_visible) * line_height;

			if (sel < 0)
				menu_print_font(screen, 0x40,0x40,0x40,
						x_start, y_start + y, msg);
			else if (p_menu->cur_sel == i) /* Selected - color */
				menu_print_font(screen, 0,255,0,
						x_start, y_start + y, msg);
			else if (IS_SUBMENU(msg))
			{
				if (p_menu->cur_sel == i-1)
					menu_print_font(screen, 0x80,0xff,0x80,
							x_start, y_start + y, msg);
				else
					menu_print_font(screen, 0x40,0x40,0x40,
							x_start, y_start + y, msg);
			}
			else if (msg[0] == '#')
			{
				switch (msg[1])
				{
				case '1':
					menu_print_font(screen, 0,0,255,
							x_start, y_start + y, msg+2);
					break;
				case '2':
					menu_print_font(screen, 0x80,0x80,0x80,
							x_start, y_start + y, msg+2);
					break;
				default:
					menu_print_font(screen, 0x40,0x40,0x40,
							x_start, y_start + y, msg);
					break;							
				}
			}
			else /* Otherwise white */
				menu_print_font(screen, 0x40,0x40,0x40,
						x_start, y_start + y, msg);
			if (IS_SUBMENU(msg))
			{
				submenu_t *p_submenu = find_submenu(p_menu, i);
				int n_pipe = 0;
				int n;

				for (n=0; msg[n] != '\0'; n++)
				{
					/* Underline the selected entry */
					if (msg[n] == '|')
					{
						int16_t n_chars;

						for (n_chars = 1; msg[n+n_chars] && msg[n+n_chars] != '|'; n_chars++);

						n_pipe++;
						if (p_submenu->sel == n_pipe-1)
						{
							int w;
							int h;

							if (TTF_SizeText(p_menu->p_font, "X", &w, &h) < 0)
							{
								fw = w;
								fh = h;
								fprintf(stderr, "%s\n", TTF_GetError());
								exit(1);
							}

							r = (SDL_Rect){ x_start + (n+1) * w-1, y_start + (i+ 1 - p_menu->start_entry_visible) * ((h + h/4)) -3, (n_chars - 1) * w, 2};
							if (p_menu->cur_sel == i-1)
								SDL_FillRect(screen, &r,
										SDL_MapRGB(screen->format, 0x0,0xff,0x80));
							else
								SDL_FillRect(screen, &r,
										SDL_MapRGB(screen->format, 0x40,0x40,0x40));
							break;
						}
					}
				}
			}
		}
	}
}

static int get_next_seq_y(menu_t *p_menu, int v, int dy)
{
	if (v + dy < 0)
		return p_menu->n_entries - 1;
	if (v + dy > p_menu->n_entries - 1)
		return 0;
	return v + dy;
}

static void select_next(menu_t *p_menu, int dx, int dy)
{
	int next;
	char buffer[256];

	p_menu->cur_sel = get_next_seq_y(p_menu, p_menu->cur_sel, dy);
	next = get_next_seq_y(p_menu, p_menu->cur_sel, dy + 1);

	if (p_menu->pp_msgs[p_menu->cur_sel][0] == ' ' ||
			p_menu->pp_msgs[p_menu->cur_sel][0] == '#' ||
			IS_SUBMENU(p_menu->pp_msgs[p_menu->cur_sel]) )
		select_next(p_menu, dx, dy);

	/* If the next is a submenu */
	if (dx != 0 && IS_SUBMENU(p_menu->pp_msgs[next]))
	{
		submenu_t *p_submenu = find_submenu(p_menu, next);

		p_submenu->sel = (p_submenu->sel + dx) < 0 ? p_submenu->n_entries - 1 :
		(p_submenu->sel + dx) % p_submenu->n_entries;
	}
	else if (dx == -1 && !strcmp(p_menu->pp_msgs[0], "[..]"))
		p_menu->cur_sel = 0;
}

static void select_one(menu_t *p_menu, int sel)
{
	if (sel >= p_menu->n_entries)
		sel = 0;
	p_menu->cur_sel = sel;
	if (p_menu->pp_msgs[p_menu->cur_sel][0] == ' ' ||
			p_menu->pp_msgs[p_menu->cur_sel][0] == '#' ||
			IS_SUBMENU(p_menu->pp_msgs[p_menu->cur_sel]))
		select_next(p_menu, 0, 1);
}

static int is_submenu_title(menu_t *p_menu, int n)
{
	if (n+1 >= p_menu->n_entries)
		return 0;
	else
		return IS_SUBMENU(p_menu->pp_msgs[n+1]);
}


uint32_t menu_wait_key_press(void)
{
	SDL_Event ev;
	uint32_t keys = 0;

	while (1)
	{
		if (SDL_PollEvent(&ev))
		{
			switch(ev.type)
			{
			case SDL_KEYDOWN:
				switch (ev.key.keysym.sym)
				{
				case SDLK_UP:
					keys |= KEY_UP;
					break;
				case SDLK_DOWN:
					keys |= KEY_DOWN;
					break;
				case SDLK_LEFT:
					keys |= KEY_LEFT;
					break;
				case SDLK_RIGHT:
					keys |= KEY_RIGHT;
					break;
				case SDLK_PAGEDOWN:
					keys |= KEY_PAGEDOWN;
					break;
				case SDLK_PAGEUP:
					keys |= KEY_PAGEUP;
					break;
				case SDLK_RETURN:
				case SDLK_SPACE:
					keys |= KEY_SELECT;
					break;
				case SDLK_HOME:
				case SDLK_ESCAPE:
					keys |= KEY_ESCAPE;
					break;
				default:
					break;
				}
				break;
				case SDL_QUIT:
					exit(0);
					break;
				default:
					break;

			}
			break;
		}
		if (keys != 0)
			return keys;
		SDL_Delay(100);
	}
	return keys;
}


static int menu_select_internal(SDL_Surface *screen,
		menu_t *p_menu, int *p_submenus, int sel,
		void (*select_next_cb)(menu_t *p, void *data) = NULL,
		void *select_next_cb_data = NULL)
{
	int ret = -1;

	for (int i = 0; i < p_menu->n_submenus; i++)
		p_menu->p_submenus[i].sel = p_submenus[i];

	while(1)
	{
		SDL_Rect r = {p_menu->x1, p_menu->y1,
				p_menu->x2 - p_menu->x1, p_menu->y2 - p_menu->y1};
		uint32_t keys;
		int sel_last = p_menu->cur_sel;

		SDL_FillRect(screen, &r, SDL_MapRGB(screen->format, 0x00, 0x80, 0x80));

		menu_draw(screen, p_menu, 0);
		SDL_Flip(screen);

		keys = menu_wait_key_press();

		if (keys & KEY_UP)
			select_next(p_menu, 0, -1);
		else if (keys & KEY_DOWN)
			select_next(p_menu, 0, 1);
		else if (keys & KEY_PAGEUP)
			select_next(p_menu, 0, -6);
		else if (keys & KEY_PAGEDOWN)
			select_next(p_menu, 0, 6);
		else if (keys & KEY_LEFT)
			select_next(p_menu, -1, 0);
		else if (keys & KEY_RIGHT)
			select_next(p_menu, 1, 0);
		else if (keys & KEY_ESCAPE)
			break;
		else if (keys & KEY_SELECT)
		{
			ret = p_menu->cur_sel;
			int i;

			for (i=0; i<p_menu->n_submenus; i++)
				p_submenus[i] = p_menu->p_submenus[i].sel;
			break;
		}
		/* Invoke the callback when an entry is selected */
		if (sel_last != p_menu->cur_sel &&
				select_next_cb != NULL)
			select_next_cb(p_menu, select_next_cb_data);
	}

	SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));
	return ret;
}

const char *menu_select_file(const char *dir_path)
{
	return menu_select_file_internal(dir_path,
			32, 32, FULL_DISPLAY_X/2, FULL_DISPLAY_Y - 32);
}

int Menu::getNextEntry(int dy)
{
	if (v + dy < 0)
		return this->n_entries - 1;
	if (v + dy > this->n_entries - 1)
		return 0;
	return v + dy;
}

void Menu::selectNext(int dx, int dy)
{
	int next;
	char buffer[256];

	this->cur_sel = this->getNextEntry(dy);
	next = this->getNextEntry(dy + 1);

	/* We want to skip this for some reason */
	if (this->pp_msgs[this->cur_sel][0] == ' ' ||
			this->pp_msgs[this->cur_sel][0] == '#' ||
			IS_SUBMENU(this->pp_msgs[this->cur_sel]) ) {
		this->selectNext(dx, dy);
		return;
	}

	/* If the next is a submenu */
	if (dx != 0 && IS_SUBMENU(this->pp_msgs[next]))
	{
		submenu_t *p_submenu = find_submenu(p_menu, next);

		p_submenu->sel = (p_submenu->sel + dx) < 0 ? p_submenu->n_entries - 1 :
		(p_submenu->sel + dx) % p_submenu->n_entries;
	}
}

void Menu::selectNext(event_t ev)
{
	switch (ev)
	{
	case KEY_UP:
		this->selectNext(0, -1); break;
	case KEY_DOWN:
		this->selectNext(0, 1); break;
	case KEY_LEFT:
		this->selectNext(-1, 0); break;
	case KEY_RIGHT:
		this->selectNext(1, 0); break;
	default:
		panic("selectNext(ev) called with event %d\n", ev);
	}
}

void Menu::runLogic()
{
	event_t ev;

	while ( (ev = this->popEvent()) != EVENT_NONE )
	{
		switch (ev)
		{
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
			this->selectNext(ev);
			break;
		case KEY_SELECT:
			this->selectCallback(this->cur_sel); break;
		case KEY_ESCAPE:
			this->escapeCallback(this->cur_sel); break;
			break;
		case KEY_ENTER:
		default:
			break;
		}
	}
}

event_t Menu::popEvent()
{
	event_t out;

	if (this->ev_head == this->ev_tail)
		return EVENT_NONE;
	out = this->event_stack[this->ev_tail];
	this->ev_tail = (this->ev_tail + 1) % 8;

	return out;
}

void Menu::pushEvent(event_t ev)
{
	/* Push... */
	this->event_stack[this->ev_head] = ev;

	/* ... and update */
	this->ev_head = (this->ev_head + 1) % 8;
	if (this->ev_head == this->ev_tail)
		this->ev_tail = (this->ev_tail + 1) % 8;
}

void Menu::pushEvent(SDL_Event *ev)
{
	switch(ev->type)
	{
	case SDL_KEYDOWN:
		switch (ev->key.keysym.sym)
		{
		case SDLK_UP:
			this->pushEvent(KEY_UP);
			break;
		case SDLK_DOWN:
			this->pushEvent(KEY_DOWN);
			break;
		case SDLK_LEFT:
			this->pushEvent(KEY_LEFT);
			break;
		case SDLK_RIGHT:
			this->pushEvent(KEY_RIGHT);
			break;
		case SDLK_PAGEDOWN:
			this->pushEvent(KEY_PAGEDOWN);
			break;
		case SDLK_PAGEUP:
			this->pushEvent(KEY_PAGEUP);
			break;
		case SDLK_RETURN:
		case SDLK_SPACE:
			this->pushEvent(KEY_SELECT);
			break;
		case SDLK_HOME:
		case SDLK_ESCAPE:
			this->pushEvent(KEY_ESCAPE);
			break;
		default:
			break;
		}
		default:
			break;

	}
}

void Menu::setText(const char *messages)
{
	int submenu;

	/* Free the old stuff */
	this->n_submenus = 0;
	free(this->p_submenus);
	free(this->pp_msgs);

	for (this->n_entries = 0; messages[p_menu->n_entries]; this->n_entries++)
	{
		/* Is this a submenu? */
		if (IS_SUBMENU(messages[this->n_entries]))
		{
			this->n_submenus++;
			continue; /* Length of submenus is unimportant */
		}
	}
	this->pp_msgs = (const char **)malloc(this->n_entries * sizeof(const char *));
	this->p_submenus = (submenu_t *)malloc(this->n_submenus * sizeof(submenu_t));
	for (int i = 0; i < this->n_entries; i++) {
		this->pp_msgs[i] = strdup(messages[i]);
		BUG_ON(!this->pp_msgs[i]);
	}

	BUG_ON(!this->pp_msgs);
	BUG_ON(!this->p_submenus);

	submenu = 0;

	for (int i = 0; i < this->n_entries; i++)
	{
		if (IS_SUBMENU(this->pp_msgs[i]))
		{
			int n;

			this->p_submenus[submenu].index = i;
			this->p_submenus[submenu].sel = 0;
			this->p_submenus[submenu].n_entries = 0;
			for (n = 0; this->pp_msgs[i][n] != '\0'; n++)
			{
				if (this->pp_msgs[i][n] == '|')
					this->p_submenus[submenu].n_entries++;
			}
			submenu++;
		}
	}
}

Menu::Menu(TTF_Font *font, SDL_Color clr, int w, int h)
{
	this->text_color = clr;
	this->font = font;
	this->w = w;
	this->h = h;

	this->pp_msgs = NULL;
	this->n_entries = 0;
	this->p_submenus = NULL;
	this->n_submenus = 0;

	this->cur_sel = 0;
	this->mouse_x = -1;
	this->mouse_y = -1;

	memset(this->event_stack, 0, sizeof(this->event_stack));
	this->ev_head = this->ev_tail = 0;
}

Menu::~Menu()
{
	free(this->pp_msgs);
	free(this->p_submenus);
}
