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
#include "tucucore/drugmodel/timeconsiderations.h"
#include "tucucore/drugmodel/drugmodelmetadata.h"
#include "tucucore/invariants.h"

struct TestConstantEliminationBolus;
struct TestMultiAnalytesMultiActiveMoieties;

#ifdef DRUGMODELTESTS
class Drugs2Manager;
#endif // DRUGMODELTESTS

namespace Tucuxi {
namespace Core {


typedef std::vector< std::unique_ptr<AnalyteSet>> AnalyteSets;

class DrugModel;
class DrugModelChecker;

///
/// \brief The ParameterDefinitionIterator class
/// This class is the only way to access Pk parameters from the DrugModel.
///     /// It ensures the parameters are ordered correctly:
/// - first the variable parameters, then the fixed parameters
/// - Within a categoy, alphabetical order is respected.
///
/// For instance, with Ka fixed, F fixed, CL variable, V variable, the iterator will give:
/// 1. CL
/// 2. V
/// 3. F
/// 4. Ka
///
class ParameterDefinitionIterator : public Tucuxi::Common::Iterator<const ParameterDefinition*>
{
public:
    ParameterDefinitionIterator(const DrugModel &_model, const AnalyteGroupId& _analyteGroupId, const Formulation &_formulation, const AdministrationRoute _route)
        : m_model(_model), m_analyteGroupId(_analyteGroupId), m_formulation(_formulation), m_route(_route)
    {
        build();
    }

    ParameterDefinitionIterator(const DrugModel &_model, const AnalyteGroupId& _analyteGroupId, const std::vector<const FullFormulationAndRoute*> &_formulation)
        : m_model(_model), m_analyteGroupId(_analyteGroupId), m_formulation(Formulation::Undefined), m_route(AdministrationRoute::Undefined), m_fullFormulationAndRoutes(_formulation)
    {
        build();
    }

    ParameterDefinitionIterator(const ParameterDefinitionIterator& _right)
        : m_model(_right.m_model), m_analyteGroupId(_right.m_analyteGroupId), m_formulation(_right.m_formulation), m_route(_right.m_route), m_index(_right.m_index)
    {
        build();
    }


    const ParameterDefinition* operator*() override;
    Tucuxi::Common::Iterator<const ParameterDefinition*>& next() override
    {
        m_index++;
        return *this;
    }
    bool isDone() const override;
    void reset() override { m_index = 0; }

private:

    void build();

    const DrugModel &m_model;
    const AnalyteGroupId m_analyteGroupId;
    const Formulation m_formulation;
    const AdministrationRoute m_route;
    const std::vector<const FullFormulationAndRoute*> m_fullFormulationAndRoutes;
    size_t m_index;
    size_t m_total;

    std::vector<const ParameterSetDefinition * > m_absorptionParameters;


    typedef struct {
        std::string id;
        bool isVariable;
    } ParameterInfo;

    std::vector<ParameterInfo> m_parametersVector;
};

class DrugModel
{

    INVARIANTS(
            INVARIANT(Invariants::INV_DRUGMODEL_0001, (m_drugId.size() > 0))
            INVARIANT(Invariants::INV_DRUGMODEL_0002, (m_drugModelId.size() > 0))
            INVARIANT(Invariants::INV_DRUGMODEL_0003, (m_analyteSets.size() > 0))
            LAMBDA_INVARIANT(Invariants::INV_DRUGMODEL_0004, {bool ok = true;for(size_t i = 0; i < m_analyteSets.size(); i++) {ok &= m_analyteSets.at(i)->checkInvariants();} return ok;})
            INVARIANT(Invariants::INV_DRUGMODEL_0005, (m_domain != nullptr))
            INVARIANT(Invariants::INV_DRUGMODEL_0006, (m_domain->checkInvariants()))
            LAMBDA_INVARIANT(Invariants::INV_DRUGMODEL_0007, {bool ok = true;for(size_t i = 0; i < m_covariates.size(); i++) {ok &= m_covariates.at(i)->checkInvariants();} return ok;})
            INVARIANT(Invariants::INV_DRUGMODEL_0008, (m_formulationAndRoutes != nullptr))
            INVARIANT(Invariants::INV_DRUGMODEL_0009, (m_formulationAndRoutes->checkInvariants()))
            LAMBDA_INVARIANT(Invariants::INV_DRUGMODEL_0010, {bool ok = true;for(size_t i = 0; i < m_interParameterSetCorrelations.size(); i++) {ok &= m_interParameterSetCorrelations.at(i).checkInvariants();} return ok;})
            INVARIANT(Invariants::INV_DRUGMODEL_0011, (m_activeMoieties.size() > 0))
            LAMBDA_INVARIANT(Invariants::INV_DRUGMODEL_0012, {bool ok = true;for(size_t i = 0; i < m_activeMoieties.size(); i++) {ok &= m_activeMoieties.at(i)->checkInvariants();} return ok;})
            INVARIANT(Invariants::INV_DRUGMODEL_0013, (m_timeConsiderations != nullptr))
            INVARIANT(Invariants::INV_DRUGMODEL_0014, (m_timeConsiderations->checkInvariants()))
            // No invariant on m_metadata

            )

public:
    DrugModel();

    void setDrugId(std::string _drugId) { m_drugId = _drugId;}
    void setDrugModelId(std::string _drugModelId) { m_drugModelId = _drugModelId;}

    std::string getDrugId() const { return m_drugId;}
    std::string getDrugModelId() const { return m_drugModelId;}

