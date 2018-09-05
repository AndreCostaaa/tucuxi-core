/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_CORE_TARGET_H
#define TUCUXI_CORE_TARGET_H

#include <vector>

#include "tucucore/timedevent.h"
#include "tucucore/drugdefinitions.h"
#include "tucucore/drugmodel/targetdefinition.h"
#include "tucucommon/duration.h"

namespace Tucuxi {
namespace Core {

class TargetExtractor;
class TargetEvaluator;

///
/// \brief A target defined within a DrugTreatment
/// It should be used to override the default DrugModel targets
///
class Target
{
public:
    Target() = delete;

    Target(const std::string& _activeMoietyId,
           TargetType _type, 
           Value _min, 
           Value _best, 
           Value _max);

    Target(const std::string& _activeMoietyId,
           TargetType _type, 
           Value _vmin, 
           Value _vbest, 
           Value _vmax,
           const Tucuxi::Common::Duration &_tmin, 
           const Tucuxi::Common::Duration &_tbest, 
           const Tucuxi::Common::Duration &_tmax);

    std::string getActiveMoietyId() const { return m_activeMoietyId;}

private:
    /// Id of the active moiety on which applying the target
    std::string m_activeMoietyId;

    /// Type of target
    TargetType m_targetType;

    /// Target minimum acceptable value
    Value m_valueMin;

    /// Target maximum acceptable value
    Value m_valueMax;

    /// Target best value
    Value m_valueBest;

    /// Target minimum time from last intake (for peak targets)
    Tucuxi::Common::Duration m_tMin;

    /// Target maximum time from last intake (for peak targets)
    Tucuxi::Common::Duration m_tMax;

    /// Target best time from last intake (for peak targets)
    Tucuxi::Common::Duration m_tBest;

    friend TargetExtractor;
    friend TargetEvaluator;
};

///
/// \brief Targets A simple vector of targets
///
typedef std::vector<std::unique_ptr<Target> > Targets;

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_TARGET_H
