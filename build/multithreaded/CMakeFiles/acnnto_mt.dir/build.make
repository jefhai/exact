# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.13.1/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.13.1/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/a.e./Dropbox/newAntColony

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/a.e./Dropbox/newAntColony/build

# Include any dependencies generated for this target.
include multithreaded/CMakeFiles/acnnto_mt.dir/depend.make

# Include the progress variables for this target.
include multithreaded/CMakeFiles/acnnto_mt.dir/progress.make

# Include the compile flags for this target's objects.
include multithreaded/CMakeFiles/acnnto_mt.dir/flags.make

multithreaded/CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.o: multithreaded/CMakeFiles/acnnto_mt.dir/flags.make
multithreaded/CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.o: ../multithreaded/acnnto_mt.cxx
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/a.e./Dropbox/newAntColony/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object multithreaded/CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.o"
	cd /Users/a.e./Dropbox/newAntColony/build/multithreaded && /Library/Developer/CommandLineTools/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.o -c /Users/a.e./Dropbox/newAntColony/multithreaded/acnnto_mt.cxx

multithreaded/CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.i"
	cd /Users/a.e./Dropbox/newAntColony/build/multithreaded && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/a.e./Dropbox/newAntColony/multithreaded/acnnto_mt.cxx > CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.i

multithreaded/CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.s"
	cd /Users/a.e./Dropbox/newAntColony/build/multithreaded && /Library/Developer/CommandLineTools/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/a.e./Dropbox/newAntColony/multithreaded/acnnto_mt.cxx -o CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.s

# Object files for target acnnto_mt
acnnto_mt_OBJECTS = \
"CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.o"

# External object files for target acnnto_mt
acnnto_mt_EXTERNAL_OBJECTS =

multithreaded/acnnto_mt: multithreaded/CMakeFiles/acnnto_mt.dir/acnnto_mt.cxx.o
multithreaded/acnnto_mt: multithreaded/CMakeFiles/acnnto_mt.dir/build.make
multithreaded/acnnto_mt: rnn/libacnnto_strategy.a
multithreaded/acnnto_mt: time_series/libexact_time_series.a
multithreaded/acnnto_mt: common/libexact_common.a
multithreaded/acnnto_mt: multithreaded/CMakeFiles/acnnto_mt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/a.e./Dropbox/newAntColony/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable acnnto_mt"
	cd /Users/a.e./Dropbox/newAntColony/build/multithreaded && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/acnnto_mt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
multithreaded/CMakeFiles/acnnto_mt.dir/build: multithreaded/acnnto_mt

.PHONY : multithreaded/CMakeFiles/acnnto_mt.dir/build

multithreaded/CMakeFiles/acnnto_mt.dir/clean:
	cd /Users/a.e./Dropbox/newAntColony/build/multithreaded && $(CMAKE_COMMAND) -P CMakeFiles/acnnto_mt.dir/cmake_clean.cmake
.PHONY : multithreaded/CMakeFiles/acnnto_mt.dir/clean

multithreaded/CMakeFiles/acnnto_mt.dir/depend:
	cd /Users/a.e./Dropbox/newAntColony/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/a.e./Dropbox/newAntColony /Users/a.e./Dropbox/newAntColony/multithreaded /Users/a.e./Dropbox/newAntColony/build /Users/a.e./Dropbox/newAntColony/build/multithreaded /Users/a.e./Dropbox/newAntColony/build/multithreaded/CMakeFiles/acnnto_mt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : multithreaded/CMakeFiles/acnnto_mt.dir/depend
