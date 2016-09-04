#include "ofxControl.h"

/// ofxControl

void ofxControl::setFrameRate(float fps) {
    _fps = (fps > 0) ? fps : OFXCONTROL_DEFAULT_RATE;
}
float ofxControl::getFrameRate() {
    return _fps;
}
float ofxControl::_fps = OFXCONTROL_DEFAULT_RATE;

/*---------------------------------------------------------------*/

/// ofxBaseControl

ofxBaseControl::ofxBaseControl(){
    init();
}

ofxBaseControl::~ofxBaseControl(){
}

void ofxBaseControl::init(){
    speed = 1.f;
    bRunning = true;
}


void ofxBaseControl::setSpeed(float newSpeed){
    speed = (newSpeed >= 0.f) ? newSpeed : 1.f;
}

float ofxBaseControl::getSpeed() const {
    return speed;
}

void ofxBaseControl::pause(){
    bRunning = false;
}

void ofxBaseControl::resume(){
    bRunning = true;
}

bool ofxBaseControl::isRunning() const {
    return bRunning;
}

/*--------------------------------------------------------------------*/

/// ofxLine

ofxLine::ofxLine() {
    init();
}

ofxLine::~ofxLine() {}

/// interface implementation
void ofxLine::init(){
    ofxBaseControl::init();
    value = 0;
    segmentList.clear();
    eventList.clear();
    shape = ofxLineShape::LIN;
    coeff = 0;
}

void ofxLine::update(){
    if (!segmentList.empty() && bRunning){
        auto segment = segmentList.begin();
        float ramp = (segment->elapsed - segment->onset) / segment->time;
        // check if elapsed time has exceeded onset
        if (ramp > 0.f){
            // check if ramp time is over
            if (ramp > 1.0){
                value = segment->target; // force target value
                // notify event listeners
                for (auto& event : segment->eventList){
                    event->onTimeOut();
                }
                // pop segment
                if (segmentList.empty()){
                   cout << "Ooops: a callback function already cleared the segment!\n";
                } else {
                    segmentList.pop_front();
                }
                // update the next one (if there is any)
                if (!segmentList.empty()){
                    segmentList.begin()->start = value;
                }
                // old segment is now invalid
                segment = segmentList.end();
            } else {
            // calculate the current value based on ramp position and segment shape
                float mult;
                switch (segment->shape){
                case ofxLineShape::STEP:
                    mult = 0.0;
                    break;
                case ofxLineShape::LIN:
                    mult = ramp;
                    break;
                case ofxLineShape::FAST_EXP:
                    mult = (segment->coeff <= 0.f) ? ramp
                        : (exp(ramp * segment->coeff * -1.0) - 1.0) / (exp(segment->coeff * -1.0) - 1.0);
                    break;
                case ofxLineShape::FAST_POW:
                    mult = pow(ramp, 1.0/pow(2.0, segment->coeff));
                    break;
                case ofxLineShape::FAST_COS:
                    mult = sin(ramp * HALF_PI);
                    break;
                case ofxLineShape::SLOW_EXP:
                    mult = (segment->coeff <= 0.f) ? ramp
                        : (exp(ramp * segment->coeff) - 1.0) / (expf(segment->coeff) - 1.0);
                    break;
                case ofxLineShape::SLOW_POW:
                    mult = pow(ramp, pow(2.0, segment->coeff));
                    break;
                case ofxLineShape::SLOW_COS:
                    mult = cos(ramp * HALF_PI) * -1.0 + 1.0;
                    break;
                case ofxLineShape::S_CURVE:
                    mult = cos(ramp * PI) * -0.5 + 0.5;
                    break;
                default:
                    mult = ramp;
                    break;
                }
                float diff = segment->target - segment->start;
				value = segment->start + diff * mult;
            }
        }
        if (segment != segmentList.end()) {
            // increment elapsed time
            segment->elapsed += (float) speed / ofxControl::getFrameRate();
        }
    }
}


// get the current value
float ofxLine::out() const {
    return value;
}

// clear all line segments and set value immediatly
void ofxLine::setValue(float newValue){
    segmentList.clear();
    value = newValue;
}

// set the shape for following segment(s) to add
void ofxLine::setShape(ofxLineShape newShape, float newCoeff) {
    shape = newShape;
    coeff = (newCoeff >= 0.f) ? newCoeff : 0.f;
}

// clear event list
void ofxLine::clearOnSegmentEnd(){
    eventList.clear();
}

