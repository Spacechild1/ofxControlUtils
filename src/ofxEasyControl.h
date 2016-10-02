#pragma once

#include "ofMain.h"
#include <list>
#include <random>

#define OFXCONTROL_DEFAULT_RATE 30

/*
	The time unit is always seconds. 

*/

/* Static class which holds the global frame rate for all ofxBaseControl and derived classes.
 * You could either set the frame rate to fixed value in the setup routine of your app,
 * or update it to the actual rate in your update routine like this:
 * ofxControl::setFrameRate(ofGetFrameRate());
 *
 * WARNING: ofGetFrameRate() uses some smoothing which can cause subtle bugs with your ofxControlObjects,
 * especially on startup, where the value ramps up from 0 to the actual frame rate
 */

class ofxControl {
public:
    ofxControl() = delete;
    static void setFrameRate(float fps);
    static float getFrameRate();
private:
    static float _fps;
};


// forward declaration of event classes
class ofxControlBaseEvent;
template<typename T> class ofxControlVarEvent;
template<typename TArg, typename TReturn, typename TObj> class ofxControlFuncEvent;



/// common base class for all ofxControl classes

class ofxBaseControl {
public:
    ofxBaseControl();
    virtual ~ofxBaseControl();
    virtual void init();
    virtual void update() = 0; // must be implemented
    virtual void setSpeed(float newSpeed);
    virtual float getSpeed() const;
    virtual void pause();
    virtual void resume();
    virtual bool isRunning() const;
protected:
    float speed;
    bool bRunning;
};

/*--------------------------------------------------------------------*/

/// ofxLine / ofxMultiLine

enum class ofxLineShape {
    STEP,
	LIN,
	FAST_EXP,
	FAST_POW,
	FAST_COS,
	SLOW_EXP,
	SLOW_POW,
	SLOW_COS,
    S_CURVE
};

template<typename T>
struct _ofxLineSegment {
    // ramp time
    float time;
    // time to wait for the line to start
    float onset;
    // target value
    T start;
    // segment shape
    T target;
    // start value
	ofxLineShape shape;
    // optional coefficient for segment shape
	float coeff;
    // elapsed time (used together with onset)
    float elapsed;
    // callback actions, executed on segment end
    list<unique_ptr<ofxControlBaseEvent>> eventList;
};

typedef _ofxLineSegment<float> ofxLineSegment;
typedef _ofxLineSegment<vector<float>> ofxMultiLineSegment; 


/// ofxLine
// change a single float value over time by a queue of line segments

class ofxLine : public ofxBaseControl {
public:
	ofxLine();
	virtual ~ofxLine();

    /* interface implementation */
    virtual void init();
	virtual void update();

    /* individual functions */
	// get the current value
    float out() const;
	// clear all line segments and set value immediatly
    void setValue(float newValue);
    // set the shape for following segment(s)
    void setShape(ofxLineShape newShape, float newCoeff = 0);
    // add a new event listener for the end of the next segment(s), writing a value to a variable
    template<typename T>
    void addOnSegmentEnd(T* var, const T & value);
    // add a new event listener for the end of the next segment(s), calling a member function with no arguments
    template<typename TReturn, typename TObj>
    void addOnSegmentEnd(TObj* obj, TReturn(TObj::*func)());
    // add a new event listener for the end of the next segment(s), calling a member function by a single argument
    template<typename TArg, typename TReturn, typename TObj>
    void addOnSegmentEnd(TObj* obj, TReturn(TObj::*func)(TArg), const TArg & arg);
    // clear event list (mostly redundand because event list is cleared automatically after each call to 'addSegment')
    void clearOnSegmentEnd();
    // add a new segment, specifing the target value, the ramp time
	// and a time onset in relation to the end of the last segment
    void addSegment(float targetValue, float rampTime = 0, float timeOnset = 0);
    // pop the last segment from the list
    void removeLastSegment();
    // pop current segment and move on to the next
    void nextSegment();
    // clear segment list (remove all segments)
	void clear();
protected:
	float value;
	ofxLineShape shape;
	float coeff;
    // temporary event list
    list<unique_ptr<ofxControlBaseEvent>> eventList;
    // queue of segments
    list<ofxLineSegment> segmentList;
};

