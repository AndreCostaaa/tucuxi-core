/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_CORE_DOSAGE_H
#define TUCUXI_CORE_DOSAGE_H

#include <iostream>
#include <memory>
#include <vector>

#include "tucucore/definitions.h"
#include "tucucore/intakeevent.h"
#include "tucucore/drugmodel/formulationandroute.h"

#include "tucucommon/datetime.h"
#include "tucucommon/duration.h"
#include "tucucommon/timeofday.h"

using Tucuxi::Common::DateTime;
using Tucuxi::Common::Duration;
using Tucuxi::Common::TimeOfDay;

using namespace std::chrono_literals;

namespace Tucuxi {
namespace Core {

class IntakeExtractor;

/// \brief An intake series is list of intake events.
/// This vector could be replaced in the future with a class that offers more capabilities (for instance, it could
/// incorporate the time interval spanned by the series). However, it will have to expose the same capabilities as the
/// underlying std::vector, thus the transition will be painless.
typedef std::vector<IntakeEvent> IntakeSeries;

void cloneIntakeSeries(const std::vector<IntakeEvent> &_input, std::vector<IntakeEvent> &_output);


void selectRecordedIntakes(
        IntakeSeries &_selectionSeries, const IntakeSeries &_intakeSeries,
        DateTime _recordFrom, DateTime _recordTo);


/// \brief Implement the extract and clone operations for Dosage subclasses.
#define DOSAGE_UTILS(BaseClassName, ClassName) \
    friend IntakeExtractor; \
    int extract(IntakeExtractor &_extractor, const DateTime &_start, const DateTime &_end, double _nbPointsPerHour, IntakeSeries &_series) const override; \
    std::unique_ptr<BaseClassName> clone() const override \
    { \
        return std::unique_ptr<BaseClassName>(new ClassName(*this)); \
    } \
    std::unique_ptr<Dosage> cloneDosage() const override \
    { \
        return clone(); \
    }


/// \ingroup TucuCore
/// \brief Abstract class at the base of the dosage class hierarchy.
/// Dosages are implemented as either bounded or unbounded dosages.
class Dosage
{
public:

    virtual ~Dosage() {}

    /// \brief Extract drugs' intake in a given time interval (visitor pattern).
    /// The intakes will be added to the series passed as a sequence
    /// \param _extractor Intake extractor.
    /// \param _start Start time/date for the considered interval.
    /// \param _end End time/date for the considered interval, could be unset.
    /// \param _nbPointsPerHour Expected density of points in number of points per hour.
    /// \param _series Returned series of intake in the considered interval.
    /// \return number of intakes in the given interval, -1 in case of error.
    ///
    /// \pre _start IS NOT unset
    /// \pre _end IS unset OR _end > _start
    /// \post _series[OUT] = _series[IN] + extracted_intakes
    /// (it would have been easier to simply empty the input _series, but this guarantees an uniform behavior across the
    /// whole set of calls)
    /// \post FORALL intake IN extracted_intakes, intake.time IN [_start, _end)
    /// \see IntakeExtractor::extract()
    virtual int extract(IntakeExtractor &_extractor, const DateTime &_start, const DateTime &_end, double _nbPointsPerHour, IntakeSeries &_series) const = 0;

    /// \brief Return a pointer to a clone of the correct subclass.
    /// \return Pointer to a new object of subclass' type.
    virtual std::unique_ptr<Dosage> cloneDosage() const = 0;

    ///
    /// \brief Returns the last formulation and route of the dosage
    /// \return  The last formulation and route of the dosage
    /// TODO : A test should be written for that
    ///
    virtual FormulationAndRoute getLastFormulationAndRoute() const = 0;
};


/// \ingroup TucuCore
/// \brief Pure virtual class upon which dosage loops are based.
class DosageUnbounded : public Dosage
{
public:

    ~DosageUnbounded() override {}

    /// \brief Return a pointer to a clone of the correct subclass.
    /// \return Pointer to a new object of subclass' type.
    virtual std::unique_ptr<DosageUnbounded> clone() const = 0;
};

/// \ingroup TucuCore
/// \brief Pure virtual class upon which all implemented dosages are based.
class DosageBounded : public Dosage
{
public:
    friend IntakeExtractor;

    ~DosageBounded() override {}

    int extract(IntakeExtractor &_extractor, const DateTime &_start, const DateTime &_end,double _nbPointsPerHour, IntakeSeries &_series) const override;

    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the interval.
    /// \return Time of the first intake.
    virtual DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const = 0;

