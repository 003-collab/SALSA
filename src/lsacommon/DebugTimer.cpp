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
#include "DebugTimer.hpp"

DebugTimer::DebugTimer(bool enabled) : isEnabled(enabled), previousTime(0)
{
    timer.start();
    if (isEnabled)
    {
        #ifdef _WIN32
        qDebug() << QString(" ");
        #else
        std::cout << std::endl;
        #endif
    }
}

int DebugTimer::getDelta()
{
    int currentTime = timer.elapsed();
    int delta = timer.elapsed() - previousTime;
    previousTime = currentTime;

    return delta;
}

void DebugTimer::logTime(std::string label, const std::string& function)
{
    if (isEnabled)
    {
        std::ostringstream oss;
        oss << "LOG_TIME " << function << " " << std::setw(25) << label << " (ms): " << getDelta();

        #ifdef _WIN32
        qDebug() << QString::fromStdString(oss.str());
        #else
        std::cout << oss.str() << std::endl;
        #endif
    }
}

// Define the static members
std::chrono::steady_clock DebugTimer2::timer;
std::unordered_map<std::string, std::stack<time_point<steady_clock>>> DebugTimer2::m_labelToStartTimeStackMap;
std::unordered_map<std::string, std::vector<long long>> DebugTimer2::m_labelToAllReadings;

DebugTimer2::DebugTimer2(bool bPrintAvg) : m_bPrintAvg(bPrintAvg)
{
}

// Print out the average collected over the lifetime of the object
DebugTimer2::~DebugTimer2()
{
    if (!m_bPrintAvg) return;
    
    qDebug() << "Averages:";
    for(auto& labelToReading : m_labelToAllReadings)
    {
        long long total = 0;
        for (auto& reading : labelToReading.second) { total += reading; }
        qDebug() << labelToReading.first.c_str() << labelToReading.second.size() << ": "
                 << total / (long long)labelToReading.second.size();
    }
}

// Get the start time, and add to the stack for that label
void DebugTimer2::tick(std::string label)
{
    m_labelToStartTimeStackMap[label].push(timer.now());
}

void DebugTimer2::tock(std::string label, bool bPrint /* = false */)
{
    // get the stack for that label
    auto stackPlate = m_labelToStartTimeStackMap.find(label);
    if(stackPlate == m_labelToStartTimeStackMap.end() || stackPlate->second.size() == 0)
    {
        qDebug() << "ruh roh! Make sure you spelled the label correctly.";
        return;
    }
    
    // get the value for last element in and pop
    auto timeGone = std::chrono::duration_cast<std::chrono::microseconds>(timer.now() - stackPlate->second.top()).count();
    if(bPrint)
    {
        qDebug() << stackPlate->first.c_str() << ": " << timeGone;
    }
    stackPlate->second.pop();
    //record the result
    m_labelToAllReadings[stackPlate->first].push_back(timeGone);
   
}
