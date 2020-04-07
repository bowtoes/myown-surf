/* modifier 0 means no modifier */
static int surfuseragent    = 1;  /* Append Surf version to default WebKit user agent */
static int URITitle         = 1; /* URI Title */
static char *fulluseragent  = ""; /* Or override the whole user agent string */
static char *scriptfile     = "~/.surf/script.js";
static char *styledir       = "~/.surf/styles/";
static char *certdir        = "~/.surf/certificates/";
static char *cachedir       = "~/.surf/cache/";
static char *cookiefile     = "~/.surf/cookies.txt";
static char *searchengine   = "https://duckduckgo.com/?q="; /* Space Search */

#define HOMEPAGE "https://duckduckgo.com/" /* Homepage */
#define BM_FILE "~/.surf/bookmarks.txt" /* Better Bookmarks */
#define HS_FILE "~/.surf/history.txt" /* Better History */
static char *historyfile    = HS_FILE; /* Better History */

/* Search Engines */
static SearchEngine searchengines[] = {
	{ "g",   "https://www.google.com/search?q=%s" },
	{ "ddg", "https://duckduckgo.com/?q=%s"       },
};

/* Webkit default features */
/* Highest priority value will be used.
 * Default parameters are priority 0
 * Per-uri parameters are priority 1
 * Command parameters are priority 2
 */
static Parameter defconfig[ParameterLast] = {
	/* parameter                    Arg value       priority */
	[AcceleratedCanvas]   =       { { .i = 1 },     },
	[AccessMicrophone]    =       { { .i = 0 },     },
	[AccessWebcam]        =       { { .i = 0 },     },
	[Certificate]         =       { { .i = 0 },     },
	[CaretBrowsing]       =       { { .i = 0 },     },
	[CookiePolicies]      =       { { .v = "@Aa" }, },
	[DefaultCharset]      =       { { .v = "UTF-8" }, },
	[DiskCache]           =       { { .i = 1 },     },
	[DNSPrefetch]         =       { { .i = 0 },     },
	[FileURLsCrossAccess] =       { { .i = 0 },     },
	[FontSize]            =       { { .i = 12 },    },
	[FrameFlattening]     =       { { .i = 0 },     },
	[Geolocation]         =       { { .i = 0 },     },
	[HideBackground]      =       { { .i = 0 },     },
	[Inspector]           =       { { .i = 0 },     },
	[Java]                =       { { .i = 1 },     },
	[JavaScript]          =       { { .i = 1 },     },
	[KioskMode]           =       { { .i = 0 },     },
	[LoadImages]          =       { { .i = 1 },     },
	[MediaManualPlay]     =       { { .i = 1 },     },
	[Plugins]             =       { { .i = 1 },     },
	[PreferredLanguages]  =       { { .v = (char *[]){ NULL } }, },
	[RunInFullscreen]     =       { { .i = 0 },     },
	[ScrollBars]          =       { { .i = 1 },     },
	[ShowIndicators]      =       { { .i = 1 },     },
	[SiteQuirks]          =       { { .i = 1 },     },
	[SmoothScrolling]     =       { { .i = 0 },     },
	[SpellChecking]       =       { { .i = 0 },     },
	[SpellLanguages]      =       { { .v = ((char *[]){ "en_US", NULL }) }, },
	[StrictTLS]           =       { { .i = 1 },     },
	[Style]               =       { { .i = 1 },     },
	[WebGL]               =       { { .i = 0 },     },
	[ZoomLevel]           =       { { .f = 1.0 },   },
};

static UriParameters uriparams[] = {
	{ "(://|\\.)suckless\\.org(/|$)", {
	  [JavaScript] = { { .i = 0 }, 1 },
	  [Plugins]    = { { .i = 0 }, 1 },
	}, },
};

/* default window size: width, height */
static int winsize[] = { 800, 600 };

static WebKitFindOptions findopts = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                                    WEBKIT_FIND_OPTIONS_WRAP_AROUND;

#define PROMPT_GO   "Go:"
#define PROMPT_FIND "Find:"
#define PROMPT_HS "History:" /* Better History */
#define PROMPT_BM "Bookmark:" /* Better Bookmarks */