    /// \brief Get the time step between two bounded dosages.
    /// This time step could represent the time interval between two weekly doses (in that case, a week), or the time
    /// taken by a more complex sequence (represented in this case by a DosageSequence), ...
    /// \return Time step size.
    virtual Duration getTimeStep() const = 0;

    /// \brief Return a pointer to a clone of the correct subclass.
    /// \return Pointer to a new object of subclass' type.
    virtual std::unique_ptr<DosageBounded> clone() const = 0;
};


/// \brief A list of bounded dosages
typedef std::vector<std::unique_ptr<DosageBounded>> DosageBoundedList;

/// \ingroup TucuCore
/// \brief List of bounded dosages that are repeated in time.
/// The bounds are imposed by the time range embedding the dosage loop. This class is created to ensure that no
/// unbounded history is part of another dosage loop (creating an infinite loop).
class DosageLoop : public DosageUnbounded
{
public:
    /// \brief Construct a dosage loop from a bounded dosage.
    /// \param _dosage Dosage to add to the loop.
    DosageLoop(const DosageBounded &_dosage)
    {
        m_dosage = _dosage.clone();
    }


    /// \brief Copy-construct a dosage loop.
    /// \param _other Dosage loop to clone.
    DosageLoop(const DosageLoop &_other) : DosageUnbounded(_other)
    {
        m_dosage = _other.m_dosage->clone();
    }

    ~DosageLoop() override {}

    FormulationAndRoute getLastFormulationAndRoute() const override
    {
        return m_dosage->getLastFormulationAndRoute();
    }

    const DosageBounded * getDosage() const {
        return m_dosage.get();
    }

    DOSAGE_UTILS(DosageUnbounded, DosageLoop);

protected:
    /// \brief Dosage that is repeated in the unbounded interval.
    std::unique_ptr<DosageBounded> m_dosage;
};

/// \ingroup TucuCore
/// \brief A single dosage at steady state.
/// In addition to the dose, this object stores the time of the last dose. While it
/// could be considered to be purely the last one, actually any dose is OK. So,
/// there is no real restriction on what dose to choose.
/// The time range pointing to a steady state dosage does not need to have a start
/// and an end time. It can, however, have an end time, in case it is followed by
/// another dosage. Therefore, a DosageSteadyState needs to be the first dosage
/// in a DosageHistory, but can be followed by other dosages.
class DosageSteadyState : public DosageLoop
{
public:

    /// \brief Construct a dosage loop from a bounded dosage.
    /// \param _dosage Dosage to add to the loop.
    DosageSteadyState(const DosageBounded &_dosage, DateTime _lastDoseTime) : DosageLoop (_dosage)
    {
        m_lastDoseTime = _lastDoseTime;
    }

    /// \brief Copy-construct a dosage loop.
    /// \param _other Dosage loop to clone.
    DosageSteadyState(const DosageSteadyState &_other) : DosageLoop (_other)
    {
        m_lastDoseTime = _other.m_lastDoseTime;
    }

    ~DosageSteadyState() override {}

    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the first element of the sequence.
    /// \return Time of the first intake immediately following _intervalStart.
    DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const
    {

        double intervalInSeconds = static_cast<double>(m_dosage->getTimeStep().toSeconds());

        double intervalToFromLastDose = (m_lastDoseTime - _intervalStart).toSeconds();

        // nbIntervals rounds the number of intervals to reach _intervalStart from the last dose
        int nbIntervals = static_cast<int>(intervalToFromLastDose / intervalInSeconds);

        DateTime start = m_lastDoseTime - Duration(std::chrono::seconds(static_cast<int>(intervalInSeconds * nbIntervals)));
        if (start < _intervalStart)
            start = start + m_dosage->getTimeStep();

        return start;
    }

    DOSAGE_UTILS(DosageUnbounded, DosageSteadyState);

private:
    DateTime m_lastDoseTime;

};

/// \ingroup TucuCore
/// \brief Dosage that is administered a given number of times.
class DosageRepeat : public DosageBounded
{
public:
    /// \brief Construct a repeated dosage starting from a dosage and the number of times it is repeated.
    /// \param _dosage Dosage to repeat.
    /// \param _nbTimes Number of times the dosage has to be repeated.
    DosageRepeat(const DosageBounded &_dosage, const unsigned int _nbTimes)
    {
        m_dosage = _dosage.clone();
        m_nbTimes = _nbTimes;
    }

