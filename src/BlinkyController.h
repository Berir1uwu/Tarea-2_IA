#pragma once

#include "Controller.h"
#include "FSM.h"
#include <memory>


class BlinkyScatterState : public FSMState {
private:
    int framesInState;
public:
    BlinkyScatterState(std::shared_ptr<Character> _character);
    void onEnter(const GameState& gs) override;
    Move onUpdate(const GameState& gs) override;
    int getFrames() const { return framesInState; }
};

class BlinkyChaseState : public FSMState {
private:
    int framesInState;
public:
    BlinkyChaseState(std::shared_ptr<Character> _character);
    void onEnter(const GameState& gs) override;
    Move onUpdate(const GameState& gs) override;
    int getFrames() const { return framesInState; }
};

class BlinkyEnragedState : public FSMState {
public:
    BlinkyEnragedState(std::shared_ptr<Character> _character);
    Move onUpdate(const GameState& gs) override;
};



class BlinkyTransitionBase : public FSMTransition {
protected:
    std::shared_ptr<FSMState> nextState;
public:
    void setNextState(std::shared_ptr<FSMState> target) { nextState = target; }
    std::shared_ptr<FSMState> getNextState() override { return nextState; }
};

class ScatterToChaseTransition : public BlinkyTransitionBase {
private:
    std::shared_ptr<BlinkyScatterState> srcState;
public:
    ScatterToChaseTransition(std::shared_ptr<BlinkyScatterState> src);
    bool isValid(const GameState& gs) override;
};

class ChaseToScatterTransition : public BlinkyTransitionBase {
private:
    std::shared_ptr<BlinkyChaseState> srcState;
public:
    ChaseToScatterTransition(std::shared_ptr<BlinkyChaseState> src);
    bool isValid(const GameState& gs) override;
};

class ChaseToEnragedTransition : public BlinkyTransitionBase {
private:
    std::shared_ptr<Character> ghostRef;
public:
    ChaseToEnragedTransition(std::shared_ptr<Character> ghost);
    bool isValid(const GameState& gs) override;
};

class EnragedToChaseTransition : public BlinkyTransitionBase {
private:
    std::shared_ptr<Character> ghostRef;
public:
    EnragedToChaseTransition(std::shared_ptr<Character> ghost);
    bool isValid(const GameState& gs) override;
};




class BlinkyFSM : public FiniteStateMachine {
public:
    BlinkyFSM(std::shared_ptr<Character> _character);
    Move update(const GameState& gs) override;
};



class BlinkyController : public Controller {
private:
    std::unique_ptr<BlinkyFSM> myFSM;

public:
    BlinkyController(std::shared_ptr<Character> character);
    virtual ~BlinkyController();
    Move getMove(const GameState& game) override;
};