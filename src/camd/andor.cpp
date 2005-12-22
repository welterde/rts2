#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "camera_cpp.h"
#include "../utils/rts2device.h"
#include "../utils/rts2block.h"

#ifdef __GNUC__
#  if(__GNUC__ > 3 || __GNUC__ ==3)
#	define _GNUC3_
#  endif
#endif

#ifdef _GNUC3_
#  include <iostream>
#  include <fstream>
using namespace std;
#else
#  include <iostream.h>
#  include <fstream.h>
#endif

#include "atmcdLXd.h"

class CameraAndorChip:public CameraChip
{
private:
  unsigned short *dest;		// for chips..
  unsigned short *dest_top;
  char *send_top;
public:
    CameraAndorChip (Rts2DevCamera * in_cam, int in_chip_id, int in_width,
		     int in_height, double in_pixelX, double in_pixelY,
		     float in_gain);
    virtual ~ CameraAndorChip (void);
  virtual long isExposing ();
  virtual int stopExposure ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int readoutOneLine ();
};

CameraAndorChip::CameraAndorChip (Rts2DevCamera * in_cam, int in_chip_id,
				  int in_width, int in_height,
				  double in_pixelX, double in_pixelY,
				  float in_gain):
CameraChip (in_cam, in_chip_id, in_width, in_height, in_pixelX, in_pixelY,
	    in_gain)
{
  dest = new unsigned short[in_width * in_height];
};

CameraAndorChip::~CameraAndorChip (void)
{
  delete dest;
};

long
CameraAndorChip::isExposing ()
{
  long ret;
  ret = CameraChip::isExposing ();
  if (ret > 0)
    return ret;

  int status;
  GetStatus (&status);
  if (status == DRV_ACQUIRING)
    return 0;
  return -2;
}

int
CameraAndorChip::stopExposure ()
{
  AbortAcquisition ();
  return CameraChip::stopExposure ();
}

int
CameraAndorChip::startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn)
{
  dest_top = dest;
  send_top = (char *) dest;
  return CameraChip::startReadout (dataConn, conn);
}

int
CameraAndorChip::readoutOneLine ()
{
  if (readoutLine < chipSize->height)
    {
      int size = chipSize->height * chipSize->width;
      readoutLine = chipSize->height;
      GetAcquiredData16 (dest, size);
      dest_top += size;
      return 0;
    }
  if (sendLine == 0)
    {
      int ret;
      ret = CameraChip::sendFirstLine ();
      if (ret)
	return ret;
    }
  int send_data_size;
  sendLine++;
  send_data_size = sendReadoutData (send_top, (char *) dest_top - send_top);
  if (send_data_size < 0)
    return -1;
  send_top += send_data_size;
  if (send_top < (char *) dest_top)
    return 0;
  return -2;
}

class Rts2DevCameraAndor:public Rts2DevCamera
{
private:
  int andorGain;
  char *andorRoot;
  int horizontalSpeed;
  int verticalSpeed;
  int adChannel;
  bool printSpeedInfo;
  // number of AD channels
  int chanNum;

  int camSetShutter (int shut_control);
  int printChannelInfo (int channel);
protected:
    virtual void help ();
public:
    Rts2DevCameraAndor (int argc, char **argv);
    virtual ~ Rts2DevCameraAndor (void);

  virtual int processOption (int in_opt);
  virtual int init ();

  // callback functions for Camera alone
  virtual int ready ();
  virtual int info ();
  virtual int baseInfo ();
  virtual int camChipInfo (int chip);
  virtual int camExpose (int chip, int light, float exptime);
  virtual int camStopExpose (int chip);
  virtual int camStopRead (int chip);
  virtual int camCoolMax ();
  virtual int camCoolHold ();
  virtual int camCoolTemp (float new_temp);
  virtual int camCoolShutdown ();
};

Rts2DevCameraAndor::Rts2DevCameraAndor (int in_argc, char **in_argv):
Rts2DevCamera (in_argc, in_argv)
{
  andorRoot = "/root/andor/examples/common";
  andorGain = 255;
  horizontalSpeed = -1;
  verticalSpeed = -1;
  adChannel = -1;
  printSpeedInfo = false;
  chanNum = 0;
  addOption ('r', "root", 1, "directory with Andor detector.ini file");
  addOption ('g', "gain", 1, "set camera gain level (0-255)");
  addOption ('H', "horizontal_speed", 1, "set horizontal readout speed");
  addOption ('C', "ad_channel", 1, "set AD channel which will be used");
  addOption ('V', "vertical_speed", 1, "set vertical readout speed");
  addOption ('S', "speed_info", 0,
	     "print speed info - information about speed available");
}

