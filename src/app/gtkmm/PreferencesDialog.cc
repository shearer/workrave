// PreferencesDialog.cc --- Preferences dialog
//
// Copyright (C) 2002, 2003 Raymond Penners <raymond@dotsphinx.com>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Id$

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "preinclude.h"

#include "nls.h"
#include "debug.hh"

#include <unistd.h>
#include <assert.h>

#include "GtkUtil.hh"
#include "Hig.hh"
#include "MainWindow.hh"
#include "PreferencesDialog.hh"
#include "SoundPlayer.hh"
#include "TimeEntry.hh"
#include "TimerBoxPreferencePage.hh"
#include "TimerPreferencesPanel.hh"
#include "Util.hh"

#include "CoreFactory.hh"
#include "ConfiguratorInterface.hh"

#ifdef HAVE_DISTRIBUTION
#include "NetworkPreferencePage.hh"
#endif


PreferencesDialog::PreferencesDialog()
  : HigDialog(_("Preferences"), false, false)
{
  TRACE_ENTER("PreferencesDialog::PreferencesDialog");

  // Pages
  Gtk::Widget *timer_page = manage(create_timer_page());
  Gtk::Widget *gui_general_page = manage(create_gui_page());
  Gtk::Notebook *gui_page = manage(new Gtk::Notebook());
  gui_page->append_page(*gui_general_page, _("General"));
  
#ifdef HAVE_X
  Gtk::Widget *gui_applet_page = manage(create_applet_page());
  gui_page->append_page(*gui_applet_page, _("Applet"));
#endif
  Gtk::Widget *gui_mainwindow_page = manage(create_mainwindow_page());
  gui_page->append_page(*gui_mainwindow_page, _("Status Window"));

#ifdef HAVE_DISTRIBUTION
  Gtk::Widget *network_page = manage(create_network_page());
#endif

  // Notebook
  add_page(_("Timers"), "time.png", *timer_page);
  add_page(_("User interface"), "display.png", *gui_page);
#ifdef HAVE_DISTRIBUTION
  add_page(_("Network"), "network.png", *network_page);
#endif
  
  // Dialog
  get_vbox()->pack_start(notebook, true, true, 0);
  add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);

  CoreInterface *core = CoreFactory::get_core();
  mode = core->get_operation_mode();

#ifdef HAVE_X
  GtkUtil::set_wmclass(*this, "Preferences");
#endif
  
  show_all();

  TRACE_EXIT();
}


//! Destructor.
PreferencesDialog::~PreferencesDialog()
{
  TRACE_ENTER("PreferencesDialog::~PreferencesDialog");

  CoreInterface *core = CoreFactory::get_core();
  core->set_operation_mode(mode);

  TRACE_EXIT();
}



Gtk::Widget *
PreferencesDialog::create_gui_page()
{
  // Sound types
  sound_button  = manage(new Gtk::OptionMenu());
  Gtk::Menu *sound_menu = manage(new Gtk::Menu());
  Gtk::Menu::MenuList &sound_list = sound_menu->items();
  sound_button->set_menu(*sound_menu);
  sound_list.push_back(Gtk::Menu_Helpers::MenuElem(_("No sounds")));
  sound_list.push_back(Gtk::Menu_Helpers::MenuElem
                       (_("Play sounds using sound card")));
  sound_list.push_back(Gtk::Menu_Helpers::MenuElem
                       (_("Play sounds using built-in speaker")));
  int idx;
  if (! SoundPlayer::is_enabled())
    idx = 0;
  else
    {
      if (SoundPlayer::DEVICE_SPEAKER == SoundPlayer::get_device())
        idx = 2;
      else
        idx = 1;
    }
  sound_button->set_history(idx);
  sound_button->signal_changed().connect(SigC::slot(*this, &PreferencesDialog::on_sound_changed));
  
  // Tray start
//   start_in_tray_cb
//     = manage(new Gtk::CheckButton(_("Hide main window at start-up")));
//   start_in_tray_cb->signal_toggled()
//     .connect(SigC::slot(*this,
// 			&PreferencesDialog::on_start_in_tray_toggled));
//   start_in_tray_cb->set_active(MainWindow::get_start_in_tray());

  // Options
  HigCategoryPanel *panel = manage(new HigCategoryPanel(_("Options")));
  //panel->add(*start_in_tray_cb);
  panel->add(_("Sound:"), *sound_button);
  panel->set_border_width(12);
  return panel;
}

Gtk::Widget *
PreferencesDialog::create_timer_page()
{
  // Timers page
  Gtk::Notebook *tnotebook = manage(new Gtk::Notebook());
  tnotebook->set_tab_pos (Gtk::POS_TOP);
  Glib::RefPtr<Gtk::SizeGroup> size_group
    = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
  for (int i = 0; i < BREAK_ID_SIZEOF; i++)
    {
      // Label
      Gtk::Widget *box = manage(GtkUtil::create_label_for_break
                                ((BreakId) i));
      TimerPreferencesPanel *tp = manage(new TimerPreferencesPanel(BreakId(i), size_group));
      box->show_all();
      tnotebook->pages().push_back(Gtk::Notebook_Helpers::TabElem(*tp, *box));
    }
  return tnotebook;
}

Gtk::Widget *
PreferencesDialog::create_mainwindow_page()
{
  // Timers page
  return new TimerBoxPreferencePage("main_window");
}


#ifdef HAVE_X
Gtk::Widget *
PreferencesDialog::create_applet_page()
{
  // Timers page
  return new TimerBoxPreferencePage("applet");
}
#endif


#ifdef HAVE_DISTRIBUTION
Gtk::Widget *
PreferencesDialog::create_network_page()
{
  return new NetworkPreferencePage();
}
#endif

void
PreferencesDialog::add_page(const char *label, const char *image,
                            Gtk::Widget &widget)
{
  string icon = Util::complete_directory(image, Util::SEARCH_PATH_IMAGES);
  Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_file(icon);
  notebook.add_page(label, pixbuf, widget);
}

void
PreferencesDialog::on_sound_changed()
{
  int idx = sound_button->get_history();
  SoundPlayer::set_enabled(idx > 0);
  if (idx > 0)
    {
      SoundPlayer::Device dev = idx == 1
        ? SoundPlayer::DEVICE_SOUNDCARD
        : SoundPlayer::DEVICE_SPEAKER;
      SoundPlayer::set_device(dev);
    }
}

// void
// PreferencesDialog::on_start_in_tray_toggled()
// {
//   MainWindow::set_start_in_tray(start_in_tray_cb->get_active());
// }


int
PreferencesDialog::run()
{
  // CoreFactory::get_configurator()->save();
  show_all();
  return 0;
}


bool
PreferencesDialog::on_focus_in_event(GdkEventFocus *event)
{ 
  TRACE_ENTER("PreferencesDialog::focus_in");

  CoreInterface *core = CoreFactory::get_core();
  mode = core->set_operation_mode(OPERATION_MODE_QUIET);
  TRACE_EXIT();
  return HigDialog::on_focus_in_event(event);
}


bool
PreferencesDialog::on_focus_out_event(GdkEventFocus *event)
{ 
  TRACE_ENTER("PreferencesDialog::focus_out");
  CoreInterface *core = CoreFactory::get_core();
  core->set_operation_mode(mode);
  TRACE_EXIT();
  return HigDialog::on_focus_out_event(event);
}