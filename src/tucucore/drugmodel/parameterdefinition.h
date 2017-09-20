/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef PARAMETERDEFINITION_H
#define PARAMETERDEFINITION_H

#include <string>
#include <vector>
#include <memory>

#include "tucucore/definitions.h"
#include "tucucore/timedevent.h"
#include "tucucore/operation.h"
#include "tucucommon/general.h"
#include "tucucore/drugdefinitions.h"

namespace Tucuxi {
namespace Core {


enum class ParameterVariabilityType
{
    Additive,
    Proportional,
    Exponential,
    None
};

class ParameterVariability
{
public:
//    ParameterVariability() : m_variabilityType(ParameterVariabilityType::None), m_value(0) {}
//    ParameterVariability(ParameterVariabilityType _type) : m_variabilityType(_type), m_value(0) {}
    ParameterVariability(ParameterVariabilityType _type = ParameterVariabilityType::None, Value _value = 0.0) : m_variabilityType(_type), m_value(_value) {}

    ParameterVariabilityType m_variabilityType;
    Value m_value;

};

/// \ingroup TucuCore
/// \brief Represents a pharmacokinetics parameter.
class ParameterDefinition : public PopulationValue
{
public:

public:
    /// \brief Constructor
    /// \param _id The name of the parameter
    /// \param _name It's default value
    ParameterDefinition(const std::string _id, Value _value, ParameterVariability _variabilityType)
        : PopulationValue(_id, _value, nullptr),
          m_variability(_variabilityType)
    {}

    ParameterDefinition(const std::string _id, Value _value)
        : PopulationValue(_id, _value, nullptr)
    {}


    ParameterDefinition(const std::string _id, Value _value, ParameterVariabilityType _variabilityType)
        : PopulationValue(_id, _value, nullptr),
          m_variability(ParameterVariability(_variabilityType))
    {}

    ParameterDefinition(const std::string _name, Value _value, Operation* _operation, ParameterVariability _variabilityType)
        : PopulationValue(_name, _value, _operation),
          m_variability(_variabilityType)
    {}

    /// \brief Get the parameter value
    /// \return Returns the parameter value
    //Value getValue() const { return m_value; }

    const std::string& getName() const { return m_id; }
    bool isVariable() const { return m_variability.m_variabilityType != ParameterVariabilityType::None; }
    ParameterVariability getVariability() const { return m_variability; }

private:

    ParameterVariability m_variability;
    Unit m_unit;

};

/// \brief A list of parameters
typedef std::vector<std::unique_ptr<ParameterDefinition> > ParameterDefinitions;



class Correlation
{
public:
    Correlation(std::string _parameter1, std::string _parameter2, Value _correlation) :
        m_correlation(_correlation)
    {
        m_parameterId.push_back(_parameter1);
        m_parameterId.push_back(_parameter2);
    }

protected:

    Value m_correlation;
    std::vector<std::string> m_parameterId;
};

typedef std::vector<Correlation> Correlations;

class InterParameterSetCorrelation
{
public:

    InterParameterSetCorrelation(
            std::string _analyteSetId1,
            std::string _analyteSetId2,
            std::string _parameterId1,
            std::string _parameterId2,
            Value _correlation) :
        m_correlation(_correlation)
    {
        m_analyteSetId.push_back(_analyteSetId1);
        m_analyteSetId.push_back(_analyteSetId2);
        m_parameterId.push_back(_parameterId1);
        m_parameterId.push_back(_parameterId2);
    }

protected:

    Value m_correlation;
    std::vector<std::string> m_analyteSetId;
    std::vector<std::string> m_parameterId;
};

typedef std::vector<InterParameterSetCorrelation> InterParameterSetCorrelations;



class ParameterSetDefinition
{
public:

    void addParameter(std::unique_ptr<ParameterDefinition> & _parameter) { m_parameters.push_back(std::move(_parameter));}

    void addAnalyteId(std::string _analyteId) { m_analyteIds.push_back(_analyteId);}

    void addCorrelation(Correlation _correlation) { m_correlations.push_back(_correlation);}


protected:

    ParameterDefinitions m_parameters;
    Correlations m_correlations;
    std::vector<std::string> m_analyteIds;
};


}
}


#endif // PARAMETERDEFINITION_H