    /// \brief Copy-construct a dosage repetition.
    /// \param _other Dosage repetition to clone.
    DosageRepeat(const DosageRepeat &_other) : DosageBounded(_other)
    {
        m_dosage = _other.m_dosage->clone();
        m_nbTimes = _other.m_nbTimes;
    }

    ~DosageRepeat() override {}

    FormulationAndRoute getLastFormulationAndRoute() const override
    {
        return m_dosage->getLastFormulationAndRoute();
    }

    DOSAGE_UTILS(DosageBounded, DosageRepeat);

    /// \brief Get the time step between two bounded dosages.
    /// \return Time step of the whole repeated sequence.
    Duration getTimeStep() const override
    {
        return m_dosage->getTimeStep() * m_nbTimes;
    }

    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the repeated sequence.
    /// \return Time of the first intake.
    DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const override
    {
        return m_dosage->getFirstIntakeInterval(_intervalStart);
    }

    const DosageBounded * getDosage() const {
        return m_dosage.get();
    }

private:
    /// \brief Dosage that is repeated.
    std::unique_ptr<DosageBounded> m_dosage;

    /// \brief Number of times a dosage is repeated.
    unsigned int m_nbTimes;
};

/// \ingroup TucuCore
/// \brief Unordered sequence of dosages that is administered in a bounded interval.
/// This class can be used, for instance, to represent a sequence of three daily doses in three different days.
class DosageSequence : public DosageBounded
{
public:
    /// \brief Create a dosage sequence taking at least a dosage (this prevents having an empty sequence).
    /// \param _dosage Dosage to add.
    DosageSequence(const DosageBounded &_dosage)
    {
        m_dosages.emplace_back(_dosage.clone());
    }

    /// \brief Copy-construct a dosage sequence.
    /// \param _other Dosage sequence to clone.
    DosageSequence(const DosageSequence &_other) : DosageBounded(_other)
    {
        for (auto&& dosage: _other.m_dosages) {
            m_dosages.emplace_back(dosage->clone());
        }
    }

    ~DosageSequence() override {}

    /// \brief Add a dosage to the sequence.
    /// \param _dosage Dosage to add.
    void addDosage(const DosageBounded &_dosage)
    {
        m_dosages.emplace_back(_dosage.clone());
    }

    FormulationAndRoute getLastFormulationAndRoute() const override
    {
        if (m_dosages.size() == 0) {
            return FormulationAndRoute(Formulation::Undefined, AdministrationRoute::Undefined, AbsorptionModel::UNDEFINED);
        }
        return m_dosages.at(m_dosages.size() - 1)->getLastFormulationAndRoute();
    }

    DOSAGE_UTILS(DosageBounded, DosageSequence);

    /// \brief Get the time step between two bounded dosages.
    /// \return Time step of the whole repeated sequence.
    Duration getTimeStep() const override
    {
        Duration totalDuration;
        for (auto&& dosage : m_dosages) {
            totalDuration += dosage->getTimeStep();
        }
        return totalDuration;
    }

    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the first element of the sequence (it is ordered).
    /// \return Time of the first intake.
    DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const override
    {
        return (m_dosages.at(0))->getFirstIntakeInterval(_intervalStart);
    }

private:
    /// \brief Sequence of bounded dosages that is administered.
    DosageBoundedList m_dosages;
};

typedef std::vector<Duration> TimeOffsetList;

/// \ingroup TucuCore
/// \brief Unordered sequence of dosages, with a common starting point and an absolute offset, that evolve in parallel.
/// This class can be used, for instance, to represent a sequence of three different doses that have to be administered
/// in the same day, at different moments, but with a daily periodicization (that is, at XX:XX dose A, at YY:YY dose B,
/// and at ZZ:ZZ dose C, and this every day).
class ParallelDosageSequence : public DosageBounded
{
public:
    /// \brief Create a dosage sequence taking at least a dosage and an absolute offset (this prevents having an empty
    /// sequence).
    /// \param _dosage Dosage to add.
    /// \param _offset Offset of the dosage.
    ParallelDosageSequence(const DosageBounded &_dosage, const Duration &_offset)
    {
        m_dosages.emplace_back(_dosage.clone());
        m_offsets.emplace_back(_offset);
    }


