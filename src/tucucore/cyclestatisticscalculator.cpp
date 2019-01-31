/*
* Copyright (C) 2017 Tucuxi SA
*/

#include <vector>
#include <numeric>
#include <iterator>

#include "tucucommon/duration.h"
#include "tucucommon/loggerhelper.h"

#include "tucucore/cyclestatisticscalculator.h"

namespace Tucuxi {
namespace Core {


CycleStatistic::CycleStatistic(const DateTime &_cycleStartDate, const CycleStatisticType _type)
    : m_cycleStartDate(_cycleStartDate),
      m_type(_type)
{
}


void CycleStatistics::calculate(const std::vector<Concentrations> &_concentrations, const
                                std::vector<TimeOffsets> &_times, Value &_cumulativeAuc)
{
    // Outer vetor size is equal to number of compartments
    for (unsigned int compartment = 0; compartment < _concentrations.size(); compartment++) {

        // if no concentrations, return... no data in concentrations
        if (!(_concentrations[compartment]).size()) {
            Tucuxi::Common::LoggerHelper logHelper;
            logHelper.error("No data in concentrations");
            return;
        }

        Value prevGradient = 0.0, gradient = 0.0, auc = 0.0, peak = 0.0;
        int peakPosition = 0;

        for (unsigned int nbPoints = 0; nbPoints < (_concentrations[compartment].size() - 1); nbPoints++) {

            // compute current gradient
            gradient = _concentrations[compartment][nbPoints+1] - _concentrations[compartment][nbPoints];

            // max: compare whether the gradient is changed from positive to negative
            if ((prevGradient > 0) && (gradient < 0)) {
                m_stats[compartment][static_cast<int>(CycleStatisticType::Maximum)].addValue(Duration(std::chrono::seconds((int)(_times[compartment][nbPoints])*60*60)), _concentrations[compartment][nbPoints]);
            }
            // min: compare whether the gradient is changed from negative to positive
            else if ((prevGradient < 0) && (gradient > 0)) {
                m_stats[compartment][static_cast<int>(CycleStatisticType::Minimum)].addValue(Duration(std::chrono::seconds((int)(_times[compartment][nbPoints])*60*60)), _concentrations[compartment][nbPoints]);
            }

            // store current gradient to previous gradient for next comparison
            prevGradient = gradient;

            // auc: calculate area under curve
            auc += (_concentrations[compartment][nbPoints] + _concentrations[compartment][nbPoints+1])
                    / 2.0 * (_times[compartment][nbPoints+1] - _times[compartment][nbPoints]);

            // peak: compare with concentrations at nbPoints and nbPoints + 1
            if (peak < _concentrations[compartment][nbPoints]) {
                peak = _concentrations[compartment][nbPoints];
                peakPosition = nbPoints;
            }
            else if (peak < _concentrations[compartment][nbPoints+1]) {
                peak = _concentrations[compartment][nbPoints+1];
                peakPosition = nbPoints + 1;
            }
        }

        _cumulativeAuc += auc;

        // add residual value
        m_stats[compartment][static_cast<int>(CycleStatisticType::Residual)].addValue(
                    Duration(std::chrono::seconds((int)(_times[compartment][_times[compartment].size() - 1])*60*60)),
                    _concentrations[compartment][_concentrations[compartment].size() - 1]);
        // add AUC value with time 0
        m_stats[compartment][static_cast<int>(CycleStatisticType::AUC)].addValue(
                    Duration(),
                    auc);
        // add cumulative AUC value with time 0
        m_stats[compartment][static_cast<int>(CycleStatisticType::CumulativeAuc)].addValue(
                    Duration(),
                    _cumulativeAuc);
        // add mean value with time 0
        m_stats[compartment][static_cast<int>(CycleStatisticType::Mean)].addValue(
                    Duration(),
                    (auc/(_times[compartment][_concentrations[compartment].size() - 1] - _times[compartment][0])));
        // add peak value with duration (change offset to duration)
        m_stats[compartment][static_cast<int>(CycleStatisticType::Peak)].addValue(
                    Duration(std::chrono::seconds((int)(_times[compartment][peakPosition])*60*60)),
                    peak);

        // add cycle interval, in hours
        m_stats[compartment][static_cast<int>(CycleStatisticType::CycleInterval)].addValue(
                    Duration(),
                    (_times[compartment][_times[compartment].size() - 1] - _times[compartment][0]));
    }
}


CycleStatistics::CycleStatistics(const CycleData &_data, Value& _cumulativeAuc)
{
    //
    // Build a new statistics with the given type
    //
    size_t nbCompartments = _data.m_concentrations.size();

    // m_stats.reserve(nbCompartments);
    for (unsigned int compartment = 0; compartment < nbCompartments; compartment++) {

        m_stats.push_back(std::vector<Tucuxi::Core::CycleStatistic> ());

        for (unsigned int type= 0; type< static_cast<int>(CycleStatisticType::CYCLE_STATISTIC_TYPE_SIZE); type++) {
                m_stats[compartment].push_back(CycleStatistic(_data.m_start, (CycleStatisticType)type));
        }

    }

    //
    // calculate cycle statistics and add values
    //
    calculate(_data.m_concentrations, _data.m_times, _cumulativeAuc);
}

} // namespace Core
} // namespace Tucuxi
