#pragma once

#include "Controller.h"
#include "FSM.h"
#include <memory>


class SueScatterState : public FSMState {
private:
    int framesInState;
public:
    SueScatterState(std::shared_ptr<Character> _character);
    void onEnter(const GameState& gs) override;
    Move onUpdate(const GameState& gs) override;
    int getFrames() const { return framesInState; }
};

class SueChaseState : public FSMState {
public:
    SueChaseState(std::shared_ptr<Character> _character);
    Move onUpdate(const GameState& gs) override;
};

class SueAmbushState : public FSMState {
public:
    SueAmbushState(std::shared_ptr<Character> _character);
    Move onUpdate(const GameState& gs) override;
};



class SueTransitionBase : public FSMTransition {
protected:
    std::shared_ptr<FSMState> nextState;
public:
    void setNextState(std::shared_ptr<FSMState> target) { nextState = target; }
    std::shared_ptr<FSMState> getNextState() override { return nextState; }
};

class SueScatterToChaseTransition : public SueTransitionBase {
private:
    std::shared_ptr<SueScatterState> srcState;
public:
    SueScatterToChaseTransition(std::shared_ptr<SueScatterState> src);
    bool isValid(const GameState& gs) override;
};

class SueChaseToScatterTransition : public SueTransitionBase {
private:
    std::shared_ptr<Character> ghostRef;
public:
    SueChaseToScatterTransition(std::shared_ptr<Character> ghost);
    bool isValid(const GameState& gs) override; 
};

class SueChaseToAmbushTransition : public SueTransitionBase {
private:
    std::shared_ptr<Character> ghostRef;
public:
    SueChaseToAmbushTransition(std::shared_ptr<Character> ghost);
    bool isValid(const GameState& gs) override; 
};

class SueAmbushToChaseTransition : public SueTransitionBase {
private:
    std::shared_ptr<Character> ghostRef;
public:
    SueAmbushToChaseTransition(std::shared_ptr<Character> ghost);
    bool isValid(const GameState& gs) override;
};


class SueFSM : public FiniteStateMachine {
public:
    SueFSM(std::shared_ptr<Character> _character);
    Move update(const GameState& gs) override;
};


class SueController : public Controller {
private:
    std::unique_ptr<SueFSM> myFSM;

public:
    SueController(std::shared_ptr<Character> character);
    virtual ~SueController();
    Move getMove(const GameState& game) override;
};