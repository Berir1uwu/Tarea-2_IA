#include "PacmanController.h"
#include <SDL2/SDL.h>
#include <vector>
#include <cmath>

// Función matemática auxiliar global para medir distancias euclidianas en la grilla
static float pacmanEuclid(std::pair<float, float> p1, std::pair<float, float> p2) {
    return std::sqrt(std::pow(p1.first - p2.first, 2) + std::pow(p1.second - p2.second, 2));
}

// Función auxiliar para obtener la dirección opuesta y evitar oscilaciones de ida y vuelta
static Move getOppositeMove(Move m) {
    if (m == UP) return DOWN;
    if (m == DOWN) return UP;
    if (m == LEFT) return RIGHT;
    if (m == RIGHT) return LEFT;
    return PASS;
}

PacmanController::PacmanController(std::shared_ptr<Character> character):
    Controller(character), root(std::make_shared<Selector>()) {
    
    // 1. RAMA DE ESCAPE: Si hay un fantasma peligroso cerca -> Huir de él
    auto filterEscape = std::make_shared<Filter>();
    filterEscape->addCondition(std::make_shared<IsGhostNear>());
    filterEscape->addAction(std::make_shared<EscapeFromGhost>());
    root->addChild(filterEscape);

    // 2. RAMA DE CAZA: Si los fantasmas están asustados -> Ir a buscarlos
    auto filterHunt = std::make_shared<Filter>();
    filterHunt->addCondition(std::make_shared<IsGhostEdible>());
    filterHunt->addAction(std::make_shared<HuntGhost>());
    root->addChild(filterHunt);

    // 3. RAMA POR DEFECTO COMPORTAMIENTO BASE: Si todo está despejado -> Recolectar pastillas de forma óptima
    root->addChild(std::make_shared<CollectPills>());
}

PacmanController::~PacmanController() {
}

Move PacmanController::getMove(const GameState& game){

    // ============================================================================
    // CÓDIGO BASE DEL ALUMNO: Control y cierre de ventana SDL (MANTENIDO INTACTO)
    // ============================================================================
    SDL_Event e;
    if( SDL_PollEvent( &e ) != 0 )
    {
        if( e.type == SDL_QUIT || 
            (e.type == SDL_KEYDOWN && 
                (e.key.keysym.sym==SDLK_ESCAPE || 
                e.key.keysym.sym==SDLK_q) ))
        {
            SDL_Quit();
            exit(0);
        }
    }
    
    // ============================================================================
    // CONFIGURACIÓN DE CONTEXTO GLOBAL (INFO SINGLETON)
    // ============================================================================
    Info::getInfo()->in_character = character;
    Info::getInfo()->in_gamestate = &game;
    
    // Ejecuta recursivamente el árbol de comportamiento en base a prioridades
    root->tick();

    // Retorna el movimiento seguro calculado por la acción activa del BT
    return Info::getInfo()->out_move;
}


// ============================================================================
// IMPLEMENTACIÓN DE LOS NODOS DEL ÁRBOL DE COMPORTAMIENTO (BT)
// ============================================================================

// CONDICIÓN: Evalúa si un fantasma no-comestible está acechando cerca de Ms. Pacman
Status IsGhostNear::update() {
    auto gs = Info::getInfo()->in_gamestate;
    auto pacmanPos = gs->getMaze().getNodePos(gs->getPacmanPos());
    std::vector<std::string> ghosts = {"Blinky", "Pinky", "Inky", "Sue"};

    // Si la píldora de poder está activa, los fantasmas no representan una amenaza directa
    if (gs->isPowerPelletActive()) {
        return BH_FAILURE;
    }

    for (const auto& name : ghosts) {
        try {
            auto ghostPos = gs->getMaze().getNodePos(gs->getGhostPosition(name));
            // Radio de alerta preventiva (100 unidades en la grilla del mapa)
            if (pacmanEuclid(pacmanPos, ghostPos) < 100.0f) { 
                return BH_SUCCESS; 
            }
        } catch(...) {}
    }
    return BH_FAILURE;
}

// ACCIÓN: Elige la ruta legal que maximice la distancia respecto al agresor más próximo
Status EscapeFromGhost::update() {
    auto character = Info::getInfo()->in_character;
    auto gs = Info::getInfo()->in_gamestate;
    auto pacmanPos = gs->getMaze().getNodePos(character->getPos());
    
    std::vector<Move> moves = gs->getMaze().getPossibleMoves(character->getPos());
    Move bestMove = PASS;
    float maxDist = -1.0f;

    std::pair<float, float> dangerGhostPos = std::make_pair(0.0f, 0.0f);
    std::vector<std::string> ghosts = {"Blinky", "Pinky", "Inky", "Sue"};
    
    // Localizar las coordenadas exactas del enemigo que activó la condición de proximidad
    for (const auto& name : ghosts) {
        try {
            auto gPos = gs->getMaze().getNodePos(gs->getGhostPosition(name));
            if (pacmanEuclid(pacmanPos, gPos) < 100.0f) {
                dangerGhostPos = gPos;
                break;
            }
        } catch(...) {}
    }

    for (auto move : moves) {
        if (move == PASS) break;
        auto nextPos = gs->getMaze().getNodePos(gs->getMaze().getNeighbour(character->getPos(), move));
        float dist = pacmanEuclid(nextPos, dangerGhostPos);
        if (dist > maxDist) {
            maxDist = dist;
            bestMove = move;
        }
    }

    Info::getInfo()->out_move = (bestMove != PASS) ? bestMove : moves[0];
    return BH_SUCCESS;
}

