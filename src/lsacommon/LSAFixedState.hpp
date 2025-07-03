/*
	Copyright (c) 2015-2024 Applied Research Laboratories, The University of Texas
	at Austin (ARL:UT).
	
	SALSA is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License version 3 (GPL-3.0-only) as published
	by the Free Software Foundation.
	
	SALSA is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License along with
	SALSA.  If not, see https://www.gnu.org/licenses/.
*/
#ifndef LSAFixedState_HPP
#define LSAFixedState_HPP

#include <string>

class LSAFixedState
{


public:

    enum State
    {
       Unknown = 0,
       FLOATING,
       CONSTRAINED,
       FIXED,
       count
    };

    /// Constructor, including empty constructor
    LSAFixedState(State t = Unknown) throw()
    {
       if(t < 0 || t >= count)
          state = Unknown;
       else
          state = t;
    }

    /// constructor from int
    LSAFixedState(int i) throw()
    {
       if(i < 0 || i >= count)
          state = Unknown;
       else
          state = static_cast<State>(i);
    }

    LSAFixedState(std::string input)
    {
        if      (input == "Floating" )   state = FLOATING;
        else if (input == "Constrained") state = CONSTRAINED;
        else if (input == "Fixed")       state = FIXED;
        else state = Unknown;
    }

    /// boolean operator==
    bool operator==(const LSAFixedState& right) const throw()
    { return state == right.state; }

    /// boolean operator< (used by STL to sort)
    bool operator<(const LSAFixedState& right) const throw()
    { return state < right.state; }

    // the rest follow from Boolean algebra...
    /// boolean operator!=
    bool operator!=(const LSAFixedState& right) const throw()
    { return !operator==(right); }

    /// boolean operator>=
    bool operator>=(const LSAFixedState& right) const throw()
    { return !operator<(right); }

    /// boolean operator<=
    bool operator<=(const LSAFixedState& right) const throw()
    { return (operator<(right) || operator==(right)); }

    /// boolean operator>
    bool operator>(const LSAFixedState& right) const throw()
    { return (!operator<(right) && !operator==(right)); }

    std::string asString()
    {
        if      (state == FLOATING)    return "Floating";
        else if (state == CONSTRAINED) return "Constrained";
        else if (state == FIXED)       return "Fixed";

        return "Unknown";
    }

    std::string asLSAFileString()
    {
        if      (state == FLOATING)    return "Flt";
        else if (state == CONSTRAINED) return "Con";
        else if (state == FIXED)       return "Fix";

        return "Unknown";
    }

    void fromLSAFileString(std::string input)
    {
        if      (input == "Flt") state = FLOATING;
        else if (input == "Con") state = CONSTRAINED;
        else if (input == "Fix") state = FIXED;
        else state = Unknown;

        return;
    }

private:
    State state;
};


#endif // LSAFixedState_HPP
