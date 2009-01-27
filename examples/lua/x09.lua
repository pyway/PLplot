--[[ $Id$

	Contour plot demo.

   This file is part of PLplot.

   PLplot is free software you can redistribute it and/or modify
   it under the terms of the GNU General Library Public License as published
   by the Free Software Foundation either version 2 of the License, or
   (at your option) any later version.

   PLplot is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with PLplot if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
--]]


-- initialise Lua bindings to PLplot
if string.sub(_VERSION,1,7)=='Lua 5.0' then
	lib=loadlib('plplotluac.dll','luaopen_plplotluac') or loadlib('plplotluac.so','luaopen_plplotluac')
	assert(lib)()
else
	require('example')
end


XPTS = 35		-- Data points in x 
YPTS = 46		-- Data points in y 

XSPA = 2/(XPTS-1)
YSPA = 2/(YPTS-1)

-- polar plot data 
PERIMETERPTS = 100
RPTS = 40
THETAPTS = 40

-- potential plot data 
PPERIMETERPTS = 100
PRPTS = 40
PTHETAPTS = 64
PNLEVEL = 20

clevel = { -1, -0.8, -0.6, -0.4, -0.2, 0, 0.2, 0.4, 0.6, 0.8, 1}

-- Transformation function 
tr = { XSPA, 0, -1, 0, YSPA, -1 }

function mypltr(x, y, pltr_data)
	tx = tr[1] * x + tr[2] * y + tr[3]
	ty = tr[4] * x + tr[5] * y + tr[6]
	
	return tx, ty
end

--polar contour plot example.
function polar()
	px = {}
	py = {}
	lev = {}

	pl.env(-1, 1, -1, 1, 0, -2)
	pl.col0(1)
		 
	--Perimeter
	for i=1, PERIMETERPTS do
		t = (2*math.pi/(PERIMETERPTS-1))*(i-1)
		px[i] = math.cos(t)
		py[i] = math.sin(t)
  end
  pl.line(px, py)
	       
	--create data to be contoured.
   plAlloc2dGrid(&cgrid2.xg, RPTS, THETAPTS)
   plAlloc2dGrid(&cgrid2.yg, RPTS, THETAPTS)
   plAlloc2dGrid(&z, RPTS, THETAPTS)
   cgrid2.nx = RPTS
   cgrid2.ny = THETAPTS
   
   for (i = 0 i < RPTS i++) {
      r = i/(double)(RPTS-1)
      for (j = 0 j < THETAPTS j++) {
	 theta = (2.*M_PI/(double)(THETAPTS-1))*(double)j
	 cgrid2.xg[i][j] = r*cos(theta)
	 cgrid2.yg[i][j] = r*sin(theta)
	 z[i][j] = r
      }
   }

   for (i = 0 i < 10 i++) {
      lev[i] = 0.05 + 0.10*(double) i
   }

   pl.col0(2)
   pl.cont(z, RPTS, THETAPTS, 1, RPTS, 1, THETAPTS, lev, 10,
	              pltr2, (void *) &cgrid2)
   pl.col0(1)
   pl.lab("", "", "Polar Contour Plot")
end


----------------------------------------------------------------------------
-- f2mnmx
--
-- Returns min & max of input 2d array.
----------------------------------------------------------------------------
function f2mnmx(f, nx, ny)
	fmax = f[1][1]
	fmin = fmax

	for i=1, nx do
		for j=1, ny do
	    fmax = math.max(fmax, f[i][j])
	    fmin = math.min(fmin, f[i][j])
		end
	end
		
	return fmin, fmax
}