Rts2DevCameraAndor::~Rts2DevCameraAndor (void)
{
  ShutDown ();
}

void
Rts2DevCameraAndor::help ()
{
  printf ("Driver for Andor CCDs (iXon & others)\n");
  Rts2DevCamera::help ();
}

int
Rts2DevCameraAndor::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'g':
      andorGain = atoi (optarg);
      if (andorGain > 255 || andorGain < 0)
	{
	  printf ("gain must be in 0-255 range\n");
	  exit (EXIT_FAILURE);
	}
      break;
    case 'r':
      andorRoot = optarg;
      break;
    case 'H':
      horizontalSpeed = atoi (optarg);
      break;
    case 'V':
      verticalSpeed = atoi (optarg);
      break;
    case 'S':
      printSpeedInfo = true;
      break;
    default:
      return Rts2DevCamera::processOption (in_opt);
    }
  return 0;
}

int
Rts2DevCameraAndor::printChannelInfo (int channel)
{
  int ret;
  int speeds;
  float value;
  int depth;
  int i;
  ret = GetBitDepth (channel, &depth);
  if (ret != DRV_SUCCESS)
    {
      syslog (LOG_ERR,
	      "Rts2DevCameraAndor::printChannelInfo cannto get depth for channel %i",
	      channel);
      return -1;
    }
  syslog (LOG_DEBUG,
	  "Rts2DevCameraAndor::printChannelInfo depth for channel %i value: %i",
	  channel, depth);
  for (int j = 0; j < 2; j++)
    {
      ret = GetNumberHSSpeeds (channel, j, &speeds);
      if (ret != DRV_SUCCESS)
	{
	  syslog (LOG_ERR,
		  "Rts2DevCameraAndor::printChannelInfo cannot get horizontal speeds for channel %i type %i",
		  channel, j);
	  return -1;
	}
      for (i = 0; i < speeds; i++)
	{
	  ret = GetHSSpeed (channel, j, i, &value);
	  if (ret != DRV_SUCCESS)
	    {
	      syslog (LOG_ERR,
		      "Rts2DevCameraAndor::printChannelInfo cannot get horizontal speed %i, channel %i type %i",
		      i, channel, j);
	      return -1;
	    }
	  syslog (LOG_DEBUG,
		  "Rts2DevCameraAndor::printChannelInfo horizontal speed: %i, channel %i type %i value: %f Hz",
		  i, channel, j, value);
	}
    }
  return 0;
}

