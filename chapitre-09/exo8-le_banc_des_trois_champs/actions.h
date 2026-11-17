#pragma once

// Etat d'une action booléenne:
// courant : la valeur de l'image
// change : courant diffère de celui de la synchronisation précédente
// actif : une source matérielle est liée à l'action
struct EtatAction {
    bool courant = false;
    bool change = false;
    bool actif = false;

    bool saisi() const { return courant && change; }
    bool lache() const { return !courant && change && actif; }
};

class Action {
public:
    explicit Action(const char* nom) : nom_(nom) {}

    const char* nom() const { return nom_; }
    const EtatAction& etat() const { return etat_; }

    // lie  : le système a trouve un organe pour cette action
    // presse : l'état brut de l'organe
    void synchroniser(bool lie, bool presse) {
        const bool avant = etat_.courant;
        etat_.actif = lie;
        etat_.courant = lie && presse;    
        etat_.change = (etat_.courant != avant);
    }

private:
    const char* nom_;
    EtatAction etat_;
};
