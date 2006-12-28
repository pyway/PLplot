# cmake/modules/wxwidgets.cmake
#
# Copyright (C) 2006  Werner Smekal
#
# This file is part of PLplot.
#
# PLplot is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library General Public License as published
# by the Free Software Foundation; version 2 of the License.
#
# PLplot is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with the file PLplot; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA

# Module for determining all configuration variables related to the wxwidgets
# device driver.
# The following variables are set/modified:
# PLD_wxwidgets		  - ON means the wxwidgets device is enabled.
# wxwidgets_COMPILE_FLAGS	  - individual COMPILE_FLAGS required to compile wxwidgets
# 			    device.
# wxwidgets_LINK_FLAGS	  - individual LINK_FLAGS for dynamic wxwidgets device.
# DRIVERS_LINK_FLAGS  	  - list of LINK_FLAGS for all static devices.

if(PLD_wxwidgets)
  SET(wxWidgets_USE_LIBS base core)
  find_package(wxWidgets)
  if(wxWidgets_FOUND)
    string(REGEX REPLACE ";" " -I" 
    wxwidgets_COMPILE_FLAGS
    "-I${wxWidgets_INCLUDE_DIRS} ${wxWidgets_DEFINITIONS}"
    )
    set(wxwidgets_LINK_FLAGS ${wxWidgets_LIBRARIES})
    if(WITH_FREETYPE)
      set(
      wxwidgets_COMPILE_FLAGS
      "${wxwidgets_COMPILE_FLAGS} -I${FREETYPE_INCLUDE_DIR}"
      )
      set(
      wxwidgets_LINK_FLAGS
      ${wxwidgets_LINK_FLAGS}
      ${FREETYPE_LIBRARIES}
      )
    endif(WITH_FREETYPE)
    include(agg)
    if(HAVE_AGG)
      message(STATUS "AGG_INCLUDE_DIR = ${AGG_INCLUDE_DIR}")
      message(STATUS "AGG_LIBRARIES = ${AGG_LIBRARIES}")
      message(STATUS "AGG_DEFINITIONS = ${AGG_DEFINITIONS}")
      set(
      wxwidgets_COMPILE_FLAGS
      "${wxwidgets_COMPILE_FLAGS} -I${AGG_INCLUDE_DIR}"
      )
      set(
      wxwidgets_LINK_FLAGS
      ${wxwidgets_LINK_FLAGS}
      ${AGG_LIBRARIES}
      )
    endif(HAVE_AGG)
    set(DRIVERS_LINK_FLAGS
    ${DRIVERS_LINK_FLAGS} 
    ${wxwidgets_LINK_FLAGS}
    )
  else(wxWidgets_FOUND)
    set(PLD_wxwidgets OFF CACHE BOOL "Enable wxwidgets device" FORCE)
  endif(wxWidgets_FOUND)
endif(PLD_wxwidgets)
