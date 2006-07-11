# cmake/modules/drivers.cmake
#
# Start driver initializations
#
# Copyright (C) 2006  Alan W. Irwin
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

# Module for configuring all device-related variables.

# Results are contained in the following variables:
# ENABLE_DYNDRIVERS (ON or OFF): whether to dynamically load device drivers.
# PLD_devicename (ON or OFF): whether each PLplot-related device is enabled
#   or not.  devicename is png, jpeg, etc.
# PLD_devicename_INCLUDE_DIR: compile (e.g., -I) options for each individual 
#   device.
# PLD_devicename_LIBRARIES: link options (e.g., -L  and -l) for each
#   individual device.
# DRIVERS_INCLUDE_DIR: concatanated compile options for all devices
# DRIVERS_LIBRARIES: concatanated link options for all devices
# DEVICES_LIST: list of devices (e.g. png, jpeg),where PLD_devicename is ON
# DRIVERS_LIST: list of device drivers (e.g., gd for the png and jpeg devices)
#   where at least one of the associated devices is enabled.

option(ENABLE_DYNDRIVERS "enable dynamic loading of device drivers" OFF)
if(ENABLE_DYNDRIVERS AND NOT BUILD_SHARED_LIBS)
  message("ENABLE_DYNDRIVERS set to OFF because shared libaries are not being built")
  set(ENABLE_DYNDRIVERS OFF)
endif(ENABLE_DYNDRIVERS AND NOT BUILD_SHARED_LIBS)

# Decide whether to enable each device or not and find special resources
# when required.

# Initialize device options (e.g., PLD_png is set to ON or OFF).
include(drivers-init)

# Find *_INC_DIR and *_LIBRARIES resources for device drivers that need them,
# and set appropriate PLD_devicename to OFF if the required resources are 
# not available.
#include(cgm)
#include(gcw)
#include(gd)
#include(gnome)
#include(linuxvga)
include(xwin)
#include(tk)
#include(psttf)
#include(wingcc)
#include(aqt)
#include(wxwidgets)

# Finalize device options.
include(drivers-finish)
