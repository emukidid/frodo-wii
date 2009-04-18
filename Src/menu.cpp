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

#if defined(GEKKO)
#include <wiiuse/wpad.h>
#endif

#include "sysdeps.h"
#include "Display.h"
#include "menu.h"
#include "menutexts.h"

char kbin[256];

typedef struct
{
	int n_entries;
	int index;
	int sel;
} submenu_t;


typedef struct
{
	char title[256];
	const char **pp_msgs;
	TTF_Font  *p_font;
	int        x1,y1;
	int        x2,y2;
	int        text_w;
	int        text_h;

	int        n_submenus;
	submenu_t *p_submenus;

	int        cur_sel; /* Main selection */
	int        start_entry_visible;
	int        n_entries;
} menu_t;

#define IS_SUBMENU(p_msg) ( (p_msg)[0] == '^' )
#define IS_TEXT(p_msg) ( (p_msg)[0] == '#' || (p_msg)[0] == ' ' )
#define IS_MARKER(p_msg) ( (p_msg)[0] == '@' )

static TTF_Font *menu_font;
static TTF_Font *menu_font64;
#if defined(GEKKO)
#define FONT_PATH "/apps/frodo/FreeMono.ttf"
#define FONT64_PATH "/apps/frodo/C64.ttf"
#else
#define FONT_PATH "FreeMono.ttf"
#define FONT64_PATH "C64.ttf"
#endif

int fh, fw;

const char *welcome[] = {
		/*01*/		"#1             WELCOME TO FRODO ON WII               ",
		/*02*/		" ------------------------------------------------- ",
		/*03*/		" In the system, hit HOME on the Wiimote to get to  ", 
		/*04*/		" the config-page. Load or autostart a D64, T64, or ", 
		/*05*/		" PRG image. Use the virtual keyboard or assign the ", 
		/*06*/		" key strokes to buttons on the Wiimote.            ",
		/*07*/		" Save the game state in the main menu of the game. ", 
		/*08*/		" Next time you can load that state instead of the  ",
		/*09*/		" game to have all configured stuff along with that ",
		/*10*/		" game.                                             ",
		/*11*/		"                                                   ",
		/*12*/		" This version features USB-keyboard support and    ", 
		/*13*/		" D64 content listing. Use first Wiimote as port 1  ", 
		/*14*/		" joystick and the second as port 2 joystick.       ",
		/*15*/		"                                                   ", 
		/*16*/		".-------------------------------------------------.",
		/*17*/		"^|       Enter Frodo       |    Don't show again    ", 
		NULL
};

