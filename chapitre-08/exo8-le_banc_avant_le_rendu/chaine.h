#pragma once

#include <cstdint>

class ChaineEchange {
public:
    static constexpr std::uint32_t MAX_IMAGES = 4;

    bool Creer(std::uint32_t nombre) noexcept {
        if (nombre == 0 || nombre > MAX_IMAGES) return false;
        mNombre = nombre;
        mSuivante = 0;
        mEmpruntee = false;
        for (std::uint64_t& p : mPoignees) p = 0;
        return true;
    }

    void Detruire() noexcept {
        mNombre = 0;
        mSuivante = 0;
        mEmpruntee = false;
        for (std::uint64_t& p : mPoignees) p = 0;
    }

    bool EnregistrerImage(std::uint32_t index, std::uint64_t poignee) noexcept {
        if (index >= mNombre) return false;
        mPoignees[index] = poignee;
        return true;
    }

    bool Acquerir(std::uint32_t& index) noexcept {
        if (mNombre == 0 || mEmpruntee) return false;
        index = mSuivante;
        mEmpruntee = true;
        return true;
    }

    bool Rendre() noexcept {
        if (mNombre == 0 || !mEmpruntee) return false;
        mEmpruntee = false;
        mSuivante = (mSuivante + 1u) % mNombre;
        return true;
    }

    std::uint32_t DerniereRendue() const noexcept {
        if (mNombre == 0) return 0;
        return (mSuivante + mNombre - 1u) % mNombre;
    }

    std::uint64_t Poignee(std::uint32_t index) const noexcept {
        return index < mNombre ? mPoignees[index] : 0;
    }

    std::uint32_t Nombre() const noexcept { return mNombre; }
    bool EstCreee() const noexcept { return mNombre != 0; }

private:
    std::uint64_t mPoignees[MAX_IMAGES] = {};
    std::uint32_t mNombre = 0;
    std::uint32_t mSuivante = 0;
    bool mEmpruntee = false;
};
