/*
	$Id: hud_common.c,v 1.69 2006-09-25 00:26:58 johnnycz Exp $
*/
//
// common HUD elements
// like clock etc..
//

#include "quakedef.h"
#include "common_draw.h"
#include "EX_misc.h"
#include "mp3_player.h"
#include "png.h"
#include "image.h"
#include "stats_grid.h"

#ifndef STAT_MINUS
#define STAT_MINUS		10
#endif

#define ROUND(f)   ((f>=0)?(int)(f + .5):(int)(f - .5))

#ifdef GLQUAKE
void Draw_AlphaFill (int x, int y, int w, int h, int c, float alpha);
#else
void Draw_Fill (int x, int y, int w, int h, int c);
#endif

hud_t *hud_netgraph;

// ----------------
// HUD planning
//

cvar_t hud_planmode = {"hud_planmode",   "0"};
int hud_stats[MAX_CL_STATS];

int HUD_Stats(int stat_num)
{
    if (hud_planmode.value)
        return hud_stats[stat_num];
    else
        return cl.stats[stat_num];
}

// ----------------
// HUD low levels
//

cvar_t hud_tp_need = {"hud_tp_need",   "0"};

/* tp need levels
int TP_IsHealthLow(void);
int TP_IsArmorLow(void);
int TP_IsAmmoLow(int weapon); */
extern cvar_t tp_need_health, tp_need_ra, tp_need_ya, tp_need_ga,
		tp_weapon_order, tp_need_weapon, tp_need_shells,
		tp_need_nails, tp_need_rockets, tp_need_cells;
int State_AmmoForWeapon(int weapon)
{
    switch (weapon)
    {
        case 2:
        case 3: return cl.stats[STAT_SHELLS];
        case 4:
        case 5: return cl.stats[STAT_NAILS];
        case 6:
        case 7: return cl.stats[STAT_ROCKETS];
        case 8: return cl.stats[STAT_CELLS];
        default: return 0;
    }
}
int TP_IsHealthLow(void)
{
    return cl.stats[STAT_HEALTH] <= tp_need_health.value;
}

int TP_IsArmorLow(void)
{
    if ((cl.stats[STAT_ARMOR] > 0) && (cl.stats[STAT_ITEMS] & IT_ARMOR3))
        return cl.stats[STAT_ARMOR] <= tp_need_ra.value;
    if ((cl.stats[STAT_ARMOR] > 0) && (cl.stats[STAT_ITEMS] & IT_ARMOR2))
        return cl.stats[STAT_ARMOR] <= tp_need_ya.value;
    if ((cl.stats[STAT_ARMOR] > 0) && (cl.stats[STAT_ITEMS] & IT_ARMOR1))
        return cl.stats[STAT_ARMOR] <= tp_need_ga.value;
    return 1;
}

int TP_IsWeaponLow(void)
{
    char *s = tp_weapon_order.string;
    while (*s  &&  *s != tp_need_weapon.string[0])
    {
        if (cl.stats[STAT_ITEMS] & (IT_SHOTGUN << (*s-'0'-2)))
            return false;
        s++;
    }
    return true;
}

int TP_IsAmmoLow(int weapon)
{
    int ammo = State_AmmoForWeapon(weapon);
    switch (weapon)
    {
    case 2:
    case 3:  return ammo <= tp_need_shells.value;
    case 4:
    case 5:  return ammo <= tp_need_nails.value;
    case 6:
    case 7:  return ammo <= tp_need_rockets.value;
    case 8:  return ammo <= tp_need_cells.value;
    default: return 0;
    }
}

qbool HUD_HealthLow(void)
{
    if (hud_tp_need.value)
        return TP_IsHealthLow();
    else
        return HUD_Stats(STAT_HEALTH) <= 25;
}

qbool HUD_ArmorLow(void)
{
    if (hud_tp_need.value)
        return (TP_IsArmorLow());
    else
        return (HUD_Stats(STAT_ARMOR) <= 25);
}

qbool HUD_AmmoLow(void)
{
    if (hud_tp_need.value)
    {
        if (HUD_Stats(STAT_ITEMS) & IT_SHELLS)
            return TP_IsAmmoLow(2);
        else if (HUD_Stats(STAT_ITEMS) & IT_NAILS)
            return TP_IsAmmoLow(4);
        else if (HUD_Stats(STAT_ITEMS) & IT_ROCKETS)
            return TP_IsAmmoLow(6);
        else if (HUD_Stats(STAT_ITEMS) & IT_CELLS)
            return TP_IsAmmoLow(8);
        return false;
    }
    else
        return (HUD_Stats(STAT_AMMO) <= 10);
}

int HUD_AmmoLowByWeapon(int weapon)
{
    if (hud_tp_need.value)
        return TP_IsAmmoLow(weapon);
    else
    {
        int a;
        switch (weapon)
        {
        case 2:
        case 3:
            a = STAT_SHELLS; break;
        case 4:
        case 5:
            a = STAT_NAILS; break;
        case 6:
        case 7:
            a = STAT_ROCKETS; break;
        case 8:
            a = STAT_CELLS; break;
        default:
            return false;
        }
        return (HUD_Stats(a) <= 10);
    }
}

// ----------------
// DrawFPS
void SCR_HUD_DrawFPS(hud_t *hud)
{
    int x, y, width, height;
    char st[80];

    static cvar_t
        *hud_fps_show_min = NULL,
        *hud_fps_title,
		*hud_fps_decimals;

    if (hud_fps_show_min == NULL)   // first time called
    {
        hud_fps_show_min = HUD_FindVar(hud, "show_min");
        hud_fps_title    = HUD_FindVar(hud, "title");
		hud_fps_decimals = HUD_FindVar(hud, "decimals");
    }


    if (hud_fps_show_min->value)
        sprintf(st, "%3.*f\xf%3.*f", (int) hud_fps_decimals->value, cls.min_fps + 0.05, (int) hud_fps_decimals->value, cls.fps + 0.05);
    else
        sprintf(st, "%3.*f", (int) hud_fps_decimals->value, cls.fps + 0.05);

    if (hud_fps_title->value)
        strcat(st, " fps");

    width = 8*strlen(st);
    height = 8;

    if (HUD_PrepareDraw(hud, strlen(st)*8, 8, &x, &y))
        Draw_String(x, y, st);
}

#ifdef WIN32
int IN_GetMouseRate(void);
#endif

void SCR_HUD_DrawMouserate(hud_t *hud)
{
    int x, y, width, height;
	static int lastresult = 0;
	int newresult;
    char st[80];	// string buffer
	double t;		// current time
	static double lastframetime;	// last refresh

    static cvar_t *hud_mouserate_interval, *hud_mouserate_title = NULL;

    if (hud_mouserate_title == NULL)   // first time called
    {
        hud_mouserate_title    = HUD_FindVar(hud, "title");
		hud_mouserate_interval = HUD_FindVar(hud, "interval");
    }

#ifdef WIN32
	t = Sys_DoubleTime();
	if ((t - lastframetime) >= hud_mouserate_interval->value) {
		newresult = IN_GetMouseRate();
		lastframetime = t;
	} else
		newresult = 0;
#else
	newresult = -1;
#endif

	if (newresult > 0) {
		snprintf(st, sizeof(st), "%4d", newresult);
		lastresult = newresult;
	} else if (!newresult)
		snprintf(st, sizeof(st), "%4d", lastresult);
	else
		snprintf(st, sizeof(st), "n/a");

    if (hud_mouserate_title->value)
        strcat(st, " Hz");

    width = 8*strlen(st);
    height = 8;

    if (HUD_PrepareDraw(hud, strlen(st)*8, 8, &x, &y))
        Draw_String(x, y, st);
}

void SCR_HUD_DrawTracking(hud_t *hud)
{
    int x, y, width, height;
    char st[512];
	static cvar_t
		*hud_tracking_format;
	hud_tracking_format    = HUD_FindVar(hud, "format");

	strlcpy(st, hud_tracking_format->string, sizeof(st));
	Replace_In_String(st, sizeof(st), '%', 2, "n", cl.players[spec_track].name, "t", cl.teamplay ? cl.players[spec_track].team : "");

	width = 8*strlen(st);
    height = 8;
    if (cl.spectator && autocam == CAM_TRACK && HUD_PrepareDraw(hud, strlen(st)*8, 8, &x, &y))
        Draw_String(x, y, st);
}

void R_MQW_NetGraph(int outgoing_sequence, int incoming_sequence, int *packet_latency,
                int lost, int minping, int avgping, int maxping, int devping,
                int posx, int posy, int width, int height, int revx, int revy);
// ----------------
// Netgraph
void SCR_HUD_Netgraph(hud_t *hud)
{
    static cvar_t
        *par_width = NULL, *par_height,
        *par_swap_x, *par_swap_y,
        *par_ploss;

    if (par_width == NULL)  // first time
    {
        par_width  = HUD_FindVar(hud, "width");
        par_height = HUD_FindVar(hud, "height");
        par_swap_x = HUD_FindVar(hud, "swap_x");
        par_swap_y = HUD_FindVar(hud, "swap_y");
        par_ploss  = HUD_FindVar(hud, "ploss");
    }

    R_MQW_NetGraph(cls.netchan.outgoing_sequence, cls.netchan.incoming_sequence,
        packet_latency, par_ploss->value ? CL_CalcNet() : -1, -1, -1, -1, -1, -1,
        -1, (int)par_width->value, (int)par_height->value,
        (int)par_swap_x->value, (int)par_swap_y->value);
}

//---------------------
//
// draw HUD ping
//
void SCR_HUD_DrawPing(hud_t *hud)
{
    double t;
    static double last_calculated;
    static int ping_avg, pl, ping_min, ping_max;
    static float ping_dev;

    int width, height;
    int x, y;
    char buf[512];

    static cvar_t
        *hud_ping_period = NULL,
        *hud_ping_show_pl,
        *hud_ping_show_dev,
        *hud_ping_show_min,
        *hud_ping_show_max,
        *hud_ping_blink;

    if (hud_ping_period == NULL)    // first time
    {
        hud_ping_period   = HUD_FindVar(hud, "period");
        hud_ping_show_pl  = HUD_FindVar(hud, "show_pl");
        hud_ping_show_dev = HUD_FindVar(hud, "show_dev");
        hud_ping_show_min = HUD_FindVar(hud, "show_min");
        hud_ping_show_max = HUD_FindVar(hud, "show_max");
        hud_ping_blink    = HUD_FindVar(hud, "blink");
    }

    t = Sys_DoubleTime();
    if (t - last_calculated  >  hud_ping_period->value)
    {
        // recalculate

        net_stat_result_t result;
        float period;

        last_calculated = t;

        period = max(hud_ping_period->value, 0);

        CL_CalcNetStatistics(
            period,             // period of time
            network_stats,      // samples table
            NETWORK_STATS_SIZE, // number of samples in table
            &result);           // results

        if (result.samples == 0)
            return; // error calculating net

        ping_avg = (int)(result.ping_avg + 0.5);
        ping_min = (int)(result.ping_min + 0.5);
        ping_max = (int)(result.ping_max + 0.5);
        ping_dev = result.ping_dev;
        pl = result.lost_lost;

        clamp(ping_avg, 0, 999);
        clamp(ping_min, 0, 999);
        clamp(ping_max, 0, 999);
        clamp(ping_dev, 0, 99.9);
        clamp(pl, 0, 100);
    }

    buf[0] = 0;

    // blink
    if (hud_ping_blink->value)   // add dot
        strcat(buf, (last_calculated + hud_ping_period->value/2 > cls.realtime) ? "\x8f" : " ");

    // min ping
    if (hud_ping_show_min->value)
        strcat(buf, va("%d\xf", ping_min));

    // ping
    strcat(buf, va("%d", ping_avg));

    // max ping
    if (hud_ping_show_max->value)
        strcat(buf, va("\xf%d", ping_max));

    // unit
    strcat(buf, " ms");

    // standard deviation
    if (hud_ping_show_dev->value)
        strcat(buf, va(" (%.1f)", ping_dev));

    // pl
    if (hud_ping_show_pl->value)
        strcat(buf, va(" \x8f %d%%", pl));

    // display that on screen
    width = strlen(buf) * 8;
    height = 8;

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
        Draw_String(x, y, buf);
}

//---------------------
//
// draw HUD clock
//
void SCR_HUD_DrawClock(hud_t *hud)
{
    int width, height;
    int x, y;

    static cvar_t
        *hud_clock_big = NULL,
        *hud_clock_style,
        *hud_clock_blink,
		*hud_clock_scale;

    if (hud_clock_big == NULL)    // first time
    {
        hud_clock_big   = HUD_FindVar(hud, "big");
        hud_clock_style = HUD_FindVar(hud, "style");
        hud_clock_blink = HUD_FindVar(hud, "blink");
		hud_clock_scale = HUD_FindVar(hud, "scale");
    }

    if (hud_clock_big->value)
    {
        width = (24+24+16+24+24+16+24+24)*hud_clock_scale->value;
        height = 24*hud_clock_scale->value;
    }
    else
    {
        width = (8*8)*hud_clock_scale->value;
        height = 8*hud_clock_scale->value;
    }

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        if (hud_clock_big->value)
            SCR_DrawBigClock(x, y, hud_clock_style->value, hud_clock_blink->value, hud_clock_scale->value, TIMETYPE_CLOCK);
        else
            SCR_DrawSmallClock(x, y, hud_clock_style->value, hud_clock_blink->value, hud_clock_scale->value, TIMETYPE_CLOCK);
    }
}

//---------------------
//
// draw HUD gameclock
//
void SCR_HUD_DrawGameClock(hud_t *hud)
{
    int width, height;
    int x, y;
	int timetype;

    static cvar_t
        *hud_gameclock_big = NULL,
        *hud_gameclock_style,
        *hud_gameclock_blink,
		*hud_gameclock_countdown,
		*hud_gameclock_scale;

    if (hud_gameclock_big == NULL)    // first time
    {
        hud_gameclock_big   = HUD_FindVar(hud, "big");
        hud_gameclock_style = HUD_FindVar(hud, "style");
        hud_gameclock_blink = HUD_FindVar(hud, "blink");
		hud_gameclock_countdown = HUD_FindVar(hud, "countdown");
		hud_gameclock_scale = HUD_FindVar(hud, "scale");
    }

    if (hud_gameclock_big->value)
    {
        width = (24+24+16+24+24)*hud_gameclock_scale->value;
        height = 24*hud_gameclock_scale->value;
    }
    else
    {
        width = (5*8)*hud_gameclock_scale->value;
        height = 8*hud_gameclock_scale->value;
    }

	timetype = (hud_gameclock_countdown->value) ? TIMETYPE_GAMECLOCKINV : TIMETYPE_GAMECLOCK;

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        if (hud_gameclock_big->value)
            SCR_DrawBigClock(x, y, hud_gameclock_style->value, hud_gameclock_blink->value, hud_gameclock_scale->value, timetype);
        else
            SCR_DrawSmallClock(x, y, hud_gameclock_style->value, hud_gameclock_blink->value, hud_gameclock_scale->value, timetype);
    }
}

//---------------------
//
// draw HUD democlock
//
void SCR_HUD_DrawDemoClock(hud_t *hud)
{
    int width, height;
    int x, y;
    static cvar_t
        *hud_democlock_big = NULL,
        *hud_democlock_style,
        *hud_democlock_blink,
		*hud_democlock_scale;

	if (!cls.demoplayback) return;

    if (hud_democlock_big == NULL)    // first time
    {
        hud_democlock_big   = HUD_FindVar(hud, "big");
        hud_democlock_style = HUD_FindVar(hud, "style");
        hud_democlock_blink = HUD_FindVar(hud, "blink");
		hud_democlock_scale = HUD_FindVar(hud, "scale");
    }

    if (hud_democlock_big->value)
    {
        width = (24+24+16+24+24)*hud_democlock_scale->value;
        height = 24*hud_democlock_scale->value;
    }
    else
    {
        width = (5*8)*hud_democlock_scale->value;
        height = 8*hud_democlock_scale->value;
    }

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        if (hud_democlock_big->value)
            SCR_DrawBigClock(x, y, hud_democlock_style->value, hud_democlock_blink->value, hud_democlock_scale->value, TIMETYPE_DEMOCLOCK);
        else
            SCR_DrawSmallClock(x, y, hud_democlock_style->value, hud_democlock_blink->value, hud_democlock_scale->value, TIMETYPE_DEMOCLOCK);
    }
}

//---------------------
//
// network statistics
//
void SCR_HUD_DrawNetStats(hud_t *hud)
{
    int width, height;
    int x, y;

    static cvar_t *hud_net_period = NULL;

    if (hud_net_period == NULL)    // first time
    {
        hud_net_period = HUD_FindVar(hud, "period");
    }

    width = 16*8 ;
    height = 12 + 8 + 8 + 8 + 8 + 16 + 8 + 8 + 8 + 8 + 16 + 8 + 8 + 8;

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
        SCR_NetStats(x, y, hud_net_period->value);
}

#define SPEED_GREEN				"52"
#define SPEED_BROWN_RED			"100"
#define SPEED_DARK_RED			"72"
#define SPEED_BLUE				"216"
#define SPEED_RED				"229"

#define	SPEED_STOPPED			SPEED_GREEN
#define	SPEED_NORMAL			SPEED_BROWN_RED
#define	SPEED_FAST				SPEED_DARK_RED
#define	SPEED_FASTEST			SPEED_BLUE
#define	SPEED_INSANE			SPEED_RED

//---------------------
//
// speed-o-meter
//
void SCR_HUD_DrawSpeed(hud_t *hud)
{
    int width, height;
    int x, y;

    static cvar_t *hud_speed_xyz = NULL,
		*hud_speed_width,
        *hud_speed_height,
		*hud_speed_tick_spacing,
		*hud_speed_opacity,
		*hud_speed_color_stopped,
		*hud_speed_color_normal,
		*hud_speed_color_fast,
		*hud_speed_color_fastest,
		*hud_speed_color_insane,
		*hud_speed_vertical,
		*hud_speed_vertical_text,
		*hud_speed_text_align;

    if (hud_speed_xyz == NULL)    // first time
    {
        hud_speed_xyz			= HUD_FindVar(hud, "xyz");
		hud_speed_width			= HUD_FindVar(hud, "width");
		hud_speed_height		= HUD_FindVar(hud, "height");
		hud_speed_tick_spacing	= HUD_FindVar(hud, "tick_spacing");
		hud_speed_opacity		= HUD_FindVar(hud, "opacity");
		hud_speed_color_stopped	= HUD_FindVar(hud, "color_stopped");
		hud_speed_color_normal	= HUD_FindVar(hud, "color_normal");
		hud_speed_color_fast	= HUD_FindVar(hud, "color_fast");
		hud_speed_color_fastest	= HUD_FindVar(hud, "color_fastest");
		hud_speed_color_insane	= HUD_FindVar(hud, "color_insane");
		hud_speed_vertical		= HUD_FindVar(hud, "vertical");
		hud_speed_vertical_text	= HUD_FindVar(hud, "vertical_text");
		hud_speed_text_align	= HUD_FindVar(hud, "text_align");
    }

	width = max(0, hud_speed_width->value);
	height = max(0, hud_speed_height->value);

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
	{
		SCR_DrawHUDSpeed(x, y, width, height, 
			hud_speed_xyz->value, 
			hud_speed_tick_spacing->value, 
			hud_speed_opacity->value,
			hud_speed_vertical->value,
			hud_speed_vertical_text->value,
			hud_speed_text_align->value,
			hud_speed_color_stopped->value,
			hud_speed_color_normal->value,
			hud_speed_color_fast->value,
			hud_speed_color_fastest->value,
			hud_speed_color_insane->value);
	}
}

#ifdef GLQUAKE

#define	HUD_SPEED2_ORIENTATION_UP		0
#define	HUD_SPEED2_ORIENTATION_DOWN		1
#define	HUD_SPEED2_ORIENTATION_RIGHT	2
#define	HUD_SPEED2_ORIENTATION_LEFT		3