    /// \brief Copy-construct a parallel dosage sequence.
    /// \param _other Dosage sequence to clone.
    ParallelDosageSequence(const ParallelDosageSequence &_other) : DosageBounded(_other)
    {
        for (auto&& dosage: _other.m_dosages) {
            m_dosages.emplace_back(dosage->clone());
        }
        for (auto& offset: _other.m_offsets) {
            m_offsets.emplace_back(offset);
        }
    }

    ~ParallelDosageSequence() override {}

    /// \brief Add a dosage to the sequence, along with its offset with respect to the beginnning of the sequence.
    /// \param _dosage Dosage to add.
    /// \param _offset Offset of the dosage.
    void addDosage(const DosageBounded &_dosage, const Duration &_offset)
    {
        m_dosages.emplace_back(_dosage.clone());
        m_offsets.emplace_back(_offset);
    }

    FormulationAndRoute getLastFormulationAndRoute() const override
    {
        if (m_dosages.size() == 0) {
            return FormulationAndRoute(Formulation::Undefined, AdministrationRoute::Undefined, AbsorptionModel::UNDEFINED);
        }
        return m_dosages.at(m_dosages.size() - 1)->getLastFormulationAndRoute();
    }

    DOSAGE_UTILS(DosageBounded, ParallelDosageSequence);


    /// \brief Get the time step between two bounded dosages.
    /// \return Time step of the whole repeated sequence.
    Duration getTimeStep() const override
    {
        Duration totalDuration;
        // We have to consider the time it takes for the dosage with the biggest time step
        for (size_t i = 0; i < m_dosages.size(); ++i) {
            if (m_offsets.at(i) + m_dosages.at(i)->getTimeStep() > totalDuration) {
                totalDuration = m_offsets.at(i) + m_dosages.at(i)->getTimeStep();
            }
        }
        return totalDuration;
    }


    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the first element of the sequence.
    /// \return Time of the first intake in the ordered list.
    DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const override
    {
        // We consider that a ParallelSequence starts immediately
        return _intervalStart;
    }

private:
    /// \brief Sequence of bounded dosages that is administered.
    DosageBoundedList m_dosages;
    /// \brief Corresponding sequence of offsets.
    TimeOffsetList m_offsets;
};


/// \ingroup TucuCore
/// \brief Abstract class specifying a specific intake.
class SingleDose : public DosageBounded
{
public:
    /// \brief Initialize a single dose from its components.
    /// \param _dose Quantity of drug administered.
    /// \param _route Route of administration.
    /// \param _infusionTime Duration in case of an infusion.
    /// \pre _dose >= 0
    /// \pre IF _routeOfAdministration == AbsorptionModel::INFUSION THEN (!_infusionTime.isEmpty() && _infusionTime > 0)
    SingleDose(const DoseValue &_dose,
               const FormulationAndRoute &_routeOfAdministration,
               const Duration &_infusionTime) :
        m_routeOfAdministration(_routeOfAdministration)
    {
        if (_dose < 0) {
            throw std::invalid_argument("Dose value = " + std::to_string(_dose) + " is invalid (must be >= 0).");
        }
        if (_routeOfAdministration.getAbsorptionModel() == AbsorptionModel::INFUSION && _infusionTime.isNegative()) {
            throw std::invalid_argument("Infusion time for INFUSION is invalid (must be >= 0).");
        }
        // Let's tolerate infusion time 0
//        if (_routeOfAdministration.getAbsorptionModel() == AbsorptionModel::INFUSION && _infusionTime.isEmpty()) {
//            throw std::invalid_argument("Route of administration is INFUSION, but empty infusion time specified.");
//        }
        m_dose = _dose;
        m_infusionTime = _infusionTime;
    }

    /// \brief Empty destructor, used to make the class abstract (and thus force the instantiation of subclasses).
    // virtual ~SingleDose() = 0;
    ~SingleDose() override {};

    FormulationAndRoute getLastFormulationAndRoute() const override
    {
        return m_routeOfAdministration;
    }


    Duration getInfusionTime() const
    {
        return m_infusionTime;
    }

