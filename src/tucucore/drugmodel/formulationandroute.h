/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TUCUXI_TUCUCORE_FORMULATIONANDROUTE_H
#define TUCUXI_TUCUCORE_FORMULATIONANDROUTE_H

#include <string>
#include <vector>

#include "tucucore/definitions.h"
#include "tucucore/drugmodel/parameterdefinition.h"
#include "tucucore/drugmodel/activesubstance.h"
#include "tucucore/drugmodel/validdose.h"
#include "tucucore/drugmodel/validduration.h"
#include "tucucore/invariants.h"

namespace Tucuxi {
namespace Core {

class DrugModelChecker;

enum class Formulation
{
    Undefined,
    ParenteralSolution,
    OralSolution,
    Test
};


enum class AdministrationRoute
{

    Undefined,
    Intramuscular,
    IntravenousBolus,
    IntravenousDrip,
    Nasal,
    Oral,
    Rectal,
    Subcutaneous,
    Sublingual,
    Transdermal,
    Vaginal
};

class StandardTreatment
{
public:
    StandardTreatment(bool _isFixedDuration = false, Value _duration = 0.0, Unit _unit = Unit("h")):
        m_isFixedDuration(_isFixedDuration), m_duration(_duration), m_unit(_unit) {}

    bool getIsFixedDuration() const { return m_isFixedDuration;}
    //Tucuxi::Common::Duration getDuration() const { return m_duration;}
    Value getDuration() const { return m_duration;}
    Unit getUnit() const { return m_unit;}

    INVARIANTS(
            INVARIANT(Invariants::INV_STANDARDTREATMENT_0001, ((!m_isFixedDuration) || (m_duration > 0.0)))
            INVARIANT(Invariants::INV_STANDARDTREATMENT_0002, ((!m_isFixedDuration) || (m_unit.isTime())))
            )

protected:
    bool m_isFixedDuration;
    Value m_duration;
    Unit m_unit;
};

class AnalyteSetToAbsorptionAssociation
{
public:
    AnalyteSetToAbsorptionAssociation(const AnalyteSet & _analyteSet) :
        m_analyteSet(_analyteSet)
    {}

    void setAbsorptionModel(AbsorptionModel _absorptionModel) { m_absorptionModel = _absorptionModel;}
    void setAbsorptionParameters(std::unique_ptr<ParameterSetDefinition> _parameters) {m_absorptionParameters = std::move(_parameters);}

    INVARIANTS(
            INVARIANT(Invariants::INV_ANALYTESETTOABSORPTIONASSOCIATION_0001, (m_absorptionModel != AbsorptionModel::Undefined))
            INVARIANT(Invariants::INV_ANALYTESETTOABSORPTIONASSOCIATION_0002, (m_absorptionParameters->checkInvariants()))
            )

protected:
    std::unique_ptr<ParameterSetDefinition> m_absorptionParameters;
    const AnalyteSet &m_analyteSet;

    ///
    /// \brief m_absorptionModel Computation absorption model
    /// This variable is an enum class and is closely related to the available Pk models implemented in the software
    AbsorptionModel m_absorptionModel;

    friend class FullFormulationAndRoute;
};

class AnalyteConversion
{
public:
    AnalyteConversion(std::string _analyteId, Value _factor) :
        m_analyteId(_analyteId), m_factor(_factor)
    {}

    AnalyteId getAnalyteId() const { return m_analyteId;}

    Value getFactor() const { return m_factor;}

    INVARIANTS(
            INVARIANT(Invariants::INV_ANALYTECONVERSION_0001, (m_factor >= 0.0))
            INVARIANT(Invariants::INV_ANALYTECONVERSION_0002, (m_analyteId.size() > 0))
            )

    protected:

    AnalyteId m_analyteId;
    Value m_factor;
};

class ForumulationAndRoutes;

class FormulationAndRoute
{
public:

    // Construction for testing purpose
    FormulationAndRoute(AbsorptionModel _absorptionModel) :
        m_formulation(Formulation::Undefined), m_route(AdministrationRoute::Undefined), m_absorptionModel(_absorptionModel),
        m_administrationName("")
    {}

    FormulationAndRoute(
            Formulation _formulation,
            AdministrationRoute _route,
            AbsorptionModel _absorptionModel,
            std::string _administrationName = "") :
        m_formulation(_formulation), m_route(_route), m_absorptionModel(_absorptionModel),
        m_administrationName(_administrationName)
    {}


    Formulation getFormulation() const { return m_formulation;}

    AdministrationRoute getAdministrationRoute() const { return m_route;}

    AbsorptionModel getAbsorptionModel() const { return m_absorptionModel;}