// add a new event listener for the end of the next segment(s), writing a value to a variable
template<typename T>
void ofxLine::addOnSegmentEnd(T* var, const T & value){
    eventList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlVarEvent<T>(var, value)));
}
// add a new event listener for the end of the next segment(s), calling a member function with no arguments
template<typename TReturn, typename TObj>
void ofxLine::addOnSegmentEnd(TObj* obj, TReturn(TObj::*func)()){
    eventList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlFuncEvent<void, TReturn, TObj>(obj, func)));
}

// add a new event listener for the end of the next segment(s), calling a member function by a single argument
template<typename TArg, typename TReturn, typename TObj>
void ofxLine::addOnSegmentEnd(TObj* obj, TReturn(TObj::*func)(TArg), const TArg & arg){
    eventList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlFuncEvent<TArg, TReturn, TObj>(obj, func, arg)));
}

/// ofxMultiLine
// change several float values over time by a queue of line segments

class ofxMultiLine : ofxLine {
public:
	ofxMultiLine();
	ofxMultiLine(int numLines);
    virtual ~ofxMultiLine();

    /* interface implementation */
    virtual void init();
	virtual void update();

    /* redefined functions */

    // add new segment
    void addSegment(const vector<float> & targetValues, float rampTime, float timeOnset = 0);

    // get all values as a vector
    vector<float> out() const; //
    void removeLastSegment();
    void nextSegment();
    void clear();

    /* new functions: */

    // clear all line segments and set values immediatly
    void setValues(const vector<float> & newValues);
    void setValues(float newValue);
	// set number of lines
    void setNumLines(int numLines);
    // get number of lines
    int getNumLines() const;
    // get the current value at a certain index
	float operator[](int index) const;
protected:
    vector<float> valueVec;
    // segment list
    list<ofxMultiLineSegment> multiSegmentList;
private:
    // hide setValue
    setValue(float newValue);
};

/*------------------------------------------------------------------------*/

/// ofxClock
// list of clocks performing some task on time out. 

class ofxClock : public ofxBaseControl {
public:
	ofxClock();
    virtual ~ofxClock();

    /* interface implementation */
    virtual void init();
	virtual void update();

    /* individual functions */
    // add a new clock, writing a value to a variable
    template<typename T>
    void add(float delayTime, T* var, const T & value);
	// add a new clock, calling a member function with no arguments
    template<typename TReturn, typename TObj>
    void add(float delayTime, TObj* obj, TReturn(TObj::*func)());
    // add a new clock, calling a member function by a single argument
    template<typename TArg, typename TReturn, typename TObj>
    void add(float delayTime, TObj* obj, TReturn(TObj::*func)(TArg), const TArg & arg);
	
	// cancle all clocks writing a value to a certain variable
    template<typename T>
    void cancel(T* var);
	// cancle all clocks calling a certain member function by no arguments
    template<typename TReturn, typename TObj>
    void cancel(TObj* obj, TReturn(TObj::*func)());
    // cancle all clocks calling a certain member function by a single argument
    template<typename TArg, typename TReturn, typename TObj>
    void cancel(TObj* obj, TReturn(TObj::*func)(TArg));
	
    // cancle the first added clock
    void cancelFirst();
	// cancle the last added clock
    void cancelLast();
	// cancle all clocks
	void clear();
protected:
	list<unique_ptr<ofxControlBaseEvent>> clockList;
    void searchAndRemove(ofxControlBaseEvent* testobj);
};


