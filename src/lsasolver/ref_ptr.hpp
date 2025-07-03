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
/// @file ref_ptr.hpp  A solution to the polymorphic class containers problem in STL.

#ifndef REFERENCE_POINTER_INCLUDE
#define REFERENCE_POINTER_INCLUDE

/// Class ref_ptr a solution to the polymorphic class containers problem in STL. It
/// allows polymorphic objects (base and derived classes) to be held in arrays,
/// including the STL. It is used like this: \code
///    list < ref_ptr<Base> > baseList;      // T=Base is the base class
///    baseList.push_back(derived1(base));
///    baseList.push_back(derived2(base)); \endcode
/// In the above usage, the T& constructor is called which sets kill to true;
/// this means that when baseList is destroyed, it will call the destructors of the
/// ref_ptr objects which will in turn destroy the referent objects. With this
/// variant of reference classes, one can also signal the class not to destroy the
/// object (if that is what is needed) by using the T* constructor instead: \code
///   static ellipse persistentEllipse(rect3);
///   baseList.push_back(&persistentEllipse); \endcode
/// When using the iterators of baseList, one must pay attention to the extra level
/// of indirection: \code
///   list < ref_ptr<base> >::iterator it;
///   for (it=baseList.begin(); it!=baseList.end(); it++)
///      (*it)->draw(); OR (**it).draw();
/// \endcode
/// One of problems with this solution is that it defeats a lot of the efficiency of
/// the STL. The STL goes to great lengths to efficiently store its objects in blocks
/// (separating memory allocation/deallocation and construction/destruction). The
/// ref_ptr class blatantly allocates and deallocates its referents one at a time.
/// NB clone() must be defined in the base (class T) and all derived classes, do with
///    \code virtual T* clone(void) const = 0; \endcode
/// in the base class, class T.
/// Reference: http://kremer.cpsc.ucalgary.ca/STL/1024x768/ref2.html
template <class T> class ref_ptr
{
protected:
   T *ptr;           ///< pointer to the object

private:
   bool kill;        ///< flag telling destructor whether or not to delete ptr.

public:
   /// constructor from const reference
   ref_ptr(const T &s)
   {  
      kill = true;
      ptr = s.clone();
   }

   /// constructor from pointer
   ref_ptr(T *s)
   {
      kill = false;
      ptr = s;
   }

   /// destructor
   ~ref_ptr()
   {
      if(ptr && kill) delete ptr;
   }

   /// copy constructor
   ref_ptr(const ref_ptr<T> &r)
   {
      kill = true;
      ptr = r.ptr ? r.ptr->clone() : NULL;
   }

   /// operator=
   ref_ptr& operator=(const ref_ptr<T> &r)
   {
      if (ptr && kill) delete ptr;
      kill = true;
      ptr = (r.ptr ? r.ptr->clone() : NULL);
      return *this;
   }

   /// ptr access operator
   T* operator->() const { return ptr; }

   /// less than operator
   int operator<(const ref_ptr<T> &r) const
   {
      return (ptr ? r.ptr ? (*ptr) < (*r.ptr) : false : true);
   }

   /// reference operator
   operator T&() const {return *ptr;}

   /// pointer operator
   operator T*() const {return ptr;}

   /// ptr dereference operator
   T& operator*() const {return *ptr;}

};    // end class ref_ptr

#endif   // REFERENCE_POINTER_INCLUDE
