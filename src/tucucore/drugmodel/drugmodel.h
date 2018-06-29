/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_CORE_DRUGMODEL_H
#define TUCUXI_CORE_DRUGMODEL_H

#include "tucucommon/iterator.h"

#include "tucucore/covariateevent.h"
#include "tucucore/drugmodel/parameterdefinition.h"
#include "tucucore/drugmodel/drugmodeldomain.h"
#include "tucucore/drugmodel/formulationandroute.h"
#include "tucucore/drugmodel/activemoiety.h"

namespace Tucuxi {
namespace Core {


typedef std::vector< std::unique_ptr<AnalyteSet>> AnalyteSets;

class DrugModel;
class ParameterDefinitionIterator : public Tucuxi::Common::Iterator<const ParameterDefinition*>
{
public:
    ParameterDefinitionIterator(const DrugModel &_model, const std::string& _analyteId, const std::string &_formulation, const Route _route) 
        : m_model(_model), m_analyteId(_analyteId), m_formulation(_formulation), m_route(_route)
    {
    }
    ParameterDefinitionIterator(const ParameterDefinitionIterator& _right)
        : m_model(_right.m_model), m_analyteId(_right.m_analyteId), m_formulation(_right.m_formulation), m_route(_right.m_route), m_index(_right.m_index)
    {
    }
    const ParameterDefinition* operator*() override;
    Tucuxi::Common::Iterator<const ParameterDefinition*>& next()
    {
        m_index++;
        return *this;
    }
    bool isDone() const override;
    void reset() override { m_index = 0; }

private:
    const DrugModel &m_model;
    const std::string m_analyteId;
    const std::string m_formulation;
    const Route m_route;
    size_t m_index;
};


class DrugModel
{
public:
    DrugModel();

    void setTimeToSteadyState(Tucuxi::Common::Duration _time);

    Tucuxi::Common::Duration getTimeToSteadyState() const;

    void addCovariate(std::unique_ptr<CovariateDefinition> _covariate);

    const CovariateDefinitions& getCovariates() const;

    const FormulationAndRoutes& getFormulationAndRoutes() const;

    ParameterDefinitionIterator getParameterDefinitions(const std::string& _analyteId, const std::string &_formulation, const Route _route) const
    {
        ParameterDefinitionIterator iterator(*this, _analyteId, _formulation, _route);
        return iterator;
    }

    ///
    /// \brief getParameters
    /// Returns a full set of parameters, adding the corresponding absorption parameters to the
    /// elimination parameters
    /// \param _formulationAndRoute
    /// \return
    ///
    //const ParameterDefinitions& getParameters(FormulationAndRoute _formulationAndRoute) const;

    void addFormulationAndRoute(std::unique_ptr<FormulationAndRoute> _formulationAndRoute);
    
    void setDomain(std::unique_ptr<DrugModelDomain> _domain);

    void setAnalyteSet(std::unique_ptr<AnalyteSet> _analyteSet);

    std::string getPkModelId() const {
        if (m_analyteSets.size() > 0) {
            return m_analyteSets.at(0)->getPkModelId();
        }
        return "";
    }

    void addActiveMoiety(std::unique_ptr<ActiveMoiety> _activeMoiety);

private:
    const AnalyteSet* getAnalyteSet(const std::string &_analyteId) const {
        for (const std::unique_ptr<AnalyteSet> &set : m_analyteSets) {
            if (set->getId() == _analyteId) {
                return set.get();
            }
        }
        return nullptr;
    }

    const FormulationAndRoute* getFormulationAndRoute(const std::string &_formulation, const Route _route) const {
        return m_formulationAndRoutes.get(_formulation, _route);
    }

    const ParameterSetDefinition* getAbsorptionParameters(const std::string &_analyteId, const std::string &_formulation, const Route _route) const {
        const FormulationAndRoute* fr = getFormulationAndRoute(_formulation, _route);
        if (fr != nullptr) {
            return fr->getParameterDefinitions(_analyteId);
        }
        return nullptr;
    }

    const ParameterSetDefinition* getDispositionParameters(const std::string &_analyteId) const {
        const AnalyteSet* pSet = getAnalyteSet("" /*_analyteId*/);
        if (pSet != nullptr) {
            return &pSet->getDispositionParameters();
        }
        return nullptr;
    }

private:

    Tucuxi::Common::Duration m_timeToSteadyState;

    AnalyteSets m_analyteSets;

    std::unique_ptr<DrugModelDomain> m_domain;

    FormulationAndRoutes m_formulationAndRoutes;

    CovariateDefinitions m_covariates;

    InterParameterSetCorrelations m_interParameterSetCorrelations;

    ActiveMoieties m_activeMoieties;

    friend ParameterDefinitionIterator;
};

}
}

#endif // TUCUXI_CORE_DRUGMODEL_H
