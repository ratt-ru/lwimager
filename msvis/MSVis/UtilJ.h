/*
 * UtilJ.h
 *
 *  Created on: Nov 4, 2010
 *      Author: jjacobs
 */

#ifndef UTILJ_H_
#define UTILJ_H_

#include <casa/aips.h>
#include <casa/BasicSL/String.h>
#include <casa/Exceptions/Error.h>
#include <cassert>
#include <cstdlib>
#include <cstdarg>
#include <sys/time.h>
#include <sys/resource.h>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

// The Assert macro is an alias to the standard assert macro when NDEBUG is defined.  When
// NDEBUG is not defined (release build) then a throw is used to report the error.

#ifdef NDEBUG
#define Assert(c) assert (c)
#else
#define Assert(c) { throwIf (! (c), "Assertion failed: " #c, __FILE__, __LINE__); }
#endif

#define AssertAlways(c) { throwIf (! (c), "Assertion failed: " #c, __FILE__, __LINE__); }

#if defined (NDEBUG)
#    define Throw(m) \
    { AipsError anAipsError ((m), __FILE__, __LINE__);\
      toStdErr (anAipsError.what());\
      throw anAipsError; }
#else
#    define Throw(m) throw AipsError ((m), __FILE__, __LINE__)
#endif

#define ThrowIf(c,m) casa::utilj::throwIf ((c), (m), __FILE__, __LINE__)

#define ThrowIfError(c,m) casa::utilj::throwIfError ((c), (m), __FILE__, __LINE__)

namespace casa {

class String;

namespace utilj {

class AipsErrorTrace : public AipsError {

public:

    AipsErrorTrace ( const String &msg, const String &filename, uInt lineNumber,
                     Category c = GENERAL);

};

class Strings : public std::vector<String> {};

//template <typename Element, typename Container>
//Bool
//contains (const Element & e, const Container & c)
//{
//	return c.find(e) != c.end();
//}


template <typename Container>
Bool
containsKey (const typename Container::key_type & key,
             const Container & container)
{
    return container.find(key) != container.end();
}

template <typename Container>
Bool
contains (const typename Container::value_type & e,
          const Container & c)
{
    // For set and map use containsKey; will work for set but
    // use with map requires specifying a pair as the first argument

    return std::find(c.begin(), c.end(), e) != c.end();
}

template <typename F, typename S>
F & first (std::pair<F,S> & pair) { return pair.first;}

template <typename F, typename S>
const F & first (const std::pair<F,S> & pair) { return pair.first;}

template <typename F, typename S>
class FirstFunctor : public std::unary_function<std::pair<F,S>, F>{
public:
    F & operator() (std::pair<F,S> & p) { return p.first; }
    const F & operator() (const std::pair<F,S> & p) { return p.first; }
};


template <typename F, typename S>
FirstFunctor<F,S> firstFunctor () { return FirstFunctor<F,S> ();}


String format (const char * formatString, ...);
String formatV (const String & formatString, va_list vaList);

template<typename T>
T
getEnv (const String & name, const T & defaultValue)
{
	char * value = getenv (name.c_str());

	if (value == NULL){
		return defaultValue;
	}
	else{
		return T (value);
	}
}

Bool
getEnv (const String & name, const Bool & defaultValue);

Int
getEnv (const String & name, const Int & defaultValue);


String getTimestamp ();

Bool isEnvDefined (const String & name);


template <typename Itr>
String
join (Itr begin, Itr end, const String & delimiter)
{
    String result;
    Itr i = begin;

    if (i != end){

        result = * i ++;

        for (; i != end; i++){
            result += delimiter + * i;
        }
    }

    return result;
}

template <typename T>
String
join (const T & strings, const String & delimiter)
{
    return join (strings.begin(), strings.end(), delimiter);
}

template <typename Itr, typename F>
String
join (Itr begin, Itr end, F f, const String & delimiter)
{
    String result;
    Itr i = begin;

    if (i != end){

        result = f(* i);
        ++ i;

        for (; i != end; i++){
            result += delimiter + f (* i);
        }
    }

    return result;
}

template <typename K, typename V>
std::vector<K>
mapKeys (const std::map<K,V> & aMap)
{
    std::vector<K> result (aMap.size());

    std::transform (aMap.begin(), aMap.end(), back_inserter (result), firstFunctor<K,V>());

    return result;
}

template <typename F, typename S>
F & second (std::pair<F,S> & pair) { return pair.second;}

template <typename F, typename S>
const F & second (const std::pair<F,S> & pair) { return pair.second;}

template <typename F, typename S>
class SecondFunctor : public std::unary_function<std::pair<F,S>, F>{
public:
    S & operator() (std::pair<F,S> & p) { return p.second; }
};

template <typename F, typename S>
SecondFunctor<F,S> secondFunctor () { return SecondFunctor<F,S> ();}

template <typename K, typename V>
std::vector<V>
mapValues (const std::map<K,V> & aMap)
{
    std::vector<K> result (aMap.size());

    std::transform (aMap.begin(), aMap.end(), back_inserter (result), second<K,V>);

    return result;
}

void printBacktrace (ostream & os, const String & prefix = "");



void sleepMs (Int milliseconds);
void toStdError (const String & m, const String & prefix = "*E* ");
void throwIf (Bool condition, const String & message, const String & file, Int line);
void throwIfError (Int errorCode, const String & prefix, const String & file, Int line);

// These two classes, Times and DeltaTimes should be moved out of this file and
// into casacore/casa/OS.  In the meantime, an ifdef should keep the apple from
// barfing.

// <summary>

// </summary>

// <use visibility=local>   or   <use visibility=export>

// <reviewed reviewer="" date="yyyy/mm/dd" tests="" demos="">
// </reviewed>

// <prerequisite>
//   <li> SomeClass
//   <li> SomeOtherClass
//   <li> some concept
// </prerequisite>
//
// <etymology>
// </etymology>
//
// <synopsis>
// </synopsis>
//
// <example>
// </example>
//
// <motivation>
// </motivation>
//
// <templating arg=T>
//    <li>
//    <li>
// </templating>
//
// <thrown>
//    <li>
//    <li>
// </thrown>
//
// <todo asof="yyyy/mm/dd">
//   <li> add this feature
//   <li> fix this bug
//   <li> start discussion of this possible extension
// </todo>

class DeltaThreadTimes;

class ThreadTimes {

public:

