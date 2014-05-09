/**
* Project: VSXu: Realtime modular visual programming language, music/audio visualizer.
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

#ifndef VSX_PROFILER_CONSUMER_H
#define VSX_PROFILER_CONSUMER_H

#include <list>

#include <vsxfst.h>
#include <vsx_avector.h>
#include <vsx_profiler_manager.h>
#include <vsxfst.h>

const size_t compute_stack_depth = 128;

class vsx_profiler_consumer_chunk
{
public:
  double time_start;
  double time_end;
  long depth;
  uint64_t   flags;
  uint64_t   cyclea_start;
  uint64_t   cycles_end;
  vsx_string tag;
};


class vsx_profiler_consumer
{
  vsx_avector<vsx_string> filenames;


  vsx_avector<vsx_profile_chunk> current_profile;

  vsx_avector<u_int64_t> current_threads;
  vsx_avector<u_int64_t> current_plots;

  vsxf filesystem;

  double current_max_time;
  double cycles_per_second;
  double one_div_cycles_per_second;

  double cycles_begin_time;

  uint64_t cpu_clock_start;
  uint64_t cpu_clock_end;


  vsx_profiler_consumer_chunk compute_stack[compute_stack_depth];
  size_t compute_stack_pointer;

public:

  vsx_profiler_consumer()
  {
    std::list<vsx_string> local_filenames;

    get_files_recursive(
      vsx_profiler_manager::profiler_directory_get(),
      &local_filenames
    );

    size_t i = 0;
    for (std::list<vsx_string>::iterator it = local_filenames.begin(); it != local_filenames.end(); it++)
    {
      vsx_string n = *it;
      vsx_string deli = DIRECTORY_SEPARATOR;
      vsx_avector<vsx_string> parts;
      explode(n, deli, parts);

      if (parts.size() < 2)
        continue;

      filenames[i] = parts[parts.size()-1];
      i++;
    }
  }

  vsx_string get_filenames()
  {
    vsx_string deli = "\n";
    return implode(filenames, deli);
  }

  void load_profile(size_t index)
  {
    vsx_string filename = vsx_profiler_manager::profiler_directory_get() + DIRECTORY_SEPARATOR + filenames[index];
    vsxf_handle* fp = filesystem.f_open( filename.c_str() , "r" );

    unsigned long file_size = filesystem.f_get_size(fp);

    // Some sanity checks
    if ( file_size % sizeof(vsx_profile_chunk) )
      ERROR_RETURN("Profile data file is not a multiple of chunk size.");

    size_t chunks_in_file = file_size / sizeof(vsx_profile_chunk);

    current_profile.allocate( chunks_in_file );

    filesystem.f_read( current_profile.get_pointer(), file_size, fp );
    vsx_printf("VSX PROFILER: loaded profile with %ld chunks\n", chunks_in_file);

    if (current_profile.size() < 3)
      ERROR_RETURN("not enough data in profile");

    vsx_profile_chunk& first_profile = current_profile[0];
    vsx_profile_chunk& last_profile = current_profile[current_profile.size()-2];

    cpu_clock_start = first_profile.cycles;

    if (last_profile.flags != VSX_PROFILE_CHUNK_FLAG_TIMESTAMP)
      ERROR_RETURN("No timestamp in data file, can't analyze it.");

    current_max_time = s2f( vsx_string( last_profile.tag, 32) );
    cpu_clock_end = last_profile.cycles;

    cycles_per_second = (cpu_clock_end-cpu_clock_start) / current_max_time;
    one_div_cycles_per_second = 1.0 / cycles_per_second;

    for (size_t i = 0; i < current_profile.size()-2; i++)
    {
      vsx_profile_chunk& chunk = current_profile[i];

      if (chunk.flags < 100) // regular
      {
        bool found = false;
        size_t j = 0;
        while (!found && j < current_threads.size())
        {
          if (current_threads[j] == chunk.id)
            found = true;
          j++;
        }

        if (!found)
          current_threads.push_back(chunk.id);
      }
      if (chunk.flags > 100) // plot
      {
        bool found = false;
        size_t j = 0;
        while (!found && j < current_plots.size())
        {
          if (current_plots[j] == chunk.id)
            found = true;
          j++;
        }

        if (!found)
          current_plots.push_back(chunk.id);
      }
    }

    for (size_t i = 0; i < current_threads.size(); i++)
    {
      vsx_printf("Current thread: %ld\n", current_threads[i]);
    }
    for (size_t i = 0; i < current_plots.size(); i++)
    {
      vsx_printf("Current plot: %ld\n", current_plots[i]);
    }


    vsx_printf("max time: %f\n", current_max_time);
    vsx_printf("clock frequency: %f\n", (double)(cpu_clock_end-cpu_clock_start) / current_max_time );
  }

  double cycles_to_time(uint64_t cycles)
  {
    return (double)(cycles - cpu_clock_start) * one_div_cycles_per_second;
  }

  /**
   * @brief get_chunks
   * @param t_start
   * @param t_end
   * @param chunks_result
   */
  void get_chunks(double t_start, double t_end, vsx_avector<vsx_profiler_consumer_chunk> &chunks_result)
  {
    VSX_UNUSED(t_start);
    VSX_UNUSED(t_end);

    if (current_profile.size() < 2)
      ERROR_RETURN("Not enough chunks in loaded profile data");

//    uint64_t cycles_begin_time = cpu_clock_start + (uint64_t)(t_start * cycles_per_second);
//    uint64_t cycles_end_time = cpu_clock_start + (uint64_t)(t_end * cycles_per_second);

    // now find the first major chunk
    for (size_t i = 0; i < current_profile.size(); i++)
    {
      vsx_profile_chunk& chunk = current_profile[i];

      if (chunk.flags == VSX_PROFILE_CHUNK_FLAG_SECTION_START /* && chunk.cycles > cycles_begin_time*/)
      {
        compute_stack[compute_stack_pointer].time_start = cycles_to_time( chunk.cycles );
        compute_stack[compute_stack_pointer].cyclea_start = chunk.cycles;
//        vsx_printf("starting time: %f\n", compute_stack[compute_stack_pointer].time_start);
        compute_stack_pointer++;
        if (compute_stack_pointer == compute_stack_depth)
          compute_stack_pointer--;
      }

      if (chunk.flags == VSX_PROFILE_CHUNK_FLAG_SECTION_END /*&& chunk.cycles > cycles_begin_time*/)
      {
        if (compute_stack_pointer != 0)
          compute_stack_pointer--;

        compute_stack[compute_stack_pointer].time_end = cycles_to_time( chunk.cycles );
        compute_stack[compute_stack_pointer].depth = compute_stack_pointer;
        compute_stack[compute_stack_pointer].cycles_end = chunk.cycles;
        chunks_result.push_back( compute_stack[compute_stack_pointer] );
//        vsx_printf("ending time: %f\n", compute_stack[compute_stack_pointer].time_end);
      }

      if (chunk.flags == VSX_PROFILE_CHUNK_FLAG_START /*&& chunk.cycles > cycles_begin_time*/)
      {
        compute_stack[compute_stack_pointer].time_start = cycles_to_time( chunk.cycles );
        compute_stack[compute_stack_pointer].tag = chunk.tag;
        compute_stack[compute_stack_pointer].cyclea_start = chunk.cycles;
//        vsx_printf("starting time inner: %f\n", compute_stack[compute_stack_pointer].time_start);
        compute_stack_pointer++;
        if (compute_stack_pointer == compute_stack_depth)
          compute_stack_pointer--;
      }

      if (chunk.flags == VSX_PROFILE_CHUNK_FLAG_END /*&& chunk.cycles > cycles_begin_time*/)
      {
        if (compute_stack_pointer != 0)
          compute_stack_pointer--;

        compute_stack[compute_stack_pointer].time_end = cycles_to_time( chunk.cycles );
        compute_stack[compute_stack_pointer].depth = compute_stack_pointer;
        compute_stack[compute_stack_pointer].cycles_end = chunk.cycles;
        chunks_result.push_back( compute_stack[compute_stack_pointer] );
//        vsx_printf("ending time inner: %f\n", compute_stack[compute_stack_pointer].time_end);
      }
      //vsx_printf("stack pointer: %d\n", compute_stack_pointer);

    }

  }



  static vsx_profiler_consumer* get_instance()
  {
    static vsx_profiler_consumer vpc;
    return &vpc;
  }
};


#endif
