## Copyright (C) 1998, 1999, 2000 Joao Cardoso.
## 
## This program is free software; you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by the
## Free Software Foundation; either version 2 of the License, or (at your
## option) any later version.
## 
## This program is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## This file is part of plplot_octave.
## It is based on the corresponding demo function of PLplot.

## Draws a histogram from sample data.

1;

function go

  NPTS=2047;

  ## Parse and process command line arguments */

  ## (void) plParseOpts(&argc, argv, PL_PARSE_FULL);

  ## Initialize plplot */
  global pldevice
  plsdev(pldevice)
  plinit();

  ## Fill up data points */

  delta = 2.0 * pi /  NPTS;
  for i=1:NPTS
    data(i) = sin(i * delta);
  endfor
  
  plcol0(1);
  plhist(data, -1.1, 1.1, 44, 0);
  plcol0(2);
  pllab("#frValue", "#frFrequency",
	"#frPLplot Example 5 - Probability function of Oscillator");
  plend();
endfunction

go
