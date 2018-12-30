/*
* Copyright (C) 2018 Tucuxi SA
*/

#ifndef TUCUXI_CORE_RK4ONECOMPARTMENTGAMMAEXTRA_H
#define TUCUXI_CORE_RK4ONECOMPARTMENTGAMMAEXTRA_H

#include "tucucore/intakeintervalcalculator.h"
#include <boost/math/special_functions/gamma.hpp>

namespace Tucuxi {
namespace Core {

enum class RK4OneCompartmentGammaExtraExponentials : int { Ke = 0, Ka };
enum class RK4OneCompartmentGammaExtraCompartments : int { First = 0, Second };

/// \ingroup TucuCore
/// \brief Intake interval calculator for the one compartment extravascular algorithm
/// \sa IntakeIntervalCalculator
class RK4OneCompartmentGammaExtraMicro : public IntakeIntervalCalculatorBase<2, RK4OneCompartmentGammaExtraExponentials>
{
    INTAKEINTERVALCALCULATOR_UTILS(RK4OneCompartmentGammaExtraMicro)
public:
    /// \brief Constructor
    RK4OneCompartmentGammaExtraMicro();

    typedef RK4OneCompartmentGammaExtraExponentials Exponentials;

protected:
    bool checkInputs(const IntakeEvent& _intakeEvent, const ParameterSetEvent& _parameters) override;
    void computeExponentials(Eigen::VectorXd& _times) override;
    bool computeConcentrations(const Residuals& _inResiduals, bool _isAll, std::vector<Concentrations>& _concentrations, Residuals& _outResiduals) override;
    bool computeConcentration(const Value& _atTime, const Residuals& _inResiduals, bool _isAll, std::vector<Concentrations>& _concentrations, Residuals& _outResiduals) override;
    void compute(const Residuals& _inResiduals, Eigen::VectorXd& _concentrations1, Eigen::VectorXd& _concentrations2);

    Value m_D;	/// Quantity of drug
    Value m_F;  /// Biodisponibility
    Value m_V;  /// Volume of the compartment
    Value m_Ke; /// Elimination constant rate = Cl/V where Cl is the clearance and V is the volume of the compartment
    int m_NbPoints; /// Number measure points during interval
    Value m_Int; /// Interval time (Hours)
    Value m_a;  /// Gamma distribution parameter
    Value m_b;  /// Gamma distribution parameter

private:
    typedef RK4OneCompartmentGammaExtraCompartments Compartments;
};

inline void RK4OneCompartmentGammaExtraMicro::compute(const Residuals& _inResiduals, Eigen::VectorXd& _concentrations1, Eigen::VectorXd& _concentrations2)
{
    const double h = m_Int/m_NbPoints;

    auto time = [&h] (const uint32_t index) -> const double {
        return h * index;
    };

    auto probDensityAbsorptionTimes = [this] (const double& t)
    {
        return pow(m_b, m_a) / boost::math::tgamma(m_a) * pow(t, m_a - 1) * exp(-m_b * t);
    };

    auto derive = [this, probDensityAbsorptionTimes] (const double& t, const double& c1, const double& c2, double& dc1dt, double& dc2dt)
    {
        dc2dt = -m_F * m_D * probDensityAbsorptionTimes(t);
        dc1dt = -dc2dt - m_Ke * c1;
    };

    /* Initial values */
    _concentrations1[0] = _inResiduals[0];
    _concentrations2[0] = _inResiduals[1] + m_D / m_V;

    /* Looping the rest of the points to calculate the conventrations 
     * of each compartment */
    double dc1dt, dc2dt;
    double c1_k1, c2_k1;
    double c1_k2, c2_k2;
    double c1_k3, c2_k3;
    double c1_k4, c2_k4;

    for (auto i = 0; (i + 1) < m_NbPoints; i = i + 1) {
        derive(time(i), _concentrations1[i], 
                _concentrations2[i],
                dc1dt, dc2dt);

        /* Set k1's*/
        auto c1_k1 = h * dc1dt; 
        auto c2_k1 = h * dc2dt;

        derive(time(i), _concentrations1[i] + c1_k1/2, 
                _concentrations2[i] + c2_k1/2,
                dc1dt, dc2dt);

        /* Set k2's*/
        auto c1_k2 = h * dc1dt; 
        auto c2_k2 = h * dc2dt;

        derive(time(i), _concentrations1[i] + c1_k2/2, 
                _concentrations2[i] + c2_k2/2,
                dc1dt, dc2dt);

        /* Set k3's*/
        auto c1_k3 = h * dc1dt; 
        auto c2_k3 = h * dc2dt;

        derive(time(i), _concentrations1[i] + c1_k3, 
                _concentrations2[i] + c2_k3,
                dc1dt, dc2dt);

        /* Set k4's*/
        auto c1_k4 = h * dc1dt; 
        auto c2_k4 = h * dc2dt;

        _concentrations1[i+1] = _concentrations1[i] + c1_k1/6 + c1_k2/3 + c1_k3/3 + c1_k4/6;
        _concentrations2[i+1] = _concentrations2[i] + c2_k1/6 + c2_k2/3 + c2_k3/3 + c2_k4/6;

        if (_concentrations1[i+1] < 0) {
            _concentrations1[i+1] = 0;
        }
        if (_concentrations2[i+1] < 0) {
            _concentrations2[i+1] = 0;
        }
    }

}

class RK4OneCompartmentGammaExtraMacro : public RK4OneCompartmentGammaExtraMicro
{
    INTAKEINTERVALCALCULATOR_UTILS(RK4OneCompartmentGammaExtraMacro)
public:
    RK4OneCompartmentGammaExtraMacro();

protected:
    bool checkInputs(const IntakeEvent& _intakeEvent, const ParameterSetEvent& _parameters) override;
};

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_RK4ONECOMPARTMENTEXTRA_H
