#pragma once

#include "Controller.h"
#include "BehaviorTree.h"
#include "BTGhostController.h" 

class PinkyController : public Controller {
private:
    std::shared_ptr<Composite> root;
public:
    PinkyController(std::shared_ptr<Character> character);
    virtual ~PinkyController();
    virtual Move getMove(const GameState& gs) override;
};

class PinkyPredictiveChase : public Behavior {
public:
    virtual Status update() override;
};


class PacmanTrapped : public Behavior {
public:
    virtual Status update() override;
};


class PinkyDirectChase : public Behavior {
public:
    virtual Status update() override;
};