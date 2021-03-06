/*
 * Class for two line elements (Satellites) targets.
 * Copyright (C) 2015 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "rts2db/tletarget.h"

#include "infoval.h"
#include "libnova_cpp.h"
#include "rts2fits/image.h"

#include "pluto/norad.h"
#include "pluto/observe.h"

using namespace rts2db;

TLETarget::TLETarget (int in_tar_id, struct ln_lnlat_posn *in_obs, double in_altitude):Target (in_tar_id, in_obs, in_altitude)
{
}

void TLETarget::load ()
{
	Target::load ();
	// split two lines..
	std::string tarInfo (getTargetInfo());
	size_t sub = tarInfo.find ('|');
	if (sub != std::string::npos)
	{
		tle1 = tarInfo.substr (0, sub - 1);
		tle2 = tarInfo.substr (sub + 1);

		int ret = parse_elements (tle1.c_str (), tle2.c_str (), &tle);
		if (ret != 0)
		{
			throw rts2core::Error ("cannot parse TLE");
		}

		ephem = 1;
		is_deep = select_ephemeris (&tle);

		if (is_deep && (ephem == 1 || ephem == 2))
			ephem += 2;    /* switch to an SDx */
		if (!is_deep && (ephem == 3 || ephem == 4))
			ephem -= 2;    /* switch to an SGx */
	}
}

void TLETarget::getPosition (struct ln_equ_posn *pos, double JD)
{
	double sat_params[N_SAT_PARAMS], observer_loc[3];
	double t_since;
	double sat_pos[3]; /* Satellite position vector */
	double dist_to_satellite;

	double r_s, r_c;

	lat_alt_to_parallax (ln_deg_to_rad (observer->lat), obs_altitude * 1000, &r_s, &r_c);

	t_since = (JD - tle.epoch) * 1440.;
	switch (ephem)
	{
		case 0:
			SGP_init (sat_params, &tle);
			SGP (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 1:
			SGP4_init (sat_params, &tle);
			SGP4 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 2:
			SGP8_init (sat_params, &tle);
			SGP8 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 3:
			SDP4_init (sat_params, &tle);
			SDP4 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		case 4:
			SDP8_init (sat_params, &tle);
			SDP8 (t_since, &tle, sat_params, sat_pos, NULL);
			break;
		default:
			throw rts2core::Error ("invalid ephem");
	}

	get_satellite_ra_dec_delta (observer_loc, sat_pos, &(pos->ra), &(pos->dec), &dist_to_satellite);
}

int TLETarget::getRST (struct ln_rst_time *rst, double JD, double horizon)
{
}

void TLETarget::printExtra (Rts2InfoValStream & _os, double JD)
{
	Target::printExtra (_os, JD);
}

void TLETarget::writeToImage (rts2image::Image * image, double JD)
{
	Target::writeToImage (image, JD);
}

double TLETarget::getEarthDistance (double JD)
{
}

double TLETarget::getSolarDistance (double JD)
{
}
