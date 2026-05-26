#include "SueController.h"
#include <cmath>
#include <vector>


// ============================================================================
// MODIFICACIÓN PROPIA: ACCIÓN/ESTADO EXTRA "SUE_AMBUSH"
// En lugar de huir directamente a su esquina (Scatter) cuando tiene pánico, Sue 
// se posiciona de manera táctica en una zona de intercepción para emboscar.
// ============================================================================
static float calculateSueDistance(std::pair<float, float> p1, std::pair<float, float> p2) {
    return std::sqrt(std::pow(p1.first - p2.first, 2) + std::pow(p1.second - p2.second, 2));
}

static Move getSueBestMoveTowards(std::shared_ptr<Character> character, const GameState& game, std::pair<float, float> target) {
    Move minMove = PASS;
    std::vector<Move> moves;
    
    if (character->getDirection() == PASS) {
        moves = game.getMaze().getPossibleMoves(character->getPos());
    } else {
        moves = game.getMaze().getGhostLegalMoves(character->getPos(), character->getDirection());
    }

    float min = 100000000;
    for (auto move : moves) {
        if (move == PASS) break;
        float dist = euclid2(target, game.getMaze().getNodePos(game.getMaze().getNeighbour(character->getPos(), move)));
        if (dist < min) {
            min = dist;
            minMove = move;
        }
    }
    return minMove;
}



SueScatterState::SueScatterState(std::shared_ptr<Character> _character) : FSMState(_character), framesInState(0) {}
void SueScatterState::onEnter(const GameState& gs) { framesInState = 0; }
Move SueScatterState::onUpdate(const GameState& gs) {
    framesInState++;

    auto target = gs.getMaze().getPowerPillPositions()[0]; 
    return getSueBestMoveTowards(character, gs, target);
}

SueChaseState::SueChaseState(std::shared_ptr<Character> _character) : FSMState(_character) {}
Move SueChaseState::onUpdate(const GameState& gs) {

    auto target = gs.getMaze().getNodePos(gs.getPacmanPos());
    return getSueBestMoveTowards(character, gs, target);
}

SueAmbushState::SueAmbushState(std::shared_ptr<Character> _character) : FSMState(_character) {}
Move SueAmbushState::onUpdate(const GameState& gs) {
    // Apunta a un nodo intermedio del mapa para intentar bloquear el paso
    auto pacmanNode = gs.getMaze().getNodePos(gs.getPacmanPos());
    std::pair<float, float> ambushTarget = std::make_pair(pacmanNode.first, pacmanNode.second + 40.0f); 
    return getSueBestMoveTowards(character, gs, ambushTarget);
}



SueScatterToChaseTransition::SueScatterToChaseTransition(std::shared_ptr<SueScatterState> src) : srcState(src) {}
bool SueScatterToChaseTransition::isValid(const GameState& gs) {
    return srcState->getFrames() >= 300;
}

SueChaseToScatterTransition::SueChaseToScatterTransition(std::shared_ptr<Character> ghost) : ghostRef(ghost) {}
bool SueChaseToScatterTransition::isValid(const GameState& gs) {
    auto ghostPos = gs.getMaze().getNodePos(ghostRef->getPos());
    auto pacmanPos = gs.getMaze().getNodePos(gs.getPacmanPos());

    return calculateSueDistance(ghostPos, pacmanPos) < 150.0f;
}

SueChaseToAmbushTransition::SueChaseToAmbushTransition(std::shared_ptr<Character> ghost) : ghostRef(ghost) {}
bool SueChaseToAmbushTransition::isValid(const GameState& gs) {

    auto pacmanPos = gs.getMaze().getNodePos(gs.getPacmanPos());
    return (pacmanPos.second > 200.0f); 
}

SueAmbushToChaseTransition::SueAmbushToChaseTransition(std::shared_ptr<Character> ghost) : ghostRef(ghost) {}
bool SueAmbushToChaseTransition::isValid(const GameState& gs) {
    auto ghostPos = gs.getMaze().getNodePos(ghostRef->getPos());
    auto pacmanPos = gs.getMaze().getNodePos(gs.getPacmanPos());
    return calculateSueDistance(ghostPos, pacmanPos) > 400.0f;
}


SueFSM::SueFSM(std::shared_ptr<Character> _character) : FiniteStateMachine(_character) {
    auto scatter = std::make_shared<SueScatterState>(character);
    auto chase = std::make_shared<SueChaseState>(character);
    auto ambush = std::make_shared<SueAmbushState>(character);

    states.push_back(scatter);
    states.push_back(chase);
    states.push_back(ambush);

    auto t1 = std::make_shared<SueScatterToChaseTransition>(scatter);
    t1->setNextState(chase);
    scatter->addTransition(t1);

    auto t2 = std::make_shared<SueChaseToScatterTransition>(character);
    t2->setNextState(scatter);
    chase->addTransition(t2);

    auto t3 = std::make_shared<SueChaseToAmbushTransition>(character);
    t3->setNextState(ambush);
    chase->addTransition(t3);

    auto t4 = std::make_shared<SueAmbushToChaseTransition>(character);
    t4->setNextState(chase);
    ambush->addTransition(t4);

    initialState = scatter;
    activeState = initialState;
    activeState->onEnter(GameState());
}

Move SueFSM::update(const GameState& gs) {
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

SueController::SueController(std::shared_ptr<Character> character) : Controller(character) {
    myFSM = std::make_unique<SueFSM>(character);
}

SueController::~SueController() {}

Move SueController::getMove(const GameState& game) {
    return myFSM->update(game);
}