--shielded potential contour plot example.
function potential()
   PLFLT dz, clevel, clevelneg[PNLEVEL], clevelpos[PNLEVEL]
   PLFLT px[PPERIMETERPTS], py[PPERIMETERPTS]

	--create data to be contoured.
   plAlloc2dGrid(&cgrid2.xg, PRPTS, PTHETAPTS)
   plAlloc2dGrid(&cgrid2.yg, PRPTS, PTHETAPTS)
   plAlloc2dGrid(&z, PRPTS, PTHETAPTS)
   cgrid2.nx = PRPTS
   cgrid2.ny = PTHETAPTS

   for (i = 0 i < PRPTS i++) {
      r = 0.5 + (double) i
      for (j = 0 j < PTHETAPTS j++) {
	 theta = (2.*M_PI/(double)(PTHETAPTS-1))*(0.5 + (double) j)
	 cgrid2.xg[i][j] = r*cos(theta)
	 cgrid2.yg[i][j] = r*sin(theta)
      }
   }

   rmax = r
   f2mnmx(cgrid2.xg, PRPTS, PTHETAPTS, &xmin, &xmax)
   f2mnmx(cgrid2.yg, PRPTS, PTHETAPTS, &ymin, &ymax)
   x0 = (xmin + xmax)/2.
   y0 = (ymin + ymax)/2.

   -- Expanded limits 
   peps = 0.05
   xpmin = xmin - fabs(xmin)*peps
   xpmax = xmax + fabs(xmax)*peps
   ypmin = ymin - fabs(ymin)*peps
   ypmax = ymax + fabs(ymax)*peps

   -- Potential inside a conducting cylinder (or sphere) by method of images.
      Charge 1 is placed at (d1, d1), with image charge at (d2, d2).
      Charge 2 is placed at (d1, -d1), with image charge at (d2, -d2).
      Also put in smoothing term at small distances.
   

   eps = 2.

   q1 = 1.
   d1 = rmax/4.

   q1i = - q1*rmax/d1
   d1i = pow(rmax, 2.)/d1

   q2 = -1.
   d2 = rmax/4.

   q2i = - q2*rmax/d2
   d2i = pow(rmax, 2.)/d2

   for (i = 0 i < PRPTS i++) {
      for (j = 0 j < PTHETAPTS j++) {
	 div1 = sqrt(pow(cgrid2.xg[i][j]-d1, 2.) + pow(cgrid2.yg[i][j]-d1, 2.) + pow(eps, 2.))
	 div1i = sqrt(pow(cgrid2.xg[i][j]-d1i, 2.) + pow(cgrid2.yg[i][j]-d1i, 2.) + pow(eps, 2.))
	 div2 = sqrt(pow(cgrid2.xg[i][j]-d2, 2.) + pow(cgrid2.yg[i][j]+d2, 2.) + pow(eps, 2.))
	 div2i = sqrt(pow(cgrid2.xg[i][j]-d2i, 2.) + pow(cgrid2.yg[i][j]+d2i, 2.) + pow(eps, 2.))
	 z[i][j] = q1/div1 + q1i/div1i + q2/div2 + q2i/div2i
      }
   }
   f2mnmx(z, PRPTS, PTHETAPTS, &zmin, &zmax)
--   printf("%.15g %.15g %.15g %.15g %.15g %.15g %.15g %.15g \n",
	  q1, d1, q1i, d1i, q2, d2, q2i, d2i)
   printf("%.15g %.15g %.15g %.15g %.15g %.15g \n",
	  xmin, xmax, ymin, ymax, zmin, zmax) 

   -- Positive and negative contour levels.
   dz = (zmax-zmin)/(double) PNLEVEL
   nlevelneg = 0
   nlevelpos = 0
   for (i = 0 i < PNLEVEL i++) {
      clevel = zmin + ((double) i + 0.5)*dz
      if (clevel <= 0.)
	clevelneg[nlevelneg++] = clevel
      else
	clevelpos[nlevelpos++] = clevel
   }
   -- Colours! 
   ncollin = 11
   ncolbox = 1
   ncollab = 2

   -- Finally start plotting this page! 
   pladv(0)
   plcol0(ncolbox)

   plvpas(0.1, 0.9, 0.1, 0.9, 1.0)
   plwind(xpmin, xpmax, ypmin, ypmax)
   plbox("", 0., 0, "", 0., 0)

   plcol0(ncollin)
   if(nlevelneg >0) {
      -- Negative contours 
      pllsty(2)
      plcont(z, PRPTS, PTHETAPTS, 1, PRPTS, 1, PTHETAPTS,
	     clevelneg, nlevelneg, pltr2, (void *) &cgrid2)
   }

   if(nlevelpos >0) {
      -- Positive contours  
      pllsty(1)
      plcont(z, PRPTS, PTHETAPTS, 1, PRPTS, 1, PTHETAPTS,
	     clevelpos, nlevelpos, pltr2, (void *) &cgrid2)
   }
		 
   -- Draw outer boundary  
   for (i = 0 i < PPERIMETERPTS i++) {
      t = (2.*M_PI/(PPERIMETERPTS-1))*(double)i
      px[i] = x0 + rmax*cos(t)
      py[i] = y0 + rmax*sin(t)
   }

   plcol0(ncolbox)
   plline(PPERIMETERPTS, px, py)
	       
   plcol0(ncollab)
   pllab("", "", "Shielded potential of charges in a conducting sphere")

   plFree2dGrid(z, PRPTS, PTHETAPTS)
   plFree2dGrid(cgrid2.xg, PRPTS, PTHETAPTS)
   plFree2dGrid(cgrid2.yg, PRPTS, PTHETAPTS)
}
  

----------------------------------------------------------------------------
-- main
--
-- Does several contour plots using different coordinate mappings.
----------------------------------------------------------------------------
mark = 1500
space = 1500
xg1 = {}
yg1 = {}

-- Parse and process command line arguments 
pl.parseopts(arg, pl.PL_PARSE_FULL)

-- Initialize plplot 
pl.init()

-- Set up function arrays 
z = {}
w = {}

for (i = 0 i < XPTS i++) {
	xx = (double) (i - (XPTS / 2)) / (double) (XPTS / 2)
	for (j = 0 j < YPTS j++) {
	    yy = (double) (j - (YPTS / 2)) / (double) (YPTS / 2) - 1.0
	    z[i][j] = xx * xx - yy * yy
	    w[i][j] = 2 * xx * yy
	}
    }