const char *new_main_menu_messages[] = {
		/*00*/		"#1.MAIN MENU                       ",
		/*01*/		"#1-------------------------------------", 
		/*02*/		".Image", 
		/*03*/		"^|Load|Start|Remove", 
		/*04*/		".States",     
		/*05*/		"^|Load|Save|Delete|Rename ",     
		/*06*/		".Keyboard", 
		/*07*/		"^|Type|Macro|Bind",
		/*08*/		"#1-------------------------------------",
		/*09*/		".Reset the C=64",           
		/*10*/  	".Options",
		/*11*/		".Advanced Options", 
		/*12*/		".Controls",                
		/*13*/		"#1-------------------------------------",
		/*14*/		".Quit",                
		/*15*/		"#1-------------------------------------",
		/*16*/		"#1'2'=SELECT, '1'=CANCEL",                
		NULL
};
const char *new_options_menu_messages[] = {
		/*00*/		"#1.OPTIONS MENU                    ",
		/*01*/		"#1-------------------------------------", 
		/*02*/  	".Map Wiimote 1 to:",
		/*03*/		"^|Port 1|Port 2",
		/*04*/		" ", 
		/*05*/		".True 1541 emulation",       
		/*06*/		"^|NO|YES",
		/*07*/		" ", 
		/*08*/	  ".1541 Floppy Drive LED", /* 0 */
		/*09*/	  "^|OFF|ON",
		/*10*/		"#1-------------------------------------",
		/*11*/		"#1'2'=SELECT, '1'=CANCEL",                
		NULL
};
const char *new_advanced_options_menu_messages[] = {
		/*00*/		"#1.ADVANCED OPTIONS MENU           ",
		/*01*/		"#1-------------------------------------", 
		/*02*/		".Display resolution", /* 0 */
		/*03*/		"^|double-center|stretched",
		/*04*/		" ", 
		/*05*/		".Speed (approx.)",     /* 2 */
		/*06*/		"^|95|100|110",
		/*07*/		" ", 
		/*08*/		".Sprite collisions", /* 0 */
		/*09*/		"^|OFF|ON",
		/*10*/		" ", 
		/*11*/		".Autostart", /* 0 */
		/*12*/		"^|Save|Clear", /* 0 */
		/*13*/		"#1-------------------------------------",
		/*14*/		"#1'2'=SELECT, '1'=CANCEL",                
		NULL
};
const char *new_help_menu_messages[] = {
		/*00*/		"#1.CONTROLS MENU                        ",
		/*01*/		"#1-------------------------------------", 
		/*02*/		".Wiimote key mappings", /* 0 */
		/*03*/		"^|Wiimote Info|Set Default", /* 0 */
		/*04*/		" ", 
		/*05*/		".Show USB-keyboard layout", /* 0 */
		/*06*/		"#1-------------------------------------",
		/*07*/		"#1'2'=SELECT, '1'=CANCEL",                
		NULL
};
const char *layout_messages[] = {
		"#1.USB-Keyboard Layout             ",
		"#1-------------------------------------", 
		"#2ESC = Run/Stop",
		"#2F9  = QuickSave",
		"#2F10 = QuickLoad",
		"#2F11 = Restore",
		"#2TAB = CTRL",
		"#2INS = Clr/Home",
		"#2PGU = ¤",
		"#2AGR = C=",
		"#2HOM = Menu",
		"#1-------------------------------------", 
		"Back",
		NULL,
};

int msgInfo(char *text, int duration, SDL_Rect *irc)
{
	int len = strlen(text);
	int X, Y, wX, wY;
	SDL_Rect src;
	SDL_Rect rc;
	SDL_Rect brc;

	X = (FULL_DISPLAY_X /2) - (len / 2 + 1)*12;
	Y = (FULL_DISPLAY_Y /2) - 24;

	brc.x = FULL_DISPLAY_X/2-2*12; 
	brc.y=Y+42;
	brc.w=48;
	brc.h=20;

	rc.x = X; 
	rc.y=Y;
	rc.w=12*(len + 2);
	rc.h=duration > 0 ? 48 : 80;

	src.x=rc.x+4;
	src.y=rc.y+4;
	src.w=rc.w;
	src.h=rc.h;


	if (irc)
	{
		irc->x=rc.x;
		irc->y=rc.y;
		irc->w=src.w;
		irc->h=src.h;
	}
	SDL_FillRect(real_screen, &src, SDL_MapRGB(real_screen->format, 0, 96, 0));	
	SDL_FillRect(real_screen, &rc, SDL_MapRGB(real_screen->format, 128, 128, 128));
	menu_print_font(real_screen, 0,0,0, X+12, Y+12, text);
	SDL_UpdateRect(real_screen, src.x, src.y, src.w, src.h);
	SDL_UpdateRect(real_screen, rc.x, rc.y, rc.w,rc.h);
	if (duration > 0)
		SDL_Delay(duration);
	else if (duration < 0)
	{
		SDL_FillRect(real_screen, &brc, SDL_MapRGB(real_screen->format, 0x00, 0x80, 0x00));
		menu_print_font(real_screen, 0,0,0, FULL_DISPLAY_X/2-12, Y+42, "OK");
		SDL_UpdateRect(real_screen, brc.x, brc.y, brc.w, brc.h);
		menu_wait_key_press();
	}

	return 1;
}

bool msgKill(SDL_Rect *rc)
{
	SDL_UpdateRect(real_screen, rc->x, rc->y, rc->w,rc->h);
}