    ThreadTimes () { * this = getTime();}

    Double cpu () const { return cpu_p;}
    Double elapsed () const { return elapsed_p;}

    static ThreadTimes
    getTime (){

        struct timeval tVal;
        gettimeofday (& tVal, NULL);

        Double elapsed = tVal.tv_sec + tVal.tv_usec * 1e-6;

        //Double cpu = ((Double) clock ()) / CLOCKS_PER_SEC; // should be in seconds



#if     defined (RUSAGE_THREAD)
        struct rusage usage;

        int failed = getrusage (RUSAGE_THREAD, & usage);
        assert (! failed);

        Double cpu = toSeconds (usage.ru_utime) + toSeconds (usage.ru_stime);
#else
        Double cpu = 0;
#endif

        return ThreadTimes (elapsed, cpu);
    }

    DeltaThreadTimes operator- (const ThreadTimes & tEarlier) const;

    static Double
    toSeconds (const struct timeval & t)
    {
        return t.tv_sec + t.tv_usec * 1e-6;
    }

protected:

    Double cpu_p;
    Double elapsed_p;

    ThreadTimes (Double elapsed, Double cpu) : cpu_p (cpu), elapsed_p (elapsed) {}
};

// <summary>
// </summary>

// <use visibility=local>   or   <use visibility=export>

// <reviewed reviewer="" date="yyyy/mm/dd" tests="" demos="">
// </reviewed>

// <prerequisite>
//   <li> SomeClass
//   <li> SomeOtherClass
//   <li> some concept
// </prerequisite>
//
// <etymology>
// </etymology>
//
// <synopsis>
// </synopsis>
//
// <example>
// </example>
//
// <motivation>
// </motivation>
//
// <templating arg=T>
//    <li>
//    <li>
// </templating>
//
// <thrown>
//    <li>
//    <li>
// </thrown>
//
// <todo asof="yyyy/mm/dd">
//   <li> add this feature
//   <li> fix this bug
//   <li> start discussion of this possible extension
// </todo>
class DeltaThreadTimes : private ThreadTimes {

