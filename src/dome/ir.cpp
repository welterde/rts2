#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <comedilib.h>

#include "dome.h"

class Rts2DevDomeIR:public Rts2DevDome
{
private:
  char *dome_file;
  comedi_t *it;
  double temp, humi, wind;

  int initDevice ();
  int comediValue (int channel, double *value);
  int getSNOW ();

  int openLeft ();
  int openRight ();
  int closeLeft ();
  int closeRight ();

protected:
//   virtual int processOption ();
//   virtual int isGoodWeather ();

public:
    Rts2DevDomeIR (int argc, char **argv);
    virtual ~ Rts2DevDomeIR (void);
  virtual int init ();
  virtual int idle ();

  virtual int ready ();
  virtual int off ();
  virtual int standby ();
  virtual int observing ();
  virtual int baseInfo ();
  virtual int info ();
/*
  virtual int openDome ();
  virtual long isOpened ();
  virtual int closeDome ();
  virtual long isClosed ();
*/
};

int
Rts2DevDomeIR::initDevice ()
{
  it = comedi_open ("/dev/comedi0");

  return 0;
}

int
Rts2DevDomeIR::comediValue (int channel, double *value)
{
  int subdev = 0;
  int max, range = 0;
  lsampl_t data;
  comedi_range *rqn;
  int aref = AREF_GROUND;

  max = comedi_get_maxdata (it, subdev, channel);
  rqn = comedi_get_range (it, subdev, channel, range);
  comedi_data_read (it, subdev, channel, range, aref, &data);
  *value = comedi_to_phys (data, rqn, max);

  return 0;
}

Rts2DevDomeIR::Rts2DevDomeIR (int in_argc, char **in_argv):Rts2DevDome (in_argc,
	     in_argv)
{
  addOption ('f', "dome_file", 1, "/dev file for dome serial port");
  // addOption ('w', "wdc_file", 1, "/dev file with watch-dog card");
  // addOption ('t', "wdc_timeout", 1, "WDC timeout (default to 30 seconds");

  dome_file = "/dev/ttyS0";
  // wdc_file = NULL;
  // wdc_port = -1;
  // wdcTimeOut = 30.0;


  // movingState = MOVE_NONE;

  // weatherConn = NULL;

  // lastClosing = 0;
  // closingNum = 0;
  // lastClosingNum = -1;
}

Rts2DevDomeIR::~Rts2DevDomeIR (void)
{

}

int
Rts2DevDomeIR::getSNOW ()
{
  int sockfd, bytes_read, ret, i, j = 0;
  struct sockaddr_in dest;
  char buffer[300], buff[300];
  fd_set rfds;
  struct timeval tv;
  double wind1, wind2;
  char *token;

  if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
      fprintf (stderr, "unable to create socket");
      return 1;
    }

  bzero (&dest, sizeof (dest));
  dest.sin_family = PF_INET;
  dest.sin_addr.s_addr = inet_addr ("193.146.151.70");
  dest.sin_port = htons (6341);
  ret = connect (sockfd, (struct sockaddr *) &dest, sizeof (dest));
  if (ret)
    {
      fprintf (stderr, "unable to connect to SNOW");
      return 1;
    }

  /* send command to SNOW */
  sprintf (buffer, "%s", "RCD");
  ret = write (sockfd, buffer, strlen (buffer));
  if (!ret)
    {
      fprintf (stderr, "unable to send command");
      return 1;
    }

  FD_ZERO (&rfds);
  FD_SET (sockfd, &rfds);

  tv.tv_sec = 10;
  tv.tv_usec = 0;

  ret = select (FD_SETSIZE, &rfds, NULL, NULL, &tv);

  if (ret == 1)
    {
      bytes_read = read (sockfd, buffer, sizeof (buffer));
      if (bytes_read > 0)
	{
	  for (i = 5; i < bytes_read; i++)
	    {
	      if (buffer[i] == ',')
		buff[j++] = '.';
	      else
		buff[j++] = buffer[i];
	    }
	}
    }
  else
    {
      fprintf (stderr, ":-( no data");
      return 1;
    }

  close (sockfd);

  token = strtok ((char *) buff, "|");
  for (j = 1; j < 40; j++)
    {
      token = (char *) strtok (NULL, "|#");
      if (j == 3)
	wind1 = atof (token);
      if (j == 6)
	wind2 = atof (token);
      if (j == 18)
	temp = atof (token);
      if (j == 24)
	humi = atof (token);
    }

  if (wind1 > wind2)
    wind = wind1;
  else
    wind = wind2;

  return 0;
}

int
Rts2DevDomeIR::info ()
{
  /*
     int ret;
     ret = zjisti_stav_portu ();
     if (ret)
     return -1;
     sw_state = getPortState (KONCAK_OTEVRENI_PRAVY);
     sw_state |= (getPortState (KONCAK_OTEVRENI_LEVY) << 1);
     sw_state |= (getPortState (KONCAK_ZAVRENI_PRAVY) << 2);
     sw_state |= (getPortState (KONCAK_ZAVRENI_LEVY) << 3);
     rain = weatherConn->getRain ();
   */
  getSNOW ();
  windspeed = wind;
  temperature = temp;
  humidity = humi;
  nextOpen = getNextOpen ();
  return 0;
}

int
Rts2DevDomeIR::baseInfo ()
{
  return 0;
}

int
Rts2DevDomeIR::off ()
{
  closeDome ();
  return 0;
}

int
Rts2DevDomeIR::standby ()
{
  closeDome ();
  return 0;
}

int
Rts2DevDomeIR::ready ()
{
  return 0;
}

int
Rts2DevDomeIR::idle ()
{
  return Rts2DevDome::idle ();
}

int
Rts2DevDomeIR::init ()
{
  int ret = Rts2DevDome::init ();
  if (ret)
    return ret;
  return 0;
}

int
Rts2DevDomeIR::observing ()
{
  if ((getState (0) & DOME_DOME_MASK) == DOME_CLOSED)
    return openDome ();
  return 0;
}

Rts2DevDomeIR *device;

int
main (int argc, char **argv)
{
  device = new Rts2DevDomeIR (argc, argv);

  int ret;
  ret = device->init ();

  if (ret)
    {
      fprintf (stderr, "Cannot initialize dome - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
