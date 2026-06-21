#include "EditorLevelIO.h"

#include "../common/CEditableLevel.h"
#include "../common/CGraphicsDevice.h"
#include "../common/error.h"
#include "../common/files.h"
#include "EditorDrawer.h"
#include "EditorGraphics.h"

#include <gtk/gtk.h>
#include <string>

namespace {
	std::string choose_file(const char* title, GtkFileChooserAction action, const char* pattern = NULL)
	{
		std::string result;
		GtkWidget* dialog = gtk_file_chooser_dialog_new(title, NULL, action,
			"_Cancel", GTK_RESPONSE_CANCEL,
			(action == GTK_FILE_CHOOSER_ACTION_SAVE) ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT,
			NULL);

		if (pattern)
		{
			GtkFileFilter* filter = gtk_file_filter_new();
			gtk_file_filter_set_name(filter, pattern);
			gtk_file_filter_add_pattern(filter, pattern);
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

			GtkFileFilter* all = gtk_file_filter_new();
			gtk_file_filter_set_name(all, "All files");
			gtk_file_filter_add_pattern(all, "*");
			gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), all);
		}

		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), DATADIR);
		gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);

		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
			g_print("filename: %s\n", filename);
			if (filename)
			{
				result = filename;
				g_free(filename);
			}
		}

		gtk_widget_destroy(dialog);

		// gtk_widget_destroy() only queues the teardown; pump the GTK main loop
		// until it is done so the dialog window (and its input grab) actually go
		// away. Otherwise control returns to the SDL editor while the dead
		// dialog still holds the grab, making the editor window appear frozen.
		while (gtk_events_pending())
			gtk_main_iteration();

		return result;
	}
};

CEditorLevelIO::CEditorLevelIO(CEditableLevel* aLevel,SDL_SysWMinfo aWndInfo,CEditorDrawer* aDrawer, CEditorGraphics* aGraphics, CGraphicsDevice* aGD)
{
	iLevel = aLevel;
	iDrawer = aDrawer;
	iWndInfo = aWndInfo;
	iGraphics = aGraphics;
	iGD = aGD;

	gtk_init(0, 0);
}

CEditorLevelIO::~CEditorLevelIO(void)
{
}

bool CEditorLevelIO::LoadINI()
{
	std::string tmp_filename = choose_file("Select INI-file", GTK_FILE_CHOOSER_ACTION_OPEN, "*.ini");

	if (!tmp_filename.empty())
	{
		try
		{
			std::string temp = getpath(tmp_filename.c_str());
			temp += "default.ini";
			iGraphics->Load(temp.c_str(), tmp_filename.c_str());
			iGD->SetPalette(*iGraphics->Palette(),256);
		}
		catch( CFailureException& e )
		{
			error("Cannot open file (%s)!", e.what());
		}

		return true;
	}
	else
	{
		// smt?
	}
	return false;
}

bool CEditorLevelIO::LoadLevel()
{
	std::string tmp_filename = choose_file("Load level:", GTK_FILE_CHOOSER_ACTION_OPEN, "*.lev");

	if (!tmp_filename.empty())
	{
		try
		{
			iLevel->Load(tmp_filename.c_str());
			std::string temp = getpath(tmp_filename.c_str());
			temp += "default.ini";
			std::string fname(tmp_filename.substr(0, tmp_filename.size()-4)+".ini");
			iGraphics->Load(temp.c_str(),fname.c_str());
			iGD->SetPalette(*iGraphics->Palette(),256);
		}
		catch( CFailureException& e )
		{
			error("Cannot open file (%s)!", e.what());
		}
	}
	else
	{
		// nothing
	}

	return false;
}


bool CEditorLevelIO::SaveLevelAs()
{
	std::string tmp_filename = choose_file("Save level as:", GTK_FILE_CHOOSER_ACTION_SAVE, "*.lev");

	if (!tmp_filename.empty())
	{
		char *tmp;

		if (tmp_filename.size()<4 || strcasecmp(tmp_filename.c_str()+tmp_filename.size()-4,".lev")!=0)
		{
			tmp=(char*)malloc(tmp_filename.size()+5);
			sprintf(tmp, "%s.lev", tmp_filename.c_str());
		}
		else
			tmp=strdup(tmp_filename.c_str());

		ASSERT( tmp );

		if (!iLevel->Save( tmp ))
			error("Saving Failed (%s)!", tmp);
		else 
		{
			free(tmp);
			return true;
		}
	}
	else
	{
		// void
	}

	return false;
}


bool CEditorLevelIO::SaveLevel()
{
	if (*iLevel->LevelFileName() == 0)
		return SaveLevelAs();
	else 
	{
		if (! iLevel->Save(iLevel->LevelFileName()))
			error("Saving Failed (%s)!",iLevel->LevelFileName());
		else
		{
			GtkWidget* dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
				GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Level saved !");
			gtk_window_set_title(GTK_WINDOW(dialog), "TK4 Editor");
			gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
			gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);

			// Pump the GTK loop so the dialog is fully torn down before control
			// returns to the SDL editor (see choose_file()).
			while (gtk_events_pending())
				gtk_main_iteration();
		}

		return false;
	}
}
