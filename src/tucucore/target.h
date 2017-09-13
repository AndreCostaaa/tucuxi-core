/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_CORE_TARGETEVENT_H
#define TUCUXI_CORE_TARGETEVENT_H

#include <vector>

#include "tucucore/timedevent.h"
#include "tucucore/drugdefinitions.h"
#include "tucucore/drugmodel/targetdefinition.h"

namespace Tucuxi {
namespace Core {


class SubTarget : public IndividualValue<SubTargetDefinition>
{
public:

    SubTarget(const SubTargetDefinition &_def) :
        IndividualValue<SubTargetDefinition>(_def)
    {}

};

class Target
{
public:
    Target(const TargetDefinition& _targetDef) :
        m_targetDefinition(_targetDef),
        m_valueMin(_targetDef.getCMin()),
        m_valueMax(_targetDef.getCMin()),
        m_valueBest(_targetDef.getCMin()),
        m_tMin(_targetDef.getCMin()),
        m_tMax(_targetDef.getCMin()),
        m_tBest(_targetDef.getCMin())
    {}

protected:

    const TargetDefinition& m_targetDefinition;

    SubTarget m_valueMin;
    SubTarget m_valueMax;
    SubTarget m_valueBest;
    SubTarget m_tMin;
    SubTarget m_tMax;
    SubTarget m_tBest;

};


typedef std::vector<Target*> Targets;

class TargetEvent : public TimedEvent
{
};

typedef std::vector<TargetEvent> TargetSeries;
}
}

#endif // TUCUXI_CORE_TARGETEVENT_H
