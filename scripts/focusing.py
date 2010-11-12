#!/usr/bin/python
#
# Autofocosing routines.
#
# You will need: scipy matplotlib sextractor
# This should work on Debian/ubuntu:
# sudo apt-get install python-matplotlib python-scipy python-pyfits sextractor
#
# If you would like to see sextractor results, get DS9 and pyds9:
#
# http://hea-www.harvard.edu/saord/ds9/
#
# Please be aware that current sextractor Ubuntu packages does not work
# properly. The best workaround is to install package, and the overwrite
# sextractor binary with one compiled from sources (so you will have access
# to sextractor configuration files, which program assumes).
#
# (C) 2002-2008 Stanislav Vitek
# (C) 2002-2010 Martin Jelinek
# (C) 2009-2010 Markus Wildi
# (C) 2010      Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import rts2comm
import sextractor

from pylab import *
from scipy import *
from scipy import optimize
import numpy

LINEAR = 0
"""Linear fit"""
P2 = 1
"""Fit using 2 power polynomial"""
P4 = 2
"""Fit using 4 power polynomial"""

class Focusing (rts2comm.Rts2Comm):
	"""Take and process focussing data."""

	def __init__(self):
		self.exptime = 10
		self.step = 0.2
		self.attempts = 20
		self.focuser = 'F0'
		# if |offset| is above this value, try linear fit
		self.linear_fit = self.step * self.attempts
		# target FWHM for linear fit
		self.linear_fit_fwhm = 3.5

	def doFit(self,fit):
		b = None
		errfunc = None
		fitfunc_r = None
		p0 = None

		# try to fit..
		# this function is for flux.. 
		#fitfunc = lambda p, x: p[0] * p[4] / (p[4] + p[3] * (abs(x - p[1])) ** (p[2]))

		# prepare fit based on its type..
		if fit == LINEAR:
			fitfunc = lambda p, x: p[0] + p[1] * x
			errfunc = lambda p, x, y: fitfunc(p, x) - y # LINEAR - distance to the target function
			p0 = [1, 1]
			fitfunc_r = lambda x, p0, p1: p0 + p1 * x
		elif fit == P2:
			fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2)
			errfunc = lambda p, x, y: fitfunc(p, x) - y # P2 - distance to the target function
			p0 = [1, 1, 1]
			fitfunc_r = lambda x, p0, p1, p2 : p0 + p1 * x + p2 * (x ** 2)
		elif fit == P4:
			fitfunc = lambda p, x: p[0] + p[1] * x + p[2] * (x ** 2) + p[3] * (x ** 3) + p[4] * (x ** 4)
			errfunc = lambda p, x, y: fitfunc(p, x) - y # P4 - distance to the target function
			p0 = [1, 1, 1, 1, 1]
			fitfunc_r = lambda x, p0, p1: p0 + p1 * x + p2 * (x ** 2) + p3 * (x ** 3) + p4 * (x ** 4)
		else:
			raise Exception('Unknow fit type {0}'.format(fit))

		self.fwhm_poly, success = optimize.leastsq(errfunc, p0[:], args=(self.focpos, self.fwhm))

		b = None

		if fit == LINEAR:
			b = (self.linear_fit_fwhm - self.fwhm_poly[0]) / self.fwhm_poly[1]
		else:
			b = optimize.fmin(fitfunc_r,self.fwhm_MinimumX,args=(self.fwhm_poly), disp=0)[0]
		self.log('I', 'found FHWM minimum at offset {0}'.format(b))
		return b

	def findBestFWHM(self,tries,rename_images=False,default_fit=P2,min_stars=15):
		# X is FWHM, Y is offset value
		self.focpos=[]
		self.fwhm=[]
		fwhm_min = None
		self.fwhm_MinimumX = None
		keys = tries.keys()
		keys.sort()
		for k in keys:
			if rename_images:
				tries[k] = self.rename(tries[k],'%b/focusing/%o/%f')
			try:
				fwhm,nstars = sextractor.getFWHM(tries[k],min_stars)
			except Exception, ex:
				self.log('W','offset {0}: {1}'.format(k,ex))
				continue
			self.log('I','offset {0} fwhm {1} with {2} stars'.format(k,fwhm,nstars))
			self.focpos.append(k)
			self.fwhm.append(fwhm)
			if (fwhm_min is None or fwhm < fwhm_min):
				self.fwhm_MinimumX = k
				fwhm_min = fwhm

		self.focpos = array(self.focpos)
		self.fwhm = array(self.fwhm)

		b = self.doFit (default_fit)
		if (abs(b - numpy.average(self.focpos)) >= self.linear_fit):
			self.log('W','cannot do find best FWHM inside limits, trying linear fit - best fit is {0}, average focuser position is {1}'.format(b, numpy.average(self.focpos)))
			b = self.doFit(LINEAR)
			return b,LINEAR
		return b,default_fit

	def beforeReadout(self):
		self.current_focus = self.getValueFloat('FOC_POS',self.focuser)
		if (self.num == self.attempts):
			self.setValue('FOC_TOFF',0,self.focuser)
		else:
			self.off += self.step
			self.setValue('FOC_TOFF',self.off,self.focuser)

	def takeImages(self):
		self.setValue('exposure',self.exptime)
		self.setValue('SHUTTER','LIGHT')
		self.off = -1 * self.step * (self.attempts / 2)
		self.setValue('FOC_TOFF',self.off,self.focuser)
		tries = {}
		# must be overwritten in beforeReadout
		self.current_focus = None

		for self.num in range(1,self.attempts+1):
		  	self.log('I','starting {0}s exposure on offset {1}'.format(self.exptime,self.off))
			img = self.exposure(self.beforeReadout)
			tries[self.current_focus] = img

		self.log('I','all focusing exposures finished, processing data')

		return self.findBestFWHM(tries,rename_images=True)

	def run(self):
		b,fit = self.takeImages()
		if fit == LINEAR:
			self.setValue('FOC_DEF',b,self.focuser)
			b,fit = self.takeImages()

		self.setValue('FOC_DEF',b,self.focuser)

if __name__ == "__main__":
	a = Focusing()
	a.run ()