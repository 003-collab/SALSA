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
#ifndef DEBUGTIMER_HPP
#define DEBUGTIMER_HPP

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <QTime>
#include <QDebug>
#include <chrono>
#include <stack>
#include <unordered_map>

/*************************************

Recommended Usage:

void someClass::myFunction()
{
    DebugTimer timer(true);

    <do some expensive stuff>
    LOG_TIME(timer, "First Step");

    <do some more expensive stuff>
    LOG_TIME(timer, "Second Step");
}

Output:
LOG_TIME myFunction        First Step (ms): 15
LOG_TIME myFunction       Second Step (ms): 0

*************************************/
class DebugTimer
{
public:
    explicit DebugTimer(bool enabled = true);

    int getDelta();
    void logTime(std::string label, const std::string& function);

private:
    QTime timer;
    int previousTime;
    bool isEnabled;
    

};


#define LOG_TIME(timer, label) { timer.logTime(label, __FUNCTION__); }
/*************************************

  This timer differs from the one above by counting from the start time decided by the dev,
  instead of counting from the previous time check. It also works across different scopes. This means
  that it should work in the case of recursion and allows averaging of those times.
  
  It works best if the function call is wrapped with tick and tock.  I don't recommend wrapping chunks
  of code, because I don't know if the compiler will reorder execution or something (especially release build).
  
Recommended Usage:

void someClass::myFunction()
{
    DebugTimer2 shineyTimer(false);

    shineyTimer.tick("maybeSlow");
    maybeSlow();
    shineyTimer.tock("maybeSlow");
    
    shineyTimer.tick("maybeAlsoSlow");
    maybeAlsoSlow();
    shineyTimer.tock("maybeAlsoSlow");
}

Output (in microseconds):
maybeSlow: 1523531
maybeAlsoSlow: 1232

*************************************/
using namespace std::chrono;
class DebugTimer2
{
public:
    // The timer will calculate and print (if bPrintAvg == true) the average time for the list of times for a particular key (label)
    // when the timer's destructor is called (when it goes out of scope)
    DebugTimer2(bool bPrintAvg = true);
    ~DebugTimer2(); // Will print averages if object was initialized with bPrintAvg = true;
    
    // label: Can be anything, but it must match a label passed in to tock. There can be multiple tick/tock's with the same label.
    // Pushes the start tiem for a label onto the label's stack of start times.
    void tick(std::string label);
    // Pops a time off of the label's stack, and records how much time has passed since that tick. Will print that difference if bPrint is true.
    void tock(std::string label, bool bPrint = false);
private:
    bool m_bPrintAvg;
    static std::chrono::steady_clock timer;
    static std::unordered_map<std::string, std::stack<time_point<steady_clock>>> m_labelToStartTimeStackMap;
    static std::unordered_map<std::string, std::vector<long long>> m_labelToAllReadings;
};


#endif // DEBUGTIMER_HPP