/* SETPROP(readprop, setprop, prompt)*/
/* Better Bookmarks */
#define SETPROP(r, s, p) { \
        .v = (const char *[]){ "/bin/sh", "-c", \
             "prop=\"$(printf '%b' \"$(xprop -id $1 $2 " \
             "| sed \"s/^$2(STRING) = //;s/^\\\"\\(.*\\)\\\"$/\\1/\")\" " \
             "| dmenu -l 10 -p \"$4\" -w $1)\" && xprop -id $1 -f $3 8s -set $3 \"$prop\"", \
             "surf-setprop", winid, r, s, p, NULL \
        } \
}

/* DOWNLOAD(URI, referer) */
#define DOWNLOAD(u, r) { \
        .v = (const char *[]){ "st", "-e", "/bin/sh", "-c",\
             "curl -g -L -J -O -A \"$1\" -b \"$2\" -c \"$2\"" \
             " -e \"$3\" \"$4\"; read", \
             "surf-download", useragent, cookiefile, r, u, NULL \
        } \
}

/* PLUMB(URI) */
/* This called when some URI which does not begin with "about:",
 * "http://" or "https://" should be opened.
 */
#define PLUMB(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "xdg-open \"$0\"", u, NULL \
        } \
}

/* VIDEOPLAY(URI) */
#define VIDEOPLAY(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "mpv --really-quiet \"$0\"", u, NULL \
        } \
}

/* HS_URI(setprop, prompt) */
/* Better History */
#define HS_URI(s, p) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
            "prop=\"`cat " HS_FILE " | dmenu -l 10 -i -p $2 | sed -e 's/\".*\"//;s/  / /g' | cut -d ' ' -f3`\" &&" \
            "xprop -id $0 -f $1 8s -set $1 \"$prop\"", \
            winid, s, p, NULL \
        } \
}

/* BM_ADD(readprop) */
/* Better Bookmarks */
#define BM_ADD(r) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "(echo $(xprop -id $0 $1) | cut -d '\"' -f2 " \
             "&& cat " BM_FILE  ") " \
             "| awk '!seen[$0]++' > " BM_FILE ".tmp && " \
             "mv " BM_FILE ".tmp " BM_FILE, \
             winid, r, NULL \
        } \
}

/* BM_SET(setprop, prompt) */
/* Better Bookmarks */
#define BM_SET(s, p) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
            "prop=\"`cat " BM_FILE " | dmenu -l 10 -b -i -p \"$2\"`\" &&" \
            "xprop -id $1 -f $0 8s -set $0 \"$prop\"", \
            s, winid, p, NULL \
        } \
}

/* styles */
/*
 * The iteration will stop at the first match, beginning at the beginning of
 * the list.
 */
static SiteSpecific styles[] = {
	/* regexp               file in $styledir */
	{ ".*",                 "default.css" },
};

/* certificates */
/*
 * Provide custom certificate for urls
 */
static SiteSpecific certs[] = {
	/* regexp               file in $certdir */
	{ "://suckless\\.org/", "suckless.org.crt" },
};

#define MOD1 GDK_MOD1_MASK
#define MOD2 GDK_MOD2_MASK
#define MOD3 GDK_MOD3_MASK
#define MOD4 GDK_MOD4_MASK
#define MOD5 GDK_MOD5_MASK
#define MODKEY GDK_CONTROL_MASK

#define MOD MODKEY
#define SHFT GDK_SHIFT_MASK
#define CASP GDK_LOCK_MASK
#define CTRL GDK_CONTROL_MASK

/* hotkeys */
/*
 * If you use anything else but MODKEY and GDK_SHIFT_MASK, don't forget to
 * edit the CLEANMASK() macro.
 */