bool msgYesNo(char *text, bool default_opt, int x, int y)
{
	int len = strlen(text);
	int X, Y, wX, wY;
	SDL_Rect src;
	SDL_Rect rc;
	SDL_Rect brc;
	uint32_t key;
	bool old;

	old = default_opt;

	if (x < 0)
		X = (FULL_DISPLAY_X /2) - (len / 2 + 1)*12;
	else
		X = x;

	if (y < 0)	
		Y = (FULL_DISPLAY_Y /2) - 48;
	else
		Y = y;

	rc.x=X; 
	rc.y=Y;
	rc.w=12*(len + 2);
	rc.h=80;

	src.x=rc.x+4;
	src.y=rc.y+4;
	src.w=rc.w;
	src.h=rc.h;

	while (1)
	{
		SDL_FillRect(real_screen, &src, SDL_MapRGB(real_screen->format, 0, 96, 0));	
		SDL_FillRect(real_screen, &rc, SDL_MapRGB(real_screen->format, 128, 128, 128));
		menu_print_font(real_screen, 0,0,0, X+12, Y+12, text);

		if (default_opt)
		{
			brc.x=rc.x + rc.w/2-5*12; 
			brc.y=rc.y+42;
			brc.w=12*3;
			brc.h=20;
			SDL_FillRect(real_screen, &brc, SDL_MapRGB(real_screen->format, 0x00, 0x80, 0x00));
		}
		else
		{
			brc.x=rc.x + rc.w/2+5*12-2*12-6; 
			brc.y=rc.y+42;
			brc.w=12*3;
			brc.h=20;
			SDL_FillRect(real_screen, &brc, SDL_MapRGB(real_screen->format, 0x80, 0x00, 0x00));
		}
		menu_print_font(real_screen, 0,0,0, rc.x + rc.w/2-5*12, Y+42, "YES");
		menu_print_font(real_screen, 0,0,0, rc.x + rc.w/2-5*12+8*12, Y+42, "NO");

		SDL_UpdateRect(real_screen, src.x, src.y, src.w, src.h);
		SDL_UpdateRect(real_screen, rc.x, rc.y, rc.w,rc.h);
		SDL_UpdateRect(real_screen, brc.x, brc.y, brc.w,brc.h);

		key = menu_wait_key_press();
		if (key & KEY_SELECT)
		{
			return default_opt;
		}
		else if (key & KEY_ESCAPE)
		{
			return false;
		}
		else if (key & KEY_LEFT)
		{
			default_opt = !default_opt;
		}
		else if (key & KEY_RIGHT)
		{
			default_opt = !default_opt;
		}
	}
}


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
#define _MAX_STRING 24
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

