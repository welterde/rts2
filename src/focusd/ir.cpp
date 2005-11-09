/*!
 * $Id$
 *
 * @author standa
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <opentpl/client.h>

#include "focuser.h"

using namespace OpenTPL;

class Rts2DevFocuserIr:public Rts2DevFocuser
{
private:
  char *ir_ip;
  int ir_port;
  Client *tplc;
  int is_focusing;
  // low-level tpl functions
    template < typename T > int tpl_get (const char *name, T & val,
					 int *status);
    template < typename T > int tpl_set (const char *name, T val,
					 int *status);
    template < typename T > int tpl_setw (const char *name, T val,
					  int *status);
  // low-level image fuctions
  // int data_handler (int sock, size_t size, struct image_info *image);
  // int readout ();
  // high-level I/O functions
  // int foc_pos_get (int * position);
  // int foc_pos_set (int pos);
protected:
    virtual int endFocusing ();
public:
    Rts2DevFocuserIr (int argc, char **argv);
    virtual ~ Rts2DevFocuserIr (void);
  virtual int processOption (int in_opt);
  virtual int init ();
  virtual int ready ();
  virtual int baseInfo ();
  virtual int info ();
  virtual int stepOut (int num);
  virtual int setTo (int num);
  virtual int isFocusing ();
};

template < typename T > int
Rts2DevFocuserIr::tpl_get (const char *name, T & val, int *status)
{
  int cstatus;

  if (!*status)
    {
      Request & r = tplc->Get (name, false);
      cstatus = r.Wait (USEC_SEC);

      if (cstatus != TPLC_OK)
	{
	  syslog (LOG_ERR, "Rts2DevFocuserIr::tpl_get error %i\n", cstatus);
	  *status = 1;
	}

      if (!*status)
	{
	  RequestAnswer & answr = r.GetAnswer ();

	  if (answr.begin ()->second.first == TPL_OK)
	    val = (T) answr.begin ()->second.second;
	  else
	    *status = 2;
	}
      r.Dispose ();
    }
  return *status;
}

template < typename T > int
Rts2DevFocuserIr::tpl_set (const char *name, T val, int *status)
{
  int cstatus;

  if (!*status)
    {
      tplc->Set (name, Value (val), true);	// change to set...?
    }
  return *status;
}

template < typename T > int
Rts2DevFocuserIr::tpl_setw (const char *name, T val, int *status)
{
  int cstatus;

  if (!*status)
    {
      Request & r = tplc->Set (name, Value (val), false);	// change to set...?
      cstatus = r.Wait ();

      if (cstatus != TPLC_OK)
	{
	  syslog (LOG_ERR, "Rts2DevFocuserIr::tpl_setw error %i\n", cstatus);
	  *status = 1;
	}
      r.Dispose ();
    }
  return *status;
}


Rts2DevFocuserIr::Rts2DevFocuserIr (int in_argc, char **in_argv):Rts2DevFocuser (in_argc,
		in_argv)
{
  is_focusing = 0;
  ir_ip = NULL;
  ir_port = 0;
  tplc = NULL;

  addOption ('I', "ir_ip", 1, "IR TCP/IP adress");
  addOption ('P', "ir_port", 1, "IR TCP/IP port number");

  strcpy (focType, "BOOTES_IR");
}

Rts2DevFocuserIr::~Rts2DevFocuserIr ()
{

}

int
Rts2DevFocuserIr::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'I':
      ir_ip = optarg;
      break;
    case 'P':
      ir_port = atoi (optarg);
      break;
    default:
      return Rts2DevFocuser::processOption (in_opt);
    }
  return 0;
}

/*!
 * Init focuser, connect on given port port, set manual regime
 *
 * @return 0 on succes, -1 & set errno otherwise
 */
int
Rts2DevFocuserIr::init ()
{
  int ret;
  ret = Rts2DevFocuser::init ();
  if (ret)
    return ret;

  if (!ir_ip || !ir_port)
    {
      fprintf (stderr, "Invalid port or IP address of mount controller PC\n");
      return -1;
    }

  tplc = new Client (ir_ip, ir_port, 2000);

  syslog (LOG_DEBUG,
	  "Status: ConnID = %i, connected: %s, authenticated %s, Welcome Message = %s",
	  tplc->ConnID (), (tplc->IsConnected ()? "yes" : "no"),
	  (tplc->IsAuth ()? "yes" : "no"), tplc->WelcomeMessage ().c_str ());

  if (!tplc->IsAuth () || !tplc->IsConnected ())
    {
      syslog (LOG_ERR, "Connection to server failed");
      return -1;
    }

  return 0;
}

int
Rts2DevFocuserIr::ready ()
{
  return !tplc->IsConnected ();
}

int
Rts2DevFocuserIr::baseInfo ()
{
  return 0;
}

int
Rts2DevFocuserIr::info ()
{
  int status = 0;

  if (!(tplc->IsAuth () && tplc->IsConnected ()))
    return -1;

  double realPos;
  status = tpl_get ("FOCUS.REALPOS", realPos, &status);
  if (status)
    return -1;

  focPos = (int) (realPos * 1000.0);
  focTemp = nan ("f");

  return 0;
}

int
Rts2DevFocuserIr::stepOut (int num)
{
  int status = 0;

  int power = 1;
  int referenced = 0;
  double offset;

  status = tpl_get ("FOCUS.REFERENCED", referenced, &status);
  if (referenced != 1)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::stepOut referenced is : %i",
	      referenced);
      return -1;
    }
  status = tpl_setw ("FOCUS.POWER", power, &status);
  if (status)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::stepOut cannot set POWER to 1");
    }
  status = tpl_get ("FOCUS.OFFSET", offset, &status);
  offset += (double) num / 1000.0;
  status = tpl_setw ("FOCUS.OFFSET", offset, &status);
  if (status)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::stepOut cannot set offset!");
      return -1;
    }
  return 0;
}

int
Rts2DevFocuserIr::setTo (int num)
{
  int status = 0;

  int power = 1;
  int referenced = 0;
  double offset;

  status = tpl_get ("FOCUS.REFERENCED", referenced, &status);
  if (referenced != 1)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::stepOut referenced is : %i",
	      referenced);
      return -1;
    }
  status = tpl_setw ("FOCUS.POWER", power, &status);
  if (status)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::stepOut cannot set POWER to 1");
    }
  offset = (double) num / 1000.0;
  status = tpl_setw ("FOCUS.OFFSET", offset, &status);
  if (status)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::stepOut cannot set offset!");
      return -1;
    }
  setFocusTimeout (100);
  return 0;
}

int
Rts2DevFocuserIr::isFocusing ()
{
  double targetdistance;
  int status = 0;
  status = tpl_get ("FOCUS.TARGETDISTANCE", targetdistance, &status);
  if (status)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::isFocusing status: %i", status);
      return -1;
    }
  return (fabs (targetdistance) < 0.001) ? -2 : USEC_SEC / 50;
}

int
Rts2DevFocuserIr::endFocusing ()
{
  int status = 0;
  int power = 0;
  status = tpl_setw ("FOCUS.POWER", power, &status);
  if (status)
    {
      syslog (LOG_ERR, "Rts2DevFocuserIr::endFocusing cannot set POWER to 0");
      return -1;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  Rts2DevFocuserIr *device = new Rts2DevFocuserIr (argc, argv);

  int ret;
  ret = device->init ();
  if (ret)
    {
      fprintf (stderr, "Cannot initialize focuser - exiting!\n");
      exit (0);
    }
  device->run ();
  delete device;
}