    friend class ThreadTimes;

public:

    DeltaThreadTimes () : ThreadTimes (0, 0), doStats_p (False), n_p (0) {}
    explicit DeltaThreadTimes (bool doStats) : ThreadTimes (0,0), doStats_p (doStats), n_p (0)
    {
        cpuSsq_p = 0;
        cpuMin_p = 1e20;
        cpuMax_p = -1e20;
        elapsedSsq_p = 0;
        elapsedMin_p = 1e20;
        elapsedMax_p = -1e20;
    }

    DeltaThreadTimes & operator += (const DeltaThreadTimes & other);

    Double cpu () const { return ThreadTimes::cpu();}
    Double cpuAvg () const { return n_p == 0 ? 0 : cpu() / n_p;}
    Double elapsed () const { return ThreadTimes::elapsed();}
    Double elapsedAvg () const { return n_p == 0 ? 0 : elapsed() / n_p;}
    String formatAverage (const String & floatFormat = "%6.1f",
                          Double scale=1000.0,
                          const String & units = "ms")  const; // to convert to ms
    String formatStats (const String & floatFormat = "%6.1f",
                        Double scale=1000.0,
                        const String & units = "ms")  const; // to convert to ms
    Int n() const { return n_p;}

protected:

    DeltaThreadTimes (Double elapsed, Double cpu) : ThreadTimes (elapsed, cpu), n_p (0) {}

private:

    Double cpuMin_p;
    Double cpuMax_p;
    Double cpuSsq_p;
    Bool doStats_p;
    Double elapsedMin_p;
    Double elapsedMax_p;
    Double elapsedSsq_p;
    Int n_p;
};

// Global Functions

// <linkfrom anchor=unique-string-within-this-file classes="class-1,...,class-n">
//     <here> Global functions </here> for foo and bar.
// </linkfrom>

// A free function is provided that is useful for
// go here...

// <group name=accumulation>


// </group>

/*

Example of using composer and unary.  The composed functors have to be derived from std::unary_function

  int f(int x) { return x*x;}
  int g(int x) { return 2 * x;}
  int h(int x) { return 100 + x;}

  vector<int> a;
  a.push_back(1);
  a.push_back(2);
  a.push_back(3);

  transform (a.begin(), a.end(), std::ostream_iterator<int> (cout, "\n"), compose (unary(f), unary(f)));

  // prints out
  // 4
  // 16
  // 36

  transform (a.begin(), a.end(), std::ostream_iterator<int> (cout, "\n"),
             compose (unary(h), compose (unary(f), unary(f))));

  // prints out
  // 104
  // 116
  // 136

*/

template <typename F, typename G>
class ComposedFunctor : public std::unary_function <typename G::argument_type, typename F::result_type> {

public:

    ComposedFunctor (F f, G g) : f_p (f), g_p (g) {}

    typename F::result_type operator() (typename G::argument_type x) { return f_p ( g_p (x)); }

private:

    F f_p;
    G g_p;
};

template <typename F, typename G>
ComposedFunctor<F, G>
compose (F f, G g)
{
    return ComposedFunctor <F, G> (f, g);
}

template <typename D, typename R>
class UnaryFunctor : public std::unary_function<D,R> {
public:
    typedef R (* F) (D);

    UnaryFunctor (F f) : f_p (f) {}
    R operator() (D x) { return f_p (x); }

private:

    F f_p;
};

template <typename D, typename R>
UnaryFunctor <D, R>
unary (R (*f) (D)) { return UnaryFunctor<D, R> (f);}

class Z {
public:

    string getName () const { return name_p;}

    string name_p;
};



} // end namespace utilj

} // end namespace casa


#endif /* UTILJ_H_ */