    DoseValue getDose() const
    {
        return m_dose;
    }

protected:
    /// \brief Administered dose.
    DoseValue m_dose;
    /// \brief Route of administration.
    FormulationAndRoute m_routeOfAdministration;
    /// \brief Duration in case of an infusion.
    Duration m_infusionTime;
};

/// \ingroup TucuCore
/// \brief Dose supposed to last for a certain, specified time interval.
/// When used in a DosageLoop, a LastingDose object allows to represent a dosage history with fixed intervals.
class LastingDose : public SingleDose
{
public:
    /// \brief Initialize a lasting dose from its components.
    /// \param _dose Quantity of drug administered.
    /// \param _route Route of administration.
    /// \param _infusionTime Duration in case of an infusion.
    /// \param _interval Interval between two doses.
    /// \pre !_interval.isEmpty() && _interval > 0
    LastingDose(const DoseValue &_dose,
                const FormulationAndRoute &_routeOfAdministration,
                const Duration &_infusionTime,
                const Duration &_interval)
        : SingleDose(_dose, _routeOfAdministration, _infusionTime)
    {
        if (_interval.isEmpty()) {
            throw std::invalid_argument("Interval between two doses not specified (empty).");
        }
        if (_interval <= Duration(std::chrono::seconds(0))) {
            throw std::invalid_argument("Interval is invalid (must be >= 0).");
        }
        m_interval = _interval;
    }

    DOSAGE_UTILS(DosageBounded, LastingDose);


    /// \brief Copy-construct a dosage repetition.
    /// \param _other Dosage repetition to clone.
    LastingDose(const LastingDose &_other) : SingleDose (_other)
    {
        m_interval = _other.m_interval;
    }

    ~LastingDose() override {}

    /// \brief Return the increment between two successive intakes.
    /// \return Interval between two lasting doses.
    Duration getTimeStep() const override
    {
        return m_interval;
    }

    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the interval.
    /// \return Time of the first intake.
    DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const override
    {
        return _intervalStart;
    }

private:
    /// \brief Interval between two doses.
    Duration m_interval;
};

/// \ingroup TucuCore
/// \brief Dose taken every day at the same time.
class DailyDose : public SingleDose
{
public:
    /// \brief Create a class representing a weekly dose.
    /// \param _dose Quantity of drug administered.
    /// \param _route Route of administration.
    /// \param _infusionTime Duration in case of an infusion.
    /// \param _timeOfDay Time of the day when the dose is administered.
    DailyDose(const DoseValue &_dose,
              const FormulationAndRoute &_routeOfAdministration,
              const Duration &_infusionTime,
              const TimeOfDay &_timeOfDay)
        : SingleDose(_dose, _routeOfAdministration, _infusionTime)
    {
        m_timeOfDay = _timeOfDay;
    }

    ~DailyDose() override {}

    DOSAGE_UTILS(DosageBounded, DailyDose);

    /// \brief Return the increment between two successive intakes.
    /// \return Interval between two daily doses.
    Duration getTimeStep() const override
    {
        return Duration(std::chrono::hours(24));
    }

    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the interval.
    /// \return Time of the first intake.
    DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const override
    {
        DateTime intakeTime = _intervalStart;
        // The hour is fixed
        intakeTime.setTimeOfDay(m_timeOfDay);

        // If the beginning of the interval is past the intake time, then move to the following day
        if (_intervalStart.getTimeOfDay() > m_timeOfDay) {
            intakeTime += Duration(24h);
        }

        return intakeTime;
    }

protected:
    /// \brief Time of the day when the dose is administered.
    TimeOfDay m_timeOfDay;
};

/// \ingroup TucuCore
/// \brief Dose taken every week on a specific day of the week.
class WeeklyDose : public DailyDose
{
public:
    /// \brief Create a class representing a weekly dose.
    /// \param _dose Quantity of drug administered.
    /// \param _route Route of administration.
    /// \param _infusionTime Duration in case of an infusion.
    /// \param _timeOfDay Time of the day when the dose is administered.
    /// \param _dayOfWeek Day of the week the dose has to be administered.
    WeeklyDose(const DoseValue &_dose,
               const FormulationAndRoute &_routeOfAdministration,
               const Duration &_infusionTime,
               const TimeOfDay &_timeOfDay,
               const DayOfWeek &_dayOfWeek)
        : DailyDose(_dose, _routeOfAdministration, _infusionTime, _timeOfDay)
    {
        if (!_dayOfWeek.ok()) {
            throw std::invalid_argument("Invalid day of week specified for weekly dose.");
        }
        m_dayOfWeek = _dayOfWeek;
    }

    ~WeeklyDose() override {}

    DOSAGE_UTILS(DosageBounded, WeeklyDose);