// add a new clock, writing a value to a variable
template<typename T>
void ofxClock::add(float delayTime, T* var, const T & value){
    delayTime = (delayTime >= 0.0) ? delayTime : 0.0;
    clockList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlVarEvent<T>(var, value, delayTime)));
}
// add a new clock, calling a member function with no arguments
template<typename TReturn, typename TObj>
void ofxClock::add(float delayTime, TObj* obj, TReturn(TObj::*func)()){
    delayTime = (delayTime >= 0.0) ? delayTime : 0.0;
    clockList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlFuncEvent<void, TReturn, TObj>(obj, func, delayTime)));
}

// add a new clock, calling a member function by a single argument
template<typename TArg, typename TReturn, typename TObj>
void ofxClock::add(float delayTime, TObj* obj, TReturn(TObj::*func)(TArg), const TArg & arg){
    delayTime = (delayTime >= 0.0) ? delayTime : 0.0;
    clockList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlFuncEvent<TArg, TReturn, TObj>(obj, func, arg, delayTime)));
}

// cancle all clocks writing a value to a certain variable
template<typename T>
void ofxClock::cancel(T* var){
    T val{};
    ofxControlVarEvent<T> test(var, val, 0);
	searchAndRemove(&test);
}
// cancle all clocks calling a certain member function by no arguments
template<typename TReturn, typename TObj>
void ofxClock::cancel(TObj* obj, TReturn(TObj::*func)()){
    ofxControlFuncEvent<void, TReturn, TObj> test(obj, func, 0);
	searchAndRemove(&test);
}
// cancle all clocks calling a certain member function by a single argument
template<typename TArg, typename TReturn, typename TObj>
void ofxClock::cancel(TObj* obj, TReturn(TObj::*func)(TArg)){
    TArg arg{};
    ofxControlFuncEvent<TArg, TReturn, TObj> test(obj, func, arg, 0);
	searchAndRemove(&test);
}

/*-------------------------------------------------------------------------*/

/// ofxBaseOsc
/* common base class for all oscillators */

class ofxBaseOsc : public ofxBaseControl {
public:
    ofxBaseOsc();
    virtual ~ofxBaseOsc();

    /* interface implementation */
    virtual void init();
    virtual void update();

    /* new functions */
    virtual float out() const; // likely to be overwritten and implemented individually

    // as these functions only affect base class members, we will probably never override them
    void setFrequency(float hz);
    float getFrequency() const;
    void setPeriod(float seconds);
    float getPeriod() const;
    void setPhase(float newPhase); // will *not* trigger events
    float getPhase() const;
    void setPhaseOffset(float newOffset);
    float getPhaseOffset() const;

    // add event listeners, being called right before the start of a new period
    // writing a value to a variable
    template<typename T>
    void add(T* var, const T & value);
    // calling a member function with no arguments
    template<typename TReturn, typename TObj>
    void add(TObj* obj, TReturn(TObj::*func)());
    // calling a member function by a single argument
    template<typename TArg, typename TReturn, typename TObj>
    void add(TObj* obj, TReturn(TObj::*func)(TArg), const TArg & arg);

    // remove all event listeners of certain type
    // writing a value to a certain variable
    template<typename T>
    void remove(T* var);
    // calling a certain member function by no arguments
    template<typename TReturn, typename TObj>
    void remove(TObj* obj, TReturn(TObj::*func)());
    // calling a certain member function by a single argument
    template<typename TArg, typename TReturn, typename TObj>
    void remove(TObj* obj, TReturn(TObj::*func)(TArg));

    void removeAll();
    int getCounter() const;
    void resetCounter();
protected:
    float freq;
    float wrapped;
    float phase;
    float offset;
    int counter;
    bool bReset;
    list<unique_ptr<ofxControlBaseEvent>> eventList;
    void searchAndRemove(ofxControlBaseEvent * testobj);
};


// add a new event listener for the end of the next segment(s), writing a value to a variable
template<typename T>
void ofxBaseOsc::add(T* var, const T & value){
    eventList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlVarEvent<T>(var, value)));
}
// add a new event listener for the end of the next segment(s), calling a member function with no arguments
template<typename TReturn, typename TObj>
void ofxBaseOsc::add(TObj* obj, TReturn(TObj::*func)()){
    eventList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlFuncEvent<void, TReturn, TObj>(obj, func)));
}