void SCR_HUD_DrawSpeed2(hud_t *hud)
{
	int width, height;
    int x, y;	

    static cvar_t *hud_speed2_xyz = NULL,
		*hud_speed2_opacity,
		*hud_speed2_color_stopped,
		*hud_speed2_color_normal,
		*hud_speed2_color_fast,
		*hud_speed2_color_fastest,
		*hud_speed2_color_insane,
		*hud_speed2_radius,
		*hud_speed2_wrapspeed,
		*hud_speed2_orientation;

    if (hud_speed2_xyz == NULL)    // first time
    {
        hud_speed2_xyz				= HUD_FindVar(hud, "xyz");
		hud_speed2_opacity			= HUD_FindVar(hud, "opacity");
		hud_speed2_color_stopped	= HUD_FindVar(hud, "color_stopped");
		hud_speed2_color_normal		= HUD_FindVar(hud, "color_normal");
		hud_speed2_color_fast		= HUD_FindVar(hud, "color_fast");
		hud_speed2_color_fastest	= HUD_FindVar(hud, "color_fastest");
		hud_speed2_color_insane		= HUD_FindVar(hud, "color_insane");
		hud_speed2_radius			= HUD_FindVar(hud, "radius");
		hud_speed2_wrapspeed		= HUD_FindVar(hud, "wrapspeed");
		hud_speed2_orientation		= HUD_FindVar(hud, "orientation");
    }

	// Calculate the height and width based on the radius.
	switch((int)hud_speed2_orientation->value)
	{
		case HUD_SPEED2_ORIENTATION_LEFT :
		case HUD_SPEED2_ORIENTATION_RIGHT :
			height = max(0, 2*hud_speed2_radius->value);
			width = max(0, (hud_speed2_radius->value));					
			break;
		case HUD_SPEED2_ORIENTATION_DOWN :
		case HUD_SPEED2_ORIENTATION_UP :
		default :
			// Include the height of the speed text in the height.
			height = max(0, (hud_speed2_radius->value));
			width = max(0, 2*hud_speed2_radius->value);			
			break;
	}

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
	{
		int player_speed;
		int arc_length;
		int color1, color2;
		int text_x = x;
		int text_y = y;
		vec_t *velocity;
		
		// Start and end points for the needle
		int needle_start_x = 0;
		int needle_start_y = 0;
		int needle_end_x = 0;
		int needle_end_y = 0;

		// The length of the arc between the zero point
		// and where the needle is pointing at.
		int needle_offset = 0;

		// The angle between the zero point and the position 
		// that the needle is drawn on.
		float needle_angle = 0.0;

		// The angle where to start drawing the half circle and where to end.
		// This depends on the orientation of the circle (left, right, up, down).
		float circle_startangle = 0.0;
		float circle_endangle = 0.0;

		// Get the velocity.
		if (cl.players[cl.playernum].spectator && Cam_TrackNum() >= 0)
		{
			velocity = cl.frames[cls.netchan.incoming_sequence & UPDATE_MASK].playerstate[Cam_TrackNum()].velocity;
		}
		else
		{
			velocity = cl.simvel;
		}

		// Calculate the speed 
		if (!hud_speed2_xyz->value)
		{
			// Based on XY.
			player_speed = sqrt(velocity[0]*velocity[0]
							  + velocity[1]*velocity[1]);
		}
		else
		{
			// Based on XYZ.
			player_speed = sqrt(velocity[0]*velocity[0]
							  + velocity[1]*velocity[1]
							  + velocity[2]*velocity[2]);
		}

		// Set the color based on the wrap speed.
		switch ((int)(player_speed / hud_speed2_wrapspeed->value))
		{
			case 0:  
				color1 = hud_speed2_color_stopped->value; 
				color2 = hud_speed2_color_normal->value; 
				break;
			case 1:  
				color1 = hud_speed2_color_normal->value; 
				color2 = hud_speed2_color_fast->value; 
				break;
			case 2:  
				color1 = hud_speed2_color_fast->value; 
				color2 = hud_speed2_color_fastest->value; 
				break;
			default: 
				color1 = hud_speed2_color_fastest->value; 
				color2 = hud_speed2_color_insane->value; 
				break;
		}

		// Set some properties how to draw the half circle, needle and text
		// based on the orientation of the hud item.
		switch((int)hud_speed2_orientation->value)
		{
			case HUD_SPEED2_ORIENTATION_LEFT :
			{
				x += width;
				y += height / 2;
				circle_startangle = M_PI / 2.0;
				circle_endangle	= (3*M_PI) / 2.0;

				text_x = x - 32;
				text_y = y - 4;
				break;
			}		
			case HUD_SPEED2_ORIENTATION_RIGHT :
			{
				y += height / 2;
				circle_startangle = (3*M_PI) / 2.0;
				circle_endangle = (5*M_PI) / 2.0;
				needle_end_y = y + hud_speed2_radius->value * sin (needle_angle);

				text_x = x;
				text_y = y - 4;
				break;
			}	
			case HUD_SPEED2_ORIENTATION_DOWN :
			{
				x += width / 2;
				circle_startangle = M_PI;
				circle_endangle = 2*M_PI;
				needle_end_y = y + hud_speed2_radius->value * sin (needle_angle);

				text_x = x - 16;
				text_y = y;
				break;
			}						
			case HUD_SPEED2_ORIENTATION_UP :
			default :
			{
				x += width / 2;
				y += height;
				circle_startangle = 0;
				circle_endangle = M_PI;
				needle_end_y = y - hud_speed2_radius->value * sin (needle_angle);

				text_x = x - 16;
				text_y = y - 8;
				break;
			}
		}

		//
		// Calculate the offsets and angles.
		//
		{
			// Calculate the arc length of the half circle background.
			arc_length = fabs((circle_endangle - circle_startangle) * hud_speed2_radius->value);

			// Calculate the angle where the speed needle should point.
			needle_offset = arc_length * (player_speed % ROUND(hud_speed2_wrapspeed->value)) / ROUND(hud_speed2_wrapspeed->value);
			needle_angle = needle_offset / hud_speed2_radius->value;

			// Draw from the center of the half circle. 
			needle_start_x = x;
			needle_start_y = y;
		}

		// Set the needle end point depending on the orientation of the hud item.
		
		switch((int)hud_speed2_orientation->value)
		{
			case HUD_SPEED2_ORIENTATION_LEFT :
			{
				needle_end_x = x - hud_speed2_radius->value * sin (needle_angle);
				needle_end_y = y + hud_speed2_radius->value * cos (needle_angle);
				break;
			}
			case HUD_SPEED2_ORIENTATION_RIGHT :
			{
				needle_end_x = x + hud_speed2_radius->value * sin (needle_angle);
				needle_end_y = y - hud_speed2_radius->value * cos (needle_angle);
				break;
			}
			case HUD_SPEED2_ORIENTATION_DOWN :
			{
				needle_end_x = x + hud_speed2_radius->value * cos (needle_angle);
				needle_end_y = y + hud_speed2_radius->value * sin (needle_angle);
				break;
			}
			case HUD_SPEED2_ORIENTATION_UP :
			default :
			{
				needle_end_x = x - hud_speed2_radius->value * cos (needle_angle);
				needle_end_y = y - hud_speed2_radius->value * sin (needle_angle);
				break;
			}
		}

		// Draw the speed-o-meter background.
		Draw_AlphaPieSlice (x, y,				// Position
			hud_speed2_radius->value,			// Radius
			circle_startangle,					// Start angle
			circle_endangle - needle_angle,		// End angle
			1,									// Thickness
			true,								// Fill
			color1,								// Color
			hud_speed2_opacity->value);			// Opacity

		// Draw a pie slice that shows the "color" of the speed. 
		Draw_AlphaPieSlice (x, y,				// Position
			hud_speed2_radius->value,			// Radius 
			circle_endangle - needle_angle,		// Start angle
			circle_endangle,					// End angle
			1,									// Thickness
			true,								// Fill
			color2,								// Color
			hud_speed2_opacity->value);			// Opacity
		
		// Draw the "needle attachment" circle.
		Draw_AlphaCircle (x, y, 2.0, 1, true, 15, hud_speed2_opacity->value);

		// Draw the speed needle.
		Draw_AlphaLine (needle_start_x, needle_start_y, needle_end_x, needle_end_y, 1, 15, hud_speed2_opacity->value);

		// Draw the speed.
		Draw_String (text_x, text_y, va("%d", player_speed));		
	}
}

#endif

// =======================================================
//
//  s t a t u s   b a r   e l e m e n t s
//
//


// -----------
// gunz
//
void SCR_HUD_DrawGunByNum (hud_t *hud, int num, float scale, int style, int wide)
{
    extern mpic_t *sb_weapons[7][8];  // sbar.c
    int i = num - 2;
    int width, height;
    int x, y;
    char *tmp;

    scale = max(scale, 0.01);

    switch (style)
    {
	case 3:
    case 1:     // text
        width = 16 * scale;
        height = 8 * scale;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if ( HUD_Stats(STAT_ITEMS) & (IT_SHOTGUN<<i) )
        {
            switch (num)
            {
            case 2: tmp = "sg"; break;
            case 3: tmp = "bs"; break;
            case 4: tmp = "ng"; break;
            case 5: tmp = "sn"; break;
            case 6: tmp = "gl"; break;
            case 7: tmp = "rl"; break;
            case 8: tmp = "lg"; break;
            default: tmp = "";
            }

            if ( ((HUD_Stats(STAT_ACTIVEWEAPON) == (IT_SHOTGUN<<i)) && (style==1)) ||
				 ((HUD_Stats(STAT_ACTIVEWEAPON) != (IT_SHOTGUN<<i)) && (style==3))
			   )
                Draw_SString(x, y, tmp, scale);
            else
                Draw_SAlt_String(x, y, tmp, scale);
        }
        break;
	case 4:
    case 2:     // numbers
        width = 8 * scale;
        height = 8 * scale;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if ( HUD_Stats(STAT_ITEMS) & (IT_SHOTGUN<<i) )
        {
            if ( HUD_Stats(STAT_ACTIVEWEAPON) == (IT_SHOTGUN<<i) )
				num += '0' + (style == 4 ? 128 : 0);
            else
				num += '0' + (style == 4 ? 0 : 128);
            Draw_SCharacter(x, y, num, scale);
        }
        break;
    default:    // classic - pictures
        width  = scale * (wide ? 48 : 24);
        height = scale * 16;

        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;

        if ( HUD_Stats(STAT_ITEMS) & (IT_SHOTGUN<<i) )
        {
            float   time;
            int     flashon;

            time = cl.item_gettime[i];
            flashon = (int)((cl.time - time)*10);
            if (flashon < 0)
                flashon = 0;
            if (flashon >= 10)
            {
                if ( HUD_Stats(STAT_ACTIVEWEAPON) == (IT_SHOTGUN<<i) )
                    flashon = 1;
                else
                    flashon = 0;
            }
            else
                flashon = (flashon%5) + 2;

            if (wide  ||  num != 8)
                Draw_SPic (x, y, sb_weapons[flashon][i], scale);
            else
                Draw_SSubPic (x, y, sb_weapons[flashon][i], 0, 0, 24, 16, scale);
        }
        break;
    }
}

void SCR_HUD_DrawGun2 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time callse
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 2, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun3 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 3, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun4 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 4, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun5 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 5, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun6 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 6, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun7 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawGunByNum (hud, 7, scale->value, style->value, 0);
}
void SCR_HUD_DrawGun8 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *wide;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
        wide  = HUD_FindVar(hud, "wide");
    }
    SCR_HUD_DrawGunByNum (hud, 8, scale->value, style->value, wide->value);
}
void SCR_HUD_DrawGunCurrent (hud_t *hud)
{
    int gun;
    static cvar_t *scale = NULL, *style, *wide;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
        wide  = HUD_FindVar(hud, "wide");
    }

    switch (HUD_Stats(STAT_ACTIVEWEAPON))
    {
    case IT_SHOTGUN << 0:   gun = 2; break;
    case IT_SHOTGUN << 1:   gun = 3; break;
    case IT_SHOTGUN << 2:   gun = 4; break;
    case IT_SHOTGUN << 3:   gun = 5; break;
    case IT_SHOTGUN << 4:   gun = 6; break;
    case IT_SHOTGUN << 5:   gun = 7; break;
    case IT_SHOTGUN << 6:   gun = 8; break;
    default: return;
    }
    SCR_HUD_DrawGunByNum (hud, gun, scale->value, style->value, wide->value);
}

// ----------------
// powerzz
//
void SCR_HUD_DrawPowerup(hud_t *hud, int num, float scale, int style)
{
    extern mpic_t *sb_items[32];
    int    x, y, width, height;
    int    c;

    scale = max(scale, 0.01);

    switch (style)
    {
    case 1:     // letter
        width = height = 8 * scale;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(17+num)))
        {
            switch (num)
            {
            case 0: c = '1'; break;
            case 1: c = '2'; break;
            case 2: c = 'r'; break;
            case 3: c = 'p'; break;
            case 4: c = 's'; break;
            case 5: c = 'q'; break;
            default: c = '?';
            }
            Draw_SCharacter(x, y, c, scale);
        }
        break;
    default:    // classic - pics
        width = height = scale * 16;
        if (!HUD_PrepareDraw(hud, width, height, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(17+num)))
            Draw_SPic (x, y, sb_items[num], scale);
        break;
    }
}

void SCR_HUD_DrawKey1(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 0, scale->value, style->value);
}
void SCR_HUD_DrawKey2(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 1, scale->value, style->value);
}
void SCR_HUD_DrawRing(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 2, scale->value, style->value);
}
void SCR_HUD_DrawPent(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 3, scale->value, style->value);
}
void SCR_HUD_DrawSuit(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 4, scale->value, style->value);
}
void SCR_HUD_DrawQuad(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawPowerup(hud, 5, scale->value, style->value);
}

// -----------
// sigils
//
void SCR_HUD_DrawSigil(hud_t *hud, int num, float scale, int style)
{
    extern mpic_t *sb_sigil[4];
    int     x, y;

    scale = max(scale, 0.01);

    switch (style)
    {
    case 1:     // sigil number
        if (!HUD_PrepareDraw(hud, 8*scale, 8*scale, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(28+num)))
            Draw_SCharacter(x, y, num + '0', scale);
        break;
    default:    // classic - picture
        if (!HUD_PrepareDraw(hud, 8*scale, 16*scale, &x, &y))
            return;
        if (HUD_Stats(STAT_ITEMS) & (1<<(28+num)))
            Draw_SPic(x, y, sb_sigil[num], scale);
        break;
    }
}

void SCR_HUD_DrawSigil1(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 0, scale->value, style->value);
}
void SCR_HUD_DrawSigil2(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 1, scale->value, style->value);
}
void SCR_HUD_DrawSigil3(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 2, scale->value, style->value);
}
void SCR_HUD_DrawSigil4(hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawSigil(hud, 3, scale->value, style->value);
}

// icons - active ammo, armor, face etc..
void SCR_HUD_DrawAmmoIcon(hud_t *hud, int num, float scale, int style)
{
    extern mpic_t *sb_ammo[4];
    int   x, y, width, height;

    scale = max(scale, 0.01);

    width = height = (style ? 8 : 24) * scale;

    if (!HUD_PrepareDraw(hud, width, height, &x, &y))
        return;

    if (style)
    {
        switch (num)
        {
        case 1: Draw_SAlt_String(x, y, "s", scale); break;
        case 2: Draw_SAlt_String(x, y, "n", scale); break;
        case 3: Draw_SAlt_String(x, y, "r", scale); break;
        case 4: Draw_SAlt_String(x, y, "c", scale); break;
        }
    }
    else
    {
        Draw_SPic (x, y, sb_ammo[num-1], scale);
    }
}
void SCR_HUD_DrawAmmoIconCurrent (hud_t *hud)
{
    int num;
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    if (HUD_Stats(STAT_ITEMS) & IT_SHELLS)
        num = 1;
    else if (HUD_Stats(STAT_ITEMS) & IT_NAILS)
        num = 2;
    else if (HUD_Stats(STAT_ITEMS) & IT_ROCKETS)
        num = 3;
    else if (HUD_Stats(STAT_ITEMS) & IT_CELLS)
        num = 4;
    else
        return;
    SCR_HUD_DrawAmmoIcon(hud, num, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon1 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 1, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon2 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 2, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon3 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 3, scale->value, style->value);
}
void SCR_HUD_DrawAmmoIcon4 (hud_t *hud)
{
    static cvar_t *scale = NULL, *style;
    if (scale == NULL)  // first time called
    {
        scale = HUD_FindVar(hud, "scale");
        style = HUD_FindVar(hud, "style");
    }
    SCR_HUD_DrawAmmoIcon(hud, 4, scale->value, style->value);
}

void SCR_HUD_DrawArmorIcon(hud_t *hud)
{
    extern mpic_t  *sb_armor[3];
    extern mpic_t  *draw_disc;
    int   x, y, width, height;

    int style;
    float scale;

    static cvar_t *v_scale = NULL, *v_style;
    if (v_scale == NULL)  // first time called
    {
        v_scale = HUD_FindVar(hud, "scale");
        v_style = HUD_FindVar(hud, "style");
    }

    scale = max(v_scale->value, 0.01);
    style = (int)(v_style->value);

    width = height = (style ? 8 : 24) * scale;

    if (!HUD_PrepareDraw(hud, width, height, &x, &y))
        return;

    if (style)
    {
        int c;

        if (HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY)
            c = '@';
        else  if (HUD_Stats(STAT_ITEMS) & IT_ARMOR3)
            c = 'r';
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR2)
            c = 'y';
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR1)
            c = 'g';
        else return;

        c += 128;

        Draw_SCharacter(x, y, c, scale);
    }
    else
    {
        mpic_t  *pic;

        if (HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY)
            pic = draw_disc;
        else  if (HUD_Stats(STAT_ITEMS) & IT_ARMOR3)
            pic = sb_armor[2];
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR2)
            pic = sb_armor[1];
        else if (HUD_Stats(STAT_ITEMS) & IT_ARMOR1)
            pic = sb_armor[0];
        else return;

        Draw_SPic (x, y, pic, scale);
    }
}

// face
void SCR_HUD_DrawFace(hud_t *hud)
{
    extern mpic_t  *sb_faces[7][2]; // 0 is gibbed, 1 is dead, 2-6 are alive
                                    // 0 is static, 1 is temporary animation
    extern mpic_t  *sb_face_invis;
    extern mpic_t  *sb_face_quad;
    extern mpic_t  *sb_face_invuln;
    extern mpic_t  *sb_face_invis_invuln;

    int     f, anim;
    int     x, y;
    float   scale;

    static cvar_t *v_scale = NULL;
    if (v_scale == NULL)  // first time called
    {
        v_scale = HUD_FindVar(hud, "scale");
    }

    scale = max(v_scale->value, 0.01);

    if (!HUD_PrepareDraw(hud, 24*scale, 24*scale, &x, &y))
        return;

    if ( (HUD_Stats(STAT_ITEMS) & (IT_INVISIBILITY | IT_INVULNERABILITY) )
    == (IT_INVISIBILITY | IT_INVULNERABILITY) )
    {
        Draw_SPic (x, y, sb_face_invis_invuln, scale);
        return;
    }
    if (HUD_Stats(STAT_ITEMS) & IT_QUAD)
    {
        Draw_SPic (x, y, sb_face_quad, scale);
        return;
    }
    if (HUD_Stats(STAT_ITEMS) & IT_INVISIBILITY)
    {
        Draw_SPic (x, y, sb_face_invis, scale);
        return;
    }
    if (HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY)
    {
        Draw_SPic (x, y, sb_face_invuln, scale);
        return;
    }

    if (HUD_Stats(STAT_HEALTH) >= 100)
        f = 4;
    else
        f = max(0, HUD_Stats(STAT_HEALTH)) / 20;

    if (cl.time <= cl.faceanimtime)
        anim = 1;
    else
        anim = 0;
    Draw_SPic (x, y, sb_faces[f][anim], scale);
}


// status numbers
void SCR_HUD_DrawNum(hud_t *hud, int num, qbool low,
                     float scale, int style, int digits, char *s_align)
{
    extern mpic_t *sb_nums[2][11];

    int  i;
    char buf[8], *t;
    int  len;

    int width, height, x, y;
    int size;
    int align;

    clamp(num, -9999, 99999);

    scale = max(scale, 0.01);

    clamp(digits, 1, 5);

    align = 2;
    switch (tolower(s_align[0]))
    {
    default:
    case 'l':   // 'l'eft
        align = 0; break;
    case 'c':   // 'c'enter
        align = 1; break;
    case 'r':   // 'r'ight
        align = 2; break;
    }

    sprintf(buf, "%d", num);
    len = strlen(buf);
    if (len > digits)
    {
        t = buf;
        for (i=0; i < digits; i++)
            *t++ = '9';
        *t = 0;
        len = strlen(buf);
    }

    switch (style)
    {
    case 1:
        size = 8;
        break;
    default:
        size = 24;
        break;
    }

    width = digits * size;
    height = size;

    switch (style)
    {
    case 1:
        if (!HUD_PrepareDraw(hud, scale*width, scale*height, &x, &y))
            return;
        switch (align)
        {
        case 0: break;
        case 1: x += scale * (width - size * len) / 2; break;
        case 2: x += scale * (width - size * len); break;
        }
        if (low)
            Draw_SAlt_String(x, y, buf, scale);
        else
            Draw_SString(x, y, buf, scale);
        break;
    default:
        if (!HUD_PrepareDraw(hud, scale*width, scale*height, &x, &y))
            return;
        switch (align)
        {
        case 0: break;
        case 1: x += scale * (width - size * len) / 2; break;
        case 2: x += scale * (width - size * len); break;
        }
        for (i=0; i < len; i++)
        {
            if (low)
				if(buf[i] == '-') {
					Draw_STransPic (x, y, sb_nums[1][STAT_MINUS], scale);
				} else {
					Draw_STransPic (x, y, sb_nums[1][buf[i] - '0'], scale);
				}
            else
                Draw_STransPic (x, y, sb_nums[0][buf[i] - '0'], scale);
            x += 24 * scale;
        }
        break;
    }
}

void SCR_HUD_DrawHealth(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
	static int value;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
	value = HUD_Stats(STAT_HEALTH);
    SCR_HUD_DrawNum(hud, (value < 0 ? 0 : value), HUD_HealthLow(),
        scale->value, style->value, digits->value, align->string);
}

void SCR_HUD_DrawArmor(hud_t *hud)
{
    int level;
    qbool low;
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }

    if (HUD_Stats(STAT_HEALTH) > 0)
    {
        if (HUD_Stats(STAT_ITEMS) & IT_INVULNERABILITY)
        {
            level = 666;
            low = true;
        }
        else
        {
            level = HUD_Stats(STAT_ARMOR);
            low = HUD_ArmorLow();
        }
    }
    else
    {
        level = 0;
        low = true;
    }

    SCR_HUD_DrawNum(hud, level, low,
        scale->value, style->value, digits->value, align->string);
}

#ifdef GLQUAKE
void SCR_HUD_DrawHealthDamage(hud_t *hud)
{
	Draw_AMFStatLoss (STAT_HEALTH, hud);
	if (HUD_Stats(STAT_HEALTH) <= 0)
	{
		Amf_Reset_DamageStats();
	}
}

void SCR_HUD_DrawArmorDamage(hud_t *hud)
{
	Draw_AMFStatLoss (STAT_ARMOR, hud);
}
#endif

void SCR_HUD_DrawAmmo(hud_t *hud, int num,
                      float scale, int style, int digits, char *s_align)
{
    extern mpic_t *sb_ibar;
    int value;
    qbool low;

    if (num < 1  ||  num > 4)
    {
        if (HUD_Stats(STAT_ITEMS) & IT_SHELLS)
            num = 1;
        else if (HUD_Stats(STAT_ITEMS) & IT_NAILS)
            num = 2;
        else if (HUD_Stats(STAT_ITEMS) & IT_ROCKETS)
            num = 3;
        else if (HUD_Stats(STAT_ITEMS) & IT_CELLS)
            num = 4;
        else
            return;
    }
    low = HUD_AmmoLowByWeapon(num * 2);
    value = HUD_Stats(STAT_SHELLS + num - 1);

    if (style < 2)
    {
        // simply draw number
        SCR_HUD_DrawNum(hud, value, low,
        scale, style, digits, s_align);
    }
    else
    {
        // else - draw classic ammo-count box with background
        char buf[8];
        int  x, y;

        scale = max(scale, 0.01);

        if (!HUD_PrepareDraw(hud, 42*scale, 11*scale, &x, &y))
            return;

        sprintf (buf, "%3i", value);
        Draw_SSubPic(x, y, sb_ibar, 3+((num-1)*48), 0, 42, 11, scale);
        if (buf[0] != ' ')  Draw_SCharacter (x +  7*scale, y, 18+buf[0]-'0', scale);
        if (buf[1] != ' ')  Draw_SCharacter (x + 15*scale, y, 18+buf[1]-'0', scale);
        if (buf[2] != ' ')  Draw_SCharacter (x + 23*scale, y, 18+buf[2]-'0', scale);
    }
}

