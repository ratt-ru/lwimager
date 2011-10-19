/*
 * AsynchronousTools.h
 *
 *  Created on: Nov 1, 2010
 *      Author: jjacobs
 */

#ifndef ASYNCHRONOUSTOOLS_H_
#define ASYNCHRONOUSTOOLS_H_

#include <casa/aips.h>
#include <casa/aipstype.h>
#include <casa/BasicSL/String.h>
#include <boost/utility.hpp>

#include <map>
#include <queue>

using std::map;
using std::queue;

//using namespace casa;

namespace casa {

namespace async {

class MutexImpl;

class Mutex {

    friend class Condition;

public:

    Mutex ();
    virtual ~Mutex ();

    void lock ();
    //Bool lock (Int milliseconds);
    void unlock ();
    Bool trylock ();

protected:

    pthread_mutex_t * getRep ();

private:

    MutexImpl * impl_p;

    Mutex (const Mutex & other); // illegal operation: do not define
    Mutex operator= (const Mutex & other); // illegal operation: do not define

};

class MutexLocker {

public:

    MutexLocker (async::Mutex & mutex);
    virtual ~MutexLocker ();

private:

    async::Mutex & mutex_p;

    MutexLocker (const MutexLocker & other); // illegal operation: do not define
    MutexLocker operator= (const MutexLocker & other); // illegal operation: do not define

};

class ConditionImpl;

class Condition {

public:

    Condition ();
    virtual ~Condition ();

    void broadcast ();
    void signal ();
    void wait (async::Mutex & mutex);
    // Bool wait (Mutex & mutex, int milliseconds);

private:

    ConditionImpl * impl_p;
};

class SemaphoreImpl;

class Semaphore {

public:

    Semaphore (int initialValue = 0);
    ~Semaphore ();

    Int getValue ();
    void post ();
    Bool trywait ();
    void wait ();
    Bool wait (int milliseconds);

private:

    SemaphoreImpl * impl_p;
    String name_p;

    Semaphore (const Semaphore & other); // illegal operation: do not define
    Semaphore operator= (const Semaphore & other); // illegal operation: do not define

};

class Thread {

public:

    typedef void * (* ThreadFunction) (void *);

    Thread ();
    virtual ~Thread ();

    pthread_t getId () const;
    pid_t gettid () const; // linux only
    bool isTerminationRequested () const;
    void * join ();
    void startThread ();
    virtual void terminate ();

protected:

    bool isStarted () const;
    virtual void * run () = 0;

    static void * threadFunction (void *);

private:

    pthread_t * id_p;
    bool started_p;
    volatile bool terminationRequested_p;

};

class Logger : private boost::noncopyable {

public:

    void log (const char * format, ...);
    void registerName (const String & threadName);
    void start (const char * logFilename);

    static Logger * get ();

protected:

    class LoggerThread : public Thread {
    public:

        LoggerThread ();
        ~LoggerThread ();

        void log (const string & text);
        void setLogFilename (const String & filename);
        void terminate ();

    protected:

        void * run ();

    private:

        Bool      deleteStream_p;
        String    logFilename_p;
        Condition loggerChanged_p;
        ostream * logStream_p;
        async::Mutex mutex_p;
        queue<string> outputQueue_p;
    };


//    void log (char * format, ...);
//    void setLogFilename (char * logFilename);
//    void terminate ();

private:

    typedef map <pthread_t, String>  ThreadNames;

    LoggerThread * loggerThread_p;
    bool loggingStarted_p;
    async::Mutex * nameMutex_p;
    ThreadNames threadNames_p;

    static Logger * singleton_p;

    Logger (); // singleton
    ~Logger ();

    static void initialize ();
};


}

}


#endif /* ASYNCHRONOUSTOOLS_H_ */