void menu_print_font64(SDL_Surface *screen, int r, int g, int b, int x, int y, const char *msg)
{
#undef _MAX_STRING
#define _MAX_STRING 26
	SDL_Surface *font_surf;
	SDL_Rect dst = {x, y,  0, 0};
	SDL_Color color = {r, g, b};
	char buf[255];
	unsigned int i;

	memset(buf, 0, sizeof(buf));
	strncpy(buf, msg, 254);
	if (buf[0] != '|' && buf[0] != '^' && buf[0] != '.'
		&& buf[0] != '#' && buf[0] != ' ' )
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

	font_surf = TTF_RenderText_Blended(menu_font64, buf, color);
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
	int entries_visible = (p_menu->y2 - p_menu->y1) / line_height - 1;

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
		r.w = p_menu->x2;
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
					menu_print_font(screen, 0x80,0x80,0x80,
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
										SDL_MapRGB(screen->format, 0x80,0x80,0x80));
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
	if (strlen(kbin))
	{
		menu_print_font(screen,  255,255,255, x_start, y_start + y, kbin);
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

static void select_one(menu_t *p_menu, int sel)
{

	if (sel >= p_menu->n_entries)
		sel = 0;
	p_menu->cur_sel = sel;
	if (p_menu->pp_msgs[p_menu->cur_sel][0] == ' ' ||
			p_menu->pp_msgs[p_menu->cur_sel][0] == '#' ||
			IS_SUBMENU(p_menu->pp_msgs[p_menu->cur_sel]))
		p_menu->cur_sel = 0;
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

static int is_submenu_title(menu_t *p_menu, int n)
{
	if (n+1 >= p_menu->n_entries)
		return 0;
	else
		return IS_SUBMENU(p_menu->pp_msgs[n+1]);
}


static void menu_init(char *title, menu_t *p_menu, TTF_Font *p_font, const char **pp_msgs,
		int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int submenu;
	int i;
	int j;

	memset(p_menu, 0, sizeof(menu_t));

	p_menu->pp_msgs = pp_msgs;
	p_menu->p_font = p_font;
	p_menu->x1 = x1;
	p_menu->y1 = y1;
	p_menu->x2 = x2;
	p_menu->y2 = y2;

	p_menu->text_w = 0;
	p_menu->n_submenus = 0;
	strcpy(p_menu->title, title);

	for (p_menu->n_entries = 0; p_menu->pp_msgs[p_menu->n_entries]; p_menu->n_entries++)
	{
		int text_w_font;

		/* Is this a submenu? */
		if (IS_SUBMENU(p_menu->pp_msgs[p_menu->n_entries]))
		{
			p_menu->n_submenus++;
			continue; /* Length of submenus is unimportant */
		}

		if (TTF_SizeText(p_font, p_menu->pp_msgs[p_menu->n_entries], &text_w_font, NULL) != 0)
		{
			fprintf(stderr, "%s\n", TTF_GetError());
			exit(1);
		}
		if (text_w_font > p_menu->text_w)
			p_menu->text_w = text_w_font;
	}
	if (p_menu->text_w > p_menu->x2 - p_menu->x1)
		p_menu->text_w = p_menu->x2 - p_menu->x1;

	if ( !(p_menu->p_submenus = (submenu_t *)malloc(sizeof(submenu_t) * p_menu->n_submenus)) )
	{
		perror("malloc failed!\n");
		exit(1);
	}

	j=0;
	submenu = 0;
	for (; j < p_menu->n_entries; j++)
	{
		if (IS_SUBMENU(p_menu->pp_msgs[j]))
		{
			int n;

			p_menu->p_submenus[submenu].index = j;
			p_menu->p_submenus[submenu].sel = 0;
			p_menu->p_submenus[submenu].n_entries = 0;
			for (n=0; p_menu->pp_msgs[j][n] != '\0'; n++)
			{
				if (p_menu->pp_msgs[j][n] == '|')
					p_menu->p_submenus[submenu].n_entries++;
			}
			submenu++;
		}
	}
	p_menu->text_h = p_menu->n_entries * (TTF_FontHeight(p_font) + TTF_FontHeight(p_font) / 4);
}

static void menu_fini(menu_t *p_menu)
{
	free(p_menu->p_submenus);
}

uint32_t menu_wait_key_press(void)
{
	SDL_Event ev;
	uint32_t keys = 0;

	while (1)
	{
#if defined(GEKKO)
		Uint32 remote_keys, classic_keys;
		WPADData *wpad, *wpad_other;

		WPAD_ScanPads();
		wpad = WPAD_Data(WPAD_CHAN_0);
		wpad_other = WPAD_Data(WPAD_CHAN_1);

		if (!wpad && !wpad_other)
			return 0;

		remote_keys = wpad->btns_d | wpad_other->btns_d;
		classic_keys = 0;

		/* Check classic controllers as well */
		if (wpad->exp.type == WPAD_EXP_CLASSIC ||
				wpad_other->exp.type == WPAD_EXP_CLASSIC)
		{
			static bool classic_keys_changed;
			static Uint32 classic_last;

			classic_keys = wpad->exp.classic.btns | wpad_other->exp.classic.btns;

			classic_keys_changed = classic_keys != classic_last;
			classic_last = classic_keys;

			/* No repeat, thank you */
			if (!classic_keys_changed)
				classic_keys = 0;
		}

		if (remote_keys & WPAD_BUTTON_B)
			keys |= KEY_HELP;
		if ( (remote_keys & WPAD_BUTTON_DOWN) || (classic_keys & CLASSIC_CTRL_BUTTON_RIGHT) )
			keys |= KEY_RIGHT;
		if ( (remote_keys & WPAD_BUTTON_UP) || (classic_keys & CLASSIC_CTRL_BUTTON_LEFT) )
			keys |= KEY_LEFT;
		if ( (remote_keys & WPAD_BUTTON_LEFT) || (classic_keys & CLASSIC_CTRL_BUTTON_DOWN) )
			keys |= KEY_DOWN;
		if ( (remote_keys & WPAD_BUTTON_RIGHT) || (classic_keys & CLASSIC_CTRL_BUTTON_UP) )
			keys |= KEY_UP;
		if ( (remote_keys & WPAD_BUTTON_PLUS) || (classic_keys & CLASSIC_CTRL_BUTTON_PLUS) )
			keys |= KEY_PAGEUP;
		if ( (remote_keys & WPAD_BUTTON_MINUS) || (classic_keys & CLASSIC_CTRL_BUTTON_MINUS) )
			keys |= KEY_PAGEDOWN;
		if ( (remote_keys & (WPAD_BUTTON_A | WPAD_BUTTON_2) ) || (classic_keys & (CLASSIC_CTRL_BUTTON_A | CLASSIC_CTRL_BUTTON_X)) )
			keys |= KEY_SELECT;
		if ( (remote_keys & (WPAD_BUTTON_1 | WPAD_BUTTON_HOME) ) || (classic_keys & (CLASSIC_CTRL_BUTTON_B | CLASSIC_CTRL_BUTTON_Y)) )
			keys |= KEY_ESCAPE;

#endif
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
		if (keys != 0 || strlen(kbin))
		{
			return keys;
		}
		SDL_Delay(100);
	}
	return keys;
}


extern void PicDisplay(char *name, int off_x, int off_y, int wait);
extern const char **get_t64_list(char *t64);
extern const char **get_prg_list(char *t64);

extern char curdir[256];

static int menu_select_internal(SDL_Surface *screen, menu_t *p_menu, int *p_submenus, int sel, bool info)
{
	int ret = -1;
	int i;
	char buffer[256];
	SDL_Rect rc;
	SDL_Rect urc;
	SDL_Rect frc;

	for (i = 0; i < p_menu->n_submenus; i++)
		p_menu->p_submenus[i].sel = p_submenus[i];

	while(1)
	{
		uint32_t keys;
		urc.x = p_menu->x1; urc.y = p_menu->y1; urc.w = p_menu->x2; urc.h = p_menu->y2;
		frc.x = p_menu->x1-1; frc.y = p_menu->y1-1; frc.w = p_menu->x2+2; frc.h = p_menu->y2+2;
		if (frc.x < 0)
			frc.x = 0;
		if (frc.y < 0)
			frc.y = 0;
		while (frc.x + frc.w > FULL_DISPLAY_X)
			frc.w--;
		while (frc.y + frc.h > FULL_DISPLAY_Y)
			frc.h--;
		if (sel < 0)
			SDL_FillRect(screen, &frc, SDL_MapRGB(screen->format, 0x40, 0x00, 0x00));
		else
			SDL_FillRect(screen, &frc, SDL_MapRGB(screen->format, 0x00, 0x00, 0xff));
		SDL_FillRect(screen, &urc, SDL_MapRGB(screen->format, 0x00, 0x00, 0x00));
		menu_draw(screen, p_menu, sel);

		if (p_menu->pp_msgs[p_menu->cur_sel][0] == '[')
		{
			rc.x = FULL_DISPLAY_X - 26*12;
			rc.y = 12;
			rc.w = 25*12;
			rc.h = FULL_DISPLAY_Y - 16;

			rc.x--;
			rc.y--;
			rc.w+=2;
			rc.h+=2;
			SDL_FillRect(screen, &rc, SDL_MapRGB(screen->format, 0x40, 0x00, 0));
			rc.x++;
			rc.y++;
			rc.w-=2;
			rc.h-=2;
			SDL_FillRect(screen, &rc, SDL_MapRGB(screen->format, 0, 0, 0));
			rc.x--;
			rc.y--;
			rc.w+=2;
			rc.h = rc.y + 15;
			SDL_FillRect(screen, &rc, SDL_MapRGB(screen->format, 0x40, 0x00, 0));
			rc.x = FULL_DISPLAY_X - 26*12-1;
			rc.y = 12-1;
			rc.w = 25*12+2;
			rc.h = FULL_DISPLAY_Y - 16 +2;
			menu_print_font(screen, 0x00,0x00,0x00, rc.x, rc.y, "Folder");
			SDL_UpdateRect(screen, rc.x, rc.y, rc.w, rc.h);

		}
		/*******/
		SDL_UpdateRect(screen, frc.x, frc.y, frc.w, frc.h);
		if (sel >= 0)
			keys = menu_wait_key_press();
		else
		{
			break;
		}
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
		{
			break;
		}
		else if (keys & KEY_SELECT)
		{
			ret = p_menu->cur_sel;
			for (i=0; i<p_menu->n_submenus; i++)
				p_submenus[i] = p_menu->p_submenus[i].sel;
			break;
		}
	}
	SDL_FillRect(screen, &frc, SDL_MapRGB(screen->format, 0, 0, 0));

	return ret;
}


int menu_select(const char **msgs, int *submenus, int sel)
{
	menu_t menu;
	int out;

	menu_init((char*)"", &menu, menu_font, msgs,
			0, 0, FULL_DISPLAY_X, FULL_DISPLAY_Y);

	if (sel >= 0)
		select_one(&menu, sel);

	out = menu_select_internal(real_screen, &menu, submenus, sel, false);

	menu_fini(&menu);

	return out;
}

int menu_select(const char **msgs, int *submenus)
{
	return menu_select(msgs, submenus, 0);
}

int menu_select_sized(char *title, SDL_Rect *rc, const char **msgs, int *submenus, int sel)
{
	menu_t menu;
	int out;
	bool info;

	if (!strcmp(title, "Folder") || !strcmp(title, "Single File") ||
			!strcmp(title, "C-64 Disc") || !strcmp(title, "C-64 Tape") || sel < 0)
		info = false;
	else
		info = true;

	if (rc == NULL)
		menu_init(title, &menu, menu_font, msgs,
				0, 0, FULL_DISPLAY_X, FULL_DISPLAY_Y);
	else
		menu_init(title, &menu, menu_font, msgs,
				rc->x, rc->y, rc->w, rc->h);

	if (sel >= 0)
		select_one(&menu, sel);
	out = menu_select_internal(real_screen, &menu, submenus, sel, info);

	menu_fini(&menu);

	return out;
}

static TTF_Font *read_font(const char *path)
{
	TTF_Font *out;
	SDL_RWops *rw;
	Uint8 *data = (Uint8*)malloc(1 * 1024*1024);
	FILE *fp = fopen(path, "r");

	if (!data) {
		fprintf(stderr, "Malloc failed\n");
		exit(1);
	}
	if (!fp) {
		fprintf(stderr, "Could not open font\n");
		exit(1);
	}
	fread(data, 1, 1 * 1024 * 1024, fp);
	rw = SDL_RWFromMem(data, 1 * 1024 * 1024);
	if (!rw) 
	{
		fprintf(stderr, "Could not create RW: %s\n", SDL_GetError());
		exit(1);
	}
	out = TTF_OpenFontRW(rw, 1, 20);
	if (!out)
	{
		fprintf(stderr, "Unable to open font %s\n", path);
		exit(1);		
	}
	fclose(fp);

	return out;
}

void menu_init()
{
	Uint8 *data64 = (Uint8*)malloc(1 * 1024*1024);

	menu_font = read_font(FONT_PATH);
	menu_font64 = read_font(FONT64_PATH);
}
