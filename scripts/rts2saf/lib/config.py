#!/usr/bin/python
#
# (C) 2013, Markus Wildi
#
#   
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2, or (at your option)
#   any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software Foundation,
#   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#   Or visit http://www.gnu.org/licenses/gpl.html.
#

__author__ = 'wildi.markus@bluewin.ch'

import ConfigParser
import os
import sys
import string
import re
try:
    import lib.devices as dev
except:
    import devices as dev


class DefaultConfiguration(object):
    """Default configuration for rts2saf"""    
    def __init__(self, debug=False, logger=None):

        self.debug=debug
        self.logger=logger

        self.ccd=None 
        self.foc=None
        self.filterWheelsDefs=dict()
        self.filterWheels=list()
        self.filterWheelsInUseDefs=list()
        self.filterWheelsInUse=list()
        self.filters=list()
        self.sexFields=list()
        self.config = ConfigParser.RawConfigParser()
        self.config.optionxform = str

        self.dcf=dict()
        self.dcf[('basic', 'BASE_DIRECTORY')]= '/tmp/rts2saf-focus'
        self.dcf[('basic', 'TEMP_DIRECTORY')]= '/tmp/'
        self.dcf[('basic', 'FILE_GLOB')]= '*fits'
        self.dcf[('basic', 'EMPTY_SLOT_NAMES')]= [ 'empty8', 'open' ]
        self.dcf[('filter wheels', 'inuse')]= '[ FILTA ]'
        # this is really ugly
        # but ConfigParser does not allow something else
        # ToDo define more!
        self.dcf[('filter wheel', 'fltw1')]= '[ FILTA, U, nof]'
        self.dcf[('filter wheel', 'fltw2')]= '[ FILTB, Y ]'
        self.dcf[('filter wheel', 'fltw3')]= '[ FILTC, nof ]'
        self.dcf[('filter wheel', 'fltw4')]= '[ FILTD, nof ]'
        #
        #                                                 relative lower acquisition limit [tick]
        #                                                       relative upper acquisition limit [tick]
        #                                                             stepsize [tick]
        #                                                                  exposure factor
        self.dcf[('filter properties', 'flt1')]= '[ U,   -1000, 1100, 100, 11.1]'
        self.dcf[('filter properties', 'flt2')]= '[ nof1,-1200, 1300, 200, 1.]'
        self.dcf[('filter properties', 'flt3')]= '[ nof2,-1200, 1300, 200, 1.]'
        self.dcf[('filter properties', 'flt4')]= '[ C,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt5')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt6')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt7')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt8')]= '[ D,   -1400, 1500, 300, 1.]'
        self.dcf[('filter properties', 'flt9')]= '[ D,   -1400, 1500, 300, 1.]'

        self.dcf[('focuser properties', 'FOCUSER_NAME')]= 'F0'
        self.dcf[('focuser properties', 'FOCUSER_RESOLUTION')]= 20. 
        self.dcf[('focuser properties', 'FOCUSER_ABSOLUTE_LOWER_LIMIT')]= 0 
        self.dcf[('focuser properties', 'FOCUSER_ABSOLUTE_UPPER_LIMIT')]= 20
        self.dcf[('focuser properties', 'FOCUSER_LOWER_LIMIT')]= 0 
        self.dcf[('focuser properties', 'FOCUSER_UPPER_LIMIT')]= 20
        self.dcf[('focuser properties', 'FOCUSER_STEP_SIZE')]= 2
        self.dcf[('focuser properties', 'FOCUSER_SPEED')]= 100.
        self.dcf[('focuser properties', 'FOCUSER_TEMPERATURE_COMPENSATION')]= False
        # not yet in use:
        self.dcf[('acceptance circle', 'CENTER_OFFSET_X')]= 0.
        self.dcf[('acceptance circle', 'CENTER_OFFSET_Y')]= 0.
        self.dcf[('acceptance circle', 'RADIUS')]= 2000.
        #
        self.dcf[('DS9', 'DS9_REGION_FILE')]= 'ds9-rts2saf.reg'
        #
        self.dcf[('analysis', 'MINIMUM_OBJECTS')]= 20
        self.dcf[('analysis', 'MINIMUM_FOCUSER_POSITIONS')]= 5
        
        self.dcf[('SExtractor', 'SEXPATH')]= 'sextractor'
        self.dcf[('SExtractor', 'SEXCFG')]= '/etc/rts2/rts2saf/sex/rts2saf-sex.cfg'
        self.dcf[('SExtractor', 'SEXPARAM')]= '/etc/rts2/rts2saf/sex/rts2saf-sex.param'
        self.dcf[('SExtractor', 'FIELDS')]= ['EXT_NUMBER','X_IMAGE','Y_IMAGE','MAG_BEST','FLAGS','CLASS_STAR','FWHM_IMAGE','A_IMAGE','B_IMAGE']
        self.dcf[('SExtractor', 'OBJECT_SEPARATION')]= 10.
        self.dcf[('SExtractor', 'ELLIPTICITY')]= .1
        self.dcf[('SExtractor', 'ELLIPTICITY_REFERENCE')]= .3
        self.dcf[('SExtractor', 'DETECT_THRESH')]=1.7 
        self.dcf[('SExtractor', 'ANALYSIS_THRESH')]=1.7 
        self.dcf[('SExtractor', 'DEBLEND_MINCONT')]= 0.1 
        self.dcf[('SExtractor', 'SATUR_LEVEL')]= 65535
        self.dcf[('SExtractor', 'STARNNW_NAME')]= '/home/wildi/svn-vermes/experiment/python/nordlys/sextractor/default.nnw'
        # ToDo so far that is good for FLI CCD
        # These factors are used for the fitting
        self.dcf[('ccd binning mapping', '1x1')] = 0
        self.dcf[('ccd binning mapping', '2x2')] = 1
        self.dcf[('ccd binning mapping', '4x4')] = 2

        self.dcf[('ccd', 'CCD_NAME')]= 'CD'
        self.dcf[('ccd', 'CCD_BINNING')]= '1x1'
        self.dcf[('ccd', 'WINDOW')]= '[ -3, -1, -1, -1 ]'
        self.dcf[('ccd', 'PIXELSIZE')]= 9.e-6 # unit meter
        self.dcf[('ccd', 'BASE_EXPOSURE')]= .01

        self.dcf[('mode', 'SET_FOCUS')]= True
        self.dcf[('mode', 'WRITE_FILTER_OFFSETS')]= True

        # mapping of fits header elements to canonical
        self.dcf[('fits header mapping', 'AMBIENTTEMPERATURE')]= 'HIERARCH DAVIS.DOME_TMP'
        self.dcf[('fits header mapping', 'DATETIME')]= 'JD'
        self.dcf[('fits header mapping', 'EXPOSURE')]= 'EXPOSURE'
        self.dcf[('fits header mapping', 'CCD_TEMP')]= 'CCD_TEMP'
        self.dcf[('fits header mapping', 'FOC_POS')] = 'FOC_POS'
        self.dcf[('fits header mapping', 'DATE-OBS')]= 'DATE-OBS'
        self.dcf[('fits header mapping', 'BINNING')]= 'BINNING'
        self.dcf[('fits header mapping', 'BINNING_X')]= 'BIN_V'
        self.dcf[('fits header mapping', 'BINNING_Y')]= 'BIN_H'

        self.dcf[('telescope', 'TEL_RADIUS')] = 0.09 # [meter]
        self.dcf[('telescope', 'TEL_FOCALLENGTH')] = 1.26 # [meter]

        self.dcf[('connection', 'URL')] = 'http://127.0.0.1:8889' 
