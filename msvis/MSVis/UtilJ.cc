/*
 * Utilj.cc
 *
 *  Created on: Nov 4, 2010
 *      Author: jjacobs
 */

#include <stdarg.h>
#include <cstdio>
#include <cstring>
#include <errno.h>

#include <casa/aips.h>
#include <casa/aipstype.h>
#include <casa/BasicSL/String.h>
#include <sys/time.h>
///#include <execinfo.h>
#include <algorithm>
#include <math.h>

using std::max;
using std::min;

using namespace casa;

#include "UtilJ.h"

namespace casa {

namespace utilj {

String
formatV (const String & formatString, va_list vaList)
{
	char buffer [4096];
	int nPrinted = vsnprintf (buffer, sizeof (buffer), formatString.c_str(), vaList);

	if (nPrinted >= (int) sizeof (buffer) - 1){
		buffer [sizeof (buffer) - 2] = '|'; // mark as truncated
	}

	return buffer;
}

String
format (const char * formatString, ...)
{

	// Return a String object created using the printf-like format string
	// and the provided arguments.  If the text is truncated because the
	// internal buffer is too small, the last character will be a pipe "|".

	va_list vaList;

	va_start (vaList, formatString);

	String result = formatV (formatString, vaList);

	va_end (vaList);

	return result;
}


Bool
getEnv (const String & name, const Bool & defaultValue)
{
	char * value = getenv (name.c_str());

	if (value == NULL){
		return defaultValue;
	}
	else{
		String stringValue = value;
		stringValue.downcase();
		Bool truthValue = True;

		if (stringValue == "false" ||
			stringValue == "f" ||
			stringValue == "0" ||
			stringValue == "no"){

			truthValue = False;
		}

		return truthValue;
	}
}

Int
getEnv (const String & name, const Int & defaultValue)
{
	char * value = getenv (name.c_str());

	if (value == NULL){
		return defaultValue;
	}
	else{

		char * next;
		long longValue = strtol (value, & next, 10);

		// If all of the value characters weren't consumed, assume
		// an error occurred and use the default value.

		if (* next != '\0')
			longValue = defaultValue;

		return longValue;
	}
}


String
getTimestamp ()
{
	// Get a possibly decent resolution time value

	struct timeval now;
	gettimeofday (& now, NULL);

	// Convert from UTC to local time

	struct tm localNow = * localtime (& now.tv_sec);

	// Output the seconds portion of the time in the format
	// hh:mm:ss

	char buffer [128];
	strftime (buffer, sizeof(buffer), "%X", & localNow);

	// Add on the higher resolution portion (if any) of the time
	// as milliseconds

	char buffer2 [128];

	snprintf (buffer2, sizeof(buffer2), "%s.%03d", buffer, (int) now.tv_usec/1000);

	// Return the final result in the format "hh:mm:ss.ttt"

	return buffer2;
}

Bool
isEnvDefined (const String & name)
{
	char * value = getenv (name.c_str());

	return value != NULL;
}

void
printBacktrace (ostream & os, const String & prefix)
{
  /*
    void * stack [512];
    int nUsed = backtrace (stack, 512);
    char ** trace = backtrace_symbols (stack, nUsed);
    if (! prefix.empty()){
        os << prefix << endl;
    }
    os << "*** Stack trace (use c++filt to demangle):" << endl;
    for (int i = 0; i < nUsed; i++){
        os << trace[i] << endl;
    }
    os.flush();
    delete trace;
  */
}

void
sleepMs (Int milliseconds)
{
    struct timespec t, tRemaining;
    t.tv_sec = milliseconds / 1000;
    t.tv_nsec = (milliseconds - t.tv_sec) * 1000000;

    // Because nanosleep can be interrupted by a signal, it is necessary
    // to continue the wait if errno is EINTR.  When interrupted, nanosleep
    // copies the amount of time remaining in the wait into tRemaining; so
    // the remainder of one interation becomes the wait value proper on the
    // next iteration.

    Bool done;
    do {
        done = nanosleep (& t, & tRemaining) == 0 || errno != EINTR;
        t = tRemaining;
    } while (! done);

}


void
toStdError (const String & m, const String & prefix)
{
    cerr << prefix << m << endl;
    cerr.flush();
}


void
throwIf (bool condition, const String & message, const String & file, Int line)
{

	// If the condition is met then throw an AipsError

	if (condition) {
	    AipsErrorTrace e (message.c_str(), file.c_str(), line);

#       if defined (NDEBUG)
	        toStdError (e.what());
#       endif

		throw e;
	}
}

void
throwIfError (int errorCode, const String & prefix, const String & file, Int line)
{
	// If the provided error code is not equal to success (0) then
	// throw an AipsError using the provided suffix and then details
	// of the error.

	if (errorCode != 0) {
		AipsErrorTrace e (format ("%s (errno=%d):%s", prefix.c_str(), errorCode, strerror (errorCode)),
				          file.c_str(), line);

#       if defined (NDEBUG)
	        toStdError (e.what());
#       endif

	    throw e;
	}
}

DeltaThreadTimes
ThreadTimes::operator- (const ThreadTimes & tEarlier) const
{
    return DeltaThreadTimes (this->elapsed() - tEarlier.elapsed(),
                             this->cpu() - tEarlier.cpu());
}

DeltaThreadTimes &
DeltaThreadTimes::operator+= (const DeltaThreadTimes & other)
{
    cpu_p += other.cpu();
    elapsed_p += other.elapsed();
    n_p += 1;

    if (doStats_p){
        cpuSsq_p += other.cpu() * other.cpu();
        cpuMin_p = min (cpuMin_p, other.cpu());
        cpuMax_p = max (cpuMax_p, other.cpu());
        elapsedSsq_p += other.elapsed() * other.elapsed();
        elapsedMin_p = min (elapsedMin_p, other.elapsed());
        elapsedMax_p = max (elapsedMax_p, other.elapsed());
    }

    return * this;
}


String
DeltaThreadTimes::formatAverage (const String & floatFormat,
                           Double scale,
                           const String & units) const // to convert to ms
{
    String realFormat = casa::utilj::format ("(el=%s,cp=%s,%%4.1f%%%%) %s",
                                             floatFormat.c_str(), floatFormat.c_str(), units.c_str());
    Int n = n_p != 0 ? n_p : 1;
    Double c = cpu_p / n * scale;
    Double e = elapsed_p / n * scale;
    Double p = c / e * 100;

    String result = casa::utilj::format (realFormat.c_str(), e, c, p);

    return result;
}

String
DeltaThreadTimes::formatStats (const String & floatFormat,
                         Double scale,
                         const String & units) const  // to convert to ms
{
    String realFormat = casa::utilj::format ("(el=%s {%s-%s,%s}, cp=%s {%s-%s,%s}, %%4.1f%%%%) %s",
                                             floatFormat.c_str(),
                                             floatFormat.c_str(),
                                             floatFormat.c_str(),
                                             floatFormat.c_str(),
                                             floatFormat.c_str(),
                                             floatFormat.c_str(),
                                             floatFormat.c_str(),
                                             floatFormat.c_str(),
                                             units.c_str());
    Int n = n_p != 0 ? n_p : 1;
    Double c = cpu_p / n * scale;
    Double cS = sqrt (cpuSsq_p / n_p - c * c);
    Double e = elapsed_p / n * scale;
    Double eS = sqrt (elapsedSsq_p / n_p - e * e);
    Double p = c / e * 100;

    String result = casa::utilj::format (realFormat.c_str(), e, elapsedMin_p, elapsedMax_p, eS,
                                         c, cpuMin_p, cpuMax_p, cS, p);

    return result;
}

AipsErrorTrace::AipsErrorTrace ( const String &msg, const String &filename, uInt lineNumber,
                                 Category c)
: AipsError (msg, filename, lineNumber, c)
{
  ///    void * stack [512];
  ///    int n = backtrace (stack, 512);
  ///    char ** trace = backtrace_symbols (stack, n);

    message += "\nStack trace (use c++filt to demangle):\n";
    ///    for (int i = 0; i < n; i++){
    ///        message += trace[i] + String ("\n");
    ///    }
    ///    free (trace);
}

} // end namespace utilj

} // end namespace casa