// add a new segment, specifing the target value, the ramp time
// and a time onset in relation to the end of the last segment
void ofxLine::addSegment(float targetValue, float rampTime, float timeOnset){
	ofxLineSegment segment;
	// initialize segment
    segment.time = (rampTime >= 0.0) ? rampTime : 0.0;
    segment.onset = (timeOnset >= 0.0) ? timeOnset : 0.0;
    segment.start = value; // will be probably overwritten later (if it's not the first segment added)
    segment.target = targetValue;
	segment.shape = shape;
	segment.coeff = coeff;
    segment.elapsed = 0.0;
    segment.eventList = std::move(eventList); // eventList is now empty
    eventList.clear(); // play it safe
	// finally add to segment list
    segmentList.push_back(std::move(segment));
}

// pop the last segment from the list
void ofxLine::removeLastSegment() {
    if (!segmentList.empty()){
        segmentList.pop_back();
        // if we have only one remaining segment, it will be the current one, so we have to initialize the start value
        if (segmentList.size() == 1){
            segmentList.begin()->start = value;
        }
    }
}

// drop current segment and move to the next
void ofxLine::nextSegment(){
    if (!segmentList.empty()){
        segmentList.pop_front();
        if (!segmentList.empty()){
            segmentList.begin()->start = value;
        }
    }
}

// clear all segments
void ofxLine::clear() {
    segmentList.clear();
    eventList.clear();
}



/*-------------------------------------------------------------------*/

/// ofxMultiLine

ofxMultiLine::ofxMultiLine() {
    init();
}

ofxMultiLine::ofxMultiLine(int numLines) {
    init();
	numLines = std::max(1, numLines);
    valueVec.resize(numLines, 0);
}

ofxMultiLine::~ofxMultiLine() {}

/// interface implementation
void ofxMultiLine::init(){
    ofxBaseControl::init();
    valueVec = {0};
    multiSegmentList.clear();
    eventList.clear();
    shape = ofxLineShape::LIN;
    coeff = 0;
}

void ofxMultiLine::update(){
    if (!multiSegmentList.empty() && bRunning){
        auto segment = multiSegmentList.begin();
        float ramp = (segment->elapsed - segment->onset) / segment->time;
        // check if elapsed time has exceeded onset
        if (ramp > 0.f){
            // check if ramp time is over
            if (ramp > 1.0){
                valueVec = std::move(segment->target); // force target value
                // notify event listeners
                for (auto& event : segment->eventList){
                    event->onTimeOut();
                }
                // pop segment
                if (multiSegmentList.empty()){
                   cout << "Ooops: a callback function already cleared the segment!\n";
                } else {
                    multiSegmentList.pop_front();
                }
                // update the next one (if there is any)
                if (!multiSegmentList.empty()){
                    multiSegmentList.begin()->start = valueVec;
                }
                // old segment is now invalid
                segment = multiSegmentList.end();
            } else {
            // calculate the current value based on ramp position and segment shape
                float mult;
                switch (segment->shape){
                case ofxLineShape::STEP:
                    mult = 0.0;
                    break;
                case ofxLineShape::LIN:
                    mult = ramp;
                    break;
                case ofxLineShape::FAST_EXP:
                    mult = (segment->coeff <= 0.f) ? ramp
                        : (exp(ramp * segment->coeff * -1.0) - 1.0) / (exp(segment->coeff * -1.0) - 1.0);
                    break;
                case ofxLineShape::FAST_POW:
                    mult = pow(ramp, 1.0/pow(2.0, segment->coeff));
                    break;
                case ofxLineShape::FAST_COS:
                    mult = sin(ramp * HALF_PI);
                    break;
                case ofxLineShape::SLOW_EXP:
                    mult = (segment->coeff <= 0.f) ? ramp
                        : (exp(ramp * segment->coeff) - 1.0) / (expf(segment->coeff) - 1.0);
                    break;
                case ofxLineShape::SLOW_POW:
                    mult = pow(ramp, pow(2.0, segment->coeff));
                    break;
                case ofxLineShape::SLOW_COS:
                    mult = cos(ramp * HALF_PI) * -1.0 + 1.0;
                    break;
                case ofxLineShape::S_CURVE:
                    mult = cos(ramp * PI) * -0.5 + 0.5;
                    break;
                default:
                    mult = ramp;
                    break;
                }
                for (int i = 0; i < valueVec.size(); ++i){
					float diff = segment->target[i] - segment->start[i];
                    valueVec[i] = segment->start[i] + diff * mult;
				}
            }
        }
        if (segment != multiSegmentList.end()) {
            // increment elapsed time
            segment->elapsed += (float) speed / ofxControl::getFrameRate();
        }
    }
}

// set number of lines
void ofxMultiLine::setNumLines(int numLines){
	numLines = std::max(1, numLines);
    valueVec.resize(numLines, 0);
	// resize stored segments
    for (auto& segment : multiSegmentList){
        segment.start.resize(numLines, 0);
        segment.target.resize(numLines, 0);
    }
}

