#include "InkyController.h"
#include <vector>

/*============================================================================
MODIFICACIÓN PROPIA (BT): CORTAR ESQUINAS
Si Blinky se queda muy lejos de Pacman y la táctica cooperativa de sándwich 
falla, Inky se vuelve autónomo e intercepta la esquina con Powerpill más cercana.
============================================================================
*/
InkyController::InkyController(std::shared_ptr<Character> character) : Controller(character), root(std::make_shared<Selector>()) {
   
    auto filterPowerpill = std::make_shared<Filter>();
    filterPowerpill->addCondition(std::make_shared<Powerpill>());
    filterPowerpill->addAction(std::make_shared<Frightened>());
    root->addChild(filterPowerpill);

    
    auto filterSolitario = std::make_shared<Filter>();
    filterSolitario->addCondition(std::make_shared<BlinkyFarFromPacman>());
    filterSolitario->addAction(std::make_shared<InkyCornerCut>());
    root->addChild(filterSolitario);

    root->addChild(std::make_shared<InkyCoordinatedChase>());
}

InkyController::~InkyController() {}

Move InkyController::getMove(const GameState& gs) {
    Info::getInfo()->in_character = character;
    Info::getInfo()->in_gamestate = &gs;
    root->tick();
    return Info::getInfo()->out_move;
}

Status BlinkyFarFromPacman::update() {
    auto gs = Info::getInfo()->in_gamestate;
    auto pacmanNodePos = gs->getMaze().getNodePos(gs->getPacmanPos());
    
   
    auto referencePos = gs->getMaze().getPowerPillPositions()[0]; 
    float distance = euclid2(pacmanNodePos, referencePos);
    
  
    if (distance > 500.0f) {
        return BH_SUCCESS;
    }
    return BH_FAILURE;
}


Status InkyCornerCut::update() {
    auto character = Info::getInfo()->in_character;
    auto gs = Info::getInfo()->in_gamestate;
  
    auto target = gs->getMaze().getPowerPillPositions()[0]; 
    
    float min = 1000000000;
    Move minMove = PASS;
    
    std::vector<Move> moves = (character->getDirection() == PASS) ? 
        gs->getMaze().getPossibleMoves(character->getPos()) : 
        gs->getMaze().getGhostLegalMoves(character->getPos(), character->getDirection());

    for (auto move : moves) {
        if (move == PASS) break;
        float dist = euclid2(target, gs->getMaze().getNodePos(gs->getMaze().getNeighbour(character->getPos(), move)));
        if (dist < min) {
            min = dist;
            minMove = move;
        }
    }
    Info::getInfo()->out_move = minMove;
    return BH_SUCCESS;
}


Status InkyCoordinatedChase::update() {
    auto character = Info::getInfo()->in_character;
    auto gs = Info::getInfo()->in_gamestate;
    
    auto pacmanNode = gs->getMaze().getNodePos(gs->getPacmanPos());
    std::pair<float, float> target = std::make_pair(pacmanNode.first - 2.0f, pacmanNode.second - 2.0f);
    
    float min = 1000000000;
    Move minMove = PASS;
    
    std::vector<Move> moves = (character->getDirection() == PASS) ? 
        gs->getMaze().getPossibleMoves(character->getPos()) : 
        gs->getMaze().getGhostLegalMoves(character->getPos(), character->getDirection());

    for (auto move : moves) {
        if (move == PASS) break;
        float dist = euclid2(target, gs->getMaze().getNodePos(gs->getMaze().getNeighbour(character->getPos(), move)));
        if (dist < min) {
            min = dist;
            minMove = move;
        }
    }
    Info::getInfo()->out_move = minMove;
    return BH_SUCCESS;
}