// CONDICIÓN: Determina si el estado global permite devorar fantasmas
Status IsGhostEdible::update() {
    auto gs = Info::getInfo()->in_gamestate;
    if (gs->isPowerPelletActive()) {
        return BH_SUCCESS;
    }
    return BH_FAILURE;
}

// ACCIÓN: Localiza al fantasma asustado más desprotegido y acorta distancia euclidiana hacia él
Status HuntGhost::update() {
    auto character = Info::getInfo()->in_character;
    auto gs = Info::getInfo()->in_gamestate;
    auto pacmanPos = gs->getMaze().getNodePos(character->getPos());
    
    std::vector<Move> moves = gs->getMaze().getPossibleMoves(character->getPos());
    Move bestMove = PASS;
    float minDist = 999999.0f;

    std::pair<float, float> targetGhost = std::make_pair(0.0f, 0.0f);
    std::vector<std::string> ghosts = {"Blinky", "Pinky", "Inky", "Sue"};
    
    for (const auto& name : ghosts) {
        try {
            auto gPos = gs->getMaze().getNodePos(gs->getGhostPosition(name));
            float d = pacmanEuclid(pacmanPos, gPos);
            if (d < minDist) {
                minDist = d;
                targetGhost = gPos;
            }
        } catch(...) {}
    }

    minDist = 999999.0f;
    for (auto move : moves) {
        if (move == PASS) break;
        auto nextPos = gs->getMaze().getNodePos(gs->getMaze().getNeighbour(character->getPos(), move));
        float dist = pacmanEuclid(nextPos, targetGhost);
        if (dist < minDist) {
            minDist = dist;
            bestMove = move;
        }
    }

    Info::getInfo()->out_move = (bestMove != PASS) ? bestMove : moves[0];
    return BH_SUCCESS;
}

// ACCIÓN POR DEFECTO REPARADA: Escanea dinámicamente el mapa buscando comida para evitar atascos en muros
Status CollectPills::update() {
    auto character = Info::getInfo()->in_character;
    auto gs = Info::getInfo()->in_gamestate;
    
    std::vector<Move> moves = gs->getMaze().getPossibleMoves(character->getPos());
    auto powerPills = gs->getMaze().getPowerPillPositions();
    auto pacmanPos = gs->getMaze().getNodePos(character->getPos());
    
    Move bestMove = PASS;
    float minDist = 999999.0f;

    if (!powerPills.empty()) {
        // 1. SOLUCIÓN AL ATASCO: Encontrar dinámicamente cuál es la pastilla más cercana en este frame específico
        std::pair<float, float> closestPill = powerPills[0];
        float minDistanceToAnyPill = 999999.0f;

        for (auto pillPos : powerPills) {
            float distToPill = pacmanEuclid(pacmanPos, pillPos);
            if (distToPill < minDistanceToAnyPill) {
                minDistanceToAnyPill = distToPill;
                closestPill = pillPos; 
            }
        }

        // 2. EVITAR OSCILACIÓN: Intentamos no regresar inmediatamente por el mismo pasillo si hay otras opciones
        Move opposite = getOppositeMove(character->getDirection());

        for (auto move : moves) {
            if (move == PASS) break;
            
            // Si la intersección nos ofrece caminos alternativos, descartamos volver en reversa
            if (moves.size() > 2 && move == opposite) {
                continue; 
            }

            auto nextPos = gs->getMaze().getNodePos(gs->getMaze().getNeighbour(character->getPos(), move));
            float dist = pacmanEuclid(nextPos, closestPill);
            if (dist < minDist) {
                minDist = dist;
                bestMove = move;
            }
        }
    }

    // 3. RESPALDO DE SEGURIDAD: Si no encontramos mejor ruta o se vació el vector, avanzar por el primer camino legal libre
    if (bestMove == PASS && !moves.empty()) {
        for (auto m : moves) {
            if (m != PASS && m != getOppositeMove(character->getDirection())) {
                bestMove = m;
                break;
            }
        }
        if (bestMove == PASS) bestMove = moves[0];
    }

    Info::getInfo()->out_move = bestMove;
    return BH_SUCCESS;
}