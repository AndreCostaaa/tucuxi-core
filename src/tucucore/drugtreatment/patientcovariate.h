#ifndef PATIENTCOVARIATE_H
#define PATIENTCOVARIATE_H

#include <vector>

#include "tucucommon/duration.h"

#include "tucucore/definitions.h"
#include "tucucore/timedevent.h"
#include "tucucore/operation.h"
#include "tucucore/operablegraphmanager.h"
#include "tucucore/drugdefinitions.h"

namespace Tucuxi {
namespace Core {

/// \brief Change of a covariate value for a patient.
class PatientCovariate : public TimedEvent
{
public:
    /// \brief Create a change of a covariate value for a patient.
    /// \param _id Identifier of the original covariate for which the change applies.
    /// \param _value Recorded value expressed in string form.
    /// \param _dataType Type of the data stored in the _value variable.
    /// \param _unit Unit of measure of the value.
    /// \param _date Time when the change happened.
    PatientCovariate(const std::string &_id, const std::string &_value, const DataType _dataType,
                     const Unit _unit, DateTime _date)
        : TimedEvent(_date), m_id{_id}, m_value{_value}, m_dataType{_dataType}, m_unit{_unit} {}

    /// \brief Comparison (equality) operator.
    /// param _rhs PatientCovariate to compare to.
    bool operator==(const PatientCovariate &_rhs) const
    {
        return  this->getId() == _rhs.getId() &&
                this->getValue() == _rhs.getValue() &&
                this->getDataType() == _rhs.getDataType() &&
                this->getEventTime() == _rhs.getEventTime();
    }

    /// \brief Comparison (difference) operator.
    /// param _rhs PatientCovariate to compare to.
    bool operator!=(const PatientCovariate &_rhs) const
    {
        return !(*this == _rhs);
    }

    /// \brief Class' output operator.
    /// \param _output Output stream.
    /// \param _pv Self reference to the PatientCovariate to print.
    /// \return Output stream given as input (for output chaining).
    friend std::ostream& operator<<(std::ostream &_output, const PatientCovariate &_pv) {
        _output << "Patient Variate:\n\tID = " << _pv.getId()
                << "\n\tValue = " << _pv.getValue()
                << "\n\tDataType = " << static_cast<int>(_pv.getDataType())
                << "\n\tEventTime = " << _pv.getEventTime() << "\n";
        return _output;
    }

    /// \brief Return the identifier of the value.
    /// \return Value's identifier.
    std::string getId() const { return m_id;}

    /// \brief Get the value (as string).
    /// \return Returns the value.
    std::string getValue() const { return m_value; }

    /// \brief Get the data type.
    /// \return Data type.
    DataType getDataType() const { return m_dataType; }

    /// \brief Set the time the event happened.
    /// \return Time of the event.
    void setEventTime(const DateTime &_time) { m_time = _time; }

    /// \brief Get the data's unit of measure.
    /// \return Data's unit of measure.
    Unit getUnit() const { return m_unit; }

protected:
    /// \brief Identifier of the original covariate for which the change applies.
    std::string m_id;
    /// \brief Recorded value of the change.
    std::string m_value;
    /// \brief Type of the data stored.
    DataType m_dataType;
    /// \brief Unit of measure of the value.
    Unit m_unit;
};

/// \brief List of patient variates.
typedef std::vector<std::unique_ptr<PatientCovariate>> PatientVariates;

/// \brief Iterator in the list of patient variates.
typedef std::vector<std::unique_ptr<PatientCovariate>>::const_iterator pvIterator_t;

} // namespace Core
} // namespace Tucuxi

#endif // PATIENTCOVARIATE_H