void SCR_HUD_DrawAmmoCurrent(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 0, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo1(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 1, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo2(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 2, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo3(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 3, scale->value, style->value, digits->value, align->string);
}
void SCR_HUD_DrawAmmo4(hud_t *hud)
{
    static cvar_t *scale = NULL, *style, *digits, *align;
    if (scale == NULL)  // first time called
    {
        scale  = HUD_FindVar(hud, "scale");
        style  = HUD_FindVar(hud, "style");
        digits = HUD_FindVar(hud, "digits");
        align  = HUD_FindVar(hud, "align");
    }
    SCR_HUD_DrawAmmo(hud, 4, scale->value, style->value, digits->value, align->string);
}

// ============================================================================0
// Groups
// ============================================================================0

mpic_t hud_pic_group1;
mpic_t hud_pic_group2;
mpic_t hud_pic_group3;
mpic_t hud_pic_group4;
mpic_t hud_pic_group5;
mpic_t hud_pic_group6;
mpic_t hud_pic_group7;
mpic_t hud_pic_group8;
mpic_t hud_pic_group9;

void SCR_HUD_DrawGroup(hud_t *hud, int width, int height, mpic_t *pic, int pic_scalemode, float pic_alpha)
{
	#define HUD_GROUP_SCALEMODE_TILE		1
	#define HUD_GROUP_SCALEMODE_STRETCH		2
	#define HUD_GROUP_SCALEMODE_GROW		3
	#define HUD_GROUP_SCALEMODE_CENTER		4

	int x, y;
	
	clamp(width, 1, 99999);
    clamp(height, 1, 99999);

	// Set it to this, because 1.0 will make the colors
	// completly saturated, and no semi-transparency will show.
	pic_alpha = (pic_alpha) >= 1.0 ? 0.99 : pic_alpha;

	// Grow the group if necessary.
	if (pic_scalemode == HUD_GROUP_SCALEMODE_GROW 
		&& pic != NULL && pic->height > 0)
	{
		width = max(pic->width, width);
		height = max(pic->height, height);
	}

	if (!HUD_PrepareDraw(hud, width, height, &x, &y))
	{
        return;
	}

	#ifdef GLQUAKE
    // Draw the picture if it's set.
	if (pic != NULL && pic->height > 0)
    {
        int pw, ph;

		if (pic_scalemode == HUD_GROUP_SCALEMODE_TILE)
        {
            // Tile.
            int cx = 0, cy = 0;
            while (cy < height)
            {
                while (cx < width)
                {
                    pw = min(pic->width, width - cx);
                    ph = min(pic->height, height - cy);

                    if (pw >= pic->width  &&  ph >= pic->height)
					{
                        Draw_AlphaPic (x + cx , y + cy, pic, pic_alpha);
					}
                    else
					{
                        Draw_AlphaSubPic (x + cx, y + cy, pic, 0, 0, pw, ph, pic_alpha);
					}

                    cx += pic->width;
                }

                cx = 0;
                cy += pic->height;
            }
        }
		else if (pic_scalemode == HUD_GROUP_SCALEMODE_STRETCH)
		{
			// Stretch or shrink the picture to fit.
			float scale_x = (float)width / pic->width;
			float scale_y = (float)height / pic->height;

			Draw_SAlphaSubPic2 (x, y, pic, 0, 0, pic->width, pic->height, scale_x, scale_y, pic_alpha);
		}
		else if (pic_scalemode == HUD_GROUP_SCALEMODE_CENTER)
		{
			// Center the picture in the group.
			int pic_x = x + (width - pic->width) / 2;
			int pic_y = y + (height - pic->height) / 2;

			int src_x = 0;
			int src_y = 0;

			if(x > pic_x)
			{
				src_x = x - pic_x;
				pic_x = x;
			}

			if(y > pic_y)
			{
				src_y = y - pic_y;
				pic_y = y;
			}

			Draw_AlphaSubPic (pic_x, pic_y,	pic, src_x, src_y, min(width, pic->width), min(height, pic->height), pic_alpha);
		}
		else
        {
			// Normal. Draw in the top left corner.
			Draw_AlphaSubPic (x, y, pic, 0, 0, min(width, pic->width), min(height, pic->height), pic_alpha);
        }
    }
	#endif
}

qbool SCR_HUD_LoadGroupPic(cvar_t *var, mpic_t *hud_pic, char *newpic)
{
#ifdef GLQUAKE
	mpic_t *temp_pic;

	// If we have no pic name.
	if(!newpic)
	{
		hud_pic->height = -1;
		return false;
	}

	// Try loading the pic.
	if (!(temp_pic = GL_LoadPicImage(va("gfx/%s", newpic), newpic, 0, 0, TEX_ALPHA)))
	{
		hud_pic->height = -1;
		Com_Printf("Couldn't load picture %s for hud group.\n", newpic);
		return false;
	}

	// Save the pic.
	(*hud_pic) = *temp_pic;

	strncpy(var->string, newpic, (strlen(newpic) + 1) * sizeof(char));
#endif
	return true;
}

qbool SCR_HUD_OnChangePic_Group1(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group1, newpic);
}

qbool SCR_HUD_OnChangePic_Group2(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group2, newpic);
}

qbool SCR_HUD_OnChangePic_Group3(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group3, newpic);
}

qbool SCR_HUD_OnChangePic_Group4(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group4, newpic);
}

qbool SCR_HUD_OnChangePic_Group5(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group5, newpic);
}

qbool SCR_HUD_OnChangePic_Group6(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group6, newpic);
}

qbool SCR_HUD_OnChangePic_Group7(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group7, newpic);
}

qbool SCR_HUD_OnChangePic_Group8(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group8, newpic);
}

qbool SCR_HUD_OnChangePic_Group9(cvar_t *var, char *newpic)
{
	return SCR_HUD_LoadGroupPic(var, &hud_pic_group9, newpic);
}

