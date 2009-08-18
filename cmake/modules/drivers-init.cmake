# cmake/modules/drivers-init.cmake
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

# Module to initialize device options.

# Results are contained in the following variables:
# PLD_devicename (ON or OFF): whether each PLplot-related device is enabled
#   or not.  devicename is png, jpeg, etc.
# DRIVERS_DEVICE_LIST: see below for the definition

# We think of a "device" as the output medium.  Could be a machine
# style (Tektronix, X11), or a file format (Postscript).
#
# A "driver" is the PLplot code that produces the output on the
# medium.
#
# Some drivers support multiple devices.  But we we need to make sure
# we compile only the selected drivers.  That is, if a driver supports
# devices not selected, we don't want to compile it at all.  Or if it
# supports multiple selected devices, we only want to compile it
# once.  To support this, we build an association list between devices
# and drivers.  This will be inspected to build up the set of drivers
# to be compiled based on which devices are selected.

option(DEFAULT_ALL_DEVICES
  "Enable all devices by default (ON) or enable devices individually by default (OFF)"
  OFF
  )

option(DEFAULT_NO_DEVICES
  "Disable all devices by default (ON) or enable devices individually by default (OFF)"
  OFF
  )

if(DEFAULT_ALL_DEVICES AND DEFAULT_NO_DEVICES)
  message(STATUS
    "WARNING. DEFAULT_ALL_DEVICES ON supersedes DEFAULT_NO_DEVICES ON.")
  set(DEFAULT_NO_DEVICES OFF
    CACHE BOOL
    "Disable all devices by default (ON) or enable devices individually by default (OFF)"
    FORCE
    )
endif(DEFAULT_ALL_DEVICES AND DEFAULT_NO_DEVICES)

option(DEFAULT_NO_QT_DEVICES
  "Disable all qt devices by default (ON) or enable qt devices individually by default (OFF)"
  OFF
  )

option(DEFAULT_NO_CAIRO_DEVICES
  "Disable all cairo devices by default (ON) or enable cairo devices individually by default (OFF)"
  OFF
  )

if(DEFAULT_ALL_DEVICES)
  if(DEFAULT_NO_QT_DEVICES)
    message(STATUS
      "WARNING. DEFAULT_ALL_DEVICES ON supersedes DEFAULT_NO_QT_DEVICES ON.")
    set(DEFAULT_NO_QT_DEVICES OFF
      CACHE BOOL
      "Disable all qt devices by default (ON) or enable qt devices individually by default (OFF)"
      FORCE
      )
  endif(DEFAULT_NO_QT_DEVICES)

  if(DEFAULT_NO_CAIRO_DEVICES)
    message(STATUS
      "WARNING. DEFAULT_ALL_DEVICES ON supersedes DEFAULT_NO_CAIRO_DEVICES ON.")
    set(DEFAULT_NO_CAIRO_DEVICES OFF
      CACHE BOOL
      "Disable all cairo devices by default (ON) or enable cairo devices individually by default (OFF)"
      FORCE
      )
  endif(DEFAULT_NO_CAIRO_DEVICES)
endif(DEFAULT_ALL_DEVICES)

# The DRIVERS_DEVICE_LIST defined below is a list of
# <device>:<drive>:<enable_default> items.  <enable_default> should be
# "ON" or "OFF" and this will reflect in inclusion/exclusion by default
# (as shown by ccmake).
#
# Ordered alphabetically by second in each pair (the driver) and then
# alphabetically by device (the first in each pair) for human
# consumption, but this is not necessary.

