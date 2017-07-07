/*
* Copyright (C) 2017 Tucuxi SA
*/

#include "tucucore/intakeintervalcalculator.h"
#include "tucucore/cachedlogarithms.h"

namespace Tucuxi {
namespace Core {

IntakeIntervalCalculator::Result IntakeIntervalCalculator::calculateIntakePoints(
    Concentrations& _concentrations,
    Times & _times,
    IntakeEvent& _intakeEvent,
    const ParameterList& _parameters,
    const Residuals& _inResiduals,
    const CycleSize _cycleSize,
    Residuals & _outResiduals,
    const bool _isDensityConstant)
{
    if (!checkInputs(_intakeEvent, _parameters))
    {
        return Result::BadParameters;
    }

    prepareComputations(_intakeEvent, _parameters);

    // Create our serie of times
    Eigen::VectorXd times = Eigen::VectorXd::LinSpaced(_cycleSize, 0, _intakeEvent.getInterval());

    // Can we reuse cached logarithms? 
    if (!m_cache.get(_intakeEvent.getInterval(), _parameters, _cycleSize, m_precomputedLogarithms))	{
        computeLogarithms(_intakeEvent, _parameters, times);
        m_cache.set(_intakeEvent.getInterval(), _parameters, _cycleSize, m_precomputedLogarithms);
    }

    computeConcentrations(_inResiduals, _concentrations, _outResiduals);

    times = times.array() + _intakeEvent.getOffsetTime();
    _times.assign(times.data(), times.data() + times.size());

    return Result::Ok;

/*
    //1.interpolate and check error (assume nbPoints is always odd)
    //2. if error ok return current result, else increase density and rerun (for now just rerun with new density
    //and discard old, but eventually should add to existing with a merge of vectors
    if (isFixedDensity || densityError(timesv, concsv) < density_error_thresh || nbPoints > MAX_PT_DENSITY) {
        //Set the new residual
        finalResiduals[0] = concsv[nbPoints - 1];
        POSTCONDCONT(finalResiduals[0] >= 0, SHOULDNTGONEGATIVE, "The concentration is negative.")
        concs.assign(concsv.data(), concsv.data() + concsv.size());
        timesv = timesv.array() + ink.offsettime;
        times.assign(timesv.data(), timesv.data() + timesv.size());
        return Result::Ok;
    }

    // Need more points!
    return Result::DensityError;
*/
}


IntakeIntervalCalculator::Result IntakeIntervalCalculator::calculateIntakeSinglePoint(
    Concentrations& _concentrations,
    const IntakeEvent& _intakeEvent,
    const ParameterList& _parameters,
    const Residuals& _inResiduals,
    const Time _atTime,
    Residuals& _outResiduals)
{
    return Result::BadParameters;
}

}
}