void SCR_HUD_Group1(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width				= HUD_FindVar(hud, "width");
        height				= HUD_FindVar(hud, "height");
        picture				= HUD_FindVar(hud, "picture");
		pic_alpha			= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode		= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group1;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group1, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group2(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group2;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group2, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group3(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group3;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group3, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group4(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group4;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group4, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group5(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group5;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group5, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group6(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group6;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group6, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group7(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group7;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group7, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group8(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group8;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group8, 
		pic_scalemode->value, 
		pic_alpha->value);
}
void SCR_HUD_Group9(hud_t *hud)
{
    static cvar_t *width = NULL, 
		*height, 
		*picture,
		*pic_alpha,
		*pic_scalemode, 
		*alpha;

    if (width == NULL)  // first time called
    {
        width			= HUD_FindVar(hud, "width");
        height			= HUD_FindVar(hud, "height");
        picture			= HUD_FindVar(hud, "picture");
		pic_alpha		= HUD_FindVar(hud, "pic_alpha");
        pic_scalemode	= HUD_FindVar(hud, "pic_scalemode");

		picture->OnChange	= SCR_HUD_OnChangePic_Group9;
    }

	SCR_HUD_DrawGroup(hud, 
		width->value, 
		height->value, 
		&hud_pic_group9, 
		pic_scalemode->value, 
		pic_alpha->value);
}

// player sorting
// for frags and players
typedef struct sort_teams_info_s
{
    char *name;
    int  frags;
    int  min_ping;
    int  avg_ping;
    int  max_ping;
    int  nplayers;
    int  top, bottom;   // leader colours
    int  order;         // should not be here...
	int  rlcount;		// Number of RL's present in the team. (Cokeman 2006-05-27)
}
sort_teams_info_t;

typedef struct sort_players_info_s
{
    int playernum;
    sort_teams_info_t *team;
}
sort_players_info_t;

static sort_players_info_t sort_info_players[MAX_CLIENTS];
static sort_teams_info_t sort_info_teams[MAX_CLIENTS];
static sort_players_info_t *sorted_players[MAX_CLIENTS];
static sort_teams_info_t *sorted_teams[MAX_CLIENTS];
static int n_teams, n_players, n_spectators;

static qbool isTeamplay()
{
    int teamplay = atoi(Info_ValueForKey(cl.serverinfo, "teamplay"));
    if (!teamplay)
        return false;
    return true;
}

static int ComparePlayers(sort_players_info_t *p1, sort_players_info_t *p2, qbool byTeams)
{
    int d;
    player_info_t *i1 = &cl.players[p1->playernum];
    player_info_t *i2 = &cl.players[p2->playernum];

    if (i1->spectator  &&  !i2->spectator)
        d = -1;
    else if (!i1->spectator  &&  i2->spectator)
        d = 1;
    else if (i1->spectator  &&  i2->spectator)
    {
        d = strcmp(i1->name, i2->name);
    }
    else    // both are players
    {
        d = 0;

		if (byTeams)
            d = p1->team->frags - p2->team->frags;

		if (!d && byTeams)
			d = strcmp(p1->team->name, p2->team->name);

		if (!d)
			d =	i1->frags - i2->frags;

        if (!d)
            d = strcmp(i1->name, i2->name);
    }

    if (!d)
        d = p1->playernum - p2->playernum;

    return d;
}

static void Sort_Scoreboard(qbool teamsort)
{
    int i, j;
    qbool isteamplay;
    int team;

    n_teams = 0;
    n_players = 0;
    n_spectators = 0;
    isteamplay = isTeamplay();

    // Set team properties.
    for (i=0; i < MAX_CLIENTS; i++)
    {
        if (cl.players[i].name[0] && !cl.players[i].spectator)
        {
            // find players team
            for (team=0; team < n_teams; team++)
			{
                if (!strcmp(cl.players[i].team, sort_info_teams[team].name)
                    &&  sort_info_teams[team].name[0])
                    break;
			}

            if (team == n_teams)   // not found
            {
                team = n_teams++;
                sort_info_teams[team].avg_ping = 0;
                sort_info_teams[team].max_ping = 0;
                sort_info_teams[team].min_ping = 999;
                sort_info_teams[team].nplayers = 0;
				sort_info_teams[team].frags = 0;
				sort_info_teams[team].top = Sbar_TopColor(&cl.players[i]);
				sort_info_teams[team].bottom = Sbar_BottomColor(&cl.players[i]);
                sort_info_teams[team].name = cl.players[i].team;
				sort_info_teams[team].rlcount = 0; // Cokeman (2006-05-27)
                sorted_teams[team] = &sort_info_teams[team];
            }
            sort_info_teams[team].nplayers++;
            sort_info_teams[team].frags += cl.players[i].frags;
            sort_info_teams[team].avg_ping += cl.players[i].ping;
            sort_info_teams[team].min_ping = min(sort_info_teams[team].min_ping, cl.players[i].ping);
            sort_info_teams[team].max_ping = max(sort_info_teams[team].max_ping, cl.players[i].ping);

			// Cokeman (2006-05-27)
			if(cl.players[i].stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER)
			{
				sort_info_teams[team].rlcount++;
			}

            // set player data
            sort_info_players[n_players+n_spectators].playernum = i;
            sort_info_players[n_players+n_spectators].team = &sort_info_teams[team];
            sorted_players[n_players+n_spectators] = &sort_info_players[n_players+n_spectators];
            if (cl.players[i].spectator)
                n_spectators++;
            else
                n_players++;
        }
    }

    // calc avg ping
    for (team = 0; team < n_teams; team++)
        sort_info_teams[team].avg_ping /= sort_info_teams[team].nplayers;

    // sort teams
    for (i=0; i < n_teams - 1; i++)
        for (j = i + 1; j < n_teams; j++)
            if (sorted_teams[i]->frags < sorted_teams[j]->frags)
            {
                sort_teams_info_t *k = sorted_teams[i];
                sorted_teams[i] = sorted_teams[j];
                sorted_teams[j] = k;
            }

    // order teams - blah
    for (i=0; i < n_teams; i++)
        sorted_teams[i]->order = i;

    // sort players
    for (i=0; i < n_players+n_spectators - 1; i++)
        for (j = i + 1; j < n_players+n_spectators; j++)
            if (ComparePlayers(sorted_players[i], sorted_players[j], (isteamplay && teamsort)) < 0)
            {
                sort_players_info_t *k = sorted_players[i];
                sorted_players[i] = sorted_players[j];
                sorted_players[j] = k;
            }
}

void Frags_DrawColors(int x, int y, int width, int height, int top_color, int bottom_color, int frags, int drawBrackets, int style)
{
	char buf[32];
	int posy = 0;
	Draw_Fill(x, y, width, height/2, top_color);
    Draw_Fill(x, y + height/2, width, height - height/2, bottom_color);

    sprintf(buf, "%3d", frags);
    posy = y+(height-8)/2;
    Draw_String(x-2+(width-8*strlen(buf)-2)/2, posy, buf);

	if(drawBrackets)
    {
        int d = width >= 32 ? 0 : 1;
		switch(style)
		{
			case 1 :
				Draw_Character(x-8, posy, 13);
				break;
			case 2 :
				Draw_Fill(x, y-1, width, 1, 0x4f);
				Draw_Fill(x, y-1, 1, height+2, 0x4f);
				Draw_Fill(x+width-1, y-1, 1, height+2, 0x4f);
				Draw_Fill(x, y+height, width, 1, 0x4f);
				break;
			case 0 :
			default :
				Draw_Character(x-2-2*d, posy, 16);
				Draw_Character(x+width-8+1+d, posy, 17);
				break;
		}
    }
}

#define	FRAGS_HEALTHBAR_WIDTH			5

#define FRAGS_HEALTHBAR_NORMAL_COLOR	75
#define FRAGS_HEALTHBAR_MEGA_COLOR		251
#define	FRAGS_HEALTHBAR_TWO_MEGA_COLOR	238
#define	FRAGS_HEALTHBAR_UNNATURAL_COLOR	144

void Frags_DrawHealthBar(int original_health, int x, int y, int height, int width)
{
	float health_height = 0.0;
	int health;

	// Get the health.
	health = original_health;
	health = min(100, health);

	// Draw a health bar.
	health_height = ROUND((height / 100.0) * health);
	health_height = (health_height > 0.0 && health_height < 1.0) ? 1 : health_height;
	health_height = (health_height < 0.0) ? 0.0 : health_height;
	Draw_Fill(x, y + height - (int)health_height, 3, (int)health_height, FRAGS_HEALTHBAR_NORMAL_COLOR);

	// Get the health again to check if health is more than 100.
	health = original_health;
	if(health > 100 && health <= 200)
	{
		health_height = (int)ROUND((height / 100.0) * (health - 100));
		Draw_Fill(x, y + height - health_height, width, health_height, FRAGS_HEALTHBAR_MEGA_COLOR);
	}
	else if(health > 200 && health <= 250)
	{
		health_height = (int)ROUND((height / 100.0) * (health - 200));
		Draw_Fill(x, y, width, height, FRAGS_HEALTHBAR_MEGA_COLOR);
		Draw_Fill(x, y + height - health_height, width, health_height, FRAGS_HEALTHBAR_TWO_MEGA_COLOR);
	}
	else if(health > 250)
	{
		// This will never happen during a normal game.
		Draw_Fill(x, y, width, health_height, FRAGS_HEALTHBAR_UNNATURAL_COLOR);
	}
}

#define	TEAMFRAGS_EXTRA_SPEC_NONE	0
#define TEAMFRAGS_EXTRA_SPEC_BEFORE	1
#define	TEAMFRAGS_EXTRA_SPEC_ONTOP	2
#define TEAMFRAGS_EXTRA_SPEC_NOICON 3
#define TEAMFRAGS_EXTRA_SPEC_RLTEXT 4

int TeamFrags_DrawExtraSpecInfo(int num, int px, int py, int width, int height, int style)
{
	extern mpic_t *sb_weapons[7][8]; // sbar.c
	mpic_t rl_picture = *sb_weapons[0][5];

	// Only allow this for spectators.
	if (!(cls.demoplayback || cl.spectator)
		|| style > TEAMFRAGS_EXTRA_SPEC_RLTEXT
		|| style <= TEAMFRAGS_EXTRA_SPEC_NONE
		|| !style)
	{
		return px;
	}

	// Check if the team has any RL's.
	if(sorted_teams[num]->rlcount > 0)
	{
		int y_pos = py;

		//
		// Draw the RL + count depending on style.
		//

		if((style == TEAMFRAGS_EXTRA_SPEC_BEFORE || style == TEAMFRAGS_EXTRA_SPEC_NOICON)
			&& style != TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = ROUND(py + (height / 2.0) - 4);
			Draw_ColoredString(px, y_pos, va("%d", sorted_teams[num]->rlcount), 0);
			px += 8 + 1;
		}

		if(style != TEAMFRAGS_EXTRA_SPEC_NOICON && style != TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = ROUND(py + (height / 2.0) - (rl_picture.height / 2.0));

			Draw_SSubPic (px, y_pos, &rl_picture, 0, 0, rl_picture.width, rl_picture.height, 1);
			px += rl_picture.width + 1;
		}

		if(style == TEAMFRAGS_EXTRA_SPEC_ONTOP && style != TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = ROUND(py + (height / 2.0) - 4);
			Draw_ColoredString(px - 14, y_pos, va("%d", sorted_teams[num]->rlcount), 0);
		}

		if(style == TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			y_pos = ROUND(py + (height / 2.0) - 4);
			Draw_ColoredString(px, y_pos, va("&ce00RL&cfff%d", sorted_teams[num]->rlcount), 0);
			px += 8*3 + 1;
		}
	}
	else
	{
		// If the team has no RL's just pad with nothing.
		if(style == TEAMFRAGS_EXTRA_SPEC_BEFORE)
		{
			// Draw the rl count before the rl icon.
			px += rl_picture.width + 8 + 1 + 1;
		}
		else if(style == TEAMFRAGS_EXTRA_SPEC_ONTOP)
		{
			// Draw the rl count on top of the RL instead of infront.
			px += rl_picture.width + 1;
		}
		else if(style == TEAMFRAGS_EXTRA_SPEC_NOICON)
		{
			// Only draw the rl count.
			px += 8 + 1;
		}
		else if(style == TEAMFRAGS_EXTRA_SPEC_RLTEXT)
		{
			px += 8*3 + 1;
		}
	}

	return px;
}

#define FRAGS_EXTRA_DONT_SHOW				0
#define FRAGS_EXTRA_SPEC_ALL				1
#define FRAGS_EXTRA_SPEC_ALL_2				2
#define FRAGS_EXTRA_SPEC_NO_RL				3
#define FRAGS_EXTRA_SPEC_NO_ARMOR			4
#define FRAGS_EXTRA_SPEC_NO_ARMOR_2			5
#define FRAGS_EXTRA_SPEC_NO_HEALTH			6
#define FRAGS_EXTRA_SPEC_NO_HEALTH_2		7
#define	FRAGS_EXTRA_SPEC_NO_POWERUPS		8
#define	FRAGS_EXTRA_SPEC_NO_POWERUPS_2		9
#define FRAGS_EXTRA_SPEC_ONLY_POWERUPS		10
#define FRAGS_EXTRA_SPEC_ONLY_HEALTH		11
#define FRAGS_EXTRA_SPEC_ONLY_ARMOR			12
#define FRAGS_EXTRA_SPEC_ONLY_RL			13
#define FRAGS_EXTRA_SPEC_ONLY_RL_2			14

int Frags_DrawExtraSpecInfo(player_info_t *info,
							 int px, int py,
							 int cell_width, int cell_height,
							 int space_x, int space_y, int style, int flip)
{
	// Styles:
	// * means that the RL pic isn't show, just the text "RL".
	//
	// 0  = Show nothing
	// 1  = Show all
	// 2  = Show all *
	// 3  = Don't show RL's
	// 4  = Don't show armors
	// 5  = Don't show armors *
	// 6  = Don't show health
	// 7  = Don't show health *
	// 8  = Don't show powerups
	// 9  = Don't show powerups *
	// 10 = Only show powerups
	// 11 = Only show health
	// 12 = Only show armors
	// 13 = Only show RL's
	// 14 = Only show RL's *

	extern mpic_t *sb_weapons[7][8]; // sbar.c ... Used for displaying the RL.
	mpic_t rl_picture;				 // Picture of RL.

	float armor_height = 0.0;
//	float armor_width = 0.0;
	int armor = 0;
	int armor_bg_color = 0;
	float armor_bg_power = 0;
//	int spec_extra_health_w = 5;
	int health_spacing = 1;
	int weapon_width = 24;
	qbool show_rl_pic = true;

	// Only allow this for spectators.
	if (!(cls.demoplayback || cl.spectator) || style == FRAGS_EXTRA_DONT_SHOW)
	{
		return px;
	}

	show_rl_pic = !(style == FRAGS_EXTRA_SPEC_ALL_2
		|| style == FRAGS_EXTRA_SPEC_NO_ARMOR_2
		|| style == FRAGS_EXTRA_SPEC_NO_HEALTH_2
		|| style == FRAGS_EXTRA_SPEC_NO_POWERUPS_2
		|| style == FRAGS_EXTRA_SPEC_ONLY_RL_2);

	rl_picture = *sb_weapons[0][5];

	weapon_width = show_rl_pic ? rl_picture.width : 24;

	// Draw health bar.
	if(flip
		&& style != FRAGS_EXTRA_SPEC_NO_HEALTH
		&& style != FRAGS_EXTRA_SPEC_ONLY_ARMOR
		&& style != FRAGS_EXTRA_SPEC_ONLY_RL
		&& style != FRAGS_EXTRA_SPEC_ONLY_POWERUPS)
	{
		Frags_DrawHealthBar(info->stats[STAT_HEALTH], px, py, cell_height, 3);
		px += 3 + health_spacing;
	}

	armor = info->stats[STAT_ARMOR];

	// If the player has any armor, draw it in the appropriate color.
	if(info->stats[STAT_ITEMS] & IT_ARMOR1)
	{
		armor_bg_power = 100;
		armor_bg_color = 178; // Green armor.
	}
	else if(info->stats[STAT_ITEMS] & IT_ARMOR2)
	{
		armor_bg_power = 150;
		armor_bg_color = 111; // Yellow armor.
	}
	else if(info->stats[STAT_ITEMS] & IT_ARMOR3)
	{
		armor_bg_power = 200;
		armor_bg_color = 79; // Red armor.
	}

	// Only draw the armor if the current player has one and if the style allows it.
	if(armor_bg_power
		&& armor_bg_color
		&& style != FRAGS_EXTRA_SPEC_NO_ARMOR
		&& style != FRAGS_EXTRA_SPEC_ONLY_HEALTH
		&& style != FRAGS_EXTRA_SPEC_ONLY_RL
		&& style != FRAGS_EXTRA_SPEC_ONLY_POWERUPS)
	{
		armor_height = ROUND((cell_height / armor_bg_power) * armor);

#ifdef GLQUAKE
		Draw_AlphaFill(px,												// x
						py + cell_height - (int)armor_height,			// y (draw from bottom up)
						weapon_width,									// width
						(int)armor_height,								// height
						armor_bg_color,									// color
						0.3);											// alpha
#else
		Draw_Fill(px,
				py + cell_height - (int)armor_height,
				weapon_width,
				(int)armor_height,
				armor_bg_color);
#endif

		// Draws the armor bar vertically instead.
		/*armor_width = ROUND((rl_picture.width  / armor_bg_power) * armor);
		Draw_AlphaFill(px + rl_picture.width - (int)armor_width,		// x (Draw from right to left)
						py,												// y
						(int)armor_width,								// width
						(int)cell_height,								// height
						armor_bg_color,									// color
						0.3);											// alpha
		*/
	}

	// Draw the rl if the current player has it and the style allows it.
	if(info->stats[STAT_ITEMS] & IT_ROCKET_LAUNCHER
		&& style != FRAGS_EXTRA_SPEC_NO_RL
		&& style != FRAGS_EXTRA_SPEC_ONLY_HEALTH
		&& style != FRAGS_EXTRA_SPEC_ONLY_ARMOR
		&& style != FRAGS_EXTRA_SPEC_ONLY_POWERUPS)
	{
		if(show_rl_pic)
		{
			// Draw the rl-pic.
			Draw_SSubPic (px, py + ROUND((cell_height/2.0)) - (rl_picture.height/2.0), &rl_picture, 0, 0, rl_picture.width, rl_picture.height, 1);
		}
		else
		{
			// just print "RL" instead.
			Draw_String(px + 12 - 8, py + ROUND((cell_height/2.0)) - 4, "RL");
		}
	}

	// Only draw powerups is the current player has it and the style allows it.
	if(style != FRAGS_EXTRA_SPEC_NO_POWERUPS
		&& style != FRAGS_EXTRA_SPEC_ONLY_HEALTH
		&& style != FRAGS_EXTRA_SPEC_ONLY_ARMOR
		&& style != FRAGS_EXTRA_SPEC_ONLY_RL)
	{

		//float powerups_x = px + (spec_extra_weapon_w / 2.0);
		float powerups_x = px + (weapon_width / 2.0);

		if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY
			&& info->stats[STAT_ITEMS] & IT_INVISIBILITY
			&& info->stats[STAT_ITEMS] & IT_QUAD)
		{
			Draw_ColoredString(ROUND(powerups_x - 10), py, "&c0ffQ&cf00P&cff0R", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_QUAD
			&& info->stats[STAT_ITEMS] & IT_INVULNERABILITY)
		{
			Draw_ColoredString(ROUND(powerups_x - 8), py, "&c0ffQ&cf00P", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_QUAD
			&& info->stats[STAT_ITEMS] & IT_INVISIBILITY)
		{
			Draw_ColoredString(ROUND(powerups_x - 8), py, "&c0ffQ&cff0R", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY
			&& info->stats[STAT_ITEMS] & IT_INVISIBILITY)
		{
			Draw_ColoredString(ROUND(powerups_x - 8), py, "&cf00P&cff0R", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_QUAD)
		{
			Draw_ColoredString(ROUND(powerups_x - 4), py, "&c0ffQ", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY)
		{
			Draw_ColoredString(ROUND(powerups_x - 4), py, "&cf00P", 0);
		}
		else if(info->stats[STAT_ITEMS] & IT_INVISIBILITY)
		{
			Draw_ColoredString(ROUND(powerups_x - 4), py, "&cff0R", 0);
		}
	}

	px += weapon_width + health_spacing;

	// Draw health bar.
	if(!flip
		&& style != FRAGS_EXTRA_SPEC_NO_HEALTH
		&& style != FRAGS_EXTRA_SPEC_ONLY_ARMOR
		&& style != FRAGS_EXTRA_SPEC_ONLY_RL
		&& style != FRAGS_EXTRA_SPEC_ONLY_POWERUPS)
	{
		Frags_DrawHealthBar(info->stats[STAT_HEALTH], px, py, cell_height, 3);
		px += 3 + health_spacing;
	}

	return px;
}

void Frags_DrawBackground(int px, int py, int cell_width, int cell_height,
						  int space_x, int space_y, int max_name_length, int max_team_length,
						  int bg_color, int shownames, int showteams, int drawBrackets, int style)
{
	int bg_width = cell_width + space_x;
	//int bg_color = Sbar_BottomColor(info);
	float bg_alpha = 0.3;

	if(style == 4
		|| style == 6
		|| style == 8)
		bg_alpha = 0;

	if(shownames)
		bg_width += max_name_length*8 + space_x;

	if(showteams)
		bg_width += max_team_length*8 + space_x;

	if(drawBrackets)
		bg_alpha = 0.7;

	if(style == 7 || style == 8)
		bg_color = 0x4f;
#ifdef GLQUAKE
	Draw_AlphaFill(px-1, py-space_y/2, bg_width, cell_height+space_y, bg_color, bg_alpha);
#else
	Draw_Fill(px-1, py-space_y/2, bg_width, cell_height+space_y, bg_color);
#endif

	if(drawBrackets && (style == 5 || style == 6))
	{
		Draw_Fill(px-1, py-1-space_y/2, bg_width, 1, 0x4f);

		Draw_Fill(px-1, py-space_y/2, 1, cell_height+space_y, 0x4f);
		Draw_Fill(px+bg_width-1, py-1-space_y/2, 1, cell_height+1+space_y, 0x4f);

		Draw_Fill(px-1, py+cell_height+space_y/2, bg_width+1, 1, 0x4f);
	}
}

int Frags_DrawText(int px, int py,
					int cell_width, int cell_height,
					int space_x, int space_y,
					int max_name_length, int max_team_length,
					int flip, int pad,
					int shownames, int showteams,
					char* name, char* team)
{
	char _name[MAX_SCOREBOARDNAME + 1];
	char _team[MAX_SCOREBOARDNAME + 1];
	int team_length = 0;
	int name_length = 0;
	int y_pos;

	y_pos = ROUND(py + (cell_height / 2.0) - 4);

	// Draw team
	if(showteams && cl.teamplay)
	{
		strlcpy(_team, team, clamp(max_team_length, 0, sizeof(_team)));
		team_length = strlen(_team);

		if(!flip)
			px += space_x;

		if(pad && flip)
		{
			px += (max_team_length - team_length)*8;
			Draw_String(px, y_pos, _team);
			px += team_length*8;
		}
		else if(pad)
		{
			Draw_String(px, y_pos, _team);
			px += max_team_length*8;
		}
		else
		{
			Draw_String(px, y_pos, _team);
			px += team_length*8;
		}

		if(flip)
			px += space_x;
	}

	if(shownames)
	{
		// Draw name
		strlcpy(_name, name, clamp(max_name_length, 0, sizeof(_name)));
		name_length = strlen(_name);

		if(flip && pad)
		{
			px += (max_name_length - name_length)*8;
			Draw_String(px, y_pos, _name);
			px += name_length*8;
		}
		else if(pad)
		{
			Draw_String(px, y_pos, _name);
			px += max_name_length*8;
		}
		else
		{
			Draw_String(px, y_pos, _name);
			px += name_length*8;
		}

		px += space_x;
	}

	return px;
}

void SCR_HUD_DrawFrags(hud_t *hud)
{
    int width, height;
    int x, y;
	int max_team_length = 0;
	int max_name_length = 0;

    int rows, cols, cell_width, cell_height, space_x, space_y;
    int a_rows, a_cols; // actual

    static cvar_t
        *hud_frags_cell_width = NULL,
        *hud_frags_cell_height,
        *hud_frags_rows,
        *hud_frags_cols,
        *hud_frags_space_x,
        *hud_frags_space_y,
        *hud_frags_vertical,
        *hud_frags_strip,
        *hud_frags_teamsort,
		*hud_frags_shownames,
		*hud_frags_teams,
		*hud_frags_padtext,
		//*hud_frags_showself,
		*hud_frags_extra_spec,
		*hud_frags_fliptext,
		*hud_frags_style;

	extern mpic_t *sb_weapons[7][8]; // sbar.c ... Used for displaying the RL.
	mpic_t rl_picture;				 // Picture of RL.
	rl_picture = *sb_weapons[0][5];

    if (hud_frags_cell_width == NULL)    // first time
    {
        hud_frags_cell_width    = HUD_FindVar(hud, "cell_width");
        hud_frags_cell_height   = HUD_FindVar(hud, "cell_height");
        hud_frags_rows          = HUD_FindVar(hud, "rows");
        hud_frags_cols          = HUD_FindVar(hud, "cols");
        hud_frags_space_x       = HUD_FindVar(hud, "space_x");
        hud_frags_space_y       = HUD_FindVar(hud, "space_y");
        hud_frags_teamsort      = HUD_FindVar(hud, "teamsort");
        hud_frags_strip         = HUD_FindVar(hud, "strip");
        hud_frags_vertical      = HUD_FindVar(hud, "vertical");
		hud_frags_shownames		= HUD_FindVar(hud, "shownames");
		hud_frags_teams			= HUD_FindVar(hud, "showteams");
		hud_frags_padtext		= HUD_FindVar(hud, "padtext");
		//hud_frags_showself		= HUD_FindVar(hud, "showself_always");
		hud_frags_extra_spec	= HUD_FindVar(hud, "extra_spec_info");
		hud_frags_fliptext		= HUD_FindVar(hud, "fliptext");
		hud_frags_style			= HUD_FindVar(hud, "style");
    }

    rows = hud_frags_rows->value;
    clamp(rows, 1, MAX_CLIENTS);
    cols = hud_frags_cols->value;
    clamp(cols, 1, MAX_CLIENTS);
    cell_width = hud_frags_cell_width->value;
    clamp(cell_width, 28, 128);
    cell_height = hud_frags_cell_height->value;
    clamp(cell_height, 7, 32);
    space_x = hud_frags_space_x->value;
    clamp(space_x, 0, 128);
    space_y = hud_frags_space_y->value;
    clamp(space_y, 0, 128);

    Sort_Scoreboard(hud_frags_teamsort->value);

    if (hud_frags_strip->value)
    {
        if (hud_frags_vertical->value)
        {
            a_cols = min((n_players+rows-1) / rows, cols);
            a_rows = min(rows, n_players);
        }
        else
        {
            a_rows = min((n_players+cols-1) / cols, rows);
            a_cols = min(cols, n_players);
        }
    }
    else
    {
        a_rows = rows;
        a_cols = cols;
    }

    width  = a_cols*cell_width  + (a_cols+1)*space_x;
    height = a_rows*cell_height + (a_rows+1)*space_y;

	// Get the longest name/team name for padding.
	if(hud_frags_shownames->value || hud_frags_teams->value)
	{
		int cur_length = 0;
		int n;
		for(n=0; n < n_players; n++)
		{
			player_info_t *info = &cl.players[sorted_players[n]->playernum];
			cur_length = strlen(info->name);

			// Name
			if(cur_length >= max_name_length)
				max_name_length = cur_length + 1;

			cur_length = strlen(info->team);

			// Team name
			if(cur_length >= max_team_length)
				max_team_length = cur_length + 1;
		}

		// We need a wider box to draw in if we show the names.
		if(hud_frags_shownames->value)
			width += a_cols*max_name_length*8 + (a_cols+1)*space_x;

		if(cl.teamplay && hud_frags_teams->value)
			width += a_cols*max_team_length*8 + (a_cols+1)*space_x;
	}

	// Make room for the extra spectator stuff.
	if(hud_frags_extra_spec->value && (cls.demoplayback || cl.spectator) )
	{
		width += a_cols*(rl_picture.width + FRAGS_HEALTHBAR_WIDTH);
	}

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        int i = 0;
        int px = 0;
        int py = 0;
        int num = 0;
		int drawBrackets = 0;
		int limit = min(n_players, a_rows*a_cols);

		// Always show my current frags (don't just show the leaders).
		/*if(hud_frags_showself->value)
		{
			// Find my position in the scoreboard.
			for(i=0; i < n_players; i++)
			{
				if (cls.demoplayback || cl.spectator)
				{
					if (spec_track == sorted_players[i]->playernum)
						break;
				}
				else if(sorted_players[i]->playernum == cl.playernum)
					break;
			}

			if(i + 1 <= a_rows*a_cols)
				num = 0; // If I'm not "outside" the shown frags, start drawing from the top.
			else
				num = abs(a_rows*a_cols - (i+1)); // Always include me in the shown frags.
		}*/

		num = 0;  // FIXME! johnnycz; (see fixme below)
        for (i = 0; i < limit; i++)
        {
            player_info_t *info = &cl.players[sorted_players[num]->playernum]; // FIXME! johnnycz; causes crashed on some demos

            if (hud_frags_vertical->value)
            {
                if (i % a_rows == 0)
                {
					if(hud_frags_shownames->value && hud_frags_teams->value)
						px = x + space_x + (i/a_rows) * (cell_width+space_x + (max_name_length + max_team_length)*8);
					else if(hud_frags_shownames->value)
						px = x + space_x + (i/a_rows) * (cell_width+space_x + (max_name_length)*8);
					else if(hud_frags_teams->value)
						px = x + space_x + (i/a_rows) * (cell_width+space_x + (max_team_length)*8);
					else
						px = x + space_x + (i/a_rows) * (cell_width+space_x);
                    py = y + space_y;
                }
            }
            else
            {
                if (i % a_cols == 0)
                {
                    px = x + space_x;
                    py = y + space_y + (i/a_cols) * (cell_height+space_y);
                }
            }

			drawBrackets = 0;

			// Bug fix. Before the wrong player would be higlighted
			// during qwd-playback, since you ARE the player that you're
			// being spectated.
			if(cls.demoplayback && !cl.spectator && !cls.mvdplayback)
			{
				drawBrackets = (sorted_players[num]->playernum == cl.playernum);
			}
			else if (cls.demoplayback || cl.spectator)
			{
				drawBrackets = (spec_track == sorted_players[num]->playernum && Cam_TrackNum() >= 0);
			}
			else
			{
				drawBrackets = (sorted_players[num]->playernum == cl.playernum);
			}

			if (cl_multiview.value && cls.demoplayback)
			{				
				// Does not work for some reason I can't grasp :/
				//drawBrackets = (sorted_players[num]->playernum == nPlayernum);

				drawBrackets = 0;
			}

			if(hud_frags_shownames->value || hud_frags_teams->value || hud_frags_extra_spec->value)
			{
				int _px = px;

				if(hud_frags_style->value >= 4 && hud_frags_style->value <= 8)
				{
					Frags_DrawBackground(px, py, cell_width, cell_height, space_x, space_y,
						max_name_length, max_team_length, Sbar_BottomColor(info),
						hud_frags_shownames->value, hud_frags_teams->value, drawBrackets,
						hud_frags_style->value);
				}

				if(hud_frags_fliptext->value)
				{
					//
					// Flip the text
					// NAME | TEAM | FRAGS | EXTRA_SPEC_INFO
					//

					// Draw name.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						hud_frags_shownames->value, 0,
						info->name, info->team);

					// Draw team.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						0, hud_frags_teams->value,
						info->name, info->team);

					Frags_DrawColors(_px, py, cell_width, cell_height,
						Sbar_TopColor(info), Sbar_BottomColor(info),
						info->frags,
						drawBrackets,
						hud_frags_style->value);

					_px += cell_width + space_x;

					// Show extra information about all the players if spectating:
					// - What armor they have.
					// - How much health.
					// - If they have RL or not.
					_px = Frags_DrawExtraSpecInfo(info, _px, py, cell_width, cell_height,
							 space_x, space_y,
							 hud_frags_extra_spec->value,
							 hud_frags_fliptext->value);

				}
				else
				{
					//
					// Don't flip the text
					// EXTRA_SPEC_INFO | FRAGS | TEAM | NAME
					//

					_px = Frags_DrawExtraSpecInfo(info, _px, py, cell_width, cell_height,
							 space_x, space_y,
							 hud_frags_extra_spec->value,
							 hud_frags_fliptext->value);

					Frags_DrawColors(_px, py, cell_width, cell_height,
						Sbar_TopColor(info), Sbar_BottomColor(info),
						info->frags,
						drawBrackets,
						hud_frags_style->value);

					_px += cell_width + space_x;

					// Draw team.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						0, hud_frags_teams->value,
						info->name, info->team);

					// Draw name.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, max_name_length, max_team_length,
						hud_frags_fliptext->value, hud_frags_padtext->value,
						hud_frags_shownames->value, 0,
						info->name, info->team);
				}

				if(hud_frags_vertical->value)
					py += cell_height + space_y;
				else
					px = _px + space_x;
			}
			else
			{
				Frags_DrawColors(px, py, cell_width, cell_height,
					Sbar_TopColor(info), Sbar_BottomColor(info),
					info->frags,
					drawBrackets,
					hud_frags_style->value);

				if (hud_frags_vertical->value)
					py += cell_height + space_y;
				else
					px += cell_width + space_x;
			}
            num ++;
        }
    }
}

void SCR_HUD_DrawTeamFrags(hud_t *hud)
{
    int width, height;
    int x, y;
	int max_team_length = 0, num = 0;
    int rows, cols, cell_width, cell_height, space_x, space_y;
    int a_rows, a_cols; // actual

    static cvar_t
        *hud_teamfrags_cell_width,
        *hud_teamfrags_cell_height,
        *hud_teamfrags_rows,
        *hud_teamfrags_cols,
        *hud_teamfrags_space_x,
        *hud_teamfrags_space_y,
        *hud_teamfrags_vertical,
        *hud_teamfrags_strip,
		*hud_teamfrags_shownames,
		*hud_teamfrags_fliptext,
		*hud_teamfrags_padtext,
		*hud_teamfrags_style,
		*hud_teamfrags_extra_spec,
		*hud_teamfrags_onlytp;

	extern mpic_t *sb_weapons[7][8]; // sbar.c
	mpic_t rl_picture = *sb_weapons[0][5];

    if (hud_teamfrags_cell_width == 0)    // first time
    {
        hud_teamfrags_cell_width    = HUD_FindVar(hud, "cell_width");
        hud_teamfrags_cell_height   = HUD_FindVar(hud, "cell_height");
        hud_teamfrags_rows          = HUD_FindVar(hud, "rows");
        hud_teamfrags_cols          = HUD_FindVar(hud, "cols");
        hud_teamfrags_space_x       = HUD_FindVar(hud, "space_x");
        hud_teamfrags_space_y       = HUD_FindVar(hud, "space_y");
        hud_teamfrags_strip         = HUD_FindVar(hud, "strip");
        hud_teamfrags_vertical      = HUD_FindVar(hud, "vertical");
		hud_teamfrags_shownames		= HUD_FindVar(hud, "shownames");
		hud_teamfrags_fliptext		= HUD_FindVar(hud, "fliptext");
		hud_teamfrags_padtext		= HUD_FindVar(hud, "padtext");
		hud_teamfrags_style			= HUD_FindVar(hud, "style");
		hud_teamfrags_extra_spec	= HUD_FindVar(hud, "extra_spec_info");
		hud_teamfrags_onlytp		= HUD_FindVar(hud, "onlytp");
    }

	// Don't draw the frags if we're note in teamplay.
	if(hud_teamfrags_onlytp->value && !cl.teamplay)
	{
		return;
	}

    rows = hud_teamfrags_rows->value;
    clamp(rows, 1, MAX_CLIENTS);
    cols = hud_teamfrags_cols->value;
    clamp(cols, 1, MAX_CLIENTS);
    cell_width = hud_teamfrags_cell_width->value;
    clamp(cell_width, 28, 128);
    cell_height = hud_teamfrags_cell_height->value;
    clamp(cell_height, 7, 32);
    space_x = hud_teamfrags_space_x->value;
    clamp(space_x, 0, 128);
    space_y = hud_teamfrags_space_y->value;
    clamp(space_y, 0, 128);

    Sort_Scoreboard(1);

    if (hud_teamfrags_strip->value)
    {
        if (hud_teamfrags_vertical->value)
        {
            a_cols = min((n_teams+rows-1) / rows, cols);
            a_rows = min(rows, n_teams);
        }
        else
        {
            a_rows = min((n_teams+cols-1) / cols, rows);
            a_cols = min(cols, n_teams);
        }
    }
    else
    {
        a_rows = rows;
        a_cols = cols;
    }

    width  = a_cols*cell_width  + (a_cols+1)*space_x;
    height = a_rows*cell_height + (a_rows+1)*space_y;

	// Get the longest team name for padding.
	if(hud_teamfrags_shownames->value || hud_teamfrags_extra_spec->value)
	{
		int rlcount_width = 0;

		int cur_length = 0;
		int n;

		for(n=0; n < n_teams; n++)
		{
			if(hud_teamfrags_shownames->value)
			{
				cur_length = strlen(sorted_teams[n]->name);

				// Team name
				if(cur_length >= max_team_length)
				{
					max_team_length = cur_length + 1;
				}
			}
		}

		// Calculate the length of the extra spec info.
		if(hud_teamfrags_extra_spec->value && (cls.demoplayback || cl.spectator))
		{
			if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_BEFORE)
			{
				// Draw the rl count before the rl icon.
				rlcount_width = rl_picture.width + 8 + 1 + 1;
			}
			else if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_ONTOP)
			{
				// Draw the rl count on top of the RL instead of infront.
				rlcount_width = rl_picture.width + 1;
			}
			else if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_NOICON)
			{
				// Only draw the rl count.
				rlcount_width = 8 + 1;
			}
			else if(hud_teamfrags_extra_spec->value == TEAMFRAGS_EXTRA_SPEC_RLTEXT)
			{
				rlcount_width = 8*3 + 1;
			}
		}

		width += a_cols*max_team_length*8 + (a_cols+1)*space_x + a_cols*rlcount_width;
	}

    if (HUD_PrepareDraw(hud, width, height, &x, &y))
    {
        int i;
        int px = 0;
        int py = 0;
		int drawBrackets;
		int limit = min(n_teams, a_rows*a_cols);

        for (i=0; i < limit; i++)
        {
            if (hud_teamfrags_vertical->value)
            {
                if (i % a_rows == 0)
                {
                    px = x + space_x + (i/a_rows) * (cell_width+space_x);
                    py = y + space_y;
                }
            }
            else
            {
                if (i % a_cols == 0)
                {
                    px = x + space_x;
                    py = y + space_y + (i/a_cols) * (cell_height+space_y);
                }
            }

			drawBrackets = 0;

			// Bug fix. Before the wrong player would be higlighted
			// during qwd-playback, since you ARE the player that you're
			// being spectated.
			if(cls.demoplayback && !cl.spectator && !cls.mvdplayback)
			{
				// QWD Playback.
				if (!strcmp(sorted_teams[num]->name, cl.players[cl.playernum].team))
				{
					drawBrackets = 1;
				}
			}
			else if (cls.demoplayback || cl.spectator)
			{
				// MVD playback / spectating.
				if (!strcmp(cl.players[spec_track].team, sorted_teams[num]->name) && Cam_TrackNum() >= 0)
				{
					drawBrackets = 1;
				}
			}
			else
			{
				// Normal player.
				if (!strcmp(sorted_teams[num]->name, cl.players[cl.playernum].team))
				{
					drawBrackets = 1;
				}
			}

			if (cl_multiview.value)
			{
				// TODO: Check if "track team" is set, if it is then draw brackets around that team.
				//cl.players[nPlayernum]

				drawBrackets = 0;
			}

			if(hud_teamfrags_shownames->value || hud_teamfrags_extra_spec->value)
			{
				int _px = px;

				// Draw a background if the style tells us to.
				if(hud_teamfrags_style->value >= 4 && hud_teamfrags_style->value <= 8)
				{
					Frags_DrawBackground(px, py, cell_width, cell_height, space_x, space_y,
						0, max_team_length, sorted_teams[num]->bottom,
						0, hud_teamfrags_shownames->value, drawBrackets,
						hud_teamfrags_style->value);
				}

				// Draw the text on the left or right side of the score?
				if(hud_teamfrags_fliptext->value)
				{
					// Draw team.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, 0, max_team_length,
						hud_teamfrags_fliptext->value, hud_teamfrags_padtext->value,
						0, hud_teamfrags_shownames->value,
						"", sorted_teams[num]->name);

					Frags_DrawColors(_px, py, cell_width, cell_height, sorted_teams[num]->top, sorted_teams[num]->bottom, sorted_teams[num]->frags, drawBrackets, hud_teamfrags_style->value);

					_px += cell_width + space_x;

					// Draw the rl if the current player has it and the style allows it.
					_px = TeamFrags_DrawExtraSpecInfo(num, _px, py, cell_width, cell_height, hud_teamfrags_extra_spec->value);

				}
				else
				{
					// Draw the rl if the current player has it and the style allows it.
					_px = TeamFrags_DrawExtraSpecInfo(num, _px, py, cell_width, cell_height, hud_teamfrags_extra_spec->value);

					Frags_DrawColors(_px, py, cell_width, cell_height, sorted_teams[num]->top, sorted_teams[num]->bottom, sorted_teams[num]->frags, drawBrackets, hud_teamfrags_style->value);

					_px += cell_width + space_x;

					// Draw team.
					_px = Frags_DrawText(_px, py, cell_width, cell_height,
						space_x, space_y, 0, max_team_length,
						hud_teamfrags_fliptext->value, hud_teamfrags_padtext->value,
						0, hud_teamfrags_shownames->value,
						"", sorted_teams[num]->name);
				}

				if(hud_teamfrags_vertical->value)
				{
					py += cell_height + space_y;
				}
				else
				{
					px = _px + space_x;
				}
			}
			else
			{
				Frags_DrawColors(px, py, cell_width, cell_height, sorted_teams[num]->top, sorted_teams[num]->bottom, sorted_teams[num]->frags, drawBrackets, hud_teamfrags_style->value);

				if (hud_teamfrags_vertical->value)
				{
					py += cell_height + space_y;
				}
				else
				{
					px += cell_width + space_x;
				}
			}
            num ++;
        }
    }
}

char *Get_MP3_HUD_style(float style, char *st)
{
	static char HUD_style[32];
	if(style == 1.0)
	{
		strlcpy(HUD_style, va("%s:", st), sizeof(HUD_style));
	}
	else if(style == 2.0)
	{
		strlcpy(HUD_style, va("\x10%s\x11", st), sizeof(HUD_style));
	}
	else
	{
		strlcpy(HUD_style, "", sizeof(HUD_style));
	}
	return HUD_style;
}

// Draws MP3 Title.
void SCR_HUD_DrawMP3_Title(hud_t *hud)
{
#if defined(_WIN32) || defined(__XMMS__)
	int x=0, y=0/*, n=1*/;
    int width = 64;
	int height = 8;
	//int width_as_text = 0;
	static int title_length = 0;
	//int row_break = 0;
	//int i=0;
	int status = 0;
	static char title[MP3_MAXSONGTITLE];
	double t;		// current time
	static double lastframetime;	// last refresh

	static cvar_t *style = NULL, *width_var, *height_var, *scroll, *scroll_delay, *on_scoreboard, *wordwrap;

	if (style == NULL)  // first time called
    {
        style =				HUD_FindVar(hud, "style");
		width_var =			HUD_FindVar(hud, "width");
		height_var =		HUD_FindVar(hud, "height");
		scroll =			HUD_FindVar(hud, "scroll");
		scroll_delay =		HUD_FindVar(hud, "scroll_delay");
		on_scoreboard =		HUD_FindVar(hud, "on_scoreboard");
		wordwrap =			HUD_FindVar(hud, "wordwrap");
    }

	if(on_scoreboard->value)
	{
		hud->flags |= HUD_ON_SCORES;
	}
	else if((int)on_scoreboard->value & HUD_ON_SCORES)
	{
		hud->flags -= HUD_ON_SCORES;
	}

	width = (int)width_var->value;
	height = (int)height_var->value;

	if(width < 0) width = 0;
	if(width > vid.width) width = vid.width;
	if(height < 0) height = 0;
	if(height > vid.width) height = vid.height;

	t = Sys_DoubleTime();

	if ((t - lastframetime) >= 2) { // 2 sec refresh rate
		lastframetime = t;
		status = MP3_GetStatus();

		switch(status)
		{
			case MP3_PLAYING :
				title_length = snprintf(title, sizeof(title)-1, "%s %s", Get_MP3_HUD_style(style->value, "Playing"), MP3_Macro_MP3Info());
				break;
			case MP3_PAUSED :
				title_length = snprintf(title, sizeof(title)-1, "%s %s", Get_MP3_HUD_style(style->value, "Paused"), MP3_Macro_MP3Info());
				break;
			case MP3_STOPPED :
				title_length = snprintf(title, sizeof(title)-1, "%s %s", Get_MP3_HUD_style(style->value, "Stopped"), MP3_Macro_MP3Info());
				break;
			case MP3_NOTRUNNING	:
			default :
				status = MP3_NOTRUNNING;
				title_length = sprintf(title, "%s is not running.", MP3_PLAYERNAME_ALLCAPS);
				break;
		}

		if(title_length < 0)
		{
			sprintf(title, "Error retrieving current song.");
		}
	}

	if (HUD_PrepareDraw(hud, width , height, &x, &y))
	{
		SCR_DrawWordWrapString(x, y, 8, width, height, (int)wordwrap->value, (int)scroll->value, (float)scroll_delay->value, title);
	}
#endif
}

// Draws MP3 Time as a HUD-element.
void SCR_HUD_DrawMP3_Time(hud_t *hud)
{
#if defined(_WIN32) || defined(__XMMS__)
	int x=0, y=0, width=0, height=0;
	int elapsed = 0;
	int remain = 0;
	int total = 0;
	static char time_string[MP3_MAXSONGTITLE];
	static char elapsed_string[MP3_MAXSONGTITLE];
	double t;		// current time
	static double lastframetime;	// last refresh

	static cvar_t *style = NULL, *on_scoreboard;

	if(style == NULL)
	{
		style			= HUD_FindVar(hud, "style");
		on_scoreboard	= HUD_FindVar(hud, "on_scoreboard");
	}

	if(on_scoreboard->value)
	{
		hud->flags |= HUD_ON_SCORES;
	}
	else if((int)on_scoreboard->value & HUD_ON_SCORES)
	{
		hud->flags -= HUD_ON_SCORES;
	}

	t = Sys_DoubleTime();
	if ((t - lastframetime) >= 2) { // 2 sec refresh rate
		lastframetime = t;

		if(!MP3_GetOutputtime(&elapsed, &total) || elapsed < 0 || total < 0)
		{
			sprintf(time_string, "\x10-:-\x11");
		}
		else
		{
			switch((int)style->value)
			{
				case 1 :
					remain = total - elapsed;
					strlcpy(elapsed_string, SecondsToMinutesString(remain), sizeof(elapsed_string));
					sprintf(time_string, va("\x10-%s/%s\x11", elapsed_string, SecondsToMinutesString(total)));
					break;
				case 2 :
					remain = total - elapsed;
					sprintf(time_string, va("\x10-%s\x11", SecondsToMinutesString(remain)));
					break;
				case 3 :
					sprintf(time_string, va("\x10%s\x11", SecondsToMinutesString(elapsed)));
					break;
				case 4 :
					remain = total - elapsed;
					strlcpy(elapsed_string, SecondsToMinutesString(remain), sizeof(elapsed_string));
					sprintf(time_string, va("%s/%s", elapsed_string, SecondsToMinutesString(total)));
					break;
				case 5 :
					strlcpy(elapsed_string, SecondsToMinutesString(elapsed), sizeof(elapsed_string));
					sprintf(time_string, va("-%s/%s", elapsed_string, SecondsToMinutesString(total)));
					break;
				case 6 :
					remain = total - elapsed;
					sprintf(time_string, va("-%s", SecondsToMinutesString(remain)));
					break;
				case 7 :
					sprintf(time_string, va("%s", SecondsToMinutesString(elapsed)));
					break;
				case 0 :
				default :
					strlcpy(elapsed_string, SecondsToMinutesString(elapsed), sizeof(elapsed_string));
					sprintf(time_string, va("\x10%s/%s\x11", elapsed_string, SecondsToMinutesString(total)));
					break;
			}
		}
	
	}

	// Don't allow showing the timer during ruleset smackdown,
	// can be used for timing powerups.
	if(!strncasecmp(Rulesets_Ruleset(), "smackdown", 9))
	{
		sprintf(time_string, va("\x10%s\x11", "Not allowed"));
	}

	width = strlen(time_string)*8;
	height = 8;

	if (HUD_PrepareDraw(hud, width , height, &x, &y))
	{
		Draw_String(x, y, time_string);
	}
#endif
}

#ifdef GLQUAKE

// Map picture to draw for the mapoverview hud control.
static mpic_t *radar_pic = NULL;
static qbool radar_pic_found = false;

// The conversion formula used for converting from quake coordinates to pixel coordinates
// when drawing on the map overview.
static float map_x_slope;
static float map_x_intercept;
static float map_y_slope;
static float map_y_intercept;
static qbool conversion_formula_found = false;

// Used for drawing the height of the player.
static float map_height_diff = 0.0;

//
// Is run when a new map is loaded.
//
void HUD_NewMap()
{
	FILE *f;
	int n_textcount = 0;
	png_textp txt;
	int i = 0;
	char radar_filename[] = "radars/%s.png";
	txt = NULL;

	// Reset the pointer so that we know if the load failed.
	if(radar_pic != NULL)
	{
		Q_free(radar_pic);
	}
	radar_pic = NULL;
	radar_pic_found = false;
	conversion_formula_found = false;

	if (FS_FOpenFile (va(radar_filename, mapname.string), &f) != -1)
	{
		mpic_t *temp = NULL;

		// Load the map picture.
		temp = GL_LoadPicImage(va(radar_filename, mapname.string), mapname.string, 0, 0, TEX_ALPHA);

		// Make a copy of the returned structure, otherwise it will be overwritten
		// the next time an image is loaded.
		if(temp != NULL)
		{
			radar_pic			= (mpic_t *)Q_malloc(sizeof(mpic_t));
			radar_pic->texnum	= temp->texnum;
			radar_pic->height	= temp->height;
			radar_pic->sh		= temp->sh;
			radar_pic->sl		= temp->sl;
			radar_pic->th		= temp->th;
			radar_pic->tl		= temp->tl;
			radar_pic->width	= temp->width;
		}

		// Check if we found something
		if(radar_pic != NULL && radar_pic->height && radar_pic->width)
		{
			radar_pic_found = true;
		}
		else
		{
			Q_free(radar_pic);
			radar_pic = NULL;
			radar_pic_found = false;
			return;
		}

		// Calculate the height of the map.
		map_height_diff = abs(cl.worldmodel->maxs[2] - cl.worldmodel->mins[2]);

		// Get the comments from the PNG.
		txt = Image_LoadPNG_Comments(f, va(radar_filename, mapname.string), &n_textcount);

		// TODO: If failed, try to read conversion formula from a .qcf file instead.
		// "quake conversion formula" :D Looking something like this:
		// X_SLOPE 0.184
		// X_INTERCEPT 95
		// Y_SLOPE 0.184
		// Y_INTERCEPT 55

		// Check if we found any comments.
		if(txt != NULL)
		{
			int found_count = 0;

			// Find the conversion formula in the comments found in the PNG.
			for(i = 0; i < n_textcount; i++)
			{
				if(!strcmp(txt[i].key, "QWLMConversionSlopeX"))
				{
					map_x_slope = atof(txt[i].text);
					found_count++;
				}
				else if(!strcmp(txt[i].key, "QWLMConversionInterceptX"))
				{
					map_x_intercept = atof(txt[i].text);
					found_count++;
				}
				else if(!strcmp(txt[i].key, "QWLMConversionSlopeY"))
				{
					map_y_slope = atof(txt[i].text);
					found_count++;
				}
				else if(!strcmp(txt[i].key, "QWLMConversionInterceptY"))
				{
					map_y_intercept = atof(txt[i].text);
					found_count++;
				}

				conversion_formula_found = (found_count == 4);
			}

			// Free the text chunks.
			Q_free(txt);
		}
		else
		{
			conversion_formula_found = false;
		}
	}
}
#endif

#define HUD_SHOW_ONLY_IN_TEAMPLAY		1
#define HUD_SHOW_ONLY_IN_DEMOPLAYBACK	2

qbool HUD_ShowInDemoplayback(int val)
{
	if(!cl.teamplay && val == HUD_SHOW_ONLY_IN_TEAMPLAY)
	{
		return false;
	}
	else if(!cls.demoplayback && val == HUD_SHOW_ONLY_IN_DEMOPLAYBACK)
	{
		return false;
	}
	else if(!cl.teamplay && !cls.demoplayback 
		&& val == HUD_SHOW_ONLY_IN_TEAMPLAY + HUD_SHOW_ONLY_IN_DEMOPLAYBACK)
	{
		return false;
	}

	return true;
}

// Team hold filters.
static qbool teamhold_show_pent		= false;
static qbool teamhold_show_quad		= false;
static qbool teamhold_show_ring		= false;
static qbool teamhold_show_suit		= false;
static qbool teamhold_show_rl		= false;
static qbool teamhold_show_lg		= false;
static qbool teamhold_show_gl		= false;
static qbool teamhold_show_sng		= false;
static qbool teamhold_show_mh		= false;
static qbool teamhold_show_ra		= false;
static qbool teamhold_show_ya		= false;
static qbool teamhold_show_ga		= false;

void TeamHold_DrawBars(int x, int y, int width, int height,
						float team1_percent, float team2_percent,
						int team1_color, int team2_color,
						float opacity)
{
	int team1_width = 0;
	int team2_width = 0;
	int bar_height = 0;

	bar_height = ROUND(height/2.0);
	team1_width = width * team1_percent;
	team2_width = width * team2_percent;
	
	team1_width = clamp(team1_width, 0, width);
	team2_width = clamp(team2_width, 0, width);

	#ifdef GLQUAKE
	Draw_AlphaFill(x, y, team1_width, bar_height, team1_color, opacity);
	#else
	Draw_Fill(x, y, team1_width, bar_height, team1_color);
	#endif

	y += bar_height;

	#ifdef GLQUAKE
	Draw_AlphaFill(x, y, team2_width, bar_height, team2_color, opacity);
	#else
	Draw_Fill(x, y, team2_width, bar_height, team2_color);
	#endif
}

void TeamHold_DrawPercentageBar(int x, int y, int width, int height, 
								float team1_percent, float team2_percent, 
								int team1_color, int team2_color,
								int show_text, int vertical,
								int vertical_text, float opacity)
{
	int _x, _y;
	int _width, _height;

	if(vertical)
	{
		//
		// Draw vertical.
		//

		// Team 1.
		_x = x;
		_y = y;
		_width = max(0, width);
		_height = ROUND(height * team1_percent);
		_height = max(0, height);

		#ifdef GLQUAKE
		Draw_AlphaFill(_x, _y, _width, _height, team1_color, opacity);
		#else
		Draw_Fill(_x, _y, _width, _height, team1_color);
		#endif

		// Team 2.
		_x = x;
		_y = ROUND(y + (height * team1_percent));
		_width = max(0, width);
		_height = ROUND(height * team2_percent);
		_height = max(0, _height);

		#ifdef GLQUAKE
		Draw_AlphaFill(_x, _y, _width, _height, team2_color, opacity);
		#else
		Draw_Fill(_x, _y, _width, _height, team2_color);
		#endif

		// Show the percentages in numbers also.
		if(show_text)
		{
			// TODO: Move this to a separate function (since it's prett much copy and paste for both teams).
			// Team 1.
			if(team1_percent > 0.05)
			{
				if(vertical_text)
				{
					int percent = 0;
					int percent10 = 0;
					int percent100 = 0;			
			
					_x = x + (width / 2) - 4;
					_y = ROUND(y + (height * team1_percent)/2 - 12);

					percent = ROUND(100 * team1_percent);

					if((percent100 = percent / 100))
					{
						Draw_String(_x, _y, va("%d", percent100));
						_y += 8;
					}

					if((percent10 = percent / 10))
					{
						Draw_String(_x, _y, va("%d", percent10));
						_y += 8;
					}

					Draw_String(_x, _y, va("%d", percent % 10));
					_y += 8;

					Draw_String(_x, _y, "%");
				}
				else
				{
					_x = x + (width / 2) - 12;
					_y = ROUND(y + (height * team1_percent)/2 - 4);					
					Draw_String(_x, _y, va("%2.0f%%", 100 * team1_percent));
				}
			}

			// Team 2.			
			if(team2_percent > 0.05)
			{
				if(vertical_text)
				{
					int percent = 0;
					int percent10 = 0;
					int percent100 = 0;			

					_x = x + (width / 2) - 4;
					_y = ROUND(y + (height * team1_percent) + (height * team2_percent)/2 - 12);

					percent = ROUND(100 * team2_percent);

					if((percent100 = percent / 100))
					{
						Draw_String(_x, _y, va("%d", percent100));
						_y += 8;
					}

					if((percent10 = percent / 10))
					{
						Draw_String(_x, _y, va("%d", percent10));
						_y += 8;
					}

					Draw_String(_x, _y, va("%d", percent % 10));
					_y += 8;

					Draw_String(_x, _y, "%");
				}
				else
				{
					_x = x + (width / 2) - 12;
					_y = ROUND(y + (height * team1_percent) + (height * team2_percent)/2 - 4);
					Draw_String(_x, _y, va("%2.0f%%", 100 * team2_percent));
				}
			}
		}
	}
	else
	{
		//
		// Draw horizontal.
		//

		// Team 1.
		_x = x;
		_y = y;
		_width = ROUND(width * team1_percent);
		_width = max(0, _width);
		_height = max(0, height);

		#ifdef GLQUAKE
		Draw_AlphaFill(_x, _y, _width, _height, team1_color, opacity);
		#else
		Draw_Fill(_x, _y, _width, _height, team1_color);
		#endif

		// Team 2.
		_x = ROUND(x + (width * team1_percent));
		_y = y;
		_width = ROUND(width * team2_percent);
		_width = max(0, _width);
		_height = max(0, height);

		#ifdef GLQUAKE
		Draw_AlphaFill(_x, _y, _width, _height, team2_color, opacity);
		#else
		Draw_Fill(_x, _y, _width, _height, team2_color);
		#endif

		// Show the percentages in numbers also.
		if(show_text)
		{
			// Team 1.
			if(team1_percent > 0.05)
			{
				_x = ROUND(x + (width * team1_percent)/2 - 8);
				_y = y + (height / 2) - 4;
				Draw_String(_x, _y, va("%2.0f%%", 100 * team1_percent));
			}

			// Team 2.
			if(team2_percent > 0.05)
			{
				_x = ROUND(x + (width * team1_percent) + (width * team2_percent)/2 - 8);
				_y = y + (height / 2) - 4;
				Draw_String(_x, _y, va("%2.0f%%", 100 * team2_percent));
			}
		}
	}			
}

void SCR_HUD_DrawTeamHoldBar(hud_t *hud)
{
	int x, y;
	int height = 8;
	int width = 0;
	float team1_percent = 0;
	float team2_percent = 0;

	static cvar_t
        *hud_teamholdbar_style = NULL,
		*hud_teamholdbar_opacity,
		*hud_teamholdbar_width,
		*hud_teamholdbar_height,
		*hud_teamholdbar_vertical,
		*hud_teamholdbar_show_text,
		*hud_teamholdbar_onlytp,
		*hud_teamholdbar_vertical_text;

    if (hud_teamholdbar_style == NULL)    // first time
    {
		hud_teamholdbar_style				= HUD_FindVar(hud, "style");
		hud_teamholdbar_opacity				= HUD_FindVar(hud, "opacity");
		hud_teamholdbar_width				= HUD_FindVar(hud, "width");
		hud_teamholdbar_height				= HUD_FindVar(hud, "height");
		hud_teamholdbar_vertical			= HUD_FindVar(hud, "vertical");
		hud_teamholdbar_show_text			= HUD_FindVar(hud, "show_text");
		hud_teamholdbar_onlytp				= HUD_FindVar(hud, "onlytp");
		hud_teamholdbar_vertical_text		= HUD_FindVar(hud, "vertical_text");
    }

	// Don't show when not in teamplay/demoplayback.
	if(!HUD_ShowInDemoplayback(hud_teamholdbar_onlytp->value))
	{
		return;
	}

	height = max(1, hud_teamholdbar_height->value);
	width = max(0, hud_teamholdbar_width->value);
	
	if (HUD_PrepareDraw(hud, width , height, &x, &y))
	{
		// We need something to work with.
		if(stats_grid != NULL)
		{	
			// Check if we have any hold values to calculate from.
			if(stats_grid->teams[STATS_TEAM1].hold_count + stats_grid->teams[STATS_TEAM2].hold_count > 0)
			{
				// Calculate the percentage for the two teams for the "team strength bar".
				team1_percent = ((float)stats_grid->teams[STATS_TEAM1].hold_count) / (stats_grid->teams[STATS_TEAM1].hold_count + stats_grid->teams[STATS_TEAM2].hold_count);
				team2_percent = ((float)stats_grid->teams[STATS_TEAM2].hold_count) / (stats_grid->teams[STATS_TEAM1].hold_count + stats_grid->teams[STATS_TEAM2].hold_count);

				team1_percent = fabs(max(0, team1_percent));
				team2_percent = fabs(max(0, team2_percent));
			}
			else
			{
				#ifdef GLQUAKE
				Draw_AlphaFill(x, y, hud_teamholdbar_width->value, height, 0, hud_teamholdbar_opacity->value*0.5);
				#else
				Draw_Fill(x, y, hud_teamholdbar_width->value, height, 0);
				#endif
				return;
			}

			// Draw the percentage bar.
			TeamHold_DrawPercentageBar(x, y, width, height, 
				team1_percent, team2_percent, 
				stats_grid->teams[STATS_TEAM1].color, 
				stats_grid->teams[STATS_TEAM2].color, 
				hud_teamholdbar_show_text->value, 
				hud_teamholdbar_vertical->value,
				hud_teamholdbar_vertical_text->value,
				hud_teamholdbar_opacity->value);	
		}
		else
		{
			// If there's no stats grid available we don't know what to show, so just show a black frame.
			#ifdef GLQUAKE
			Draw_AlphaFill(x, y, hud_teamholdbar_width->value, height, 0, hud_teamholdbar_opacity->value*0.5);
			#else
			Draw_Fill(x, y, hud_teamholdbar_width->value, height, 0);
			#endif
		}
	}
}

qbool TeamHold_OnChangeItemFilterInfo(cvar_t *var, char *s)
{
	// Parse the item filter.
	teamhold_show_rl		= HUD_RegExpMatch("RL",		s);
	teamhold_show_quad		= HUD_RegExpMatch("QUAD",	s);
	teamhold_show_ring		= HUD_RegExpMatch("RING",	s);
	teamhold_show_pent		= HUD_RegExpMatch("PENT",	s);
	teamhold_show_suit		= HUD_RegExpMatch("SUIT",	s);
	teamhold_show_lg		= HUD_RegExpMatch("LG",		s);
	teamhold_show_gl		= HUD_RegExpMatch("GL",		s);
	teamhold_show_sng		= HUD_RegExpMatch("SNG",	s);
	teamhold_show_mh		= HUD_RegExpMatch("MH",		s);
	teamhold_show_ra		= HUD_RegExpMatch("RA",		s);
	teamhold_show_ya		= HUD_RegExpMatch("YA",		s);
	teamhold_show_ga		= HUD_RegExpMatch("GA",		s);

	return false;
}

#define HUD_TEAMHOLDINFO_STYLE_TEAM_NAMES		0
#define HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS		1
#define HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS2	2

void SCR_HUD_DrawTeamHoldInfo(hud_t *hud)
{
	int i;
	int x, y;
	int width, height;

	static cvar_t
        *hud_teamholdinfo_style = NULL,
		*hud_teamholdinfo_opacity,
		*hud_teamholdinfo_width,
		*hud_teamholdinfo_height,
		*hud_teamholdinfo_onlytp,
		*hud_teamholdinfo_itemfilter;

    if (hud_teamholdinfo_style == NULL)    // first time
    {
		hud_teamholdinfo_style				= HUD_FindVar(hud, "style");
		hud_teamholdinfo_opacity			= HUD_FindVar(hud, "opacity");
		hud_teamholdinfo_width				= HUD_FindVar(hud, "width");
		hud_teamholdinfo_height				= HUD_FindVar(hud, "height");
		hud_teamholdinfo_onlytp				= HUD_FindVar(hud, "onlytp");
		hud_teamholdinfo_itemfilter			= HUD_FindVar(hud, "itemfilter");

		// Unecessary to parse the item filter string on each frame.
		hud_teamholdinfo_itemfilter->OnChange = TeamHold_OnChangeItemFilterInfo;

		// Parse the item filter the first time.
		TeamHold_OnChangeItemFilterInfo(hud_teamholdinfo_itemfilter, hud_teamholdinfo_itemfilter->string);
    }

	// Don't show when not in teamplay/demoplayback.
	if(!HUD_ShowInDemoplayback(hud_teamholdinfo_onlytp->value))
	{
		return;
	}

	// We don't have anything to show.
	if(stats_important_ents == NULL || stats_grid == NULL)
	{
		return;
	}

	// Get the height based on how many items we have, or what the user has set it to.
	height = max(0, hud_teamholdinfo_height->value);
	width = max(0, hud_teamholdinfo_width->value);
	
	if (HUD_PrepareDraw(hud, width , height, &x, &y))
	{
		int _y = 0;

		_y = y;

		// Go through all the items and print the stats for them.
		for(i = 0; i < stats_important_ents->count; i++)
		{
			float team1_percent;
			float team2_percent;
			int team1_hold_count = 0;
			int team2_hold_count = 0;
			int names_width = 0;			

			// Don't draw outside the specified height.
			if((_y - y) + 8 > height)
			{
				break;
			}

			// If the item isn't of the specified type, then skip it.
			if(!(	(teamhold_show_rl	&& !strncmp(stats_important_ents->list[i].name, "RL", 2))
				||	(teamhold_show_quad	&& !strncmp(stats_important_ents->list[i].name, "QUAD", 4))
				||	(teamhold_show_ring	&& !strncmp(stats_important_ents->list[i].name, "RING", 4))
				||	(teamhold_show_pent	&& !strncmp(stats_important_ents->list[i].name, "PENT", 4))
				||	(teamhold_show_suit	&& !strncmp(stats_important_ents->list[i].name, "SUIT", 4))
				||	(teamhold_show_lg	&& !strncmp(stats_important_ents->list[i].name, "LG", 2))
				||	(teamhold_show_gl	&& !strncmp(stats_important_ents->list[i].name, "GL", 2))
				||	(teamhold_show_sng	&& !strncmp(stats_important_ents->list[i].name, "SNG", 3))
				||	(teamhold_show_mh	&& !strncmp(stats_important_ents->list[i].name, "MH", 2))
				||	(teamhold_show_ra	&& !strncmp(stats_important_ents->list[i].name, "RA", 2))
				||	(teamhold_show_ya	&& !strncmp(stats_important_ents->list[i].name, "YA", 2))
				||	(teamhold_show_ga	&& !strncmp(stats_important_ents->list[i].name, "GA", 2))
				))
			{
				continue;
			}

			// Calculate the width of the longest item name so we can use it for padding.
			names_width = 8 * (stats_important_ents->longest_name + 1);

			// Calculate the percentages of this item that the two teams holds.
			team1_hold_count = stats_important_ents->list[i].teams_hold_count[STATS_TEAM1];
			team2_hold_count = stats_important_ents->list[i].teams_hold_count[STATS_TEAM2];

			team1_percent = ((float)team1_hold_count) / (team1_hold_count + team2_hold_count);
			team2_percent = ((float)team2_hold_count) / (team1_hold_count + team2_hold_count);

			team1_percent = fabs(max(0, team1_percent));
			team2_percent = fabs(max(0, team2_percent));

			// Write the name of the item.
			Draw_ColoredString(x, _y, va("&cff0%s:", stats_important_ents->list[i].name), 0);
			
			if(hud_teamholdinfo_style->value == HUD_TEAMHOLDINFO_STYLE_TEAM_NAMES)
			{
				//
				// Prints the team name that holds the item.
				//
				if(team1_percent > team2_percent)
				{
					Draw_ColoredString(x + names_width, _y, stats_important_ents->teams[STATS_TEAM1].name, 0);
				}
				else if(team1_percent < team2_percent)
				{
					Draw_ColoredString(x + names_width, _y, stats_important_ents->teams[STATS_TEAM2].name, 0);
				}
			}
			else if(hud_teamholdinfo_style->value == HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS)
			{
				//
				// Show a percenteage bar for the item.
				//
				TeamHold_DrawPercentageBar(x + names_width, _y, 
					ROUND(hud_teamholdinfo_width->value - names_width), 8, 
					team1_percent, team2_percent, 
					stats_important_ents->teams[STATS_TEAM1].color, 
					stats_important_ents->teams[STATS_TEAM2].color, 
					0, // Don't show percentage values, get's too cluttered.
					false,
					false,
					hud_teamholdinfo_opacity->value);			
			}
			else if(hud_teamholdinfo_style->value == HUD_TEAMHOLDINFO_STYLE_PERCENT_BARS2)
			{
				TeamHold_DrawBars(x + names_width, _y, 
					ROUND(hud_teamholdinfo_width->value - names_width), 8,
					team1_percent, team2_percent,
					stats_important_ents->teams[STATS_TEAM1].color, 
					stats_important_ents->teams[STATS_TEAM2].color, 
					hud_teamholdinfo_opacity->value);		
			}

			// Next line.
			_y += 8;			
		}
	}
}

#ifdef GLQUAKE

// What stats to draw.
#define HUD_RADAR_STATS_NONE				0
#define HUD_RADAR_STATS_BOTH_TEAMS_HOLD		1
#define HUD_RADAR_STATS_TEAM1_HOLD			2
#define HUD_RADAR_STATS_TEAM2_HOLD			3
#define HUD_RADAR_STATS_BOTH_TEAMS_DEATHS	4
#define HUD_RADAR_STATS_TEAM1_DEATHS		5
#define HUD_RADAR_STATS_TEAM2_DEATHS		6

void Radar_DrawGrid(stats_weight_grid_t *grid, int x, int y, float scale, int pic_width, int pic_height, int style)
{
	int row, col;	

	// Don't try to draw anything if we got no data.
	if(grid == NULL || style == HUD_RADAR_STATS_NONE)
	{
		return;
	}

	// Go through all the cells and draw them based on their weight.
	for(row = 0; row < grid->row_count; row++)
	{
		for(col = 0; col < grid->col_count; col++)
		{
			float weight = 0.0;
			int color = 0;

			float tl_x, tl_y;				// The pixel coordinate of the top left corner of a grid cell.
			int p_cell_length;				// The pixel length of a cell.
			
			// Calculate the pixel coordinates of the top left corner of the current cell.
			// (This is times 8 because the conversion formula was calculated from a .loc-file)
			tl_x = (map_x_slope * (8 * grid->cells[row][col].tl_x) + map_x_intercept) * scale;
			tl_y = (map_y_slope * (8 * grid->cells[row][col].tl_y) + map_y_intercept) * scale;

			// Calculate the cell length in pixel length.
			p_cell_length = ROUND(map_x_slope*(8*grid->cell_length) * scale);
			
			// Don't draw the stats stuff outside the picture.
			if(tl_x + p_cell_length > pic_width || tl_y + p_cell_length > pic_height || x + tl_x < x || y + tl_y < y)
			{
				continue;
			}

			// This is only a test so that I can see my wonderful grid. :D
			//Draw_AlphaOutline(x + tl_x, y + tl_y, p_cell_width, p_cell_height, 0, 1, 1);

			//
			// Death stats.
			//
			if(grid->cells[row][col].teams[STATS_TEAM1].death_weight + grid->cells[row][col].teams[STATS_TEAM2].death_weight > 0)
			{
				weight = 0;

				if(style == HUD_RADAR_STATS_BOTH_TEAMS_DEATHS || style == HUD_RADAR_STATS_TEAM1_DEATHS)
				{
					weight = grid->cells[row][col].teams[STATS_TEAM1].death_weight;
				}

				if(style == HUD_RADAR_STATS_BOTH_TEAMS_DEATHS || style == HUD_RADAR_STATS_TEAM2_DEATHS)
				{
					weight += grid->cells[row][col].teams[STATS_TEAM2].death_weight;
				}

				color = 79;
			}
			
			//
			// Team stats.
			//
			{
				// No point in drawing if we have no weight.
				if(grid->cells[row][col].teams[STATS_TEAM1].weight + grid->cells[row][col].teams[STATS_TEAM2].weight <= 0 
					&& (style == HUD_RADAR_STATS_BOTH_TEAMS_HOLD 
					||	style == HUD_RADAR_STATS_TEAM1_HOLD
					||	style == HUD_RADAR_STATS_TEAM2_HOLD))
				{
					continue;
				}

				// Get the team with the highest weight for this cell.
				if(grid->cells[row][col].teams[STATS_TEAM1].weight > grid->cells[row][col].teams[STATS_TEAM2].weight
					&& (style == HUD_RADAR_STATS_BOTH_TEAMS_HOLD
					||	style == HUD_RADAR_STATS_TEAM1_HOLD))
				{
					weight = grid->cells[row][col].teams[STATS_TEAM1].weight;
					color = stats_grid->teams[STATS_TEAM1].color;
				}
				else if(style == HUD_RADAR_STATS_BOTH_TEAMS_HOLD ||	style == HUD_RADAR_STATS_TEAM2_HOLD)
				{
					weight = grid->cells[row][col].teams[STATS_TEAM2].weight;
					color = stats_grid->teams[STATS_TEAM2].color;
				}
			}

			// Draw the cell in the color of the team with the
			// biggest weight for this cell. Or draw deaths.
			Draw_AlphaFill(
				ROUND(x + tl_x),	// X.
				ROUND(y + tl_y),	// Y.
				p_cell_length,		// Width.
				p_cell_length,		// Height.
				color,				// Color.
				weight);			// Alpha.
		}
	}
}

// The skinnum property in the entity_s structure is used
// for determening what type of armor to draw on the radar.
#define HUD_RADAR_GA					0
#define HUD_RADAR_YA					1
#define HUD_RADAR_RA					2

// Radar filters.
#define RADAR_SHOW_WEAPONS (radar_show_ssg || radar_show_ng || radar_show_sng || radar_show_gl || radar_show_rl || radar_show_lg)
static qbool radar_show_ssg			= false;
static qbool radar_show_ng			= false;
static qbool radar_show_sng			= false;
static qbool radar_show_gl			= false;
static qbool radar_show_rl			= false;
static qbool radar_show_lg			= false;

#define RADAR_SHOW_ITEMS (radar_show_backpacks || radar_show_health || radar_show_ra || radar_show_ya || radar_show_ga || radar_show_rockets || radar_show_nails || radar_show_cells || radar_show_shells || radar_show_quad || radar_show_pent || radar_show_ring || radar_show_suit)
static qbool radar_show_backpacks	= false;
static qbool radar_show_health		= false;
static qbool radar_show_ra			= false;
static qbool radar_show_ya			= false;
static qbool radar_show_ga			= false;
static qbool radar_show_rockets		= false;
static qbool radar_show_nails		= false;
static qbool radar_show_cells		= false;
static qbool radar_show_shells		= false;
static qbool radar_show_quad		= false;
static qbool radar_show_pent		= false;
static qbool radar_show_ring		= false;
static qbool radar_show_suit		= false;
static qbool radar_show_mega		= false;

#define RADAR_SHOW_OTHER (radar_show_gibs || radar_show_explosions || radar_show_nails_p || radar_show_rockets_p || radar_show_shaft_p || radar_show_teleport || radar_show_shotgun)
static qbool radar_show_nails_p		= false;
static qbool radar_show_rockets_p	= false;
static qbool radar_show_shaft_p		= false;
static qbool radar_show_gibs		= false;
static qbool radar_show_explosions	= false;
static qbool radar_show_teleport	= false;
static qbool radar_show_shotgun		= false;

qbool Radar_OnChangeWeaponFilter(cvar_t *var, char *newval)
{
	// Parse the weapon filter.
	radar_show_ssg		= HUD_RegExpMatch("SSG|SUPERSHOTGUN|ALL",		newval);
	radar_show_ng		= HUD_RegExpMatch("([^S]|^)NG|NAILGUN|ALL",		newval); // Yes very ugly, but we don't want to match SNG.
	radar_show_sng		= HUD_RegExpMatch("SNG|SUPERNAILGUN|ALL",		newval);
	radar_show_rl		= HUD_RegExpMatch("RL|ROCKETLAUNCHER|ALL",		newval);
	radar_show_gl		= HUD_RegExpMatch("GL|GRENADELAUNCHER|ALL",		newval);
	radar_show_lg		= HUD_RegExpMatch("LG|SHAFT|LIGHTNING|ALL",		newval);

	return false;
}

qbool Radar_OnChangeItemFilter(cvar_t *var, char *newval)
{
	// Parse the item filter.
	radar_show_backpacks		= HUD_RegExpMatch("BP|BACKPACK|ALL",					newval);
	radar_show_health			= HUD_RegExpMatch("HP|HEALTH|ALL",						newval);
	radar_show_ra				= HUD_RegExpMatch("RA|REDARMOR|ARMOR|ALL",				newval);
	radar_show_ya				= HUD_RegExpMatch("YA|YELLOWARMOR|ARMOR|ALL",			newval);
	radar_show_ga				= HUD_RegExpMatch("GA|GREENARMOR|ARMOR|ALL",			newval);
	radar_show_rockets			= HUD_RegExpMatch("ROCKETS|ROCKS|AMMO|ALL",				newval);
	radar_show_nails			= HUD_RegExpMatch("NAILS|SPIKES|AMMO|ALL",				newval);
	radar_show_cells			= HUD_RegExpMatch("CELLS|BATTERY|AMMO|ALL",				newval);
	radar_show_shells			= HUD_RegExpMatch("SHELLS|AMMO|ALL",					newval);
	radar_show_quad				= HUD_RegExpMatch("QUAD|POWERUPS|ALL",					newval);
	radar_show_pent				= HUD_RegExpMatch("PENT|PENTAGRAM|666|POWERUPS|ALL",	newval);
	radar_show_ring				= HUD_RegExpMatch("RING|INVISIBLE|EYES|POWERUPS|ALL",	newval);
	radar_show_suit				= HUD_RegExpMatch("SUIT|POWERUPS|ALL",					newval);
	radar_show_mega				= HUD_RegExpMatch("MH|MEGA|MEGAHEALTH|100\\+|ALL",		newval);

	return false;
}

qbool Radar_OnChangeOtherFilter(cvar_t *var, char *newval)
{
	// Parse the "other" filter.
	radar_show_nails_p			= HUD_RegExpMatch("NAILS|PROJECTILES|ALL",		newval);
	radar_show_rockets_p		= HUD_RegExpMatch("ROCKETS|PROJECTILES|ALL",	newval);
	radar_show_shaft_p			= HUD_RegExpMatch("SHAFT|PROJECTILES|ALL",		newval);
	radar_show_gibs				= HUD_RegExpMatch("GIBS|ALL",					newval);
	radar_show_explosions		= HUD_RegExpMatch("EXPLOSIONS|ALL",				newval);
	radar_show_teleport			= HUD_RegExpMatch("TELE|ALL",					newval);
	radar_show_shotgun			= HUD_RegExpMatch("SHOTGUN|SG|BUCK|ALL",		newval);

	return false;
}


#define HUD_COLOR_DEFAULT_TRANSPARENCY	0.3

float hud_radar_highlight_color[4] = {1.0, 1.0, 0.0, HUD_COLOR_DEFAULT_TRANSPARENCY};

qbool Radar_OnChangeHighlightColor(cvar_t *var, char *newval)
{
	char *new_color;
	qbool radar_highlight_color_valid = false;

	new_color = HUD_ColorNameToRGB(newval); 

	// Make sure the values have been entered properly.
	radar_highlight_color_valid = HUD_RegExpMatch(HUD_COLOR_REGEX, new_color);

	// Get the RGB values.
	if (radar_highlight_color_valid)
	{
		float colors[4];

		strncpy(var->string, new_color, (strlen(new_color) + 1) * sizeof(char));

		HUD_RGBValuesFromString (var->string, 
			&colors[0], 
			&colors[1], 
			&colors[2], 
			&colors[3]);

		// RGB.
		if(colors[0] >= 0 && colors[1] >= 0 && colors[2] >= 0)
		{
			hud_radar_highlight_color[0] = colors[0];
			hud_radar_highlight_color[1] = colors[1];
			hud_radar_highlight_color[2] = colors[2];
		}

		// Alpha.
		if(colors[3] >= 0)
		{
			hud_radar_highlight_color[3] = colors[3];
		}
	}
	else
	{
		// Default to yellow.
		hud_radar_highlight_color[0] = 1.0;
		hud_radar_highlight_color[1] = 1.0;
		hud_radar_highlight_color[2] = 0.0;
		hud_radar_highlight_color[3] = HUD_COLOR_DEFAULT_TRANSPARENCY;

		strncpy(var->string, HUD_COLOR_YELLOW, (strlen(HUD_COLOR_YELLOW) + 1) * sizeof(char));
		radar_highlight_color_valid = true;
	}

	return true;
}

void Radar_DrawEntities(int x, int y, float scale, float player_size, int show_hold_areas)
{
	int i;

	// Entities (weapons and such). cl_main.c
	extern visentlist_t cl_visents;

	// Go through all the entities and draw the ones we're supposed to.
	for (i = 0; i < cl_visents.count; i++)
	{
		int entity_q_x = 0;
		int entity_q_y = 0;
		int entity_p_x = 0;
		int entity_p_y = 0;

		// Get quake coordinates (times 8 to get them in the same format as .locs).
		entity_q_x = cl_visents.list[i].origin[0]*8;
		entity_q_y = cl_visents.list[i].origin[1]*8;

		// Convert from quake coordiantes -> pixel coordinates.
		entity_p_x = x + ROUND((map_x_slope*entity_q_x + map_x_intercept) * scale);
		entity_p_y = y + ROUND((map_y_slope*entity_q_y + map_y_intercept) * scale);

		// TODO: Replace all model name comparison below with MOD_HINT's instead for less comparisons (create new ones in Mod_LoadAliasModel() in r_model.c and gl_model.c/.h for the ones that don't have one already).

		//
		// Powerups.
		//

		if(radar_show_pent && !strcmp(cl_visents.list[i].model->name, "progs/invulner.mdl"))
		{
			// Pentagram.
			Draw_ColoredString(entity_p_x, entity_p_y, "&cf00P", 0);
		}
		else if(radar_show_quad && !strcmp(cl_visents.list[i].model->name, "progs/quaddama.mdl"))					
		{	
			// Quad.
			Draw_ColoredString(entity_p_x, entity_p_y, "&c0ffQ", 0);
		}
		else if(radar_show_ring && !strcmp(cl_visents.list[i].model->name, "progs/invisibl.mdl"))
		{
			// Ring.
			Draw_ColoredString(entity_p_x, entity_p_y, "&cff0R", 0);
		}
		else if(radar_show_suit && !strcmp(cl_visents.list[i].model->name, "progs/suit.mdl"))
		{
			// Suit.
			Draw_ColoredString(entity_p_x, entity_p_y, "&c0f0S", 0);
		}					

		//
		// Show RL, LG and backpacks.
		//
		if(radar_show_rl && !strcmp(cl_visents.list[i].model->name, "progs/g_rock2.mdl"))
		{
			// RL.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "RL");
		}		
		else if(radar_show_lg && !strcmp(cl_visents.list[i].model->name, "progs/g_light.mdl"))
		{
			// LG.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "LG");
		}
		else if(radar_show_backpacks && cl_visents.list[i].model->modhint == MOD_BACKPACK)
		{
			// Back packs.
			float back_pack_size = 0;

			back_pack_size = max(player_size * 0.5, 0.05);

			Draw_AlphaCircleFill (entity_p_x, entity_p_y, back_pack_size, 114, 1);
			Draw_AlphaCircleOutline (entity_p_x, entity_p_y, back_pack_size, 1.0, 0, 1);
		}					

		if(!strcmp(cl_visents.list[i].model->name, "progs/armor.mdl"))
		{
			//
			// Show armors.
			//

			if(radar_show_ga && cl_visents.list[i].skinnum == HUD_RADAR_GA)
			{
				// GA.
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, 3.0, 178, 1.0);
			}
			else if(radar_show_ya && cl_visents.list[i].skinnum == HUD_RADAR_YA)
			{
				// YA.
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, 3.0, 192, 1.0);
			}
			else if(radar_show_ra && cl_visents.list[i].skinnum == HUD_RADAR_RA)
			{
				// RA.
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, 3.0, 251, 1.0);
			}
			
			Draw_AlphaCircleOutline (entity_p_x, entity_p_y, 3.0, 1.0, 0, 1.0);
		}

		if(radar_show_mega && !strcmp(cl_visents.list[i].model->name, "maps/b_bh100.bsp"))
		{
			//
			// Show megahealth.
			//

			// Draw a red border around the cross.
			Draw_AlphaOutline (entity_p_x - 3, entity_p_y - 3, 8, 8, 79, 1, 0.8);

			// Draw a black outline cross.
			Draw_AlphaFill (entity_p_x - 3, entity_p_y - 1, 8, 4, 0, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 3, 4, 8, 0, 1);

			// Draw a 2 pixel cross.
			Draw_AlphaFill (entity_p_x - 2, entity_p_y, 6, 2, 79, 1);
			Draw_AlphaFill (entity_p_x, entity_p_y - 2, 2, 6, 79, 1);
		}

		if(radar_show_ssg && !strcmp(cl_visents.list[i].model->name, "progs/g_shot.mdl"))
		{
			// SSG.
			Draw_String(entity_p_x - (3*8)/2, entity_p_y - 4, "SSG");
		}
		else if(radar_show_ng && !strcmp(cl_visents.list[i].model->name, "progs/g_nail.mdl"))
		{
			// NG.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "NG");
		}
		else if(radar_show_sng && !strcmp(cl_visents.list[i].model->name, "progs/g_nail2.mdl"))
		{
			// SNG.
			Draw_String(entity_p_x - (3*8)/2, entity_p_y - 4, "SNG");
		}
		else if(radar_show_gl && !strcmp(cl_visents.list[i].model->name, "progs/g_rock.mdl"))
		{
			// GL.
			Draw_String(entity_p_x - (2*8)/2, entity_p_y - 4, "GL");
		}					

		if(radar_show_gibs 
			&&(!strcmp(cl_visents.list[i].model->name, "progs/gib1.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/gib2.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/gib3.mdl")))
		{
			//
			// Gibs.
			//

			Draw_AlphaCircleFill(entity_p_x, entity_p_y, 2.0, 251, 1);					
		}

		if(radar_show_health 
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_bh25.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_bh10.bsp")))
		{
			//
			// Health.
			//

			// Draw a black outline cross.
			Draw_AlphaFill (entity_p_x - 3, entity_p_y - 1, 7, 3, 0, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 3, 3, 7, 0, 1);
			
			// Draw a cross.
			Draw_AlphaFill (entity_p_x - 2, entity_p_y, 5, 1, 79, 1);
			Draw_AlphaFill (entity_p_x, entity_p_y - 2, 1, 5, 79, 1);
		}

		//
		// Ammo.
		//
		if(radar_show_rockets
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_rock0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_rock1.bsp")))
		{
			//
			// Rockets.
			//

			// Draw a black outline.
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 6, 3, 5, 0, 1);
			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 1, 5, 5, 0, 1);

			// The brown rocket.
			Draw_AlphaFill (entity_p_x, entity_p_y - 5, 1, 5, 120, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y, 1, 3, 120, 1);
			Draw_AlphaFill (entity_p_x + 1, entity_p_y, 1, 3, 120, 1);
		}

		if(radar_show_cells 
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_batt0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_batt1.bsp")))
		{
			//
			// Cells.
			//

			// Draw a black outline.
			Draw_AlphaLine(entity_p_x - 3, entity_p_y, entity_p_x + 4, entity_p_y - 5, 3, 0, 1);
			Draw_AlphaLine(entity_p_x - 3, entity_p_y, entity_p_x + 3 , entity_p_y, 3, 0, 1);
			Draw_AlphaLine(entity_p_x + 3, entity_p_y, entity_p_x - 3, entity_p_y + 4, 3, 0, 1);

			// Draw a yellow lightning!
			Draw_AlphaLine(entity_p_x - 2, entity_p_y, entity_p_x + 3, entity_p_y - 4, 1, 111, 1);
			Draw_AlphaLine(entity_p_x - 2, entity_p_y, entity_p_x + 2 , entity_p_y, 1, 111, 1);
			Draw_AlphaLine(entity_p_x + 2, entity_p_y, entity_p_x - 2, entity_p_y + 3, 1, 111, 1);
		}

		if(radar_show_nails
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_nail0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_nail1.bsp")))
		{
			//
			// Nails.
			//

			// Draw a black outline.
			Draw_AlphaFill (entity_p_x - 3, entity_p_y - 3, 7, 3, 0, 1);
			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 2, 5, 3, 0, 0.5);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y, 3, 3, 0, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y + 3, 1, 1, 0, 0.5);
			Draw_AlphaFill (entity_p_x + 1, entity_p_y + 3, 1, 1, 0, 0.5);
			Draw_AlphaFill (entity_p_x, entity_p_y + 4, 1, 1, 0, 1);

			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 2, 5, 1, 6, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 1, 3, 1, 6, 0.5);
			Draw_AlphaFill (entity_p_x, entity_p_y, 1, 4, 6, 1);
		}

		if(radar_show_shells 
			&&(!strcmp(cl_visents.list[i].model->name, "maps/b_shell0.bsp")
			|| !strcmp(cl_visents.list[i].model->name, "maps/b_shell1.bsp")))
		{
			//
			// Shells.
			//

			// Draw a black outline.
			Draw_AlphaFill (entity_p_x - 2, entity_p_y - 3, 5, 9, 0, 1);

			// Draw 2 shotgun shells.
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 2, 1, 4, 73, 1);
			Draw_AlphaFill (entity_p_x - 1, entity_p_y - 2 + 5, 1, 2, 104, 1);

			Draw_AlphaFill (entity_p_x + 1, entity_p_y - 2, 1, 4, 73, 1);
			Draw_AlphaFill (entity_p_x + 1, entity_p_y - 2 + 5, 1, 2, 104, 1);
		} 

		//
		// Show projectiles (rockets, grenades, nails, shaft).
		//

		if(radar_show_nails_p 
			&& (!strcmp(cl_visents.list[i].model->name, "progs/s_spike.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/spike.mdl")))
		{
			//
			// Spikes from SNG and NG.
			//

			Draw_AlphaFill(entity_p_x, entity_p_y, 1, 1, 254, 1);
		}					
		else if(radar_show_rockets_p 
			&& (!strcmp(cl_visents.list[i].model->name, "progs/missile.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/grenade.mdl")))
		{
			//
			// Rockets and grenades.
			//

			float entity_angle = 0;
			int x_line_end = 0;
			int y_line_end = 0;

			// Get the entity angle in radians.
			entity_angle = DEG2RAD(cl_visents.list[i].angles[1]);

			x_line_end = entity_p_x + 5 * cos(entity_angle) * scale;
			y_line_end = entity_p_y - 5 * sin(entity_angle) * scale;

			// Draw the rocket/grenade showing it's angle also.
			Draw_AlphaLine (entity_p_x, entity_p_y, x_line_end, y_line_end, 1.0, 254, 1);				
		}
		else if(radar_show_shaft_p 
			&& (!strcmp(cl_visents.list[i].model->name, "progs/bolt.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/bolt2.mdl")
			|| !strcmp(cl_visents.list[i].model->name, "progs/bolt3.mdl")))
		{
			//
			// Shaft beam.
			//

			float entity_angle = 0;
			float shaft_length = 0;
			float x_line_end = 0;
			float y_line_end = 0;

			// Get the length and angle of the shaft.
			shaft_length = cl_visents.list[i].model->maxs[1];
			entity_angle = (cl_visents.list[i].angles[1]*M_PI)/180;

			// Calculate where the shaft beam's ending point.
			x_line_end = entity_p_x + shaft_length * cos(entity_angle);
			y_line_end = entity_p_y - shaft_length * sin(entity_angle);

			// Draw the shaft beam.
			Draw_AlphaLine (entity_p_x, entity_p_y, x_line_end, y_line_end, 1.0, 254, 1);
		}
	}

	// Draw a circle around "hold areas", the grid cells within this circle
	// are the ones that are counted for that particular hold area. The team
	// that has the most percentage of these cells is considered to hold that area.
	if(show_hold_areas && stats_important_ents != NULL && stats_important_ents->list != NULL)
	{
		int entity_p_x = 0;
		int entity_p_y = 0;

		for(i = 0; i < stats_important_ents->count; i++)
		{
			entity_p_x = x + ROUND((map_x_slope*8*stats_important_ents->list[i].origin[0] + map_x_intercept) * scale);
			entity_p_y = y + ROUND((map_y_slope*8*stats_important_ents->list[i].origin[1] + map_y_intercept) * scale);

			Draw_ColoredString(entity_p_x  - (8 * strlen(stats_important_ents->list[i].name)) / 2.0, entity_p_y - 4, 
				va("&c55f%s", stats_important_ents->list[i].name), 0);

			Draw_AlphaCircleOutline(entity_p_x , entity_p_y, map_x_slope * 8 * stats_important_ents->hold_radius * scale, 1.0, 15, 0.2);
		}
	}

	//
	// Draw temp entities (explosions, blood, teleport effects).
	//
	for(i = 0; i < MAX_TEMP_ENTITIES; i++)
	{
		float time_diff = 0.0;

		int entity_q_x = 0;
		int entity_q_y = 0;
		int entity_p_x = 0;
		int entity_p_y = 0;

		// Get the time since the entity spawned.
		time_diff = cls.demotime - temp_entities.list[i].time;

		// Don't show temp entities for long.
		if(time_diff < 0.25)
		{
			float radius = 0.0;
			radius = (time_diff < 0.125) ? (time_diff * 32.0) : (time_diff * 32.0) - time_diff;
			radius *= scale;

			// Get quake coordinates (times 8 to get them in the same format as .locs).
			entity_q_x = temp_entities.list[i].pos[0]*8;
			entity_q_y = temp_entities.list[i].pos[1]*8;

			entity_p_x = x + ROUND((map_x_slope*entity_q_x + map_x_intercept) * scale);
			entity_p_y = y + ROUND((map_y_slope*entity_q_y + map_y_intercept) * scale);

			if(radar_show_explosions
				&& (temp_entities.list[i].type == TE_EXPLOSION 
				|| temp_entities.list[i].type == TE_TAREXPLOSION))
			{
				//
				// Explosions.
				//

				Draw_AlphaCircleFill (entity_p_x, entity_p_y, radius, 235, 0.8);
			}
			else if(radar_show_teleport && temp_entities.list[i].type == TE_TELEPORT)
			{
				//
				// Teleport effect.
				//

				radius *= 1.5;
				Draw_AlphaCircleFill (entity_p_x, entity_p_y, radius, 244, 0.8);
			}
			else if(radar_show_shotgun && temp_entities.list[i].type == TE_GUNSHOT)
			{
				//
				// Shotgun fire.
				//

				#define SHOTGUN_SPREAD 10
				int spread_x = 0;
				int spread_y = 0;
				int n = 0;

				for(n = 0; n < 10; n++)
				{
					spread_x = (int)(rand() / (((double)RAND_MAX + 1) / SHOTGUN_SPREAD));
					spread_y = (int)(rand() / (((double)RAND_MAX + 1) / SHOTGUN_SPREAD));
					
					Draw_AlphaFill (entity_p_x + spread_x - (SHOTGUN_SPREAD/2), entity_p_y + spread_y - (SHOTGUN_SPREAD/2), 1, 1, 8, 0.9);
				}
			}
		}
	}
}