// add a new event listener for the end of the next segment(s), calling a member function by a single argument
template<typename TArg, typename TReturn, typename TObj>
void ofxBaseOsc::add(TObj* obj, TReturn(TObj::*func)(TArg), const TArg & arg){
    eventList.push_back(unique_ptr<ofxControlBaseEvent>(new ofxControlFuncEvent<TArg, TReturn, TObj>(obj, func, arg)));
}

// add a new event listener for the end of the next segment(s), writing a value to a variable
template<typename T>
void ofxBaseOsc::remove(T* var){
    T val{};
    ofxControlVarEvent<T> test(var, val, 0);
    searchAndRemove(&test);
}
// add a new event listener for the end of the next segment(s), calling a member function with no arguments
template<typename TReturn, typename TObj>
void ofxBaseOsc::remove(TObj* obj, TReturn(TObj::*func)()){
    ofxControlFuncEvent<void, TReturn, TObj> test(obj, func, 0);
    searchAndRemove(&test);
}

// add a new event listener for the end of the next segment(s), calling a member function by a single argument
template<typename TArg, typename TReturn, typename TObj>
void ofxBaseOsc::remove(TObj* obj, TReturn(TObj::*func)(TArg)){
    TArg arg{};
    ofxControlFuncEvent<TArg, TReturn, TObj> test(obj, func, arg, 0);
    searchAndRemove(&test);
}


/*-------------------------------------------------------------------------*/

/// ofxSawOsc / ofxPhasor

using ofxSawOsc = ofxBaseOsc;
using ofxPhasor = ofxBaseOsc;

/* These are the most basic oscillators because the 'value' equals the phase.
 * Therefore ofxSawOsc and ofxPhasor are identical with ofxBaseOsc. */

/*-------------------------------------------------------------------------*/

/// ofxSinOsc / ofxCosOsc

class ofxSinOsc : public ofxBaseOsc {
public:
    virtual float out() const;
};

class ofxCosOsc : public ofxBaseOsc {
public:
    virtual ~ofxCosOsc();
    virtual float out() const;
};

/*-------------------------------------------------------------------------*/

/// ofxPulseOsc

class ofxPulseOsc : public ofxBaseOsc {
public:
    ofxPulseOsc();
    virtual ~ofxPulseOsc();
    virtual void init();
    virtual float out() const;
    void setPulseWidth(float width);
    float getPulseWidth() const;
protected:
    float width;
};

/*--------------------------------------------------------------------------*/

/// ofxTriOsc

class ofxTriOsc : public ofxBaseOsc {
public:
    ofxTriOsc();
    virtual ~ofxTriOsc();
    virtual void init();
    virtual float out() const;
    void setVertex(float v);
    float getVertex() const;
private:
    float vertex;
};


/*--------------------------------------------------------------------------*/

/// ofxNoiseOsc

// ofxLineShape for noise shape
using ofxNoiseShape = ofxLineShape;

class ofxNoiseOsc : public ofxBaseOsc {
private:
    enum t_type {
        UNIFORM,
        NORMAL
    } type;
public:
    ofxNoiseOsc();
    virtual ~ofxNoiseOsc();
    virtual void init();
    virtual float out();
    void setUniform(float high = 1.f, float low = 0.f);
    void setNormal(float stddev = 1.f, float mean = 0.f);
    void setNoiseShape(ofxNoiseShape shape, float coeff);
    static void seed(int val);
private:
    ofxNoiseShape newShape, _shape;
    float newCoeff, _coeff;
    float a, b;
    static default_random_engine gen;
};

// setUniform(float high = 1.f, float low = 0.f)
// isUniform()
// setNormal(float stddev = 1.f, float mean = 0.f)
// isNormal()

// setNoiseShape() (only updated on new Period)

