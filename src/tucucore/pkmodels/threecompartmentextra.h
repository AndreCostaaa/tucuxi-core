/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_CORE_THREECOMPARTMENTEXTRA_H
#define TUCUXI_CORE_THREECOMPARTMENTEXTRA_H

#include "tucucore/intakeintervalcalculatoranalytical.h"

namespace Tucuxi {
namespace Core {

enum class ThreeCompartmentExtraExponentials : int
{
    Alpha,
    Beta,
    Gamma,
    Ka
};

enum class ThreeCompartmentExtraCompartments : int
{
    First,
    Second,
    Third
};

/// \ingroup TucuCore
/// \brief Intake interval calculator for the three compartment extra algorithm
/// \sa IntakeIntervalCalculator
class ThreeCompartmentExtraMicro : public IntakeIntervalCalculatorBase<3, ThreeCompartmentExtraExponentials>
{
    INTAKEINTERVALCALCULATOR_UTILS(ThreeCompartmentExtraMicro)
public:
    /// \brief Constructor
    ThreeCompartmentExtraMicro();

    /// \brief Returns the list of required PK parameters Ids
    /// \return The list of required PK parameters Ids
    static std::vector<std::string> getParametersId();

    typedef ThreeCompartmentExtraExponentials Exponentials;

protected:
    bool checkInputs(const IntakeEvent& _intakeEvent, const ParameterSetEvent& _parameters) override;

    void computeExponentials(Eigen::VectorXd& _times) override;

    bool computeConcentrations(
            const Residuals& _inResiduals,
            bool _isAll,
            std::vector<Concentrations>& _concentrations,
            Residuals& _outResiduals) override;

    bool computeConcentration(
            const Value& _atTime,
            const Residuals& _inResiduals,
            bool _isAll,
            std::vector<Concentrations>& _concentrations,
            Residuals& _outResiduals) override;

    void compute(
            const Residuals& _inResiduals,
            Eigen::VectorXd& _concentrations1,
            Value& _concentrations2,
            Value& _concentrations3);

    Value m_D;  /// Quantity of drug
    Value m_F;  /// ???
    Value m_V1; /// Volume of the compartment 1
    Value m_Ka; /// Absorption rate constant
    Value m_Ke; /// Elimination constant rate = Cl/V1 where Cl is the clearance and V1 is the volume of the compartment 1
    Value m_K12; /// Q/V1
    Value m_K21; /// Q/V2
    Value m_K13; /// Q/V1
    Value m_K31; /// Q/V2
    Value m_Alpha;
    Value m_Beta;
    Value m_Gamma;
    Eigen::Index m_nbPoints; /// number measure points during interval
    Value m_Int;             /// Interval (hours)

private:
    typedef ThreeCompartmentExtraCompartments Compartments;
};

inline void ThreeCompartmentExtraMicro::compute(
        const Residuals& _inResiduals,
        Eigen::VectorXd& _concentrations1,
        Value& _concentrations2,
        Value& _concentrations3)
{
    Concentration resid1 = _inResiduals[0] + m_F * m_D / m_V1;
    Concentration resid2 = _inResiduals[1];
    Concentration resid3 = _inResiduals[2];

    Value A = 1 / m_V1 * (m_Ka / (m_Ka - m_Alpha)) * (m_K21 - m_Alpha) * (m_K31 - m_Alpha) / (m_Alpha - m_Beta)
              / (m_Alpha - m_Gamma); // NOLINT(readability-identifier-naming)
    Value B = 1 / m_V1 * (m_Ka / (m_Ka - m_Beta)) * (m_K21 - m_Beta) * (m_K31 - m_Beta) / (m_Beta - m_Alpha)
              / (m_Beta - m_Gamma); // NOLINT(readability-identifier-naming)
    Value C = 1 / m_V1 * (m_Ka / (m_Ka - m_Gamma)) * (m_K21 - m_Gamma) * (m_K31 - m_Gamma) / (m_Gamma - m_Beta)
              / (m_Gamma - m_Alpha); // NOLINT(readability-identifier-naming)

    Value A2 = m_K12 / (m_K21 - m_Alpha) * A; // NOLINT(readability-identifier-naming)
    Value B2 = m_K12 / (m_K21 - m_Beta) * B;  // NOLINT(readability-identifier-naming)
    Value C2 = m_K12 / (m_K21 - m_Gamma) * C; // NOLINT(readability-identifier-naming)
    Value A3 = m_K13 / (m_K31 - m_Alpha) * A; // NOLINT(readability-identifier-naming)
    Value B3 = m_K13 / (m_K31 - m_Beta) * B;  // NOLINT(readability-identifier-naming)
    Value C3 = m_K13 / (m_K31 - m_Gamma) * C; // NOLINT(readability-identifier-naming)

    // Calculate concentrations for comp1, comp2 and comp3
    _concentrations1 = resid1
                       * (B * exponentials(Exponentials::Beta) + A * exponentials(Exponentials::Alpha)
                          + C * exponentials(Exponentials::Gamma) - (A + B + C) * exponentials(Exponentials::Ka));

    // Do NOT use m_nbPoints because in case of single point calculation m_nbPoints = 0.
    _concentrations2 =
            resid2
            + resid1
                      * (B2 * exponentials(Exponentials::Beta)((exponentials(Exponentials::Beta)).size() - 1)
                         + A2 * exponentials(Exponentials::Alpha)((exponentials(Exponentials::Alpha)).size() - 1)
                         + C2 * exponentials(Exponentials::Gamma)((exponentials(Exponentials::Gamma)).size() - 1)
                         - (A2 + B2 + C2)
                                   * exponentials(Exponentials::Ka)((exponentials(Exponentials::Ka)).size() - 1));

    _concentrations3 =
            resid3
            + resid1
                      * (B3 * exponentials(Exponentials::Beta)((exponentials(Exponentials::Beta)).size() - 1)
                         + A3 * exponentials(Exponentials::Alpha)((exponentials(Exponentials::Alpha)).size() - 1)
                         + C3 * exponentials(Exponentials::Gamma)((exponentials(Exponentials::Gamma)).size() - 1)
                         - (A3 + B3 + C3)
                                   * exponentials(Exponentials::Ka)((exponentials(Exponentials::Ka)).size() - 1));
}

class ThreeCompartmentExtraMacro : public ThreeCompartmentExtraMicro
{
    INTAKEINTERVALCALCULATOR_UTILS(ThreeCompartmentExtraMacro)
public:
    ThreeCompartmentExtraMacro();

    /// \brief Returns the list of required PK parameters Ids
    /// \return The list of required PK parameters Ids
    static std::vector<std::string> getParametersId();

protected:
    bool checkInputs(const IntakeEvent& _intakeEvent, const ParameterSetEvent& _parameters) override;
};

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_THREECOMPARTMENTEXTRA_H
