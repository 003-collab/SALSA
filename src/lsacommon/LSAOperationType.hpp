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
#ifndef LSAOPERATIONTYPE_HPP
#define LSAOPERATIONTYPE_HPP

class LSAOperationType
{
public:
    enum Types
    {
        // Unknown must be first, and must = 0
        Unknown = 0,         ///< unknown record type
        Insert,
        Delete,
        Modify,
        count
    };

    LSAOperationType(Types t = Unknown)
    {
       if(t < 0 || t >= count)
          type = Unknown;
       else
          type = t;
    }

    /// boolean operator==
    bool operator==(const LSAOperationType& right) const throw()
    { return type == right.type; }

    /// boolean operator< (used by STL to sort)
    bool operator<(const LSAOperationType& right) const throw()
    { return type < right.type; }

    // the rest follow from Boolean algebra...
    /// boolean operator!=
    bool operator!=(const LSAOperationType& right) const throw()
    { return !operator==(right); }

    /// boolean operator>=
    bool operator>=(const LSAOperationType& right) const throw()
    { return !operator<(right); }

    /// boolean operator<=
    bool operator<=(const LSAOperationType& right) const throw()
    { return (operator<(right) || operator==(right)); }

    /// boolean operator>
    bool operator>(const LSAOperationType& right) const throw()
    { return (!operator<(right) && !operator==(right)); }

private:
   /// record type
   Types type;
};

#endif // LSAOPERATIONTYPE_HPP
