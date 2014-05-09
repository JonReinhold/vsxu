/**
* Project: VSXu Profiler - Data collection and data visualizer
*
* This file is part of Vovoid VSXu.
*
* @author Jonatan Wallmander, Vovoid Media Technologies AB Copyright (C) 2014
* @see The GNU Public License (GPL)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef VSX_WIDGET_PROFILER_H
#define VSX_WIDGET_PROFILER_H

// widget
#include <vsx_widget.h>

// engine_graphics
#include <vsx_vbo_bucket.h>


#include "vsx_widget_time_holder.h"
#include "vsx_profiler_consumer.h"

#include "vsx_widget_profiler_thread.h"


class vsx_widget_profiler : public vsx_widget
{
  vsx_widget_time_holder time;

  std::vector<vsx_widget_profiler_thread*> threads;
  vsx_widget* file_list;
  vsx_widget* timeline_window;

  // profiler for profiling ourselves
  vsx_profiler* profiler;

public:

  void init();
  void update_list();

  void i_draw();

  void load_profile(int id);

  void load_thread(int id);
  void load_plot(int id);

  void command_process_back_queue(vsx_command_s *t);
  bool event_key_down(signed long key, bool alt, bool ctrl, bool shift);

  void event_mouse_wheel(float y);

  void event_mouse_move_passive(vsx_widget_distance distance,vsx_widget_coords coords);

  void interpolate_size();
};

#endif
