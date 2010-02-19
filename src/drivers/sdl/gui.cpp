#include<gtk/gtk.h>
#include<gdk/gdkx.h>

#include<SDL/SDL.h>

#include "../../types.h"
#include "../../fceu.h"
#include "../../driver.h"
#include "../../version.h"
#include "../../movie.h"


#include "../common/configSys.h"
#include "sdl.h"
#include "gui.h"
#include "dface.h"
#include "input.h"

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

void toggleSound(GtkWidget* check, gpointer data);

extern Config *g_config;

GtkWidget* MainWindow = NULL;
GtkWidget* padNoCombo;

// This function configures a single button on a gamepad
int configGamepadButton(GtkButton* button, gpointer p)
{
	int x = GPOINTER_TO_INT(p);
	int padNo = 0;
	char* padStr = gtk_combo_box_get_active_text(GTK_COMBO_BOX(padNoCombo));

	if(!strcmp(padStr, "1"))
		padNo = 0;
	if(!strcmp(padStr, "2"))
		padNo = 1;
	if(!strcmp(padStr, "3"))
		padNo = 2;
	if(!strcmp(padStr, "4"))
		padNo = 3;
		
    char buf[256];
    std::string prefix;
    
    ButtonConfigBegin();
    
    snprintf(buf, 256, "SDL.Input.GamePad.%d", padNo);
    prefix = buf;
    ConfigButton((char*)GamePadNames[x], &GamePadConfig[padNo][x]);

    g_config->setOption(prefix + GamePadNames[x], GamePadConfig[padNo][x].ButtonNum[0]);

    if(GamePadConfig[padNo][x].ButtType[0] == BUTTC_KEYBOARD)
    {
		g_config->setOption(prefix + "DeviceType", "Keyboard");
    } else if(GamePadConfig[padNo][x].ButtType[0] == BUTTC_JOYSTICK) {
        g_config->setOption(prefix + "DeviceType", "Joystick");
    } else {
        g_config->setOption(prefix + "DeviceType", "Unknown");
    }
    g_config->setOption(prefix + "DeviceNum", GamePadConfig[padNo][0].DeviceNum[0]);

    ButtonConfigEnd();
    
    return 0;
}

// creates and opens the gamepad config window
void openGamepadConfig()
{
	GtkWidget* win;
	GtkWidget* vbox;
	GtkWidget* hboxPadNo;
	GtkWidget* padNoLabel;
	
	GtkWidget* buttons[10];
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Gamepad Config");
	gtk_widget_set_size_request(win, 250, 500);
	vbox = gtk_vbox_new(TRUE, 2);
	hboxPadNo = gtk_hbox_new(TRUE, 1);
	padNoLabel = gtk_label_new("Gamepad Number:");

	padNoCombo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "1");
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "2");
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "3");
	gtk_combo_box_append_text(GTK_COMBO_BOX(padNoCombo), "4");
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(padNoCombo), 0);
	
	gtk_box_pack_start(GTK_BOX(hboxPadNo), padNoLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hboxPadNo), padNoCombo, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hboxPadNo, TRUE, TRUE, 5);
	
	// create gamepad buttons
	for(int i=0; i<10; i++)
	{
		buttons[i] = gtk_button_new_with_label(GamePadNames[i]);
		gtk_box_pack_start(GTK_BOX(vbox), buttons[i], TRUE, TRUE, 2);
		gtk_signal_connect(GTK_OBJECT(buttons[i]), "clicked", G_CALLBACK(configGamepadButton), GINT_TO_POINTER(i));	
	}
	
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	gtk_widget_show_all(win);
	
	return;
}

int setBufSize(GtkWidget* w, gpointer p)
{
	int x = gtk_range_get_value(GTK_RANGE(w));
	g_config->setOption("SDL.SoundBufSize", x);
	// reset sound subsystem for changes to take effect
	KillSound();
	InitSound();
	
	return false;
}