    /// \brief Return the increment between two successive intakes.
    /// \return Interval between two weekly doses.
    Duration getTimeStep() const override
    {
        return Duration(std::chrono::hours(7 * 24));
    }

    /// \brief Return the instant of the first intake in the given interval.
    /// \param _intervalStart Starting point of the interval.
    /// \return Time of the first intake.
    DateTime getFirstIntakeInterval(const DateTime &_intervalStart) const override
    {
        // Really ugly cast required by the library to extract the day of week from the date
        const int numStartDayOfWeek = (unsigned)(DayOfWeek(_intervalStart.getDate()));
        const int numPlannedDayOfWeek = (unsigned)m_dayOfWeek;
        int dayDiff = numPlannedDayOfWeek - numStartDayOfWeek;

        if (dayDiff < 0) {
            // The intake was supposed to be some days before, we have to advance by the days that are missing to
            // encompass a full week time
            dayDiff = 7 + dayDiff;

        } else if (dayDiff == 0 && _intervalStart.getTimeOfDay() > m_timeOfDay) {
            // We just missed the intake, we have to advance by one week
            dayDiff = 7;
        }

        DateTime intakeTime = _intervalStart;
        intakeTime += Duration(std::chrono::hours(24 * dayDiff));
        intakeTime.setTimeOfDay(m_timeOfDay);

        return intakeTime;
    }

protected:
    /// \brief Day of the week the dose has to be administered.
    DayOfWeek m_dayOfWeek;
};

class DosageTimeRange;
/// \brief A list of dosage time range
typedef std::vector<std::unique_ptr<DosageTimeRange>> DosageTimeRangeList;

class DosageHistory;

/// \ingroup TucuCore
/// \brief Doses administered in a given time interval (which might have no upper bound).
class DosageTimeRange
{
    friend IntakeExtractor;
    friend DosageHistory;

public:
    /// \brief Initialize an open-ended list of doses (the end date is not specified).
    /// \param _startDate Interval's starting date.
    /// \param _dosage Administered dosage.
    /// \pre !_startDate.isUndefined()
    /// \post m_startDate == _startDate
    /// \post _endDate.isUndefined()
    DosageTimeRange(const DateTime &_startDate, const Dosage &_dosage) : m_startDate(_startDate)
    {
        if (_startDate.isUndefined()) {
            throw std::invalid_argument("Undefined start date for a time range specified.");
        }
        m_endDate.reset();
        m_dosage = _dosage.cloneDosage();
    }

    /// \brief Copy-construct a time range.
    /// \param _other Time range to clone.
    DosageTimeRange(const DosageTimeRange &_other) : m_startDate(_other.m_startDate), m_endDate(_other.m_endDate)
    {
        m_dosage = _other.m_dosage->cloneDosage();
        m_addedIntakes = _other.m_addedIntakes;
        m_skippedIntakes = _other.m_skippedIntakes;
    }

    /// \brief Initialize an list of doses (the end date is specified, though it could be unset).
    /// \param _startDate Interval's starting date.
    /// \param _endDate Interval's ending date.
    /// \param _dosage Administered dosage.
    /// \pre !_startDate.isUndefined()
    /// \post m_startDate == _startDate
    /// \post m_endDate == _endDate
    DosageTimeRange(const DateTime& _startDate, const DateTime& _endDate, const Dosage &_dosage)
        : m_startDate(_startDate), m_endDate(_endDate)
    {
        if (m_startDate.isUndefined()) {
            throw std::invalid_argument("Undefined start date for a time range specified.");
        }
        if (!m_endDate.isUndefined() && m_startDate > m_endDate) {
            throw std::invalid_argument("The start date for a time range has to precede the end date.");
        }
        m_dosage = _dosage.cloneDosage();
    }

    /// \brief Add a change in a scheduled intake.
    /// \param _intake Intake to either mark as skipped or add (if unplanned).
    /// \param _operation Operation to perform
    /// \return 0 if the change has been successfully recorded, -1 otherwise.
    int addIntakeChange(const IntakeEvent &_intake, const ScheduledIntakeOp &_operation)
    {
        switch (_operation) {
        case ScheduledIntakeOp::SKIP:
            m_skippedIntakes.push_back(_intake);
            break;
        case ScheduledIntakeOp::ADD:
            m_addedIntakes.push_back(_intake);
            break;
        default:
            std::cerr << "Unimplemented operation!\n";
            return -1;
        }
        return 0;
    }