int
Rts2DevCameraAndor::init ()
{
  CameraAndorChip *cc;
  unsigned long error;
  int width;
  int height;
  int ret;

  ret = Rts2DevCamera::init ();
  if (ret)
    return ret;

  error = Initialize (andorRoot);

  if (error != DRV_SUCCESS)
    {
      cout << "Initialisation error...exiting" << endl;
      return -1;
    }

  sleep (2);			//sleep to allow initialization to complete

  //Set Read Mode to --Image--
  SetReadMode (4);

  //Set Acquisition mode to --Single scan--
  SetAcquisitionMode (1);

  //Get Detector dimensions
  GetDetector (&width, &height);

  //Initialize Shutter
  camSetShutter (0);

  SetExposureTime (5.0);
  SetEMCCDGain (andorGain);
  if (adChannel >= 0)
    {
      ret = SetADChannel (adChannel);
      if (ret != DRV_SUCCESS)
	{
	  syslog (LOG_ERR,
		  "Rts2DevCameraAndor::init cannot set AD channel to %i",
		  adChannel);
	  return -1;
	}
    }

  if (horizontalSpeed >= 0)
    {
      for (int i = 0; i < 2; i++)
	{
	  ret = SetHSSpeed (i, horizontalSpeed);
	  if (ret != DRV_SUCCESS)
	    {
	      syslog (LOG_ERR,
		      "Rts2DevCameraAndor::init cannot set horizontal speed to channel %i, type %i, value %i",
		      adChannel, i, horizontalSpeed);
	      return -1;
	    }
	}
    }

  if (verticalSpeed >= 0)
    {
      ret = SetVSSpeed (verticalSpeed);
      if (ret != DRV_SUCCESS)
	{
	  syslog (LOG_ERR,
		  "Rts2DevCameraAndor::init cannot set vertical speed to %i",
		  verticalSpeed);
	  return -1;
	}
    }

  chipNum = 1;

  cc = new CameraAndorChip (this, 0, width, height, 10, 10, 1);
  chips[0] = cc;
  chips[1] = NULL;

  if (printSpeedInfo)
    {
      int channels;
      int speeds;
      float value;
      int i;
      ret = GetNumberADChannels (&channels);
      if (ret != DRV_SUCCESS)
	{
	  syslog (LOG_ERR,
		  "Rts2DevCameraAndor::init cannot get number of AD channels");
	  return -1;
	}
      syslog (LOG_DEBUG, "Rts2DevCameraAndor::init channels: %i", channels);
      // print horizontal channels..
      for (i = 0; i < channels; i++)
	{
	  ret = printChannelInfo (i);
	  if (ret)
	    {
	      return ret;
	    }
	}

      // print vertical channels..
      ret = GetNumberVSSpeeds (&speeds);
      if (ret != DRV_SUCCESS)
	{
	  syslog (LOG_ERR,
		  "Rts2DevCameraAndor::init cannot get horizontal speeds");
	  return -1;
	}
      for (i = 0; i < speeds; i++)
	{
	  ret = GetVSSpeed (i, &value);
	  if (ret != DRV_SUCCESS)
	    {
	      syslog (LOG_ERR,
		      "Rts2DevCameraAndor::init cannot get vertical speed %i",
		      i);
	      return -1;
	    }
	  syslog (LOG_DEBUG,
		  "Rts2DevCameraAndor::init vertical speed: %i value: %f Hz",
		  i, value);
	}
    }
  return Rts2DevCamera::initChips ();
}

int
Rts2DevCameraAndor::ready ()
{
  return 0;
}

int
Rts2DevCameraAndor::info ()
{
  int c_status;
  c_status = GetTemperatureF (&tempCCD);
  tempAir = nan ("f");
  coolingPower = (int) (50 * 1000);
  tempRegulation = (c_status != DRV_TEMPERATURE_OFF);
  return 0;
}

int
Rts2DevCameraAndor::baseInfo ()
{
  sprintf (ccdType, "ANDOR");
  // get serial number
  return 0;
}

int
Rts2DevCameraAndor::camChipInfo (int chip)
{
  return 0;
}

int
Rts2DevCameraAndor::camExpose (int chip, int light, float exptime)
{
  SetExposureTime (exptime);
  camSetShutter (light == 1 ? 0 : 2);
  StartAcquisition ();
  return 0;
}

int
Rts2DevCameraAndor::camStopExpose (int chip)
{
  // not supported
  return -1;
}

int
Rts2DevCameraAndor::camStopRead (int chip)
{
  return -1;
}

int
Rts2DevCameraAndor::camCoolMax ()
{
  return camCoolHold ();
}

int
Rts2DevCameraAndor::camCoolHold ()
{
  if (isnan (nightCoolTemp))
    return camCoolTemp (-5);
  else
    return camCoolTemp (nightCoolTemp);
}

int
Rts2DevCameraAndor::camCoolTemp (float new_temp)
{
  CoolerON ();
  SetTemperature ((int) new_temp);
  tempSet = new_temp;
  return 0;
}

int
Rts2DevCameraAndor::camCoolShutdown ()
{
  CoolerOFF ();
  return 0;
}

int
Rts2DevCameraAndor::camSetShutter (int shut_control)
{
  return SetShutter (1, shut_control, 50, 50);
}

Rts2DevCameraAndor *device = NULL;

void
killSignal (int sig)
{
  delete device;
  exit (0);
}

int
main (int argc, char **argv)
{
  device = new Rts2DevCameraAndor (argc, argv);

  signal (SIGINT, killSignal);
  signal (SIGTERM, killSignal);

  int ret;
  ret = device->init ();
  if (ret)
    {
      syslog (LOG_ERR, "Cannot initialize Andor camera - exiting!");
      exit (1);
    }
  device->run ();
  delete device;
}