void setRate(GtkWidget* w, gpointer p)
{
	char* str = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	g_config->setOption("SDL.SoundRate", atoi(str));
	// reset sound subsystem for changes to take effect
	KillSound();
	InitSound();
		
	return;
}

void setQuality(GtkWidget* w, gpointer p)
{
	char* str = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	if(!strcmp(str, "High"))
		g_config->setOption("SDL.SoundQuality", 1);
	if(!strcmp(str, "Low"))
		g_config->setOption("SDL.SoundQuality", 0);
	// reset sound subsystem for changes to take effect
	KillSound();
	InitSound();
		
	return;
}

void setScaler(GtkWidget* w, gpointer p)
{
	int x = gtk_combo_box_get_active(GTK_COMBO_BOX(w));
	g_config->setOption("SDL.SpecialFilter", x);
}

void setGL(GtkWidget* w, gpointer p)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
		g_config->setOption("SDL.OpenGL", 1);
	else
		g_config->setOption("SDL.OpenGL", 0);
}

void openVideoConfig()
{
	GtkWidget* win;
	GtkWidget* vbox;
	GtkWidget* lbl;
	GtkWidget* hbox1;
	GtkWidget* scalerLbl;
	GtkWidget* scalerCombo;
	GtkWidget* glChk;
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Video Prefernces");
	gtk_widget_set_size_request(win, 250, 400);
	
	vbox = gtk_vbox_new(FALSE, 3);
	
	lbl = gtk_label_new("Video options do not take \neffect until the emulator is restared.");
	
	// scalar widgets
	hbox1 = gtk_hbox_new(FALSE, 3);
	scalerLbl = gtk_label_new("Special Scaler: ");
	scalerCombo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "none");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "hq2x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "scale2x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "hq3x");
	gtk_combo_box_append_text(GTK_COMBO_BOX(scalerCombo), "scale3x");
	
	// sync with cfg
	int buf;
	g_config->getOption("SDL.SpecialFilter", &buf);
	gtk_combo_box_set_active(GTK_COMBO_BOX(scalerCombo), buf);
	
	g_signal_connect(GTK_OBJECT(scalerCombo), "changed", G_CALLBACK(setScaler), NULL);
	gtk_box_pack_start(GTK_BOX(hbox1), scalerLbl, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox1), scalerCombo, FALSE, FALSE, 2);
	
	// openGL check
	glChk = gtk_check_button_new_with_label("Enable OpenGL");
	g_signal_connect(GTK_OBJECT(glChk), "clicked", G_CALLBACK(setGL), NULL);
	
	// sync with config
	g_config->getOption("SDL.OpenGL", &buf);
	if(buf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glChk), 1);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glChk), 0);
	
	
	gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, FALSE, 2);	
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), glChk, FALSE, FALSE, 2);
	
	
	gtk_container_add(GTK_CONTAINER(win), vbox);
	gtk_widget_show_all(win);
	
	return;
}