    std::string getAdministrationName() const { return m_administrationName;}

    friend bool operator==(const FormulationAndRoute& _v1, const FormulationAndRoute& _v2)
    {
        return (_v1.m_absorptionModel == _v2.m_absorptionModel) &&
                (_v1.m_route == _v2.m_route);
    }


    /// \brief Is the duration smaller?
    bool operator<(const FormulationAndRoute &_f) const
    {
        if (m_formulation < _f.m_formulation)
            return true;
        if (m_route < _f.m_route)
            return true;
        if (m_absorptionModel < _f.m_absorptionModel)
            return true;
        if (m_administrationName < _f.m_administrationName)
            return true;
        return false;
    }

    INVARIANTS(
            INVARIANT(Invariants::INV_FORMULATIONANDROUTE_0001, (m_formulation != Formulation::Undefined))
            INVARIANT(Invariants::INV_FORMULATIONANDROUTE_0001, (m_route != AdministrationRoute::Undefined))
            INVARIANT(Invariants::INV_FORMULATIONANDROUTE_0001, (m_absorptionModel != AbsorptionModel::Undefined))
            INVARIANT(Invariants::INV_FORMULATIONANDROUTE_0001, (m_administrationName.size() > 0))
            )

protected:


    /// Formulation, based on an Enum type
    Formulation m_formulation;

    /// Route of administration, based on an Enum type
    AdministrationRoute m_route;
    ///
    /// \brief m_absorptionModel Computation absorption model
    /// This variable is an enum class and is closely related to the available Pk models implemented in the software
    AbsorptionModel m_absorptionModel;

    /// Administration name, as a free text field
    std::string m_administrationName;
};

std::vector<FormulationAndRoute> mergeFormulationAndRouteList(const std::vector<FormulationAndRoute> &_v1,
                                                              const std::vector<FormulationAndRoute> &_v2);



class FullFormulationAndRoute
{
public:

    FullFormulationAndRoute(const FormulationAndRoute& _specs, std::string _id)
        : m_id(_id), m_specs(_specs)
    {}

    void setValidDoses(std::unique_ptr<ValidDoses> _validDoses) {m_validDoses = std::move(_validDoses);}
    void setValidIntervals(std::unique_ptr<ValidDurations> _validIntervals) {m_validIntervals = std::move(_validIntervals);}
    void setValidInfusionTimes(std::unique_ptr<ValidDurations> _validInfusionTimes) {m_validInfusionTimes = std::move(_validInfusionTimes);}
    void addAssociation(std::unique_ptr< AnalyteSetToAbsorptionAssociation > _association) {m_associations.push_back(std::move(_association));}

    void setStandardTreatment(std::unique_ptr<StandardTreatment> _standardTreatment) {m_standardTreatment = std::move(_standardTreatment);}

    void addAnalyteConversion(std::unique_ptr<AnalyteConversion> _analyteConversion) { m_analyteConversions.push_back(std::move(_analyteConversion));}
    const std::vector<std::unique_ptr<AnalyteConversion> >& getAnalyteConversions() const { return m_analyteConversions;}
    const AnalyteConversion *getAnalyteConversion(AnalyteId _analyteId) const {
        for (const auto &analyteConversion : m_analyteConversions) {
            if (analyteConversion->getAnalyteId() == _analyteId) {
                return analyteConversion.get();
            }
        }
        return nullptr;
    }

    const ParameterSetDefinition* getParameterDefinitions(const AnalyteGroupId &_analyteGroupId) const;

    std::string getId() const { return m_id;}

    const ValidDoses* getValidDoses() const { return m_validDoses.get();}

    const ValidDurations* getValidIntervals() const { return m_validIntervals.get();}
    const ValidDurations* getValidInfusionTimes() const { return m_validInfusionTimes.get();}
    const FormulationAndRoute& getFormulationAndRoute() const { return m_specs;}

    const StandardTreatment* getStandardTreatment() const { return m_standardTreatment.get();}

