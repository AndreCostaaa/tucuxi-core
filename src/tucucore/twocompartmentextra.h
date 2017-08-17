/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_MATH_TWOCOMPARTMENTEXTRA_H
#define TUCUXI_MATH_TWOCOMPARTMENTEXTRA_H

#include "tucucore/intakeintervalcalculator.h"

namespace Tucuxi {
namespace Core {

/// \ingroup TucuCore
/// \brief Intake interval calculator for the two compartment extra algorithm
/// \sa IntakeIntervalCalculator
class TwoCompartmentExtra : public IntakeIntervalCalculator
{
public:
    /// \brief Constructor
    TwoCompartmentExtra();

protected:
    virtual bool checkInputs(const IntakeEvent& _intakeEvent, const ParameterList& _parameters) override;
    virtual void prepareComputations(const IntakeEvent& _intakeEvent, const ParameterList& _parameters) override;
    virtual void computeLogarithms(const IntakeEvent& _intakeEvent, const ParameterList& _parameters, Eigen::VectorXd& _times) override;
    virtual bool computeConcentrations(const Residuals& _inResiduals, Concentrations& _concentrations, Residuals& _outResiduals) override;
    virtual bool computeConcentration(const Value& _atTime, const Residuals& _inResiduals, Concentrations& _concentrations, Residuals& _outResiduals) override;
    bool compute(const Residuals& _inResiduals, Eigen::VectorXd& _concentrations1, Value& _concentrations2, Value& _concentrations3);

private:
    Value m_D;	/// Quantity of drug
    Value m_Cl;	/// Clearance
    Value m_F;  /// Biodisponibility
    Value m_Ka; /// Absorption rate constant
    Value m_Q;	/// ???
    Value m_V1;	/// Volume of the compartment 1
    Value m_V2;	/// Volume of the compartment 2
    Value m_Ke; /// Elimination constant rate = Cl/V1 where Cl is the clearance and V1 is the volume of the compartment 1
    Value m_K12; /// Q/V1
    Value m_K21; /// Q/V2
    Value m_RootK; /// sqrt(sumK*sumK - 4*K21*Ke)
    Value m_Alpha; /// (sumK + root)/2
    Value m_Beta; /// (sumK - root)/2
    int m_NbPoints; /// number measure points during interval
    Value m_Int; /// Interval (hours)
};

inline bool TwoCompartmentExtra::compute(const Residuals& _inResiduals, Eigen::VectorXd&
_concentrations1, Value& _concentrations2, Value& _concentrations3)
{
    Value A, B, C, divider;
    Concentration resid1 = _inResiduals[0];
    Concentration resid2 = _inResiduals[1];
    Concentration resid3 = _inResiduals[2] + (m_F * m_D / m_V1);
    Value sumResid13 = resid1 + resid3;
    Value sumK12K21 = m_K12 + m_K21;
    Value sumK21Ke = m_K21 + m_Ke;
    Value diffK21Ka = m_K21 - m_Ka;
    Value diffK21Ke = m_K21 - m_Ke;
    Value powDiffK21Ke = std::pow(diffK21Ke, 2);

    // For compartment1, calculate A, B, C and divider
    A = 
        std::pow(m_K12, 3) * m_Ka * resid1 
        + diffK21Ka * (powDiffK21Ke * (-m_Ke * resid1 + m_Ka * sumResid13) 
            + ((m_Ka - m_Ke) * (m_Ke * resid1 - m_K21 * (resid1 + 2 * resid2)) 
            + m_Ka * (-m_K21 + m_Ke) * resid3) * m_RootK) 
        + std::pow(m_K12, 2) * (m_K21 * (-m_Ke * resid1 + m_Ka * (3 * resid1 + resid3))
            + m_Ka * (-m_Ka * sumResid13 + resid1 * (3 * m_Ke + m_RootK))) 
        + m_K12 * (std::pow(m_K21, 2) * (3 * m_Ka * resid1 - 2 * m_Ke * resid1 + 2 * m_Ka * resid3) 
            - m_K21 * (2 * std::pow(m_Ka, 2) * sumResid13 - 2 * m_Ka * m_Ke * sumResid13 
	        + m_Ka * (2 * resid2 + resid3) * m_RootK + m_Ke * resid1 * (2 * m_Ke + m_RootK)) 
            - m_Ka * (m_Ka * sumResid13 * (2 * m_Ke + m_RootK) 
	        - m_Ke * resid1 * (3 * m_Ke + 2 * m_RootK)));

    B = 
        std::pow(m_K12, 3) * m_Ka * resid1 
        + diffK21Ka * (powDiffK21Ke * (-m_Ke * resid1 + m_Ka * sumResid13) 
            + ((m_Ka - m_Ke) * (-m_Ke * resid1 + m_K21 * (resid1 + 2 * resid2)) 
            + m_Ka * diffK21Ke * resid3) * m_RootK) 
        + std::pow(m_K12, 2) * (m_K21 * (-m_Ke * resid1 + m_Ka * (3 * resid1 + resid3)) 
            - m_Ka * (m_Ka * sumResid13 + resid1 * (-3 * m_Ke + m_RootK))) 
        + m_K12 * (std::pow(m_K21, 2) * (3 * m_Ka * resid1 - 2 * m_Ke * resid1 + 2 * m_Ka * resid3) 
            + m_Ka * (m_Ke * resid1 * (3 * m_Ke - 2 * m_RootK) 
	        - m_Ka * sumResid13 * (2 * m_Ke - m_RootK)) 
            + m_K21 * (-2 * std::pow(m_Ka, 2) * sumResid13 + 2 * m_Ka * m_Ke * sumResid13 
	        + m_Ka * (2 * resid2 + resid3) * m_RootK + m_Ke * resid1 * (-2 * m_Ke + m_RootK)));

    C = 
	- 2 * diffK21Ka * m_Ka * (std::pow(m_K12, 2) + powDiffK21Ke + 2 * m_K12 * sumK21Ke) * resid3;

    divider = 
	std::pow((sumK12K21 - 2 * m_Ka + m_Ke) * m_RootK,  2) 
	- std::pow(std::pow(m_K12,2) + powDiffK21Ke + 2 * m_K12 * sumK21Ke, 2);

    if(!checkValue(divider != 0.0, "Dividing by zero."))
	    return false;

    // Calculate concentrations of compartment 1
    _concentrations1 = 
	-2 * (B * m_precomputedLogarithms["Beta"] 
		+ A * m_precomputedLogarithms["Alpha"] + C * m_precomputedLogarithms["Ka"]) / divider;

    // For compartment1, calculate A, B, C and divider
    A = 
        -(std::pow(m_K12, 2) + powDiffK21Ke + 2 * m_K12 * sumK21Ke) 
        * (diffK21Ka * (m_Ka - m_Ke) * resid2 + m_K12 * m_Ka * (resid2 + resid3)) 
        + ((m_K12 * m_Ka + diffK21Ka * (m_Ka -m_Ke)) * (2 * m_K12 * resid1 + (m_K12 - diffK21Ke) * resid2) 
	    + m_K12 * m_Ka * (sumK12K21 - 2 * m_Ka + m_Ke) * resid3) * m_RootK;
    
    B = 
        -(std::pow(m_K12, 2) + powDiffK21Ke + 2 * m_K12 * sumK21Ke) 
        * (diffK21Ka * (m_Ka - m_Ke) * resid2 + m_K12 * m_Ka * (resid2 + resid3)) 
        - ((m_K12 * m_Ka + diffK21Ka * (m_Ka - m_Ke)) * (2 * m_K12 * resid1 + (m_K12 - diffK21Ke) * resid2) 
            + m_K12 * m_Ka * (sumK12K21 - 2 * m_Ka + m_Ke) * resid3) * m_RootK;
    
    C = 
        2 * m_K12 * m_Ka * (std::pow(m_K12, 2) + powDiffK21Ke + 2 * m_K12 * sumK21Ke) *resid3;
    
    divider = 
        -std::pow(std::pow(m_K12, 2) + powDiffK21Ke + 2 * m_K12 * sumK21Ke, 2) 
        + std::pow(sumK12K21 - 2 * m_Ka + m_Ke, 2) * std::pow(m_RootK, 2);
    
    if(!checkValue(divider != 0.0, "Dividing by zero."))
	    return false;

    // Calculate concentrations of compartment 2 and 3
    _concentrations2 = 
        2 * (B * m_precomputedLogarithms["Beta"](m_precomputedLogarithms["Beta"].size() - 1) 
        + A * m_precomputedLogarithms["Alpha"](m_precomputedLogarithms["Alpha"].size() - 1) 
        + C * m_precomputedLogarithms["Ka"](m_precomputedLogarithms["Ka"].size() - 1)) / divider;
    _concentrations3 = 
	m_precomputedLogarithms["Ka"](m_precomputedLogarithms["Ka"].size() - 1) * resid3;

    return true;
}

}
}

#endif // TUCUXI_MATH_TWOCOMPARTMENTEXTRA_H