void openSoundConfig()
{
	GtkWidget* win;
	GtkWidget* vbox;
	GtkWidget* soundChk;
	GtkWidget* hbox1;
	GtkWidget* qualityCombo;
	GtkWidget* qualityLbl;
	GtkWidget* hbox2;
	GtkWidget* rateCombo;
	GtkWidget* rateLbl;
	GtkWidget* hbox3;
	GtkWidget* bufferLbl;
	GtkWidget* bufferHscale;
	
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(win), "Sound Preferences");
	vbox = gtk_vbox_new(False, 2);
	gtk_widget_set_size_request(win, 350, 250);
	
	
	// sound enable check
	soundChk = gtk_check_button_new_with_label("Enable sound");
	
	// sync with cfg
	int cfgBuf;
	g_config->getOption("SDL.Sound", &cfgBuf);
	if(cfgBuf)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soundChk), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(soundChk), FALSE);
	
	gtk_signal_connect(GTK_OBJECT(soundChk), "clicked",
	 G_CALLBACK(toggleSound), NULL);
	
	// sound quality combo box
	hbox1 = gtk_hbox_new(FALSE, 3);
	qualityCombo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(qualityCombo), "Low");
	gtk_combo_box_append_text(GTK_COMBO_BOX(qualityCombo), "High");

	// sync widget with cfg 
	g_config->getOption("SDL.SoundQuality", &cfgBuf);
	if(cfgBuf)
		gtk_combo_box_set_active(GTK_COMBO_BOX(qualityCombo), 1);
	else
		gtk_combo_box_set_active(GTK_COMBO_BOX(qualityCombo), 0);
		
	g_signal_connect(qualityCombo, "changed", G_CALLBACK(setQuality), NULL);
	
	qualityLbl = gtk_label_new("Quality: ");
	
	gtk_box_pack_start(GTK_BOX(hbox1), qualityLbl, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox1), qualityCombo, FALSE, FALSE, 3);
	
	// sound rate widgets
	hbox2 = gtk_hbox_new(FALSE, 3);
	rateCombo = gtk_combo_box_new_text();
	
	const int rates[5] = {11025, 22050, 44100, 48000, 96000};
	
	char buf[8];
	for(int i=0; i<5;i++)
	{
		sprintf(buf, "%d", rates[i]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(rateCombo), buf);	
	}
	
	// sync widget with cfg 
	g_config->getOption("SDL.SoundRate", &cfgBuf);
	for(int i=0; i<5; i++)
		if(cfgBuf == rates[i])
			gtk_combo_box_set_active(GTK_COMBO_BOX(rateCombo), i);
		
	g_signal_connect(rateCombo, "changed", G_CALLBACK(setRate), NULL);
	
	
	// sound rate widgets
	rateLbl = gtk_label_new("Rate (Hz): ");
	
	gtk_box_pack_start(GTK_BOX(hbox2), rateLbl, FALSE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbox2), rateCombo, FALSE, FALSE, 3);
	
	hbox3 = gtk_hbox_new(FALSE, 2);
	bufferHscale = gtk_hscale_new_with_range(15, 200, 2);
	bufferLbl = gtk_label_new("Buffer size (ms): ");
	gtk_box_pack_start(GTK_BOX(hbox3), bufferLbl, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(hbox3), bufferHscale, TRUE, TRUE, 2);
	
	// sync widget with cfg 
	g_config->getOption("SDL.SoundBufSize", &cfgBuf);
	gtk_range_set_value(GTK_RANGE(bufferHscale), cfgBuf);
	
	g_signal_connect(bufferHscale, "button-release-event", G_CALLBACK(setBufSize), NULL);
	
	// packing some boxes
	gtk_box_pack_start(GTK_BOX(vbox), soundChk, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox3, FALSE, FALSE, 2);
	gtk_container_add(GTK_CONTAINER(win), vbox);
	
	gtk_widget_show_all(win);
	
	return;
}

void quit ()
{
	FCEUI_Kill();
	SDL_Quit();
	gtk_main_quit();
	exit(0);
}


GtkWidget* aboutDialog;

inline void quitAbout(void) { gtk_widget_hide_all(aboutDialog);}

void showAbout ()
{
	
	aboutDialog = gtk_about_dialog_new ();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(aboutDialog), "About fceuX");
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(aboutDialog), "fceuX");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(aboutDialog), FCEU_VERSION_STRING);
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(aboutDialog), "GPL-2; See COPYING");
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(aboutDialog), "http://fceux.com");
	
	
	gtk_widget_show_all(GTK_WIDGET(aboutDialog));
	
	g_signal_connect(G_OBJECT(aboutDialog), "delete-event", quitAbout, NULL);
}

void toggleSound(GtkWidget* check, gpointer data)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)))
	{
		g_config->setOption("SDL.Sound", 1);
		InitSound();
	}
	else
	{
		g_config->setOption("SDL.Sound", 0);
		KillSound();
	}
}

