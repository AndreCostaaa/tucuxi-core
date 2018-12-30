#include "targetevaluator.h"

#include "tucucore/cyclestatisticscalculator.h"
#include "tucucore/concentrationprediction.h"
#include "tucucore/intakeevent.h"
#include "tucucore/dosage.h"
#include "tucucore/drugtreatment/target.h"
#include "tucucore/targetevaluationresult.h"

namespace Tucuxi {
namespace Core {

TargetEvaluator::TargetEvaluator()
{

}

TargetEvaluator::Result TargetEvaluator::evaluate(
        const ConcentrationPrediction& _prediction,
        const IntakeSeries& _intakeSeries,
        const Target &_target,
        TargetEvaluationResult & _result)
{

    assert(_prediction.getTimes().size() > 0);

    bool bOk = true;
    double score = 0.0;
    double value = 0.0;

    std::size_t lastCycleIndex = _prediction.getTimes().size() - 1;
    TimeOffsets times = _prediction.getTimes().at(lastCycleIndex);
    DateTime start = _intakeSeries.at(lastCycleIndex).getEventTime();
    DateTime end = start + std::chrono::milliseconds(static_cast<int>(times.at(times.size() - 1)) * 1000);
    CycleData cycle(start, end, Unit("ug/l"));
    cycle.addData(times, _prediction.getValues().at(lastCycleIndex), 0);


    // Only valid for a single cycle, so 0.0 as cumulative AUC here.
    // It cannot be used as is for cumulative AUC of multiple cycles.
    Value fakeCumulativeAuc = 0.0;
    CycleStatistics statisticsCalculator(cycle, fakeCumulativeAuc);
    DateTime dateTime;
    std::vector< std::vector<Tucuxi::Core::CycleStatistic> > stats;

    switch( _target.m_targetType) {
    case TargetType::Peak :
    {
        double peakConcentration = 0.0;
        CycleStatistic cycleStatistic = statisticsCalculator.getStatistic(0, CycleStatisticType::Peak);
        if (!cycleStatistic.getValue(dateTime, peakConcentration)) {
            // Something wrong here
        }

        double peakTime = (dateTime - cycleStatistic.getCycleStartDate()).toMinutes();

        bOk = peakTime >= _target.m_tMin.toMinutes() && peakTime <= _target.m_tMax.toMinutes();
        if (bOk) {
            bOk = (peakConcentration < _target.m_valueMax) &&
                    (peakConcentration > _target.m_valueMin);
            // Check if concentration is within the target range
            if (bOk) {
                score = 1.0 - pow(log(peakConcentration) - log(_target.m_valueBest), 2)
                        / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
                value = peakConcentration;
            }
        }
    } break;

    case TargetType::Residual :
    {
        // Take compartment 0, could be different in the future
        //double lastResidual = cycle.m_concentrations[0][cycle.m_concentrations[0].size() - 1];
        // We have two options, I choose the cycle statistic calculator here
        double lastResidual = -1.0;
        CycleStatistic cycleStatistic = statisticsCalculator.getStatistic(0, CycleStatisticType::Residual);
        bOk = cycleStatistic.getValue(dateTime, lastResidual);

        // The following was to test vancomycin (Unit issue)
        // lastResidual /= 1000.0;

        if (bOk) {
            // Check if concentration is within the target range
            bOk = (lastResidual < _target.m_valueMax) && (lastResidual > _target.m_valueMin);
            if (bOk) {
                score = 1.0 - pow(log(lastResidual) - log(_target.m_valueBest), 2) / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
                value = lastResidual;
            }
        }
    } break;

    case TargetType::Auc :
    {
        double auc = -1.0;
        CycleStatistic cycleStatistic = statisticsCalculator.getStatistic(0, CycleStatisticType::AUC);
        bOk = cycleStatistic.getValue(dateTime, auc);

        if (bOk) {
            // Check if concentration is within the target range
            bOk = (auc < _target.m_valueMax) && (auc > _target.m_valueMin);
            if (bOk) {
                score = 1.0 - pow(log(auc) - log(_target.m_valueBest), 2) / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
                value = auc;
            }
        }
    } break;

    case TargetType::Auc24 :
    {
        double auc = -1.0;
        CycleStatistic cycleStatistic = statisticsCalculator.getStatistic(0, CycleStatisticType::AUC);
        bOk = cycleStatistic.getValue(dateTime, auc);

        double intervalInHours = 0.0;
        CycleStatistic cycleStatisticForCycleInterval = statisticsCalculator.getStatistic(0, CycleStatisticType::CycleInterval);
        bOk &= cycleStatisticForCycleInterval.getValue(dateTime, intervalInHours);


        if (bOk) {
            // Calculate the AUC on 24h
            double auc24 = auc * 24.0 / intervalInHours;

            // Check if concentration is within the target range
            bOk = (auc24 < _target.m_valueMax) && (auc24 > _target.m_valueMin);
            if (bOk) {
                score = 1.0 - pow(log(auc24) - log(_target.m_valueBest), 2) / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
                value = auc24;
            }
        }
    } break;

    case TargetType::CumulativeAuc :
    {
        double cumulativeAuc = 0.0;



        //Tucuxi::Core::CycleStatistics stats(cycleData, cumulativeAuc);
        for(std::size_t i = 0; i < _prediction.getTimes().size(); i++) {
            TimeOffsets times = _prediction.getTimes().at(i);
            DateTime start = _intakeSeries.at(i).getEventTime();
            DateTime end = start + std::chrono::milliseconds(static_cast<int>(times.at(times.size() - 1)) * 1000);
            CycleData cycle(start, end, Unit("ug/l"));
            cycle.addData(times, _prediction.getValues().at(i), 0);

            // The constructor accumulates in cumulativeAuc.
            CycleStatistics statisticsCalculator(cycle, cumulativeAuc);
        }

        // Check if cumulative AUC is within the target range
        bOk = (cumulativeAuc < _target.m_valueMax) && (cumulativeAuc > _target.m_valueMin);
        if (bOk) {
            score = 1.0 - pow(log(cumulativeAuc) - log(_target.m_valueBest), 2) / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
            value = cumulativeAuc;
        }
    } break;

    case TargetType::Mean :
    {
        double mean = -1.0;
        CycleStatistic cycleStatistic = statisticsCalculator.getStatistic(0, CycleStatisticType::Mean);
        bOk = cycleStatistic.getValue(dateTime, mean);

        if (bOk) {
            // Check if concentration is within the target range
            bOk = (mean < _target.m_valueMax) && (mean > _target.m_valueMin);
            if (bOk) {
                score = 1.0 - pow(log(mean) - log(_target.m_valueBest), 2) / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
                value = mean;
            }
        }
    } break;

    case TargetType::AucDividedByMic :
    {
        double auc = -1.0;
        CycleStatistic cycleStatistic = statisticsCalculator.getStatistic(0, CycleStatisticType::AUC);
        bOk = cycleStatistic.getValue(dateTime, auc);

        double intervalInHours = 0.0;
        CycleStatistic cycleStatisticForCycleInterval = statisticsCalculator.getStatistic(0, CycleStatisticType::CycleInterval);
        bOk &= cycleStatisticForCycleInterval.getValue(dateTime, intervalInHours);


        if (bOk) {
            // Calculate the AUC
            double aucDividedByMic = auc / _target.m_mic;

            // Check if concentration is within the target range
            bOk = (aucDividedByMic < _target.m_valueMax) && (aucDividedByMic > _target.m_valueMin);
            if (bOk) {
                score = 1.0 - pow(log(aucDividedByMic) - log(_target.m_valueBest), 2) / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
                value = aucDividedByMic;
            }
        }
    } break;

    case TargetType::Auc24DividedByMic :
    {
        double auc = -1.0;
        CycleStatistic cycleStatistic = statisticsCalculator.getStatistic(0, CycleStatisticType::AUC);
        bOk = cycleStatistic.getValue(dateTime, auc);

        double intervalInHours = 0.0;
        CycleStatistic cycleStatisticForCycleInterval = statisticsCalculator.getStatistic(0, CycleStatisticType::CycleInterval);
        bOk &= cycleStatisticForCycleInterval.getValue(dateTime, intervalInHours);


        if (bOk) {
            // Calculate the AUC on 24h
            double auc24 = auc * 24.0 / intervalInHours;
            double aucDividedByMic = auc24 / _target.m_mic;

            // Check if concentration is within the target range
            bOk = (aucDividedByMic < _target.m_valueMax) && (aucDividedByMic > _target.m_valueMin);
            if (bOk) {
                score = 1.0 - pow(log(aucDividedByMic) - log(_target.m_valueBest), 2) / pow(0.5 *(log(_target.m_valueMax) - log(_target.m_valueMin)), 2);
                value = aucDividedByMic;
            }
        }
    } break;

    case TargetType::AucOverMic :
    {
        return Result::EvaluationError;
    } break;

    case TargetType::Auc24OverMic :
    {
        return Result::EvaluationError;
    } break;

    case TargetType::TimeOverMic :
    {
        return Result::EvaluationError;
    } break;

    case TargetType::PeakDividedByMic :
    {
        return Result::EvaluationError;
    } break;

    case TargetType::UnknownTarget :
    {
        return Result::EvaluationError;

    } break;


    }

    if (!bOk) {
        return Result::InvalidCandidate;
    }

    // We build the result, as there was no error
    _result = TargetEvaluationResult(_target.m_targetType, score, translateToUnit(value, _target.m_unit, _target.m_finalUnit), _target.m_finalUnit);
    return Result::Ok;
}

} // namespace Core
} // namespace Tucuxi