// return (max * rand() / float(RAND_MAX)) * (1.0f - std::numeric_limits<float>::epsilon());
// better use c++11 random library
// static generator and distribution objects? static seed method?


/*--------------------------------------------------------------------------*/

/// ofxMetro

/* almost the same as ofxBaseOsc */

class ofxMetro : public ofxBaseOsc {
public:
    // force(): will reset phase AND trigger events
    void forceNext();
};



/*--------------------------------------------------------------------------*/

/// ofxTimer

/* like a stopwatch */

class ofxTimer : ofxBaseControl {
public:
    ofxTimer();
    virtual ~ofxTimer();
    /* interface implementation */
    virtual void init();
    virtual void update();
    /* new functions */
    void reset();
    float getTime() const;
protected:
    float elapsed;
};


/*--------------------------------------------------------------------------*/

/// ofxControlBaseEvent
/// abstract base class for different types of control events

class ofxControlBaseEvent {
public:
    ofxControlBaseEvent(float _delay)
        : delay(_delay), elapsed(0) {}
	virtual ~ofxControlBaseEvent() {}
	virtual void onTimeOut() = 0;
	virtual bool compare(ofxControlBaseEvent * testObj) = 0;
    float delay;
    float elapsed;
};

/// clock event writing a value into a variable

template<typename T>
class ofxControlVarEvent : public ofxControlBaseEvent {
public:
    ofxControlVarEvent(T* _var, const T & _val, float _delay = 0.0)
        : ofxControlBaseEvent(_delay), var(_var), val(_val) {}
    virtual ~ofxControlVarEvent() {}
	virtual void onTimeOut(){
        if (var) {*var = val;}
	}
	virtual bool compare(ofxControlBaseEvent * testObj){
		// test if testObj is of same type
		if (auto * ptr = dynamic_cast<ofxControlVarEvent<T>*>(testObj)){
			// test if both point to the same variable
			return (var == ptr->var);
		} else {
			return false;
		}
	}	
protected:
	T* var;
	T val;	
};


/// clock event calling a member function by a single argument

template<typename TArg, typename TReturn, typename TObj>
class ofxControlFuncEvent : public ofxControlBaseEvent {
public:
    ofxControlFuncEvent(TObj* _obj, TReturn(TObj::*_func)(TArg), const TArg & _arg, float _delay = 0.0)
        : ofxControlBaseEvent(_delay), obj(_obj), func(_func), arg(_arg) {}
    virtual ~ofxControlFuncEvent() {}
	virtual void onTimeOut(){
        if (obj) {(obj->*func)(arg);}
	}
	virtual bool compare(ofxControlBaseEvent * testObj){
		// test if testObj is of same type
		if (auto * ptr = dynamic_cast<ofxControlFuncEvent<TArg, TReturn, TObj>*>(testObj)){
			// test if both point to the same object and member function
			return (obj == ptr->obj && func == ptr->func);
		} else {
			return false;
		}
	}	
protected:
    TObj* obj;
    TReturn(TObj::*func)(TArg);
	TArg arg;
};

// void specialization

template<typename TReturn, typename TObj>
class ofxControlFuncEvent<void, TReturn, TObj> : public ofxControlBaseEvent {
public:
    ofxControlFuncEvent(TObj* _obj, TReturn(TObj::*_func)(), float _delay = 0.0)
        : ofxControlBaseEvent(_delay), obj(_obj), func(_func) {}
    virtual ~ofxControlFuncEvent() {}
	virtual void onTimeOut(){
		if (obj) {(obj->*func)();}
	}
	virtual bool compare(ofxControlBaseEvent * testObj){
		// test if testObj is of same type
		if (auto * ptr = dynamic_cast<ofxControlFuncEvent<void, TReturn, TObj>*>(testObj)){
			// test if both point to the same object and member function
			return (obj == ptr->obj && func == ptr->func);
		} else {
			return false;
		}
	}	
protected:
    TObj* obj;
    TReturn(TObj::*func)();	
};