-- Set up grids 

    cgrid1.xg = xg1
    cgrid1.yg = yg1
    cgrid1.nx = XPTS
    cgrid1.ny = YPTS

    plAlloc2dGrid(&cgrid2.xg, XPTS, YPTS)
    plAlloc2dGrid(&cgrid2.yg, XPTS, YPTS)
    cgrid2.nx = XPTS
    cgrid2.ny = YPTS

    for (i = 0 i < XPTS i++) {
	for (j = 0 j < YPTS j++) {
	    mypltr((PLFLT) i, (PLFLT) j, &xx, &yy, NULL)

	    argx = xx * M_PI/2
	    argy = yy * M_PI/2
	    distort = 0.4

	    cgrid1.xg[i] = xx + distort * cos(argx)
	    cgrid1.yg[j] = yy - distort * cos(argy)

	    cgrid2.xg[i][j] = xx + distort * cos(argx) * cos(argy)
	    cgrid2.yg[i][j] = yy - distort * cos(argx) * cos(argy)
	}
    }

-- Plot using identity transform 
--
    plenv(-1.0, 1.0, -1.0, 1.0, 0, 0)
    plcol0(2)
    plcont(z, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11, mypltr, NULL)
    plstyl(1, &mark, &space)
    plcol0(3)
    plcont(w, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11, mypltr, NULL)
    plstyl(0, &mark, &space)
    plcol0(1)
    pllab("X Coordinate", "Y Coordinate", "Streamlines of flow")

    pl_setcontlabelformat(4,3)
    pl_setcontlabelparam(0.006, 0.3, 0.1, 1)
    plenv(-1.0, 1.0, -1.0, 1.0, 0, 0)
    plcol0(2)
    plcont(z, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11, mypltr, NULL)
    plstyl(1, &mark, &space)
    plcol0(3)
    plcont(w, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11, mypltr, NULL)
    plstyl(0, &mark, &space)
    plcol0(1)
    pllab("X Coordinate", "Y Coordinate", "Streamlines of flow")
    pl_setcontlabelparam(0.006, 0.3, 0.1, 0)
    
-- Plot using 1d coordinate transform 

    plenv(-1.0, 1.0, -1.0, 1.0, 0, 0)
    plcol0(2)
    plcont(z, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr1, (void *) &cgrid1)

    plstyl(1, &mark, &space)
    plcol0(3)
    plcont(w, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr1, (void *) &cgrid1)
    plstyl(0, &mark, &space)
    plcol0(1)
    pllab("X Coordinate", "Y Coordinate", "Streamlines of flow")
    --
    pl_setcontlabelparam(0.006, 0.3, 0.1, 1)
    plenv(-1.0, 1.0, -1.0, 1.0, 0, 0)
    plcol0(2)
    plcont(z, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr1, (void *) &cgrid1)

    plstyl(1, &mark, &space)
    plcol0(3)
    plcont(w, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr1, (void *) &cgrid1)
    plstyl(0, &mark, &space)
    plcol0(1)
    pllab("X Coordinate", "Y Coordinate", "Streamlines of flow")
    pl_setcontlabelparam(0.006, 0.3, 0.1, 0)
    
-- Plot using 2d coordinate transform 

    plenv(-1.0, 1.0, -1.0, 1.0, 0, 0)
    plcol0(2)
    plcont(z, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr2, (void *) &cgrid2)

    plstyl(1, &mark, &space)
    plcol0(3)
    plcont(w, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr2, (void *) &cgrid2)
    plstyl(0, &mark, &space)
    plcol0(1)
    pllab("X Coordinate", "Y Coordinate", "Streamlines of flow")
    --
    pl_setcontlabelparam(0.006, 0.3, 0.1, 1)
    plenv(-1.0, 1.0, -1.0, 1.0, 0, 0)
    plcol0(2)
    plcont(z, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr2, (void *) &cgrid2)

    plstyl(1, &mark, &space)
    plcol0(3)
    plcont(w, XPTS, YPTS, 1, XPTS, 1, YPTS, clevel, 11,
	   pltr2, (void *) &cgrid2)
    plstyl(0, &mark, &space)
    plcol0(1)
    pllab("X Coordinate", "Y Coordinate", "Streamlines of flow")
    
    pl_setcontlabelparam(0.006, 0.3, 0.1, 0)
    polar()
    --
    pl_setcontlabelparam(0.006, 0.3, 0.1, 1)
    polar()
    
    pl_setcontlabelparam(0.006, 0.3, 0.1, 0)
    potential()
    --
    pl_setcontlabelparam(0.006, 0.3, 0.1, 1)
    potential()
    

-- Clean up 
           
    plFree2dGrid(z, XPTS, YPTS)
    plFree2dGrid(w, XPTS, YPTS)
    plFree2dGrid(cgrid2.xg, XPTS, YPTS)
    plFree2dGrid(cgrid2.yg, XPTS, YPTS)
   
    plend()

    exit(0)
}