    /// \brief Getter for the start date of the interval.
    /// \return Start date of the interval.
    DateTime getStartDate() const
    {
        return m_startDate;
    }

    /// \brief Getter for the end date of the interval.
    /// \return End date of the interval.
    DateTime getEndDate() const
    {
        return m_endDate;
    }

    /// \brief Check a time range against a list of time ranges for overlaps.
    /// \param _timeRange Time range to check.
    /// \param _timeRangeList List of time ranges to compare against.
    /// \return true if the given time range overlaps with at least one time range of the list, false otherwise.
    friend bool timeRangesOverlap(const DosageTimeRange &_timeRange, const DosageTimeRangeList &_timeRangeList);

    /// \brief Check if two time ranges overlap.
    /// \param _first First time range to check.
    /// \param _second Second time range to check.
    /// \return true if the two time ranges overlap, false otherwise.
    friend bool timeRangesOverlap(const DosageTimeRange &_first, const DosageTimeRange &_second);

    const Dosage* getDosage() const {
        return m_dosage.get();
    }

private:
    /// Doses administered in the interval.
    std::unique_ptr<Dosage> m_dosage;
    /// Start date of the interval. Must be a valid date.
    DateTime m_startDate;
    /// End date of the interval. Can be unset.
    DateTime m_endDate;
    /// Added unscheduled intakes in the given time range
    IntakeSeries m_addedIntakes;
    /// Skipped scheduled intakes in the given time range
    IntakeSeries m_skippedIntakes;
};

/// \ingroup TucuCore
/// \brief Represents a dosage history.
/// A dosage history represents every intake of a drug treatment, dealing with multiple intake scenarios.
class DosageHistory
{
    friend IntakeExtractor;

public:
    /// \brief Create a new empty dosage history.
    DosageHistory() { }

    ///
    /// \brief DosageHistory copy constructor
    /// \param obj original DosageHistory object
    ///
    /// TODO : A test for this function needs to be written
    DosageHistory( const DosageHistory &obj)
    {
        for (const auto& timeRange : obj.m_history) {
            this->addTimeRange(*timeRange.get());
        }
    }

    ///
    /// \brief DosageHistory copy constructor
    /// \param obj original DosageHistory object
    ///
    /// TODO : A test for this function needs to be written
    DosageHistory( const DosageHistory &&obj)
    {
        for (const auto& timeRange : obj.m_history) {
            this->addTimeRange(*timeRange.get());
        }
    }

    ///
    /// \brief operator = assignement operator
    /// \param other original DosageHistory object
    /// \return The modified DosageHistory
    ///
    /// TODO : A test for this function needs to be written
    DosageHistory& operator=(DosageHistory other)
    {
        this->m_history.clear();
        for (const auto& timeRange : other.m_history) {
            this->addTimeRange(*timeRange.get());
        }
        return *this;
    }

    ///
    /// \brief isEmpty Indicates if the dosage history is empty
    /// \return true if the dosage history is empty, false else
    ///
    bool isEmpty() const
    {
        return (m_history.size() == 0);
    }



    /// \brief Add a time range to the history.
    /// \param _timeRange Time range to add.
    void addTimeRange(const DosageTimeRange &_timeRange)
    {
        m_history.emplace_back(new DosageTimeRange(_timeRange));
    }

    ///
    /// \brief Clone the dosage history
    /// \return The clone
    /// TODO : A test for this function needs to be written
    ///
    DosageHistory *clone() const
    {
        DosageHistory *newDosageHistory = new DosageHistory();
        for (const auto& timeRange : m_history) {
            newDosageHistory->addTimeRange(*timeRange.get());
        }
        return newDosageHistory;
    }

    ///
    /// \brief mergeDosage Add a new dosage and modifies the existing
    /// \param newDosage The new dosage to be added
    /// This function will modify the existing dosages in order to
    /// replace them by the new one in case of overlapping.
    /// TODO : A test for this function needs to be written
    ///
    void mergeDosage(const DosageTimeRange *newDosage);

    FormulationAndRoute getLastFormulationAndRoute() const;

    const DosageTimeRangeList& getDosageTimeRanges() const {
        return m_history;
    }

private:
    /// Usage history, expressed as a list of non-overlapping time intervals. The last one might have no end date (if
    /// the interval is open, that is, up to present date).
    DosageTimeRangeList m_history;
};

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_DOSAGE_H
