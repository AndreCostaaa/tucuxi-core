/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_CORE_THREECOMPARTMENTINFUSION_H
#define TUCUXI_CORE_THREECOMPARTMENTINFUSION_H

#include "tucucore/intakeintervalcalculator.h"

namespace Tucuxi {
namespace Core {

enum class ThreeCompartmentInfusionExponentials : int { Alpha, Beta, Gamma };
enum class ThreeCompartmentInfusionCompartments : int { First, Second, Third };

/// \ingroup TucuCore
/// \brief Intake interval calculator for the three compartment infusion algorithm
/// \sa IntakeIntervalCalculator
class ThreeCompartmentInfusionMicro : public IntakeIntervalCalculatorBase<3, ThreeCompartmentInfusionExponentials>
{
    INTAKEINTERVALCALCULATOR_UTILS(ThreeCompartmentInfusionMicro)
public:
    /// \brief Constructor
    ThreeCompartmentInfusionMicro();

    typedef ThreeCompartmentInfusionExponentials Exponentials;

protected:
    virtual bool checkInputs(const IntakeEvent& _intakeEvent, const ParameterSetEvent& _parameters) override;
    virtual void computeExponentials(Eigen::VectorXd& _times) override;
    virtual bool computeConcentrations(const Residuals& _inResiduals, const bool _isAll, std::vector<Concentrations>& _concentrations, Residuals& _outResiduals) override;
    virtual bool computeConcentration(const Value& _atTime, const Residuals& _inResiduals, const bool _isAll, std::vector<Concentrations>& _concentrations, Residuals& _outResiduals) override;
    void compute(const int _forcesize, Eigen::VectorXd& _concentrations1, Value& _concentrations2, Value& _concentrations3);

    Value m_D;	/// Quantity of drug
    Value m_F;	/// ???
    Value m_V1;	/// Volume of the compartment 1
    Value m_Ke; /// Elimination constant rate = Cl/V1 where Cl is the clearance and V1 is the volume of the compartment 1
    Value m_K12; /// Q/V1
    Value m_K21; /// Q/V2
    Value m_K13; /// Q/V1
    Value m_K31; /// Q/V2
    Value m_Alpha;
    Value m_Beta;
    Value m_Gamma;
    Value m_Tinf; /// Infusion time (hours)
    Value m_Int; /// Interval (hours)
    int m_NbPoints; /// number measure points during interval

private:
    typedef ThreeCompartmentInfusionCompartments Compartments;
};

inline void ThreeCompartmentInfusionMicro::compute(const int _forcesize, Eigen::VectorXd& _concentrations1, Value& _concentrations2, Value& _concentrations3)
{
    Eigen::VectorXd& alphaLogV = exponentials(Exponentials::Alpha);
    Eigen::VectorXd& betaLogV = exponentials(Exponentials::Beta);
    Eigen::VectorXd& gammaLogV = exponentials(Exponentials::Gamma);

    Value deltaD = (m_D / m_V1) / m_Tinf; 
    Value alphaTinf = std::exp(-m_Alpha* m_Tinf);
    Value betaTinf = std::exp(-m_Beta * m_Tinf);
    Value gammaTinf = std::exp(-m_Gamma * m_Tinf);

    Value A = (1 / m_V1) * (m_K21 - m_Alpha) * (m_K31 - m_Alpha) / (m_Alpha - m_Beta) / (m_Alpha - m_Gamma);
    Value B = (1 / m_V1) * (m_K21 - m_Beta) * (m_K31 - m_Beta) / (m_Beta - m_Alpha) / (m_Beta - m_Gamma);
    Value C = (1 / m_V1) * (m_K21 - m_Gamma) * (m_K31 - m_Gamma) / (m_Gamma - m_Beta) / (m_Gamma - m_Alpha);
    Value A2 = m_K12 / (m_K21 - m_Alpha) * A;
    Value B2 = m_K12 / (m_K21 - m_Beta) * B;
    Value C2 = m_K12 / (m_K21 - m_Gamma) * C;
    Value A3 = m_K13 / (m_K31 - m_Alpha) * A;
    Value B3 = m_K13 / (m_K31 - m_Beta) * B;
    Value C3 = m_K13 / (m_K31 - m_Gamma) * C;

    if (_forcesize != 0)
    {
	    _concentrations1.head(_forcesize) = 
	        deltaD 
	        * (A/m_Alpha * (1*Eigen::VectorXd::Ones(_forcesize)- alphaLogV.head(_forcesize)) 
		    + B/m_Beta * (1*Eigen::VectorXd::Ones(_forcesize) - betaLogV.head(_forcesize)) 
	            + C/m_Gamma * (1*Eigen::VectorXd::Ones(_forcesize) - gammaLogV.head(_forcesize)));
    } 

    int therest = static_cast<int>(alphaLogV.size() - _forcesize);
    
    _concentrations1.tail(therest) = 
	deltaD 
	* (A/m_Alpha * (1 - alphaTinf) * alphaLogV.tail(therest) / alphaTinf 
		+ B/m_Beta * (1 - betaTinf) * betaLogV.tail(therest) / betaTinf 
	        + C/m_Gamma * (1 - gammaTinf) * gammaLogV.tail(therest) / gammaTinf);

    // Do NOT use m_NbPoints because in case of single calculation "m_NbPoints = 0"
    _concentrations2 =
        deltaD * 
        (A2/m_Alpha * (1 - alphaTinf) * alphaLogV(alphaLogV.size() - 1) / alphaTinf 
	    + B2/m_Beta * (1 - betaTinf) * betaLogV(betaLogV.size() - 1) / betaTinf 
            + C2/m_Gamma * (1 - gammaTinf) * gammaLogV(gammaLogV.size() - 1) / gammaTinf);

    _concentrations3 = 
        deltaD * 
        (A3/m_Alpha * (1 - alphaTinf) * alphaLogV(alphaLogV.size() - 1) / alphaTinf 
	    + B3/m_Beta * (1 - betaTinf) * betaLogV(betaLogV.size() - 1) / betaTinf 
            + C3/m_Gamma * (1 - gammaTinf) * gammaLogV(gammaLogV.size() - 1) / gammaTinf);

}

class ThreeCompartmentInfusionMacro : public ThreeCompartmentInfusionMicro
{
    INTAKEINTERVALCALCULATOR_UTILS(ThreeCompartmentInfusionMacro)
public:
    ThreeCompartmentInfusionMacro();

protected:
    virtual bool checkInputs(const IntakeEvent& _intakeEvent, const ParameterSetEvent& _parameters) override;
};

}
}

#endif // TUCUXI_CORE_THREECOMPARTMENTINFUSION_H