#pragma once

#include "Controller.h"
#include "BehaviorTree.h"
#include "BTGhostController.h" // Para heredar la estructura de Info y Status del profesor

class PacmanController: public Controller {
private:
    std::shared_ptr<Composite> root;

public:
    PacmanController(std::shared_ptr<Character> character);
    virtual ~PacmanController();
    virtual Move getMove(const GameState& game) override;

private:
    void setupBehaviorTree();
};

// --- CONDICIÓN: ¿Hay peligro inminente (fantasmas cerca)? ---
class IsGhostNear : public Behavior {
public:
    virtual Status update() override;
};

// --- ACCIÓN: Huir del fantasma más cercano ---
class EscapeFromGhost : public Behavior {
public:
    virtual Status update() override;
};

// --- CONDICIÓN: ¿Hay algún fantasma comestible en el mapa? ---
class IsGhostEdible : public Behavior {
public:
    virtual Status update() override;
};

// --- ACCIÓN: Perseguir al fantasma comestible ---
class HuntGhost : public Behavior {
public:
    virtual Status update() override;
};

// --- ACCIÓN POR DEFECTO CORREGIDA: Buscar y comer pastillas sin trabarse ---
class CollectPills : public Behavior {
public:
    virtual Status update() override;
};