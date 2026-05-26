#include "BlinkyController.h"
#include <cmath>
#include <vector>

/*============================================================================
MODIFICACIÓN PROPIA: ESTADO EXTRA "ENRIGED" (FRENESÍ AGRESIVO)
Si Blinky está muy cerca de Ms. Pacman, deja de seguirla por detrás y calcula 
una posición 2 nodos por delante de su dirección actual para interceptarla.
============================================================================
*/

static double calculateDistance(Position p1, Position p2) {
    return std::sqrt(std::pow(p1.x - p2.x, 2) + std::pow(p1.y - p2.y, 2));
}

static Move getBestMoveTowards(std::shared_ptr<Character> character, const GameState& game, int targetX, int targetY) {
    std::vector<Move> validMoves = game.getValidMoves(character);
    Move bestMove = PASS;
    double minDistance = 999999.0;
    
    for (Move move : validMoves) {
        Position nextPos = game.getNextPosition(character->getPosition(), move);
        double dist = std::sqrt(std::pow(nextPos.x - targetX, 2) + std::pow(nextPos.y - targetY, 2));
        if (dist < minDistance) {
            minDistance = dist;
            bestMove = move;
        }
    }
    return bestMove;
}


BlinkyScatterState::BlinkyScatterState(std::shared_ptr<Character> _character) : FSMState(_character), framesInState(0) {}
void BlinkyScatterState::onEnter(const GameState& gs) { framesInState = 0; }
Move BlinkyScatterState::onUpdate(const GameState& gs) {
    framesInState++;
    return getBestMoveTowards(character, gs, 25, 1); // Esquina superior derecha
}

BlinkyChaseState::BlinkyChaseState(std::shared_ptr<Character> _character) : FSMState(_character), framesInState(0) {}
void BlinkyChaseState::onEnter(const GameState& gs) { framesInState = 0; }
Move BlinkyChaseState::onUpdate(const GameState& gs) {
    framesInState++;
    Position pacmanPos = gs.getMsPacmanPosition();
    return getBestMoveTowards(character, gs, pacmanPos.x, pacmanPos.y);
}

BlinkyEnragedState::BlinkyEnragedState(std::shared_ptr<Character> _character) : FSMState(_character) {}
Move BlinkyEnragedState::onUpdate(const GameState& gs) {
    Position pacmanPos = gs.getMsPacmanPosition();
    Move pacmanDir = gs.getMsPacmanDirection();
    int tx = pacmanPos.x;
    int ty = pacmanPos.y;
    
    if (pacmanDir == UP) ty -= 2;
    else if (pacmanDir == DOWN) ty += 2;
    else if (pacmanDir == LEFT) tx -= 2;
    else if (pacmanDir == RIGHT) tx += 2;

    return getBestMoveTowards(character, gs, tx, ty);
}



ScatterToChaseTransition::ScatterToChaseTransition(std::shared_ptr<BlinkyScatterState> src) : srcState(src) {}
bool ScatterToChaseTransition::isValid(const GameState& gs) {
    return srcState->getFrames() >= 400;
}

ChaseToScatterTransition::ChaseToScatterTransition(std::shared_ptr<BlinkyChaseState> src) : srcState(src) {}
bool ChaseToScatterTransition::isValid(const GameState& gs) {
    return srcState->getFrames() >= 1200;
}

ChaseToEnragedTransition::ChaseToEnragedTransition(std::shared_ptr<Character> ghost) : ghostRef(ghost) {}
bool ChaseToEnragedTransition::isValid(const GameState& gs) {
    return calculateDistance(ghostRef->getPosition(), gs.getMsPacmanPosition()) < 5.0;
}

EnragedToChaseTransition::EnragedToChaseTransition(std::shared_ptr<Character> ghost) : ghostRef(ghost) {}
bool EnragedToChaseTransition::isValid(const GameState& gs) {
    return calculateDistance(ghostRef->getPosition(), gs.getMsPacmanPosition()) > 8.0;
}



BlinkyFSM::BlinkyFSM(std::shared_ptr<Character> _character) : FiniteStateMachine(_character) {
    // 1. Crear los estados
    auto scatter = std::make_shared<BlinkyScatterState>(character);
    auto chase = std::make_shared<BlinkyChaseState>(character);
    auto enraged = std::make_shared<BlinkyEnragedState>(character);

    states.push_back(scatter);
    states.push_back(chase);
    states.push_back(enraged);


    auto t1 = std::make_shared<ScatterToChaseTransition>(scatter);
    t1->setNextState(chase);
    scatter->addTransition(t1);

    auto t2 = std::make_shared<ChaseToScatterTransition>(chase);
    t2->setNextState(scatter);
    chase->addTransition(t2);

    auto t3 = std::make_shared<ChaseToEnragedTransition>(character);
    t3->setNextState(enraged);
    chase->addTransition(t3);

    auto t4 = std::make_shared<EnragedToChaseTransition>(character);
    t4->setNextState(chase);
    enraged->addTransition(t4);

  
    initialState = scatter;
    activeState = initialState;
    activeState->onEnter(GameState());
}

Move BlinkyFSM::update(const GameState& gs) {
    if (!activeState) return PASS;

  
    std::shared_ptr<FSMTransition> activeTrans = activeState->getActiveTransition(gs);
    if (activeTrans) {
        activeState->onExit(gs);
        activeTrans->onTransition(gs); 
        activeState = activeTrans->getNextState(); 
        activeState->onEnter(gs);
    }

    return activeState->onUpdate(gs);
}


BlinkyController::BlinkyController(std::shared_ptr<Character> character) : Controller(character) {
    myFSM = std::make_unique<BlinkyFSM>(character);
}

BlinkyController::~BlinkyController() {}

Move BlinkyController::getMove(const GameState& game) {
    return myFSM->update(game);
}