    void setTimeToSteadyState(Tucuxi::Common::Duration _time);

    Tucuxi::Common::Duration getTimeToSteadyState() const;

    void addCovariate(std::unique_ptr<CovariateDefinition> _covariate);

    const CovariateDefinitions& getCovariates() const;

    const FormulationAndRoutes& getFormulationAndRoutes() const;

    ParameterDefinitionIterator getParameterDefinitions(const AnalyteGroupId& _analyteGroupId, const Formulation &_formulation, const AdministrationRoute _route) const
    {
        ParameterDefinitionIterator iterator(*this, _analyteGroupId, _formulation, _route);
        return iterator;
    }

    ParameterDefinitionIterator getParameterDefinitions(const AnalyteGroupId& _analyteGroupId, const std::vector<const FullFormulationAndRoute*> &_formulation) const
    {
        ParameterDefinitionIterator iterator(*this, _analyteGroupId, _formulation);
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

    void addFormulationAndRoute(std::unique_ptr<FullFormulationAndRoute> _formulationAndRoute, bool _isDefault = false);

    void setDomain(std::unique_ptr<DrugModelDomain> _domain);
    const DrugModelDomain& getDomain() const { return *m_domain.get();}

    void setMetadata(std::unique_ptr<DrugModelMetadata> _data);
    const DrugModelMetadata& getMetadata() const { return *m_metadata.get();}

    void addAnalyteSet(std::unique_ptr<AnalyteSet> _analyteSet);

    const AnalyteSets& getAnalyteSets() const { return m_analyteSets;}

    void addActiveMoiety(std::unique_ptr<ActiveMoiety> _activeMoiety);

    const ActiveMoieties& getActiveMoieties() const { return m_activeMoieties;}

    void setTimeConsiderations(std::unique_ptr<TimeConsiderations> _timeConsiderations)
    {
        m_timeConsiderations = std::move(_timeConsiderations);
    }

    void setFormulationAndRoutes(std::unique_ptr<FormulationAndRoutes> _formulationAndRoutes)
    {
        m_formulationAndRoutes = std::move(_formulationAndRoutes);
    }

    const TimeConsiderations &getTimeConsiderations() const { return *m_timeConsiderations;}

    const AnalyteSet* getAnalyteSet(const AnalyteGroupId &_analyteGroupId = "") const {
        if ((_analyteGroupId == "") && (m_analyteSets.size() == 1)) {
            return m_analyteSets.at(0).get();
        }

        // TODO : Maybe get rid of this:
        if (_analyteGroupId == "") {
            return m_analyteSets.at(0).get();
        }


        for (const std::unique_ptr<AnalyteSet> &set : m_analyteSets) {
            if (set->getId() == _analyteGroupId) {
                return set.get();
            }
        }
        return nullptr;
    }

    bool isSingleAnalyte() const
    {
        return (m_activeMoieties.size() == 1) && (m_analyteSets.size() == 1) && (m_analyteSets[0]->getAnalytes().size() == 1);
    }


private:

    const FullFormulationAndRoute* getFormulationAndRoute(const Formulation &_formulation, const AdministrationRoute _route) const {
        return m_formulationAndRoutes->get(_formulation, _route);
    }

    const FullFormulationAndRoute* getFormulationAndRoute(const FormulationAndRoute &_formulation) const {
        return m_formulationAndRoutes->get(_formulation);
    }

    const ParameterSetDefinition* getAbsorptionParameters(const AnalyteGroupId &_analyteGroupId, const Formulation &_formulation, const AdministrationRoute _route) const {
        const FullFormulationAndRoute* fr = getFormulationAndRoute(_formulation, _route);
        if (fr != nullptr) {
            return fr->getParameterDefinitions(_analyteGroupId);
        }
        return nullptr;
    }

    const ParameterSetDefinition* getAbsorptionParameters(const AnalyteGroupId &_analyteGroupId, const FormulationAndRoute &_formulation) const {
        const FullFormulationAndRoute* fr = getFormulationAndRoute(_formulation);
        if (fr != nullptr) {
            return fr->getParameterDefinitions(_analyteGroupId);
        }
        return nullptr;
    }

    const ParameterSetDefinition* getDispositionParameters(const AnalyteGroupId &_analyteGroupId) const {
        const AnalyteSet* pSet = getAnalyteSet(_analyteGroupId);
        if (pSet != nullptr) {
            return &pSet->getDispositionParameters();
        }
        return nullptr;
    }

private:

    std::string m_drugId;

    std::string m_drugModelId;

    Tucuxi::Common::Duration m_timeToSteadyState;

    AnalyteSets m_analyteSets;

    std::unique_ptr<DrugModelDomain> m_domain;

    std::unique_ptr<FormulationAndRoutes> m_formulationAndRoutes;

    CovariateDefinitions m_covariates;

    InterParameterSetCorrelations m_interParameterSetCorrelations;

    ActiveMoieties m_activeMoieties;

    std::unique_ptr<TimeConsiderations> m_timeConsiderations;

    std::unique_ptr<DrugModelMetadata> m_metadata;

    friend ParameterDefinitionIterator;
    friend DrugModelChecker;
    friend TestConstantEliminationBolus;
    friend TestMultiAnalytesMultiActiveMoieties;
#ifdef DRUGMODELTESTS
    friend Drugs2Manager;
#endif // DRUGMODELTESTS
};

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_DRUGMODEL_H