static Key keys[] = {
	/* modifier              keyval          function    arg */
	{ MOD,                   GDK_KEY_g,      spawn,      SETPROP("_SURF_URI", "_SURF_GO", PROMPT_GO) },
	{ MOD,                   GDK_KEY_f,      spawn,      SETPROP("_SURF_FIND", "_SURF_FIND", PROMPT_FIND) },
	{ MOD,                   GDK_KEY_slash,  spawn,      SETPROP("_SURF_FIND", "_SURF_FIND", PROMPT_FIND) },
	{ MOD,                   GDK_KEY_m,      bmadd,      BM_ADD("_SURF_URI") }, /* Better Bookmarks */
	{ MOD,                   GDK_KEY_z,      spawn,      BM_SET("_SURF_GO", PROMPT_BM) }, /* Better Bookmarks */
	{ MOD,                   GDK_KEY_Return, spawn,      HS_URI("_SURF_GO", PROMPT_HS) }, /* Better History */

	{ 0,                     GDK_KEY_Escape, stop,       { 0 } },
	{ MOD,                   GDK_KEY_c,      stop,       { 0 } },

	{ MOD|SHFT,              GDK_KEY_r,      reload,     { .i = 1 } },
	{ MOD,                   GDK_KEY_r,      reload,     { .i = 0 } },

	{ MOD,                   GDK_KEY_l,      navigate,   { .i = +1 } },
	{ MOD,                   GDK_KEY_h,      navigate,   { .i = -1 } },

	/* vertical and horizontal scrolling, in viewport percentage */
	{ MOD,                   GDK_KEY_j,      scrollv,    { .i = +10 } },
	{ MOD,                   GDK_KEY_k,      scrollv,    { .i = -10 } },
	{ MOD,                   GDK_KEY_space,  scrollv,    { .i = +50 } },
	{ MOD,                   GDK_KEY_b,      scrollv,    { .i = -50 } },
	{ MOD,                   GDK_KEY_i,      scrollh,    { .i = +10 } },
	{ MOD,                   GDK_KEY_u,      scrollh,    { .i = -10 } },


	{ MOD|SHFT,              GDK_KEY_j,      zoom,       { .i = -1 } },
	{ MOD|SHFT,              GDK_KEY_k,      zoom,       { .i = +1 } },
	{ MOD|SHFT,              GDK_KEY_q,      zoom,       { .i = 0  } },
	{ MOD,                   GDK_KEY_minus,  zoom,       { .i = -1 } },
	{ MOD,                   GDK_KEY_plus,   zoom,       { .i = +1 } },

	{ MOD,                   GDK_KEY_p,      clipboard,  { .i = 1 } },
	{ MOD,                   GDK_KEY_y,      clipboard,  { .i = 0 } },

	{ MOD,                   GDK_KEY_n,      find,       { .i = +1 } },
	{ MOD|SHFT,              GDK_KEY_n,      find,       { .i = -1 } },

	{ MOD|SHFT,              GDK_KEY_p,      print,      { 0 } },
	{ MOD,                   GDK_KEY_t,      showcert,   { 0 } },

	{ MOD|SHFT,              GDK_KEY_a,      togglecookiepolicy, { 0 } },
	{ 0,                     GDK_KEY_F11,    togglefullscreen, { 0 } },
	{ MOD|SHFT,              GDK_KEY_o,      toggleinspector, { 0 } },

	{ MOD|SHFT,              GDK_KEY_c,      toggle,     { .i = CaretBrowsing } },
	{ MOD|SHFT,              GDK_KEY_f,      toggle,     { .i = FrameFlattening } },
	{ MOD|SHFT,              GDK_KEY_g,      toggle,     { .i = Geolocation } },
	{ MOD|SHFT,              GDK_KEY_s,      toggle,     { .i = JavaScript } },
	{ MOD|SHFT,              GDK_KEY_i,      toggle,     { .i = LoadImages } },
	{ MOD|SHFT,              GDK_KEY_v,      toggle,     { .i = Plugins } },
	{ MOD|SHFT,              GDK_KEY_b,      toggle,     { .i = ScrollBars } },
	{ MOD|SHFT,              GDK_KEY_t,      toggle,     { .i = StrictTLS } },
	{ MOD|SHFT,              GDK_KEY_m,      toggle,     { .i = Style } },
};

/* button definitions */
/* target can be OnDoc, OnLink, OnImg, OnMedia, OnEdit, OnBar, OnSel, OnAny */
static Button buttons[] = {
	/* target       event mask      button  function        argument        stop event */
	{ OnLink,       0,              2,      clicknewwindow, { .i = 0 },     1 },
	{ OnLink,       MOD,            2,      clicknewwindow, { .i = 1 },     1 },
	{ OnLink,       MOD,            1,      clicknewwindow, { .i = 1 },     1 },
	{ OnAny,        0,              8,      clicknavigate,  { .i = -1 },    1 },
	{ OnAny,        0,              9,      clicknavigate,  { .i = +1 },    1 },
	{ OnMedia,      MOD,            1,      clickexternplayer, { 0 },       1 },
};