int ofxMultiLine::getNumLines() const{
    return valueVec.size();
}

// get the current value at a certain index
float ofxMultiLine::operator[](int index) const{
    index = std::max(0, std::min((int)valueVec.size()-1, index));
    return valueVec[index];
}


// get the current values
vector<float> ofxMultiLine::out() const {
    return valueVec;
}

// clear all line segments and set values immediatly
void ofxMultiLine::setValues(const vector<float>& newValues){
    multiSegmentList.clear();
    valueVec = newValues;
}

void ofxMultiLine::setValues(float newValue){
    multiSegmentList.clear();
    valueVec.assign(valueVec.size(), newValue);
}

// add a new segment, specifing the target value, the ramp time
// and a time onset in relation to the end of the last segment
void ofxMultiLine::addSegment(const vector<float> & targetValues, float rampTime, float timeOnset){
	// make new segment
    ofxMultiLineSegment segment;
    segment.time = (rampTime >= 0.0) ? rampTime : 0.0;
    segment.onset = (timeOnset >= 0.0) ? timeOnset : 0.0;
    segment.start = valueVec; // will probably be overwritten later (if it's not the first segment added)
    segment.target = targetValues;
    segment.target.resize(valueVec.size(), 0); // make sure that target vector has the right dimension
	segment.shape = shape;
	segment.coeff = coeff;
    segment.elapsed = 0.0;
    segment.eventList = std::move(eventList); // eventList is now empty
    eventList.clear(); // play it safe
	// finally add to segment list
    multiSegmentList.push_back(std::move(segment));
}

// pop the last segment from the list
void ofxMultiLine::removeLastSegment() {
    if (!multiSegmentList.empty()){
        multiSegmentList.pop_back();
        // if we have only one remaining segment, it will be the current one, so we have to update the start value
        if (multiSegmentList.size() == 1){
            multiSegmentList.begin()->start = valueVec;
        }
    }
}

// drop current segment and move to the next
void ofxMultiLine::nextSegment(){
    if (!multiSegmentList.empty()){
        multiSegmentList.pop_front();
        if (!multiSegmentList.empty()){
			// update the start values for the now current segment
            multiSegmentList.begin()->start = valueVec;
        }
    }
}

// clear all segments
void ofxMultiLine::clear() {
    multiSegmentList.clear();
    eventList.clear();
}



/*-------------------------------------------------------------------*/


/// ofxClock


ofxClock::ofxClock(){
    init();
}

ofxClock::~ofxClock() {
}

void ofxClock::init(){
    ofxBaseControl::init();
    clockList.clear();
}
// walk through the clock list and check for timeouts
void ofxClock::update(){
    if (bRunning){
        auto clock = clockList.begin();
        while (clock != clockList.end()){
            // increment elapsed time
            (*clock)->elapsed += (float) speed / ofxControl::getFrameRate();
            if ((*clock)->elapsed > (*clock)->delay){
                (*clock)->onTimeOut();
                clock = clockList.erase(clock);
            } else {
                ++clock;
            }
        }
    }
}

/* template definitions for 'add' and 'cancel' functions are in header file */

// cancle the first added clock
void ofxClock::cancelFirst(){
	if (!clockList.empty()) {clockList.pop_front();}
}
// cancle the last added clock
void ofxClock::cancelLast(){
	if (!clockList.empty()) {clockList.pop_back();}
}
// delete all clocks
void ofxClock::clear(){
	if (!clockList.empty()) {clockList.clear();}
}

void ofxClock::searchAndRemove(ofxControlBaseEvent* testobj){
    auto clock = clockList.begin();
    while (clock != clockList.end()){
        if ((*clock)->compare(testobj)){
            clock = clockList.erase(clock);
        } else {
            ++clock;
        }
    }
}


/*-------------------------------------------------------------------------*/

/// ofxBaseOsc
/* common base class for all oscillators */

ofxBaseOsc::ofxBaseOsc(){
    init();
}

ofxBaseOsc::~ofxBaseOsc(){
}

void ofxBaseOsc::init(){
    ofxBaseControl::init();
    freq = 1.f;
    wrapped = 0.0;
    phase = 0.0;
    offset = 0.0;
    counter = 0;
    bReset = true;
    eventList.clear();
}

