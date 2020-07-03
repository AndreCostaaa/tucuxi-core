#include "unit.h"

#include <iostream>

#include "tucucommon/loggerhelper.h"


namespace Tucuxi {
namespace Common {

bool Unit::isTime() const
{
    return ((m_unitString == "d") || (m_unitString == "h") || (m_unitString == "m") || (m_unitString == "s"));
}

bool Unit::isEmpty() const {
    return m_unitString == "";
}


std::string Unit::toString() const {
    return m_unitString;
}

const std::map<UnitManager::UnitType, std::map<std::string, double>>& UnitManager::getConversionMap()
{
    static const std::map<UnitType, std::map<std::string, double>> sm_conversionMap =
    {
        {
            UnitType::Weight,
            {
                {"kg", 1000.0},
                {"g", 1.0},
                {"mg", 0.001},
                {"ug", 0.000001}
            }
        },
        {
            UnitType::Concentration,
            {
                {"g/l", 1.0},
                {"mg/l", 1000.0},
                {"ug/l", 1000000.0},
                {"g/ml", 0.001},
                {"mg/ml", 1.0},
                {"ug/ml", 1000.0}

            }
        },
        {
            UnitType::ConcentrationTime,
            {
                {"ug*h/l", 1.0},
                {"mg*h/l", 1000.0},
                {"h*mg/l", 1000.0},
                {"h*ug/l", 1.0},
                {"h*g/l", 1000000.0},
                {"g*h/l", 1000000.0},
                {"mg*min/l", 1000.0 / 60.0},
                {"min*mg/l", 1000.0 / 60.0},
                {"g*min/l", 10000000.0 / 60.0},
                {"min*g/l", 10000000.0 / 60.0},
                {"ug*min/l", 1.0 / 60.0},
                {"min*ug/l", 1.0 / 60.0},

                {"ug*h/ml", 1000.0},
                {"mg*h/ml", 1000000.0},
                {"h*mg/ml", 1000000.0},
                {"h*ug/ml", 1000.0},
                {"h*g/ml", 1000000000.0},
                {"g*h/ml", 1000000000.0},
                {"mg*min/ml", 1000000.0 / 60.0},
                {"min*mg/ml", 1000000.0 / 60.0},
                {"g*min/ml", 10000000000.0 / 60.0},
                {"min*g/ml", 10000000000.0 / 60.0},
                {"ug*min/ml", 1.0 / 60000.0},
                {"min*ug/ml", 1.0 / 60000.0}
            }
        },
        {
            UnitType::MoleConcentration,
            {
                {"mol/l", 1.0},
                {"mmol/l", 0.001},
                {"umol/l", 0.000001},
                {"mol/ml", 1000},
                {"mmol/ml", 1.0},
                {"umol/ml", 0.001}
            }
        },
        {
            UnitType::Time,
            {
                {"min", 1.0},
                {"s", 1 / 60.0},
                {"h", 60.0},
                {"d", 24.0 * 60.0},
                {"m", 30.0 * 24.0 * 60.0},
                {"y", 12.0 * 30.0 * 24.0 * 60.0}
            }
        },
        {
            UnitType::Height,
            {
                {"m", 1.0},
                {"cm", 0.01},
                {"dm", 0.1},
                {"mm", 0.001},
                {"ft", 0.3048},
                {"in", 0.0254}
            }
        },
        {
            UnitType::NoUnit,
            {
                {"-", 1.0}
            }
        }
    };

    return sm_conversionMap;
}

void UnitManager::logConversionError(const Unit &_initialUnit, const Unit &_finalUnit)
{
    Tucuxi::Common::LoggerHelper logHelper;
    logHelper.error("Error in unit conversion. No known conversion from {} to {}", _initialUnit.toString(), _finalUnit.toString());
}

double UnitManager::convertToUnit(double _value, Unit _initialUnit, Unit _finalUnit)
{
    std::string initialKey = _initialUnit.toString();
    std::string finalKey = _finalUnit.toString();
    for(const auto &map : getConversionMap())
    {
        if ((map.second.count(initialKey) != 0) && (map.second.count(finalKey) != 0))
        {
            return _value * map.second.at(initialKey) / map.second.at(finalKey);
        }
    }

    logConversionError(_initialUnit, _finalUnit);
    throw std::invalid_argument("Error in unit conversion");

    // TODO : Iterate over the conversion map to see if we find the initial and final unit strings.
    // If yes and they are of the same type, then conversion is applied,
    // Else throw an std::invalid_argument

}

bool UnitManager::isKnown(const Unit& _unit)
{
    std::string key = _unit.toString();
    for(const auto &map : getConversionMap()){
        if (map.second.count(key) != 0){
            return true;
        }
    }
    return false;
}



} // namespace Common
} // namespace Tucuxi
