/* 
 * Plan target class.
 * Copyright (C) 2003-2007 Petr Kubanek <petr@kubanek.net>
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

#ifndef __RTS2_PLAN__
#define __RTS2_PLAN__

#include "target.h"
#include "observation.h"

#include <ostream>

namespace rts2db
{

class Observation;

/**
 * Plan target class.
 *
 * This class support target chaining method for supplying targets from prepared plan.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Plan
{
	public:
		Plan ();
		Plan (int in_plan_id);
		virtual ~ Plan (void);
		int load ();
		int save ();
		int del ();

		moveType startSlew (struct ln_equ_posn *position);

		Target *getTarget ();
		void clearTarget () { target = NULL; }

		Observation *getObservation ();

		int getPlanId () { return plan_id; }

		time_t getPlanStart ()  { return plan_start; }

		time_t getPlanEnd () { return plan_end; }

		friend std::ostream & operator << (std::ostream & _os, Plan * plan) { plan->print (_os); return _os; }
		friend Rts2InfoValStream & operator << (Rts2InfoValStream & _os, Plan * plan) { plan->printInfoVal (_os); return _os; }

		friend std::istream & operator >> (std::istream & _is, Plan & plan) { plan.read (_is); return _is; }

	protected:
		int plan_id;
		int prop_id;
		int tar_id;
		int obs_id;
		time_t plan_start;
		time_t plan_end;

		int plan_status;

		Target *target;
		Observation *observation;

		void print (std::ostream & _os);
		void printInfoVal (Rts2InfoValStream & _os);

		void read (std::istream & _is);
};

}

#endif							 /* !__RTS2_PLAN__ */