    INVARIANTS(
            INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0001, (m_id.size() > 0))
            INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0002, (m_validDoses != nullptr))
            INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0003, (m_validDoses->checkInvariants()))
            INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0004, (m_validIntervals != nullptr))
            INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0005, (m_validIntervals->checkInvariants()))
            // INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0006, (m_validInfusionTimes != nullptr))
            LAMBDA_INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0007, {bool ok = true;if (m_validInfusionTimes != nullptr) { ok &= m_validInfusionTimes->checkInvariants();} return ok;})
            LAMBDA_INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0008, {bool ok = true;if (m_standardTreatment != nullptr) { ok &= m_standardTreatment->checkInvariants();} return ok;})
            // To check : do we need at least one association?
            LAMBDA_INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0009, {bool ok = true;for(size_t i = 0; i < m_associations.size(); i++) {ok &= m_associations.at(i)->checkInvariants();} return ok;})
            INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0010, (m_analyteConversions.size() > 0))
            LAMBDA_INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0011, {bool ok = true;for(size_t i = 0; i < m_analyteConversions.size(); i++) {ok &= m_analyteConversions.at(i)->checkInvariants();} return ok;})
            INVARIANT(Invariants::INV_FULLFORMULATIONANDROUTE_0012, (m_specs.checkInvariants()))
            )

protected:

    /// A unique Id, useful when a DrugModel embeds more than one Formulation
    std::string m_id;

    FormulationAndRoute m_specs;

    std::vector<std::unique_ptr<AnalyteConversion> > m_analyteConversions;

    std::unique_ptr<ValidDoses> m_validDoses;
    std::unique_ptr<ValidDurations> m_validIntervals;
    std::unique_ptr<ValidDurations> m_validInfusionTimes;

    std::vector<std::unique_ptr< AnalyteSetToAbsorptionAssociation > > m_associations;

    std::unique_ptr<StandardTreatment> m_standardTreatment;

    friend class FormulationAndRoutes;
    friend class DrugModelChecker;
};

///
/// \brief The FormulationAndRoutes class
/// This class embeds a vector of FormulationAndRoute objects. It also offers a mechanism to identify the default to be used.
///
class FormulationAndRoutes
{
public:

    ///
    /// \brief FormulationAndRoutes Empty constructor
    ///
    FormulationAndRoutes();

    ///
    /// \brief add Adds a new formulation and route to the set
    /// \param _far A unique pointer to the formulation and route
    /// \param isDefault If true, then the newly added is the default formulation and route, else not.
    /// If there is a single formulation and route in the set, then the single one is the default.
    ///
    void add(std::unique_ptr<FullFormulationAndRoute> _far, bool _isDefault = false);

    ///
    /// \brief get Get a formulation and route object based on the formulation and the route
    /// \param _formulation Formulation to look for
    /// \param _route Route to look for
    /// \return A pointer to a FormulationAndRoute, nullptr if not in the set
    ///
    const FullFormulationAndRoute* get(const Formulation& _formulation, AdministrationRoute _route) const;

    ///
    /// \brief get Get a formulation and route object based on the formulation and the route
    /// \param _formulation Formulation and route to look for
    /// \return A pointer to a FullFormulationAndRoute, nullptr if not in the set
    ///
    const FullFormulationAndRoute* get(const FormulationAndRoute& _formulation) const;

    ///
    /// \brief getDefault Get the default formulation and route object.
    /// \return  The default formulation and route, nullptr if the set is empty
    ///
    const FullFormulationAndRoute* getDefault() const;

    ///
    /// \brief getList Get a reference to the vector of formulation and routes
    /// \return A vector of Formulation and routes
    ///
    const std::vector<std::unique_ptr<FullFormulationAndRoute> >& getList() const;


    ///
    /// \brief Iterator Definition of an iterator for the FormulationAndRoute objects
    ///
    typedef std::vector<std::unique_ptr<FullFormulationAndRoute> >::const_iterator Iterator;

    ///
    /// \brief begin returns an iterator on the beginning of the list
    /// \return The iterator on the beginning of the list
    ///
    Iterator begin() const { return m_fars.begin(); }

    ///
    /// \brief end returns an iterator on the end of the list
    /// \return The iterator on the end of the list
    ///
    Iterator end() const { return m_fars.end(); }



    INVARIANTS(
            INVARIANT(Invariants::INV_FORMULATIONANDROUTE_0001, (m_fars.size() > 0))
            LAMBDA_INVARIANT(Invariants::INV_FORMULATIONANDROUTE_0002, {bool ok = true;for(size_t i = 0; i < m_fars.size(); i++) {ok &= m_fars.at(i)->checkInvariants();} return ok;})
            INVARIANT(Invariants::INV_FORMULATIONANDROUTE_0003, (m_defaultIndex < m_fars.size()))
            )

private:

    //! Vector of formulation and routes
    std::vector<std::unique_ptr<FullFormulationAndRoute> > m_fars;

    //! Index of the default formulation and route. 0 by default
    std::size_t m_defaultIndex;

    friend DrugModelChecker;
};

} // namespace Core
} // namespace Tucuxi


#endif // TUCUXI_TUCUCORE_FORMULATIONANDROUTE_H