void emuReset ()
{
	ResetNES();
}

void emuPause ()
{
	FCEUI_SetEmulationPaused(1);
}
void emuResume ()
{
	FCEUI_SetEmulationPaused(0);
}

void enableFullscreen ()
{
	ToggleFS();
}

void loadMovie ()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Open FM2 Movie", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* fname;
		
		fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		static int pauseframe;
        g_config->getOption("SDL.PauseFrame", &pauseframe);
        g_config->setOption("SDL.PauseFrame", 0);
        FCEUI_printf("Playing back movie located at %s\n", fname);
        FCEUI_LoadMovie(fname, false, false, pauseframe ? pauseframe : false);
		g_free(fname);
	}
	gtk_widget_destroy (fileChooser);
}

#ifdef _S9XLUA_H
void loadLua ()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Open LUA Script", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEU_LoadLuaCode(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);
}
#endif

void loadGame ()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Open ROM", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		LoadGame(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);

}

void closeGame() { CloseGame(); }

// this is not used currently; it is used in rendering sdl in
// the gtk window which is broken
gint configureEvent (GtkWidget* widget, GdkEventConfigure* event)
{
	//neccessary for SDL rendering on gtk win (i think?)
	//s_screen = SDL_SetVideoMode(event->width, event->height, 0, 0);
	
	return TRUE;
}

void saveStateAs()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Save State As", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEUI_SaveState(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);
	
	
}

void loadStateFrom()
{
	GtkWidget* fileChooser;
	
	fileChooser = gtk_file_chooser_dialog_new ("Load State From", GTK_WINDOW(MainWindow),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (fileChooser)) ==GTK_RESPONSE_ACCEPT)
	{
		char* filename;
		
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fileChooser));
		FCEUI_LoadState(filename);
		g_free(filename);
	}
	gtk_widget_destroy (fileChooser);
	
	
}
	

/* Our menu, an array of GtkItemFactoryEntry structures that defines each menu item */
static GtkItemFactoryEntry menu_items[] = {
  { "/_File",         NULL,         NULL,           0, "<Branch>" },
  //{ "/File/_New",     "<control>N", NULL,    0, "<StockItem>", GTK_STOCK_NEW },
  { "/File/_Open ROM",    "<control>O", loadGame,    0, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/_Close ROM",    "<control>C", closeGame,    0, "<StockItem>", GTK_STOCK_CLOSE },
 // { "/File/Save _As", NULL,         NULL,           0, "<Item>" },
  { "/File/sep1",     NULL,         NULL,           0, "<Separator>" },
  { "/File/Savestate", NULL, NULL, 0, "<Branch>" },
  { "/File/Savestate/Load State _From", NULL, loadStateFrom, 0, "<Item>"},
  { "/File/Savestate/Save State _As", NULL, saveStateAs, 0, "<Item>"},
#ifdef _S9XLUA_H  
  { "/File/Load _Lua Script", NULL, loadLua, 0, "<Item>"},
#endif
  { "/File/Load _Movie", NULL, loadMovie, 0, "<Item>"},
  { "/File/_Screenshot", "F12", FCEUI_SaveSnapshot, 0, "<Item>"},
  { "/File/sep2",     NULL,         NULL,           0, "<Separator>" },
  { "/File/_Quit",    "<CTRL>Q", quit, 0, "<StockItem>", GTK_STOCK_QUIT },
  { "/_Options",      NULL,         NULL,           0, "<Branch>" },
  { "/_Emulator",	NULL,			NULL,			0, "<Branch>"  },
  { "/Emulator/_Reset", 	NULL, emuReset, 0, "<Item>"},
  { "/Emulator/_Pause", NULL, emuPause, 0, "<Item>"},
  { "/Emulator/R_esume", NULL, emuResume, 0, "<Item>"},
  { "/Options/_Gamepad Config", NULL , openGamepadConfig, 0, "<StockItem>", GTK_STOCK_PREFERENCES },
  { "/Options/_Sound Config", NULL , openSoundConfig, 0, "<Item>" },
  { "/Options/_Vound Config", NULL , openVideoConfig, 0, "<Item>" },
  { "/Options/sep1",  NULL,         NULL,           0, "<Separator>" },
  { "/Options/_Fullscreen", NULL,         enableFullscreen,	   0, "<Item>" },
  { "/_Help",         NULL,         NULL,           0, "<LastBranch>" },
  { "/_Help/About",   NULL,         showAbout,           0, "<Item>" },
};

static gint nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

static GtkWidget* CreateMenubar( GtkWidget* window)
{
	GtkItemFactory *item_factory;
	GtkAccelGroup *accel_group;

	/* Make an accelerator group (shortcut keys) */
	accel_group = gtk_accel_group_new ();

	/* Make an ItemFactory (that makes a menubar) */
	item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);

	/* This function generates the menu items. Pass the item factory,
		 the number of items in the array, the array itself, and any
		 callback data for the the menu items. */
	gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);

	/* Attach the new accelerator group to the window. */
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	/* Finally, return the actual menu bar created by the item factory. */
	return gtk_item_factory_get_widget (item_factory, "<main>");
}