void Radar_DrawPlayers(int x, int y, int width, int height, float scale, 
					   float show_height, float show_powerups, 
					   float player_size, float show_names, 
					   float fade_players, float highlight,
					   char *highlight_color)
{
	int i;
	player_state_t *state;
	player_info_t *info;

	// Get player state so we can know where he is (or on rare occassions, she).
	state = cl.frames[cl.oldparsecount & UPDATE_MASK].playerstate;

	// Get the info for the player.
	info = cl.players;

	//
	// Draw the players.
	//
	for (i = 0; i < MAX_CLIENTS; i++, info++, state++)
	{
		// Players quake coordinates
		// (these are multiplied by 8, since the conversion formula was
		// calculated using the coordinates in a .loc-file, which are in
		// the format quake-coordainte*8).
		int player_q_x = 0;
		int player_q_y = 0;

		// The height of the player.
		float player_z = 1.0;
		float player_z_relative = 1.0;

		// Players pixel coordinates.
		int player_p_x = 0;
		int player_p_y = 0;

		// Used for drawing the the direction the
		// player is looking at.
		float player_angle = 0;
		int x_line_start = 0;
		int y_line_start = 0;
		int x_line_end = 0;
		int y_line_end = 0;

		// Color and opacity of the player.
		int player_color = 0;
		float player_alpha = 1.0;

		// Make sure we're not drawing any ghosts.
		if(!info->name[0])
		{
			continue;
		}

		if (state->messagenum == cl.oldparsecount)
		{
			// Get the quake coordinates. Multiply by 8 since
			// the conversion formula has been calculated using
			// a .loc-file which is in that format.
			player_q_x = state->origin[0]*8;
			player_q_y = state->origin[1]*8;

			// Get the players view angle.
			player_angle = state->viewangles[1];
			// TODO: Get the "current" viewangle for the player being spectated when not watching demos here also.

			// Convert from quake coordiantes -> pixel coordinates.
			player_p_x = ROUND((map_x_slope*player_q_x + map_x_intercept) * scale);
			player_p_y = ROUND((map_y_slope*player_q_y + map_y_intercept) * scale);

			player_color = Sbar_BottomColor(info);

			// Calculate the height of the player.
			if(show_height)
			{
				player_z = state->origin[2];
				player_z += (player_z >= 0) ? fabs(cl.worldmodel->mins[2]) : fabs(cl.worldmodel->maxs[2]);
				player_z_relative = min(fabs(player_z / map_height_diff), 1.0);
				player_z_relative = max(player_z_relative, 0.2);
			}

			// Make the players fade out as they get less armor/health.
			if(fade_players)
			{
				int armor_strength = 0;
				armor_strength = (info->stats[STAT_ITEMS] & IT_ARMOR1) ? 100 : 
					((info->stats[STAT_ITEMS] & IT_ARMOR2) ? 150 : 
					((info->stats[STAT_ITEMS] & IT_ARMOR3) ? 200 : 0));

				// Don't let the players get completly transparent so add 0.2 to the final value.
				player_alpha = ((info->stats[STAT_HEALTH] + (info->stats[STAT_ARMOR] * armor_strength)) / 100.0) + 0.2;
			}

			// Turn dead people red.
			if(info->stats[STAT_HEALTH] <= 0)
			{
				player_alpha = 1.0;
				player_color = 79;
			}

			// Draw a ring around players with powerups if it's enabled.
			if(show_powerups)
			{
				if(info->stats[STAT_ITEMS] & IT_INVISIBILITY)
				{
					Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size*2*player_z_relative, 161, 0.2);
				}

				if(info->stats[STAT_ITEMS] & IT_INVULNERABILITY)
				{
					Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size*2*player_z_relative, 79, 0.5);
				}

				if(info->stats[STAT_ITEMS] & IT_QUAD)
				{
					Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size*2*player_z_relative, 244, 0.2);
				}
			}

			#define HUD_RADAR_HIGHLIGHT_NONE			0
			#define HUD_RADAR_HIGHLIGHT_TEXT_ONLY		1
			#define HUD_RADAR_HIGHLIGHT_OUTLINE			2
			#define HUD_RADAR_HIGHLIGHT_FIXED_OUTLINE	3
			#define HUD_RADAR_HIGHLIGHT_CIRCLE			4
			#define HUD_RADAR_HIGHLIGHT_FIXED_CIRCLE	5
			#define HUD_RADAR_HIGHLIGHT_ARROW_BOTTOM	6
			#define HUD_RADAR_HIGHLIGHT_ARROW_CENTER	7
			#define HUD_RADAR_HIGHLIGHT_ARROW_TOP		8 
			#define HUD_RADAR_HIGHLIGHT_CROSS_CORNERS	9

			// Draw a circle around the tracked player.
			if (highlight != HUD_RADAR_HIGHLIGHT_NONE && info->userid == cl.players[Cam_TrackNum()].userid)
			{
				extern int HexToInt(char c);
				float r, g, b, highlight_alpha;

				// Get the highlight color.
				r = hud_radar_highlight_color[0];
				g = hud_radar_highlight_color[1];
				b = hud_radar_highlight_color[2];
				highlight_alpha = hud_radar_highlight_color[3];
				
				// Draw the highlight.
				switch ((int)highlight)
				{
					case HUD_RADAR_HIGHLIGHT_CROSS_CORNERS :
					{
						// Top left
						Draw_AlphaLineRGB (x, y, x + player_p_x, y + player_p_y, 2, r, g, b, highlight_alpha);

						// Top right.
						Draw_AlphaLineRGB (x + width, y, x + player_p_x, y + player_p_y, 2, r, g, b, highlight_alpha);

						// Bottom left.
						Draw_AlphaLineRGB (x, y + height, x + player_p_x, y + player_p_y, 2, r, g, b, highlight_alpha);

						// Bottom right.
						Draw_AlphaLineRGB (x + width, y + height, x + player_p_x, y + player_p_y, 2, r, g, b, highlight_alpha);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_ARROW_TOP :
					{
						// Top center.
						Draw_AlphaLineRGB (x + width / 2, y, x + player_p_x, y + player_p_y, 2, r, g, b, highlight_alpha);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_ARROW_CENTER :
					{
						// Center.
						Draw_AlphaLineRGB (x + width / 2, y + height / 2, x + player_p_x, y + player_p_y, 2, r, g, b, highlight_alpha);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_ARROW_BOTTOM :
					{
						// Bottom center.
						Draw_AlphaLineRGB (x + width / 2, y + height, x + player_p_x, y + player_p_y, 2, r, g, b, highlight_alpha);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_FIXED_CIRCLE :
					{
						Draw_AlphaCircleRGB (x + player_p_x, y + player_p_y, player_size * 1.5, 1.0, true, r, g, b, highlight_alpha);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_CIRCLE :
					{
						Draw_AlphaCircleRGB (x + player_p_x, y + player_p_y, player_size * player_z_relative * 2.0, 1.0, true, r, g, b, highlight_alpha);
						break;
					}					
					case HUD_RADAR_HIGHLIGHT_FIXED_OUTLINE :
					{
						Draw_AlphaCircleOutlineRGB (x + player_p_x, y + player_p_y, player_size * 1.5, 1.0, r, g, b, highlight_alpha);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_OUTLINE :
					{
						Draw_AlphaCircleOutlineRGB (x + player_p_x, y + player_p_y, player_size * player_z_relative * 2.0, 1.0, r, g, b, highlight_alpha);
						break;
					}
					case HUD_RADAR_HIGHLIGHT_TEXT_ONLY :
					default :
					{						
						break;
					}
				}
			}

			// Draw the actual player and a line showing what direction the player is looking in.
			{				
				float relative_x = 0;
				float relative_y = 0;

				x_line_start = x + player_p_x;
				y_line_start = y + player_p_y;

				// Translate the angle into radians.
				player_angle = DEG2RAD(player_angle);

				relative_x = cos(player_angle);
				relative_y = sin(player_angle);

				// Draw a slightly larger line behind the colored one
				// so that it get's an outline.
				x_line_end = x_line_start + (player_size*2*player_z_relative+1)*relative_x;
				y_line_end = y_line_start - (player_size*2*player_z_relative+1)*relative_y;
				Draw_AlphaLine (x_line_start, y_line_start, x_line_end, y_line_end, 4.0, 0, 1.0);

				// Draw the colored line.
				x_line_end = x_line_start + (player_size*2*player_z_relative)*relative_x;
				y_line_end = y_line_start - (player_size*2*player_z_relative)*relative_y;
				Draw_AlphaLine (x_line_start, y_line_start, x_line_end, y_line_end, 2.0, player_color, player_alpha);

				// Draw the player on the map.
				Draw_AlphaCircleFill (x + player_p_x, y + player_p_y, player_size * player_z_relative, player_color, player_alpha);
				Draw_AlphaCircleOutline (x + player_p_x, y + player_p_y, player_size * player_z_relative, 1.0, 0, 1.0);
			}

			// Draw the players name.
			if(show_names)
			{
				int name_x = 0;
				int name_y = 0;

				name_x = x + player_p_x;
				name_y = y + player_p_y;

				// Make sure we're not too far right.
				while(name_x + 8 * strlen(info->name) > x + width)
				{
					name_x--;
				}

				// Make sure we're not outside the radar to the left.
				name_x = max(name_x, x);

				// Draw the name.
				if (highlight >= HUD_RADAR_HIGHLIGHT_TEXT_ONLY 
					&& info->userid == cl.players[Cam_TrackNum()].userid)
				{
					// Draw the tracked players name in the user specified color.
					Draw_ColoredString (name_x, name_y, 
						va("&c%x%x%x%s", 
						(unsigned int)(hud_radar_highlight_color[0] * 15), 
						(unsigned int)(hud_radar_highlight_color[1] * 15), 
						(unsigned int)(hud_radar_highlight_color[2] * 15), info->name), 0);
				}
				else
				{
					// Draw other players in normal character color.
					Draw_String (name_x, name_y, info->name);
				}
			}

			// Show if a person lost an RL-pack.
			if(info->stats[STAT_HEALTH] <= 0 && info->stats[STAT_ACTIVEWEAPON] == IT_ROCKET_LAUNCHER)
			{
				Draw_AlphaCircleOutline (x + player_p_x, y + player_p_y, player_size*player_z_relative*2, 1.0, 254, player_alpha);
				Draw_ColoredString (x + player_p_x, y + player_p_y, va("&cf00PACK!"), 1);
			}
		}
	}		
}

//
// Draws a map of the current level and plots player movements on it.
//
void SCR_HUD_DrawRadar(hud_t *hud)
{
	int width, height, x, y;
	float width_limit, height_limit;
	float scale;
	float x_scale;
	float y_scale;

    static cvar_t
		*hud_radar_opacity = NULL,
		*hud_radar_width,
		*hud_radar_height,
		*hud_radar_autosize,
		*hud_radar_fade_players,
		*hud_radar_show_powerups,
		*hud_radar_show_names,
		*hud_radar_highlight,
		*hud_radar_highlight_color,
		*hud_radar_player_size,
		*hud_radar_show_height,
		*hud_radar_show_stats,
		*hud_radar_show_hold,
		*hud_radar_weaponfilter,
		*hud_radar_itemfilter,
		*hud_radar_onlytp,
		*hud_radar_otherfilter;

    if (hud_radar_opacity == NULL)    // first time
    {
		hud_radar_opacity			= HUD_FindVar(hud, "opacity");
		hud_radar_width				= HUD_FindVar(hud, "width");
		hud_radar_height			= HUD_FindVar(hud, "height");
		hud_radar_autosize			= HUD_FindVar(hud, "autosize");
		hud_radar_fade_players		= HUD_FindVar(hud, "fade_players");
		hud_radar_show_powerups		= HUD_FindVar(hud, "show_powerups");
		hud_radar_show_names		= HUD_FindVar(hud, "show_names");
		hud_radar_player_size		= HUD_FindVar(hud, "player_size");
		hud_radar_show_height		= HUD_FindVar(hud, "show_height");
		hud_radar_show_stats		= HUD_FindVar(hud, "show_stats");
		hud_radar_show_hold			= HUD_FindVar(hud, "show_hold");
		hud_radar_weaponfilter		= HUD_FindVar(hud, "weaponfilter");
		hud_radar_itemfilter		= HUD_FindVar(hud, "itemfilter");
		hud_radar_otherfilter		= HUD_FindVar(hud, "otherfilter");
		hud_radar_onlytp			= HUD_FindVar(hud, "onlytp");
		hud_radar_highlight			= HUD_FindVar(hud, "highlight");
		hud_radar_highlight_color	= HUD_FindVar(hud, "highlight_color");

		//
		// Only parse the the filters when they change, not on each frame.
		//

		// Weapon filter.
		hud_radar_weaponfilter->OnChange = Radar_OnChangeWeaponFilter;
		Radar_OnChangeWeaponFilter (hud_radar_weaponfilter, hud_radar_weaponfilter->string);

		// Item filter.
		hud_radar_itemfilter->OnChange = Radar_OnChangeItemFilter;
		Radar_OnChangeItemFilter (hud_radar_itemfilter, hud_radar_itemfilter->string);

		// Other filter.
		hud_radar_otherfilter->OnChange = Radar_OnChangeOtherFilter;
		Radar_OnChangeOtherFilter (hud_radar_otherfilter, hud_radar_otherfilter->string);

		// Highlight color.
		hud_radar_highlight_color->OnChange = Radar_OnChangeHighlightColor;
		Radar_OnChangeHighlightColor (hud_radar_highlight_color, hud_radar_highlight_color->string);
    }

	// Don't show anything if it's a normal player.
	if(!(cls.demoplayback || cl.spectator))
	{
		return;
	}

	// Don't show when not in teamplay/demoplayback.
	if(!HUD_ShowInDemoplayback(hud_radar_onlytp->value))
	{
		return;
	}

	// Save the width and height of the HUD. We're using these because
	// if autosize is on these will be altered and we don't want to change
	// the settings that the user set, if we try, and the user turns off
	// autosize again the size of the HUD will remain "autosized" until the user
	// resets it by hand again.
	width_limit = hud_radar_width->value;
	height_limit = hud_radar_height->value;

	// This map doesn't have a map pic.
	if((radar_pic == NULL || !radar_pic_found) && HUD_PrepareDraw(hud, ROUND(width_limit) , ROUND(height_limit), &x, &y))
	{
		Draw_String(x, y, "No radar picture found!");
		return;
	}

	// Make sure we can translate the coordinates.
	if(!conversion_formula_found && HUD_PrepareDraw(hud, ROUND(width_limit) , ROUND(height_limit), &x, &y))
	{
		Draw_String(x, y, "No conversion formula found!");
		return;
	}

	x = 0;
	y = 0;

	scale = 1;

	if(hud_radar_autosize->value)
	{
		//
		// Autosize the hud element based on the size of the radar picture.
		//

		width = width_limit = radar_pic->width;
		height = height_limit = radar_pic->height;
	}
	else
	{
		//
		// Size the picture so that it fits inside the hud element.
		//

		// Set the scaling based on the picture dimensions.
		x_scale = (width_limit / radar_pic->width);
		y_scale = (height_limit / radar_pic->height);

		scale = (x_scale < y_scale) ? x_scale : y_scale;

		width = radar_pic->width * scale;
		height = radar_pic->height * scale;
	}

	if (HUD_PrepareDraw(hud, ROUND(width_limit) , ROUND(height_limit), &x, &y))
	{
		float player_size = 1.0;
		static int lastframecount = -1;

		// Place the map picture in the center of the HUD element.
		x += ROUND((width_limit / 2.0) - (width / 2.0));
		x = max(0, x);
		x = min(x + width, x);

		y += ROUND((height_limit / 2.0) - (height / 2.0));
		y = max(0, y);
		y = min(y + height, y);

		// Draw the radar background.
		Draw_SAlphaPic (x, y, radar_pic, hud_radar_opacity->value, scale);

		// Only draw once per frame.
		if (cls.framecount == lastframecount)
		{
			return;
		}
		lastframecount = cls.framecount;

		if (!cl.oldparsecount || !cl.parsecount || cls.state < ca_active)
		{
			return;
		}

		// Scale the player size after the size of the radar.
		player_size = hud_radar_player_size->value * scale;

		// Draw team stats.
		if(hud_radar_show_stats->value)
		{
			Radar_DrawGrid(stats_grid, x, y, scale, width, height, hud_radar_show_stats->value);
		}

		// Draw entities such as powerups, weapons and backpacks.
		if(RADAR_SHOW_WEAPONS || RADAR_SHOW_ITEMS || RADAR_SHOW_OTHER)
		{
			Radar_DrawEntities(x, y, scale, 
				player_size,
				hud_radar_show_hold->value);
		}

		// Draw the players.
		Radar_DrawPlayers(x, y, width, height, scale, 
			hud_radar_show_height->value,
			hud_radar_show_powerups->value,
			player_size,
			hud_radar_show_names->value,
			hud_radar_fade_players->value,
			hud_radar_highlight->value,
			hud_radar_highlight_color->string);
	}
}

#endif

// ----------------
// Init
// and add some common elements to hud (clock etc)
//

void CommonDraw_Init(void)
{
    int i;

    // variables
    Cvar_SetCurrentGroup(CVAR_GROUP_HUD);
	Cvar_Register (&hud_planmode);
    Cvar_Register (&hud_tp_need);
    Cvar_ResetCurrentGroup();

    // init HUD STAT table
    for (i=0; i < MAX_CL_STATS; i++)
        hud_stats[i] = 0;
    hud_stats[STAT_HEALTH]  = 200;
    hud_stats[STAT_AMMO]    = 100;
    hud_stats[STAT_ARMOR]   = 200;
    hud_stats[STAT_SHELLS]  = 200;
    hud_stats[STAT_NAILS]   = 200;
    hud_stats[STAT_ROCKETS] = 100;
    hud_stats[STAT_CELLS]   = 100;
    hud_stats[STAT_ACTIVEWEAPON] = 32;
    hud_stats[STAT_ITEMS] = 0xffffffff - IT_ARMOR2 - IT_ARMOR1;

    // init gameclock
	HUD_Register("gameclock", NULL, "Shows current game time (hh:mm:ss).",
        HUD_PLUSMINUS, ca_disconnected, 8, SCR_HUD_DrawGameClock,
        "1", "top", "right", "console", "0", "0", "0", "0 0 0",
        "big",      "1",
        "style",    "0",
		"scale",    "1",
        "blink",    "1",
		"countdown","0",
        NULL);

	// fps
	HUD_Register("fps", /*"show_fps"*/ NULL, // hexum -> don't support aliases for now
        "Shows your current framerate in frames per second (fps). "
        "Can also show minimum framerate, that occured in last measure period.",
        HUD_PLUSMINUS, ca_active, 9, SCR_HUD_DrawFPS,
        "1", "gameclock", "center", "after", "0", "0", "0", "0 0 0",
        "show_min", "0",
        "title",    "1",
		"decimals", "1",
        NULL);

	HUD_Register("mouserate", NULL, "Show your current mouse input rate", HUD_PLUSMINUS, ca_active, 9,
		SCR_HUD_DrawMouserate,
		"0", "screen", "left", "bottom", "0", "0", "0", "0 0 0",
		"title", "1",
		"interval", "1",
		NULL);

    // init clock
	HUD_Register("clock", NULL, "Shows current local time (hh:mm:ss).",
        HUD_PLUSMINUS, ca_disconnected, 8, SCR_HUD_DrawClock,
        "0", "top", "right", "console", "0", "0", "0", "0 0 0",
        "big",      "1",
        "style",    "0",
		"scale",    "1",
        "blink",    "1",
        NULL);

    // init democlock
	HUD_Register("democlock", NULL, "Shows current demo time (hh:mm:ss).",
        HUD_PLUSMINUS, ca_disconnected, 7, SCR_HUD_DrawDemoClock,
        "1", "top", "right", "console", "0", "8", "0", "0 0 0",
        "big",      "0",
        "style",    "0",
		"scale",    "1",
        "blink",    "0",
        NULL);

    // init ping
    HUD_Register("ping", NULL, "Shows most important net conditions, like ping and pl. Shown only when you are connected to a server.",
        HUD_PLUSMINUS, ca_active, 9, SCR_HUD_DrawPing,
        "0", "screen", "left", "bottom", "0", "0", "0", "0 0 0",
        "period",       "1",
        "show_pl",      "1",
        "show_min",     "0",
        "show_max",     "0",
        "show_dev",     "0",
        "blink",        "1",
        NULL);

	// init net
    HUD_Register("net", NULL, "Shows network statistics, like latency, packet loss, average packet sizes and bandwidth. Shown only when you are connected to a server.",
        HUD_PLUSMINUS, ca_active, 7, SCR_HUD_DrawNetStats,
        "0", "top", "left", "center", "0", "0", "0.2", "0 0 0",
        "period",  "1",
        NULL);

    // init speed
    HUD_Register("speed", NULL, "Shows your current running speed. It is measured over XY or XYZ axis depending on \'xyz\' property.",
        HUD_PLUSMINUS, ca_active, 7, SCR_HUD_DrawSpeed,
        "0", "top", "center", "bottom", "0", "-5", "0", "0 0 0",
        "xyz",  "0",
		"width", "160",
		"height", "15",
		"opacity", "1.0",
		"tick_spacing", "0.2",
		"color_stopped", SPEED_STOPPED,
		"color_normal", SPEED_NORMAL,
		"color_fast", SPEED_FAST,
		"color_fastest", SPEED_FASTEST,
		"color_insane", SPEED_INSANE,
		"vertical", "0",
		"vertical_text", "1",
		"text_align", "1",
		NULL);

#ifdef GLQUAKE
	// Init speed2 (half circle thingie).
	HUD_Register("speed2", NULL, "Shows your current running speed. It is measured over XY or XYZ axis depending on \'xyz\' property.",
        HUD_PLUSMINUS, ca_active, 7, SCR_HUD_DrawSpeed2,
        "0", "top", "center", "bottom", "0", "0", "0", "0 0 0",
        "xyz",  "0",
		"opacity", "1.0",
		"color_stopped", SPEED_STOPPED,
		"color_normal", SPEED_NORMAL,
		"color_fast", SPEED_FAST,
		"color_fastest", SPEED_FASTEST,
		"color_insane", SPEED_INSANE,
		"radius", "50.0",
		"wrapspeed", "500",
		"orientation", "0",
		NULL);
#endif

    // init guns
    HUD_Register("gun", NULL, "Part of your inventory - current weapon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGunCurrent,
        "0", "ibar", "center", "bottom", "0", "0", "0", "0 0 0",
        "wide",  "0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun2", NULL, "Part of your inventory - shotgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun2,
        "1", "ibar", "left", "bottom", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun3", NULL, "Part of your inventory - super shotgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun3,
        "1", "gun2", "after", "center", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun4", NULL, "Part of your inventory - nailgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun4,
        "1", "gun3", "after", "center", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun5", NULL, "Part of your inventory - super nailgun.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun5,
        "1", "gun4", "after", "center", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun6", NULL, "Part of your inventory - grenade launcher.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun6,
        "1", "gun5", "after", "center", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun7", NULL, "Part of your inventory - rocket launcher.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun7,
        "1", "gun6", "after", "center", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("gun8", NULL, "Part of your inventory - thunderbolt.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawGun8,
        "1", "gun7", "after", "center", "0", "0", "0", "0 0 0",
        "wide",  "0",
        "style", "0",
        "scale", "1",
        NULL);

    // init powerzz
    HUD_Register("key1", NULL, "Part of your inventory - silver key.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawKey1,
        "1", "ibar", "top", "left", "0", "64", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("key2", NULL, "Part of your inventory - gold key.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawKey2,
        "1", "key1", "left", "after", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("ring", NULL, "Part of your inventory - invisibility.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawRing,
        "1", "key2", "left", "after", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("pent", NULL, "Part of your inventory - invulnerability.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawPent,
        "1", "ring", "left", "after", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("suit", NULL, "Part of your inventory - biosuit.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSuit,
        "1", "pent", "left", "after", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("quad", NULL, "Part of your inventory - quad damage.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawQuad,
        "1", "suit", "left", "after", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);

    // sigilzz
    HUD_Register("sigil1", NULL, "Part of your inventory - sigil 1.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil1,
        "0", "ibar", "left", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("sigil2", NULL, "Part of your inventory - sigil 2.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil2,
        "0", "sigil1", "after", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("sigil3", NULL, "Part of your inventory - sigil 3.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil3,
        "0", "sigil2", "after", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("sigil4", NULL, "Part of your inventory - sigil 4.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawSigil4,
        "0", "sigil3", "after", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);

	// health
    HUD_Register("health", NULL, "Part of your status - health level.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawHealth,
        "1", "screen", "center", "bottom", "0", "0", "0", "0 0 0",
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
        NULL);

    // ammo/s
    HUD_Register("ammo", NULL, "Part of your inventory - ammo for active weapon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoCurrent,
        "1", "health", "after", "center", "32", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo1", NULL, "Part of your inventory - ammo - shells.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo1,
        "0", "ibar", "left", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo2", NULL, "Part of your inventory - ammo - nails.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo2,
        "0", "ammo1", "after", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo3", NULL, "Part of your inventory - ammo - rockets.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo3,
        "0", "ammo2", "after", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);
    HUD_Register("ammo4", NULL, "Part of your inventory - ammo - cells.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmo4,
        "0", "ammo3", "after", "top", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        "align", "right",
        "digits", "3",
        NULL);

    // ammo icon/s
    HUD_Register("iammo", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIconCurrent,
        "1", "ammo", "before", "center", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);
    HUD_Register("iammo1", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon1,
        "0", "ibar", "left", "top", "0", "0", "0", "0 0 0",
        "style", "2",
        "scale", "1",
        NULL);
    HUD_Register("iammo2", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon2,
        "0", "iammo1", "after", "top", "0", "0", "0", "0 0 0",
        "style", "2",
        "scale", "1",
        NULL);
    HUD_Register("iammo3", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon3,
        "0", "iammo2", "after", "top", "0", "0", "0", "0 0 0",
        "style", "2",
        "scale", "1",
        NULL);
    HUD_Register("iammo4", NULL, "Part of your inventory - ammo icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawAmmoIcon4,
        "0", "iammo3", "after", "top", "0", "0", "0", "0 0 0",
        "style", "2",
        "scale", "1",
        NULL);

    // player face (health indicator)
    HUD_Register("face", NULL, "Your bloody face.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawFace,
        "1", "screen", "center", "bottom", "0", "0", "0", "0 0 0",
        "scale", "1",
        NULL);

    // armor count
    HUD_Register("armor", NULL, "Part of your inventory - armor level.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawArmor,
        "1", "face", "before", "center", "-32", "0", "0", "0 0 0",
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
        NULL);

	// armor icon
    HUD_Register("iarmor", NULL, "Part of your inventory - armor icon.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawArmorIcon,
        "1", "armor", "before", "center", "0", "0", "0", "0 0 0",
        "style", "0",
        "scale", "1",
        NULL);

	// Tracking JohnNy_cz (Contains name of the player who's player we're watching at the moment)
	HUD_Register("tracking", NULL, "Shows the name of tracked player.",
		HUD_PLUSMINUS, ca_active, 9, SCR_HUD_DrawTracking,
		"1", "face", "center", "before", "0", "0", "0", "0 0 0",
		"format", "Tracking %t %n, [JUMP] for next",
		NULL);

    // groups
    HUD_Register("group1", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group1,
        "1", "screen", "center", "bottom", "0", "0", ".5", "0 0 0",
        "name", "group1",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group2", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group2,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group2",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group3", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group3,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group3",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group4", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group4,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group4",
        "width", "64",
        "height", "64",
        "picture", "",
        "pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group5", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group5,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group5",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group6", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group6,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group6",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group7", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group7,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group7",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group8", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group8,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group8",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);
    HUD_Register("group9", NULL, "Group element.",
        HUD_NO_GROW, ca_disconnected, 0, SCR_HUD_Group9,
        "0", "screen", "left", "top", "0", "0", ".5", "0 0 0",
        "name", "group9",
        "width", "64",
        "height", "64",
        "picture", "",
		"pic_alpha", "1.0",
        "pic_scalemode", "0",
        NULL);

#ifdef GLQUAKE
	// healthdamage
    HUD_Register("healthdamage", NULL, "Shows amount of damage done to your health.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawHealthDamage,
        "0", "health", "left", "before", "0", "0", "0", "0 0 0",
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
		"duration", "0.8",
        NULL);

    // armordamage
    HUD_Register("armordamage", NULL, "Shows amount of damage done to your armour.",
        HUD_INVENTORY, ca_active, 0, SCR_HUD_DrawArmorDamage,
        "0", "armor", "left", "before", "0", "0", "0", "0 0 0",
        "style",  "0",
        "scale",  "1",
        "align",  "right",
        "digits", "3",
		"duration", "0.8",
        NULL);
#endif

    HUD_Register("frags", NULL, "Show list of player frags in short form.",
        0, ca_active, 0, SCR_HUD_DrawFrags,
        "0", "top", "right", "bottom", "0", "0", "0", "0 0 0",
        "cell_width", "32",
        "cell_height", "8",
        "rows", "1",
        "cols", "4",
        "space_x", "1",
        "space_y", "1",
        "teamsort", "0",
        "strip", "1",
        "vertical", "0",
		"shownames", "0",
		"showteams", "0",
		"padtext", "1",
		//"showself_always", "1", // Doesn't work anyway.
		"extra_spec_info", "1",
		"fliptext", "0",
		"style", "0",
        NULL);

    HUD_Register("teamfrags", NULL, "Show list of team frags in short form.",
        0, ca_active, 0, SCR_HUD_DrawTeamFrags,
        "1", "ibar", "center", "before", "0", "0", "0", "0 0 0",
        "cell_width", "32",
        "cell_height", "8",
        "rows", "1",
        "cols", "2",
        "space_x", "1",
        "space_y", "1",
        "strip", "1",
        "vertical", "0",
		"shownames", "0",
		"padtext", "1",
		"fliptext", "1",
		"style", "0",
		"extra_spec_info", "1",
		"onlytp", "0",
		NULL);

	HUD_Register("mp3_title", NULL, "Shows current mp3 playing.",
        HUD_PLUSMINUS, ca_disconnected, 0, SCR_HUD_DrawMP3_Title,
        "0", "top", "right", "bottom", "0", "0", "0", "0 0 0",
		"style",	"2",
		"width",	"512",
		"height",	"8",
		"scroll",	"1",
		"scroll_delay", "0.5",
		"on_scoreboard", "0",
		"wordwrap", "0",
        NULL);

	HUD_Register("mp3_time", NULL, "Shows the time of the current mp3 playing.",
        HUD_PLUSMINUS, ca_disconnected, 0, SCR_HUD_DrawMP3_Time,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0",
		"style",	"0",
		"on_scoreboard", "0",
        NULL);

#ifdef GLQUAKE

	HUD_Register("radar", NULL, "Plots the players on a picture of the map. (Only when watching MVD's or QTV).",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawRadar,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0",
		"opacity", "0.5",
		"width", "200",
		"height", "200",
		"autosize", "0",
		"show_powerups", "1",
		"show_names", "0",
		"highlight_color", "yellow",
		"highlight", "0",
		"player_size", "10",
		"show_height", "1",
		"show_stats", "1",
		"fade_players", "1",
		"show_hold", "0",
		"weaponfilter", "ssg ng sng gl rl lg",
		"itemfilter", "backpack quad pent suit ring health armor shells cells rockets nails mega",
		"otherfilter", "projectiles gibs explosions shotgun",
		"onlytp", "0",
        NULL);
#endif

	HUD_Register("teamholdbar", NULL, "Shows how much of the level (in percent) that is currently being held by either team.",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawTeamHoldBar,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0",
		"opacity", "0.8",
		"width", "200",
		"height", "8",
		"vertical", "0",		
		"vertical_text", "0",
		"show_text", "1",
		"onlytp", "0",
        NULL);

	HUD_Register("teamholdinfo", NULL, "Shows which important items in the level that are being held by the teams.",
        HUD_PLUSMINUS, ca_active, 0, SCR_HUD_DrawTeamHoldInfo,
        "0", "top", "left", "bottom", "0", "0", "0", "0 0 0",
		"opacity", "0.8",
		"width", "200",
		"height", "8",
		"onlytp", "0",
		"style", "1",
		"itemfilter", "quad ra ya ga mega pent rl quad",
        NULL);

/* hexum -> FIXME? this is used only for debug purposes, I wont bother to port it (it shouldnt be too difficult if anyone cares)
#ifdef GLQUAKE
#ifdef _DEBUG
    HUD_Register("framegraph", NULL, "Shows different frame times for debug/profiling purposes.",
        HUD_PLUSMINUS | HUD_ON_SCORES, ca_disconnected, 0, SCR_HUD_DrawFrameGraph,
        "0", "top", "left", "bottom", "0", "0", "2",
        "swap_x",       "0",
        "swap_y",       "0",
        "scale",        "14",
        "width",        "256",
        "height",       "64",
        "alpha",        "1",
        NULL);
#endif
#endif
*/

}