#        self.dcf[('connection', 'USERNAME')] = 'rts2saf'
#        self.dcf[('connection', 'PASSWORD')] = 'writeToDevice'
        self.dcf[('connection', 'USERNAME')] = 'petr'
        self.dcf[('connection', 'PASSWORD')] = 'test'

        self.dcf[('queue focus run', 'FWHM_LOWER_THRESH')] = 35.

        self.dcf[('analysis', 'FWHM_MIN')] = 1.5 
        self.dcf[('analysis', 'FWHM_MAX')] = 12. 
        self.dcf[('analysis', '')] = '' 


    def dumpDefaults(self):
        for (identifier), value in self.dcf.iteritems():
            print "dump defaults :", ', ', identifier, '=>', value


    def writeDefaultConfiguration(self, cfn='./rts2saf-default.cfg'):
        for (section, identifier), value in sorted(self.dcf.iteritems()):
            print section, '=>', identifier, '=>', value
            if self.config.has_section(section)== False:
                self.config.add_section(section)

            self.config.set(section, identifier, value)

        with open( cfn, 'w') as configfile:
            configfile.write('# 2013-09-10, Markus Wildi\n')
            configfile.write('# default configuration for rts2saf\n')
            configfile.write('#\n')
            configfile.write('#\n')
            self.config.write(configfile)