set(DRIVERS_DEVICE_LIST
  "aqt:aqt:ON"
  # memcairo does not work so turn it off by default.
  # The remaining cairo devices work well so turn them on by default.
  "memcairo:cairo:OFF"
  "extcairo:cairo:ON"
  "pdfcairo:cairo:ON"
  "pngcairo:cairo:ON"
  "pscairo:cairo:ON"
  "svgcairo:cairo:ON"
  "xcairo:cairo:ON"
  # new example 16 shows severe valgrind issues with this device.
  "cgm:cgm:OFF"
  # This unmaintained device driver builds but does not actually work 
  # (with error message: "Unable to locate dispatch table initialization
  # function for driver: dg300.") so retire it.
  # "dg300:dg300:OFF"
  "epsqt:qt:ON"
  "pdfqt:qt:ON"
  "qtwidget:qt:ON"
  "bmpqt:qt:ON"
  "jpgqt:qt:ON"
  "pngqt:qt:ON"
  "ppmqt:qt:ON"
  "tiffqt:qt:ON"
  "extqt:qt:ON"
  # Currently does not validate at http://validator.w3.org/, but that appears
  # to be a result of the validator being unfamiliar with SVG-1.2 (a draft
  # SVG standard that has not been finalized yet) which is produced
  # by Qt4.  The validation errors
  # referred to the version number (1.2) and one attribute (vector-effect)
  # which is clearly documented in the 1.2 draft.  Also, text offset issues
  # which were in SVG results produced by QT-4.4.3 are gone for QT-4.5.0.
  # Therefore, the conclusion is to enable svgqt by default.
  "svgqt:qt:ON"
  # gd related devices are not maintained.
  "gif:gd:OFF"
  "jpeg:gd:OFF"
  "png:gd:OFF" 
  # gcw is not maintained.
  "gcw:gcw:OFF"
  # Do not implement gnome which is superseded by gcw
  #"gnome:gnome:OFF"
  # hpgl devices produce tons of "Invalid pen selection." messages and the
  # lj_hpgl device produces the error message "Unable to locate dispatch
  # table initialization function for driver: hpgl."
  # Retire this elderly device driver rather than fixing it.
  #"hp7470:hpgl:OFF"
  #"hp7580:hpgl:OFF"
  #"lj_hpgl:hpgl:OFF"
  # This unmaintained driver generates double frees for example 14.
  # Retire this elderly device driver rather than fixing it.
  # "imp:impress:OFF"
  # Default off because poorly maintained (e.g., colours are incorrect)
  # must use software fill, and must run as root.
  "linuxvga:linuxvga:OFF"
  # ljii is unmaintained and both the ljii and ljiip segfault on example 14.
  # Retire this elderly device driver rather than fixing it.
  #"ljii:ljii:OFF"
  #"ljiip:ljiip:OFF"
  "mem:mem:ON"
  "ntk:ntk:OFF"
  "null:null:ON"
  # glibc detects double free
  "pbm:pbm:OFF"
  "pdf:pdf:OFF"
  # (2007-09-01) As discussed on list, don't enable plmeta until we sort
  # out the known issues (e.g., strings, aspect ratio, and TrueType fonts).
  # This is going to take time/energy for some volunteer who has not volunteered
  # yet.... :-)
  "plmeta:plmeta:OFF"
  "ps:ps:ON"
  # No longer segfaults, but still default OFF because cleaner/better ways 
  # (psttf and pscairo) to get modern fonts for postscript results.
  "pstex:pstex:OFF"
  "psttf:psttf:ON"
  "svg:svg:ON"
  # Build issues (inconsistent use of PLD names).  We believe there is no
  # user interest in the tek devices any more so retire them rather than
  # fix them.
  #"conex:tek:OFF" 
  #"mskermit:tek:OFF"
  #"tek4010:tek:OFF"
  #"tek4010f:tek:OFF"
  #"tek4107:tek:OFF"
  #"tek4107f:tek:OFF"
  #"versaterm:tek:OFF"
  #"vlt:tek:OFF"
  #"xterm:tek:OFF" 
  "tk:tk:ON"
  "tkwin:tkwin:ON"
  "wingcc:wingcc:ON"
  "wxwidgets:wxwidgets:ON"
  "wxpng:wxwidgets:OFF"
  "xfig:xfig:ON"
  "xwin:xwin:ON"
  )

if(DEFAULT_ALL_DEVICES)
  set(DEFAULT ON)
  set(PRESET_DEFAULT ON)
elseif(DEFAULT_NO_DEVICES)
  set(DEFAULT OFF)
  set(PRESET_DEFAULT ON)
else(DEFAULT_ALL_DEVICES)
  set(PRESET_DEFAULT OFF)
endif(DEFAULT_ALL_DEVICES)

foreach(DRIVERS_DEVICE ${DRIVERS_DEVICE_LIST})
  string(REGEX REPLACE "^(.*):.*:.*$" "\\1" DEVICE ${DRIVERS_DEVICE})
  if(NOT PRESET_DEFAULT)
    if(DEFAULT_NO_QT_DEVICES AND DEVICE MATCHES ".*qt.*" AND NOT DEVICE STREQUAL "aqt")
      set(DEFAULT OFF)
    elseif(DEFAULT_NO_CAIRO_DEVICES AND DEVICE MATCHES ".*cairo.*")
      set(DEFAULT OFF)
    else(DEFAULT_NO_QT_DEVICES AND DEVICE MATCHES ".*qt.*" AND NOT DEVICE STREQUAL "aqt")
      string(REGEX REPLACE "^.*:.*:(.*)$" "\\1" DEFAULT ${DRIVERS_DEVICE})
    endif(DEFAULT_NO_QT_DEVICES AND DEVICE MATCHES ".*qt.*" AND NOT DEVICE STREQUAL "aqt")
  endif(NOT PRESET_DEFAULT)
  option(PLD_${DEVICE} "Enable ${DEVICE} device" ${DEFAULT})
endforeach(DRIVERS_DEVICE)
