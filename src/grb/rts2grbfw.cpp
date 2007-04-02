#include "grbd.h"
#include "rts2grbfw.h"
#include "grbconst.h"

#include <errno.h>

Rts2GrbForwardConnection::Rts2GrbForwardConnection (Rts2Block * in_master,
						    int in_forwardPort):
Rts2ConnNoSend (in_master)
{
  forwardPort = in_forwardPort;
}

int
Rts2GrbForwardConnection::init ()
{
  int ret;
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    {
      logStream (MESSAGE_ERROR)
	<< "Rts2GrbForwardConnection cannot create listen socket "
	<< strerror (errno) << sendLog;
      return -1;
    }
  // do some listen stuff..
  const int so_reuseaddr = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr,
	      sizeof (so_reuseaddr));
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons (forwardPort);
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  ret = bind (sock, (struct sockaddr *) &server, sizeof (server));
  if (ret == -1)
    {
      logStream (MESSAGE_ERROR) << "Rts2GrbForwardConnection::init bind " <<
	strerror (errno) << sendLog;
      return -1;
    }
  ret = listen (sock, 5);
  if (ret)
    {
      logStream (MESSAGE_ERROR) <<
	"Rts2GrbForwardConnection::init cannot listen: " << strerror (errno)
	<< sendLog;
      close (sock);
      sock = -1;
      return -1;
    }
  return 0;
}

int
Rts2GrbForwardConnection::receive (fd_set * set)
{
  int new_sock;
  struct sockaddr_in other_side;
  socklen_t addr_size = sizeof (struct sockaddr_in);
  if (FD_ISSET (sock, set))
    {
      new_sock = accept (sock, (struct sockaddr *) &other_side, &addr_size);
      if (new_sock == -1)
	{
	  logStream (MESSAGE_ERROR) <<
	    "Rts2GrbForwardConnection::receive accept " << strerror (errno) <<
	    sendLog;
	  return 0;
	}
      logStream (MESSAGE_INFO)
	<< "Rts2GrbForwardClientConn::accept connection from "
	<< inet_ntoa (other_side.sin_addr) << " port " << ntohs (other_side.
								 sin_port) <<
	sendLog;
      Rts2GrbForwardClientConn *newConn =
	new Rts2GrbForwardClientConn (new_sock, getMaster ());
      getMaster ()->addConnection (newConn);
    }
  return 0;
}

/**
 * Takes cares of client connections.
 */

Rts2GrbForwardClientConn::Rts2GrbForwardClientConn (int in_sock, Rts2Block * in_master):Rts2ConnNoSend (in_sock,
		in_master)
{
}

void
Rts2GrbForwardClientConn::forwardPacket (long *nbuf)
{
  int ret;
  ret = write (sock, nbuf, SIZ_PKT * sizeof (nbuf[0]));
  if (ret != SIZ_PKT)
    {
      logStream (MESSAGE_ERROR)
	<< "Rts2GrbForwardClientConn::forwardPacket cannot forward "
	<< strerror (errno) << " (" << errno << ", " << ret << ")" << sendLog;
      connectionError (-1);
    }
}

void
Rts2GrbForwardClientConn::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case RTS2_EVENT_GRB_PACKET:
      forwardPacket ((long *) event->getArg ());
      break;
    }
  Rts2ConnNoSend::postEvent (event);
}

int
Rts2GrbForwardClientConn::receive (fd_set * set)
{
  static long loc_buf[SIZ_PKT];
  if (sock < 0)
    return -1;

  if (FD_ISSET (sock, set))
    {
      int ret;
      ret = read (sock, loc_buf, SIZ_PKT);
      if (ret != SIZ_PKT)
	{
	  logStream (MESSAGE_ERROR) << "Rts2GrbForwardClientConn::receive "
	    << strerror (errno) << " (" << errno << ", " << ret << ")"
	    << sendLog;
	  connectionError (ret);
	  return -1;
	}
      // get some data back..
      successfullRead ();
      return ret;
    }
  return 0;
}