class Configuration(DefaultConfiguration):
    # init from base class
    def readConfiguration(self, fileName=None):
        # make the values accessible
        self.cfg=dict()

        config = ConfigParser.ConfigParser()
        config.optionxform = str

        if os.path.exists(fileName):
            try:
                config.readfp(open(fileName))
            except Exception, e:                
                self.logger.error('Configuration.readConfiguration: config file {0} has wrong syntax:\n{1},\nexiting'.format(fileName,e))
                sys.exit(1)
            # ok, I misuse ConfigParser
            # check additional elements or typo
            for sct in config.sections():
                for k,v in config.items(sct): 
                    try:
                        val=self.dcf[(sct, k)]
                    except Exception, e:
                        self.logger.error('Configuration.readConfiguration: config file {0} has wrong syntax:\n{1},\nexiting'.format(fileName,e))
                        sys.exit(1)

        else:
            self.logger.error('Configuration.readConfiguration: config file {0} not found, exiting'.format(fileName))
            sys.exit(1)

        # read the defaults
        for (section, identifier), value in self.dcf.iteritems():
            self.cfg[identifier]= value

        # over write the defaults
        ftds=list()
        for (section, identifier), value in self.dcf.iteritems():
            try:
                value = config.get( section, identifier)
            except Exception, e:
                # exception if section, identifier value are not present in config file
                #self.logger.error('Configuration.readConfiguration: config file {0} has an error at section:{1}, identifier:{2}, value:{3}'.format(fileName, section, identifier, value))
                #sys.exit(1)
                continue

            value= string.replace( value, ' ', '')
            if self.debug: print '{} {} {}'.format(section, identifier, value)

            items=list()
            # decode the compound configuration expressions first, the rest is copied to  
            # after completion
            if  section in 'SExtractor':
                if identifier in 'FIELDS':
                    value=value.replace("'", '')
                    self.cfg['FIELDS']=value[1:-1].split(',')
                else:
                    self.cfg[identifier]= value

            elif section in 'basic': 
                if identifier in 'EMPTY_SLOT_NAMES':
                    self.cfg[identifier]=value[1:-1].split(',')
                else:
                    self.cfg[identifier]= value

            elif section in 'filter properties': 
                self.cfg[identifier]= value
                ftds.append(value)

            elif( section in 'filter wheel'):
                items= value[1:-1].split(',')
                self.filterWheelsDefs[items[0]]=items[1:]

            elif( section in 'filter wheels'):
                items= value[1:-1].split(',')
                for item in items:
                    self.filterWheelsInUseDefs.append(item)
            # first bool, then int !
            elif isinstance(self.cfg[identifier], bool):
                # ToDO, looking for a direct way
                if value in 'True':
                    self.cfg[identifier]= True
                else:
                    self.cfg[identifier]= False
            elif( isinstance(self.cfg[identifier], int)):
                try:
                    self.cfg[identifier]= int(value)
                except:
                    self.logger.error('Configuration.readConfiguration: no int '+ value+ ' in section ' +  section + ', identifier ' +  identifier + ' in file ' + fileName)
                    
            elif(isinstance(self.cfg[identifier], float)):
                try:
                    self.cfg[identifier]= float(value)
                except:
                    self.logger.error('Configuration.readConfiguration: no float '+ value+ 'in section ' +  section + ', identifier ' +  identifier + ' in file ' + fileName)

            else:
                self.cfg[identifier]= value

        # configuration has been read in, now create objects
        # create object CCD
        # ToDo, what does RTS2::ccd driver expect: int or str list?
        # for now: int
        wItems= re.split('[,]+', self.cfg['WINDOW'][1:-1])
        if len(wItems) < 4:
            self.logger.warn( 'Configuration.readConfiguration: too few ccd window items {0} {1}, using the whole CCD area'.format(len(items), value))
            wItems= [ -1, -1, -1, -1]
        else:
            wItems[:]= map(lambda x: int(x), wItems)

        self.ccd= dev.CCD( 
            name         =self.cfg['CCD_NAME'],
            binning      =self.cfg['CCD_BINNING'],
            windowOffsetX=wItems[0],
            windowOffsetY=wItems[1],
            windowHeight =wItems[2],
            windowWidth  =wItems[3],
            pixelSize    =float(self.cfg['PIXELSIZE']),
            baseExposure =float(self.cfg['BASE_EXPOSURE'])
            )

        # create object focuser
        self.foc= dev.Focuser(
            name         =self.cfg['FOCUSER_NAME'],
            resolution   =float(self.cfg['FOCUSER_RESOLUTION']),
            absLowerLimit=int(self.cfg['FOCUSER_ABSOLUTE_LOWER_LIMIT']),
            absUpperLimit=int(self.cfg['FOCUSER_ABSOLUTE_UPPER_LIMIT']),
            lowerLimit   =int(self.cfg['FOCUSER_LOWER_LIMIT']),
            upperLimit   =int(self.cfg['FOCUSER_UPPER_LIMIT']),
            stepSize     =int(self.cfg['FOCUSER_STEP_SIZE']),
            speed        =float(self.cfg['FOCUSER_SPEED']),
            temperatureCompensation=bool(self.cfg['FOCUSER_TEMPERATURE_COMPENSATION']),
            # set at run time:
            #focFoff=None,
            #focDef=None,
            #focMn=None,
            #focMx=None,
            #focSt=None
            )
        # create objects filter
        for ftd in ftds:
            ftItems= ftd[1:-1].split(',')
            lowerLimit    = int(ftItems[1])
            upperLimit    = int(ftItems[2])
            stepSize      = int(ftItems[3])
            focFoff=range(lowerLimit, (upperLimit + stepSize), stepSize)

            ft=dev.Filter( 
                name          = ftItems[0],
                lowerLimit    =lowerLimit,
                upperLimit    =upperLimit,
                stepSize      =stepSize,
                exposureFactor=string.atof(ftItems[4]),
                focFoff=focFoff
                )

            self.filters.append(ft)

        # create objects FilterWheel whith filter wheels names and with filter objects
        #  ftwn: W2
        #  ftds: ['nof1', 'U', 'Y', 'O2']
        for ftwn,ftds in self.filterWheelsDefs.iteritems():
            # ToDo (Python) if filters=list() is not present, then all filters appear in all filter wheels
            ftw=dev.FilterWheel(name=ftwn,filters=list())
            for ftd in ftds:
                for ft in self.filters:
                    if ftd in ft.name: 
                        ftw.filters.append(ft)
                        break
                else:
                    self.logger.error('Configuration.readConfiguration: no filter named: {0} found in config: {1}, exiting'.format(ftd, fileName)  )
                    sys.exit(1)
            self.filterWheels.append(ftw)

        # create from
        # ftwd: W0
        for ftwd in self.filterWheelsInUseDefs:
            for ftw in self.filterWheels:
                if ftwd in ftw.name:
                    break
            else:
                self.logger.error('Configuration.readConfiguration: no filter wheel named: {0} found in config: {1}'.format(ftwd, fileName))
                sys.exit(1)
                
            self.filterWheelsInUse.append(ftw)
        # for convenience
        self.cfg['AVAILABLE FILTERS']=self.filters
        self.cfg['AVAILABLE FILTER WHEELS']=self.filterWheels 
        self.cfg['FILTER WHEELS IN USE']=self.filterWheelsInUse 

    def writeConfiguration(self, cfn='./rts2saf-my-new.cfg'):
        for (section, identifier), value in sorted(self.dcf.iteritems()):
            print section, '=>', identifier, '=>', value
            if self.config.has_section(section)== False:
                self.config.add_section(section)

            self.config.set(section, identifier, value)
        
        with open( cfn, 'w') as configfile:
            configfile.write(' 2013-09-10, Markus Wildi\n')
            configfile.write(' default configuration for rts2saf\n')
            configfile.write('\n')
            configfile.write('\n')
            self.config.write(configfile)


    def checkConfiguration(self):
        # rts2.sextractur excepts the file not found error and uses internal defaults, we check that here
        if not os.path.exists(self.cfg['SEXPATH']):
            self.logger.warn( 'sextract: sextractor path:{0} not valid, returning'.format(self.cfg['SEXPATH']))            
            return False
        if not os.path.exists(self.cfg['SEXCFG']):
            self.logger.warn( 'sextract: config file:{0} not found, returning'.format(self.cfg['SEXCFG']))            
            return False
        if not os.path.exists(self.cfg['STARNNW_NAME']):
            self.logger.warn( 'sextract: config file:{0} not found, returning'.format(self.cfg['STARNNW_NAME']))            
            return False
        if not self.cfg['FIELDS']:
            self.logger.warn( 'sextract: no sextractor fields defined')
            return False
        return True
        # more to come


