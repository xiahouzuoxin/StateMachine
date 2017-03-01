
/**
 * Implemention Simple StateMachine by C++11
 */

#ifndef _STATEMACHINE_H
#define _STATEMACHINE_H

#include <vector>
#include <queue>
#include <stack>
#include <string>
#include <memory>
#include <functional>
#include <assert.h>

#define _HSM_LOG(msg)            printf("%s\n", msg.c_str())
#define HSM_LOG(msg)             printf("%s:%d	%s\n", __FILE__, __LINE__, msg.c_str())
#define HSM_ASSERT(expr, msg)    assert( (expr) && (msg) )

#define HSM_RESERVE_EVENT_QUEUE_FOR_NEXT_FRAME

class HSM_Event {
public:
	enum EventStatus {
		EVENT_STATUS_WAIT_RESPONSE,
		EVENT_STATUS_WAIT_IDLE
	};

	HSM_Event(const std::string& name = "Unkonw") : mName(name) {
		stat_id++; 
		event_id = stat_id;
		is_handled = EVENT_STATUS_WAIT_IDLE;
	}

	const int  getEventID() { return event_id; }
	const bool isEventIdle() { return (is_handled==EVENT_STATUS_WAIT_IDLE) ? true : false; }

	void changeEventStatus(const enum EventStatus to_status) { is_handled = to_status; }

private:
	std::string mName;
	int         event_id;          // current event unique ID
	static int  stat_id;    // stat how many event registered
	EventStatus is_handled;
};

class HSM_TransFunc {
public:
	HSM_TransFunc(std::function<bool(void)> func) : mFunction(func) {};
	virtual void run() { mFunction(); }

private:
	std::function<bool(void)> mFunction;
};

class HSM_Region {
public:
	HSM_Region(const std::string& name = "Unknow", HSM_Region* parent=nullptr) 
		: mName(name), mParentState(parent) {
		mInitialState = nullptr;
	}

	typedef struct _stStateTuple {
		_stStateTuple(HSM_Event* _event, HSM_Region* _state, HSM_TransFunc* _trans_effect)
			: _event(_event), _state(_state), _trans_effect(_trans_effect) {}
		HSM_Event* _event;
		HSM_Region* _state;
		HSM_TransFunc* _trans_effect;
	} _stStateTuple;

	void setInitialState(HSM_Region* init_state) {
		mInitialState = init_state;
		mCurState = init_state;
	}

	HSM_Region* getInitialState(void) {
		return mInitialState;
	}

	bool addTransitionTuple(HSM_Event* _event, HSM_Region* to_state, HSM_TransFunc* _func) {
		HSM_ASSERT(this != to_state, "Self state transition is need to define.");
		mTriggers.push_back(_stStateTuple(_event, to_state, _func));
		return true;
	}

	bool addTransitionTuple(HSM_Event* _event, HSM_Region* to_state, std::function<bool(void)> _func = [] { return true; }) {
		HSM_ASSERT(this != to_state, "Self state transition is need to define.");
		mTriggers.push_back(_stStateTuple(_event, to_state, new HSM_TransFunc(_func)));
		return true;
	}

	virtual void run(void) {
		while ( !mEventQueue.empty() ) {
			HSM_Event *prior_event = mEventQueue.front();
			mEventQueue.pop();
			prior_event->changeEventStatus(HSM_Event::EVENT_STATUS_WAIT_IDLE);

			// Recursive parent states
			HSM_Region* recursiveState = mCurState;
			std::stack<HSM_Region*> stackParents;
			while (recursiveState->mParentState) {
				stackParents.push(recursiveState->mParentState);
				recursiveState = recursiveState->mParentState;
			}
			while (!stackParents.empty()) {
				if ( handleTrigger(stackParents.top(), prior_event) ) {
					return;
				}
				stackParents.pop();
			}

			// Recursive current && child states
			recursiveState = mCurState;
			while (recursiveState != nullptr) {
				if ( handleTrigger(recursiveState, prior_event) ) {
					return;
				}
				recursiveState = recursiveState->getInitialState();
			}
		}
	}

	virtual bool enqueueEvent(HSM_Event* _event, const std::string& msg="") {
		if (_event->isEventIdle()) {
			mEventQueue.push(_event);
			_event->changeEventStatus(HSM_Event::EVENT_STATUS_WAIT_RESPONSE);
			if (!msg.empty()) {
				_HSM_LOG(msg);
			}
			return true;
		} else {
			HSM_ASSERT(0, "Event is currenttly in EventQueue and not handled.");
			return false;
		}	
	}

	virtual bool clearEventQuene(void) {
		// std::queue<HSM_Event *> empty;
		// mEventQueue.swap(empty);
		while (!mEventQueue.empty()) { mEventQueue.pop(); }
		return true;
	}

	virtual bool isInState(HSM_Region*) {
		return (this == mCurState) ? true : false;
	}

protected:
	const std::string& mName;

	// 
	HSM_Region* mInitialState;
	HSM_Region* mCurState;
	HSM_Region* const mParentState;

	std::queue<HSM_Event *> mEventQueue;
	std::vector<_stStateTuple> mTriggers;

	std::vector<_stStateTuple> getTriggers() { return mTriggers; }

	// return state that transition to
	HSM_Region* handleTrigger(HSM_Region* cur_state, HSM_Event *prior_event) {
		for (auto trigger : cur_state->getTriggers()) {
			if (trigger._event->getEventID() == prior_event->getEventID()) {
				cur_state = trigger._state;
				trigger._trans_effect->run();

#ifdef HSM_RESERVE_EVENT_QUEUE_FOR_NEXT_FRAME
#else           // clear queue for next frame trigger
				while (!mEventQueue.empty()) { mEventQueue.pop(); }
#endif
				mCurState = cur_state;

				return cur_state;
			}
		}
		return nullptr;
	}
};

// \Deprecated: HSM_State merged to HSM_Region
// class HSM_State {
// public:
// 	HSM_State(const std::string& name = "Unknow") : mName(name) {}
// 
// 	typedef struct _stStateTuple {
// 		_stStateTuple(HSM_Event* _event, HSM_State* _state, HSM_TransFunc* _trans_effect)
// 			: _event(_event), _state(_state), _trans_effect(_trans_effect) {}
// 		HSM_Event* _event;
// 		HSM_State* _state;
// 		HSM_TransFunc* _trans_effect;
// 	} _stStateTuple;
// 
// 	std::vector<_stStateTuple> getTriggers() { return mTriggers; }
// 
// 	bool addTransitionTuple(HSM_Event* _event, HSM_State* to_state, HSM_TransFunc* _func) {
// 		HSM_ASSERT(this != to_state, "Self state transition is need to define.");
// 		mTriggers.push_back(_stStateTuple(_event, to_state, _func));
// 		return true;
// 	}
// 
// 	bool addTransitionTuple(HSM_Event* _event, HSM_State* to_state, std::function<bool(void)> _func = [] { return true; }) {
// 		HSM_ASSERT(this != to_state, "Self state transition is need to define.");
// 		mTriggers.push_back( _stStateTuple(_event, to_state, new HSM_TransFunc(_func) ));
// 		return true;
// 	}
// 	
// private: 
// 	const std::string& mName;
// 	std::vector<_stStateTuple> mTriggers;
// };

#endif