void ofxBaseOsc::update(){
    if (bRunning){
        float old = wrapped;
        if (offset != 0.0){
            wrapped = fmod(phase + offset, 1.0);
            if (wrapped < 0.0){
                wrapped += 1.0;
            }
        } else {
            wrapped = phase;
        }

        if (!bReset){
            if ((freq > 0.0 && (wrapped - old) <= 0.0) ||
                (freq < 0.0 && (old - wrapped) <= 0.0)){
                for (auto & event : eventList){
                    event->onTimeOut();
                }
                ++counter;
            }
        }

        bReset = false;
        phase += speed * freq / ofxControl::getFrameRate();
        phase = fmod(phase + numeric_limits<float>::epsilon(), 1.0); // add a very little offset to compensate for precision errors.
        if (phase < 0.0){
            phase += 1.0;
        }
    }
}

float ofxBaseOsc::out() const {
    // value equals wrapped phase
    return wrapped;
}


void ofxBaseOsc::setFrequency(float hz){
    freq = hz;
}

float ofxBaseOsc::getFrequency() const {
    return freq;
}

void ofxBaseOsc::setPeriod(float seconds){
    freq = 1.f/seconds;
}

float ofxBaseOsc::getPeriod() const {
    return 1.f/freq;
}

// will *not* trigger events
void ofxBaseOsc::setPhase(float newPhase){
    phase = newPhase;
    bReset = true;
}

float ofxBaseOsc::getPhase() const{
    return wrapped; // return wrapped phase
}

void ofxBaseOsc::setPhaseOffset(float newOffset){
    offset = newOffset;
}

float ofxBaseOsc::getPhaseOffset() const{
    return offset;
}

void ofxBaseOsc::removeAll(){
    eventList.clear();
}

int ofxBaseOsc::getCounter() const {
    return counter;
}

void ofxBaseOsc::resetCounter(){
    counter = 0;
}


// protected function to remove listeners from event list
void ofxBaseOsc::searchAndRemove(ofxControlBaseEvent* testobj){
    auto event = eventList.begin();
    while (event != eventList.end()){
        if ((*event)->compare(testobj)){
            event = eventList.erase(event);
        } else {
            ++event;
        }
    }
}



/*-------------------------------------------------------------------------*/

/// ofxSawOsc / ofxPhasor


/* These are the most basic oscillators because the 'value' equals the phase.
 * Therefore ofxSawOsc and ofxPhasor are identical with ofxBaseOsc. */

/*-------------------------------------------------------------------------*/

/// ofxSinOsc / ofxCosOsc

float ofxSinOsc::out() const {
    return sin(wrapped*TWO_PI);
}

float ofxCosOsc::out() const {
    return cos(wrapped*TWO_PI);
}


/*-------------------------------------------------------------------------*/

/// ofxPulseOsc

ofxPulseOsc::ofxPulseOsc() {
    init();
}

ofxPulseOsc::~ofxPulseOsc() {}

void ofxPulseOsc::init() {
    width = 0.5f;
    ofxBaseOsc::init();
}

float ofxPulseOsc::out() const {
    return (wrapped < width);
}

void ofxPulseOsc::setPulseWidth(float w){
    width = std::max(0.f, std::min(1.f, w));
}

float ofxPulseOsc::getPulseWidth() const {
    return width;
}

/*--------------------------------------------------------------------------*/

/// ofxTriOsc

ofxTriOsc::ofxTriOsc() {
    init();
}


ofxTriOsc::~ofxTriOsc() {}

void ofxTriOsc::init() {
    vertex = 0.5f;
    ofxBaseOsc::init();
}

float ofxTriOsc::out() const {
    if (vertex == 0.f){
        // actually reversed sawtooth
        return 1.f - wrapped;
    }
    else if (vertex == 1.f){
        // actually sawtooth
        return wrapped;
    }
    else {
        float x = wrapped - vertex;
        x = (x < 0.f) ? x / (-vertex) : x/(1-vertex);
        x *= -1;
        x += 1;
        return x;
    }
}
void ofxTriOsc::setVertex(float v){
    vertex = std::max(0.f, std::min(1.f, v));
}

float ofxTriOsc::getVertex() const {
    return vertex;
}


/*--------------------------------------------------------------------------*/

/// ofxNoiseOsc

// ofxLineShape for noise shape


/*--------------------------------------------------------------------------*/

/// ofxMetro

/* almost the same as ofxBaseOsc */

// force(): will reset phase AND trigger events
void ofxMetro::forceNext(){
    phase = 0.f;
    bReset = false;
}


/*---------------------------------------------------------------------------*/

/// ofxTimer

ofxTimer::ofxTimer() {
    init();
}

ofxTimer::~ofxTimer() {
}

void ofxTimer::init(){
    bRunning = true;
    speed = 1.f;
    elapsed = 0.0;
}

void ofxTimer::update(){
   if (bRunning){
        elapsed += speed / ofxControl::getFrameRate();
    }
}

void ofxTimer::reset(){
    elapsed = 0.0;
}

float ofxTimer::getTime() const {
    return elapsed;
}