int InitGTKSubsystem(int argc, char** argv)
{
	//GtkWidget* MainWindow;
	GtkWidget* Menubar;
	GtkWidget* vbox;

	int xres, yres;
	
	g_config->getOption("SDL.XResolution", &xres);
		g_config->getOption("SDL.YResolution", &yres);
	
	gtk_init(&argc, &argv);
	
	MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(MainWindow), "FceuX");
	gtk_window_set_default_size(GTK_WINDOW(MainWindow), 359, 200);
	
	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(MainWindow), vbox);
	
	Menubar = CreateMenubar(MainWindow);
	
		
	//gtk_container_add(GTK_CONTAINER(vbox), Menubar);
	gtk_box_pack_start (GTK_BOX(vbox), Menubar, FALSE, TRUE, 0);
	

	
	
	// broken SDL embedding code
	//gtk_widget_set_usize(MainWindow, xres, yres);
	//gtk_widget_realize(MainWindow);
	
	// event handlers
	gtk_widget_add_events(MainWindow, GDK_BUTTON_PRESS_MASK);
	//gtk_signal_connect(GTK_OBJECT(MainWindow), "configure_event",
	//	GTK_SIGNAL_FUNC(configureEvent), 0);
	
	
	// PRG: this code here is the the windowID "hack" to render SDL
	// in a GTK window.	however, I can't get it to work right now
	// so i'm commenting it out and haivng a seperate GTK2 window with 
	// controls
	// 12/21/09
	/*
	GtkWidget* socket = gtk_socket_new();
	gtk_widget_show (socket) ; 
	gtk_container_add (GTK_CONTAINER(MainWindow), socket);
	
	gtk_widget_realize (socket);

	char SDL_windowhack[24];
	sprintf(SDL_windowhack, "SDL_WINDOWID=%ld", (long int)gtk_socket_get_id (GTK_SOCKET(socket)));
	putenv(SDL_windowhack); 
	
	
	// init SDL
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		fprintf(stderr, "Couldn't init SDL: %s\n", SDL_GetError());
		gtk_main_quit();
	}
	
	
	
	// test rendering
	//screen = SDL_SetVideoMode(xres, yres, 0, 0);
	//hello = SDL_LoadBMP( "hello.bmp" );
	*/
	
	// signal handlers
	g_signal_connect(G_OBJECT(MainWindow), "delete-event", quit, NULL);
	//g_signal_connect(G_OBJECT(MainWindow), "destroy-event", quit, NULL);
	
		//gtk_idle_add(mainLoop, MainWindow);
	gtk_widget_set_size_request (GTK_WIDGET(MainWindow), 300, 200);

	gtk_widget_show_all(MainWindow);
	
	return 0;
}