if __name__ == '__main__':

    import argparse
    try:
        import lib.log as  lg
    except:
        import log as lg

    prg= re.split('/', sys.argv[0])[-1]
    parser= argparse.ArgumentParser(prog=prg, description='rts2asaf analysis')
    parser.add_argument('--debug', dest='debug', action='store_true', default=False, help=': %(default)s,add more output')
    parser.add_argument('--level', dest='level', default='INFO', help=': %(default)s, debug level')
    parser.add_argument('--topath', dest='toPath', metavar='PATH', action='store', default='.', help=': %(default)s, write log file to path')
    parser.add_argument('--logfile',dest='logfile', default='{0}.log'.format(prg), help=': %(default)s, logfile name')
    parser.add_argument('--toconsole', dest='toconsole', action='store_true', default=False, help=': %(default)s, log to console')
    parser.add_argument('--config', dest='config', action='store', default='/etc/rts2/rts2saf/rts2saf.cfg', help=': %(default)s, configuration file path')

    args=parser.parse_args()

    lgd= lg.Logger(debug=args.debug, args=args) # if you need to chage the log format do it here
    logger= lgd.logger 

    rt=Configuration(logger=logger)
    rt.writeDefaultConfiguration()
#    rt.dumpDefaults()

    rt.readConfiguration(fileName=args.config)

    if not rt.checkConfiguration():
        print 'Something wrong with configuration'
        sys.exit(1)

#    for c,v in rt.cfg.iteritems():
#        print c,v


    for ftw in rt.cfg['AVAILABLE FILTER WHEELS']:
        print '---------------------------- {} {}'.format(ftw.name, len(ftw.filters))
        for ft in ftw.filters:
            print 'name {}'.format(ft.name)
            print 'step {}'.format(ft.stepSize)
        print '----------------------------'

#    rt.writeConfiguration(cfn='./rts2saf-my-new.cfg')