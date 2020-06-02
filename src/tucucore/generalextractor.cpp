#include "generalextractor.h"

#include "tucucore/computingservice/computingrequest.h"
#include "tucucore/computingservice/icomputingservice.h"
#include "tucucore/sampleevent.h"
#include "tucucore/sampleextractor.h"
#include "tucucore/residualerrormodelextractor.h"
#include "tucucore/aposteriorietascalculator.h"
#include "tucucore/dosage.h"
#include "tucucore/parameter.h"
#include "tucucore/intakeextractor.h"
#include "tucucore/parametersextractor.h"
#include "tucucore/covariateextractor.h"
#include "tucucore/intaketocalculatorassociator.h"
#include "tucucore/overloadevaluator.h"
#include "tucucore/drugmodel/timeconsiderations.h"
#include "tucucore/drugdomainconstraintsevaluator.h"

using namespace Tucuxi::Common;

namespace Tucuxi {
namespace Core {

GeneralExtractor::GeneralExtractor()
= default;

Duration GeneralExtractor::secureStartDuration(const HalfLife &_halfLife)
{
    Duration duration;
    if (_halfLife.getUnit() == Unit("d")) {
        duration = Duration(24h) * _halfLife.getValue() * _halfLife.getMultiplier();
    }
    else if (_halfLife.getUnit() == Unit("h")) {
        duration = Duration(1h) * _halfLife.getValue() * _halfLife.getMultiplier();
    }
    else if (_halfLife.getUnit() == Unit("m")) {
        duration = Duration(1min) * _halfLife.getValue() * _halfLife.getMultiplier();
    }
    else if (_halfLife.getUnit() == Unit("s")) {
        duration = Duration(1s) * _halfLife.getValue() * _halfLife.getMultiplier();
    }
    return duration;
}

ComputingStatus GeneralExtractor::extractAposterioriEtas(
        Etas &_etas,
        const ComputingRequest &_request,
        const AnalyteGroupId& _analyteGroupId,
        const IntakeSeries &_intakeSeries,
        const ParameterSetSeries &_parameterSeries,
        const CovariateSeries &_covariateSeries,
        DateTime _calculationStartTime,
        DateTime _endTime)
{
    Tucuxi::Core::OmegaMatrix omega;

    std::vector<const FullFormulationAndRoute *> formulationAndRoutes = extractFormulationAndRoutes(_request.getDrugModel(), _intakeSeries);

    ComputingStatus omegaComputingResult = extractOmega(_request.getDrugModel(), _analyteGroupId, formulationAndRoutes, omega);
    if (omegaComputingResult != ComputingStatus::Ok) {
        return omegaComputingResult;
    }

    SampleSeries sampleSeries;
    SampleExtractor sampleExtractor;
    ComputingStatus sampleExtractionResult =
    sampleExtractor.extract(_request.getDrugTreatment().getSamples(), _request.getDrugModel().getAnalyteSet(_analyteGroupId), _calculationStartTime, _endTime, sampleSeries);

    if (sampleExtractionResult != ComputingStatus::Ok) {
        return sampleExtractionResult;
    }

    if (sampleSeries.empty()) {
        // Surprising. Something maybe wrong with the sample extractor

    }
    else {
        ResidualErrorModelExtractor errorModelExtractor;
        std::unique_ptr<IResidualErrorModel> residualErrorModel;
        ComputingStatus errorModelExtractionResult = errorModelExtractor.extract(_request.getDrugModel().getAnalyteSet(_analyteGroupId)->getAnalytes()[0]->getResidualErrorModel(),
                                                                                _request.getDrugModel().getAnalyteSet(_analyteGroupId)->getAnalytes()[0]->getUnit(),
                                                                                _covariateSeries, residualErrorModel);
        if (errorModelExtractionResult != ComputingStatus::Ok) {
            return errorModelExtractionResult;
        }

        APosterioriEtasCalculator etasCalculator;
        etasCalculator.computeAposterioriEtas(_intakeSeries, _parameterSeries, omega, *residualErrorModel, sampleSeries, _etas);

    }
    return ComputingStatus::Ok;
}

ComputingStatus GeneralExtractor::extractOmega(
        const DrugModel &_drugModel,
        const AnalyteGroupId& _analyteGroupId,
        std::vector<const FullFormulationAndRoute*> &_formulationAndRoutes,
        OmegaMatrix &_omega)
{
    ParameterDefinitionIterator it = _drugModel.getParameterDefinitions(_analyteGroupId, _formulationAndRoutes);

    int nbVariableParameters = 0;

    it.reset();
    while (!it.isDone()) {
        if ((*it)->getVariability().getType() != ParameterVariabilityType::None) {
            nbVariableParameters ++;
        }
        it.next();
    }

    _omega = Tucuxi::Core::OmegaMatrix(nbVariableParameters, nbVariableParameters);

    for(int x = 0; x < nbVariableParameters; x++) {
        for(int y = 0; y < nbVariableParameters; y++) {
            _omega(x,y) = 0.0;
        }
    }

    std::map<std::string, int> paramMap;
    int nbParameter = 0;
    it.reset();
    while (!it.isDone()) {
        if ((*it)->getVariability().getType() != ParameterVariabilityType::None) {
            // const ParameterDefinition *p = (*it);
            // Value v = (*it)->getVariability().getValue() ;
            _omega(nbParameter, nbParameter) = (*it)->getVariability().getValue() * (*it)->getVariability().getValue();
            paramMap[(*it)->getId()] = nbParameter;
            nbParameter ++;
        }
        it.next();
    }

    const AnalyteSet *analyteSet = _drugModel.getAnalyteSet(_analyteGroupId);
    Correlations correlations = analyteSet->getDispositionParameters().getCorrelations();
    for(auto & correlation : correlations) {
        std::string p1 = correlation.getParamId1();
        std::string p2 = correlation.getParamId2();
        Value correlationValue = correlation.getValue();

        int index1 = paramMap[p1];
        int index2 = paramMap[p2];

        Value covariance = correlationValue * (std::sqrt(_omega(index1, index1) * _omega(index2, index2)));
        _omega(index1, index2) = covariance;
        _omega(index2, index1) = covariance;
    }

    return ComputingStatus::Ok;
}

std::vector<const FullFormulationAndRoute*> GeneralExtractor::extractFormulationAndRoutes(const DrugModel &_drugModel, const IntakeSeries &_intakeSeries)
{
    std::vector<const FullFormulationAndRoute*> result;
    std::vector<FormulationAndRoute> allFormulationAndRoutes;
    for (const auto & intake : _intakeSeries) {
        FormulationAndRoute f = intake.getFormulationAndRoute();
        if (std::find(allFormulationAndRoutes.begin(), allFormulationAndRoutes.end(), f) == allFormulationAndRoutes.end()) {
            allFormulationAndRoutes.push_back(f);
        }
    }
    result.reserve(allFormulationAndRoutes.size());
    for (const auto & f : allFormulationAndRoutes) {
        result.push_back(_drugModel.getFormulationAndRoutes().get(f));
    }
    return result;
}

ComputingStatus GeneralExtractor::generalExtractions(const ComputingTraitStandard *_traits,
        const ComputingRequest &_request,
        const PkModelCollection *_modelCollection,
        std::map<AnalyteGroupId, std::shared_ptr<PkModel> > &_pkModel,
        GroupsIntakeSeries &_intakeSeries,
        CovariateSeries &_covariatesSeries,
        GroupsParameterSetSeries &_parameterSeries,
        DateTime &_calculationStartTime
        )
{


    const HalfLife &halfLife = _request.getDrugModel().getTimeConsiderations().getHalfLife();

    Duration fantomDuration = secureStartDuration(halfLife);

    Tucuxi::Common::DateTime firstEvent = _traits->getStart();

    if ((_traits->getComputingOption().getParametersType() == PredictionParameterType::Aposteriori)
            && (!_request.getDrugTreatment().getSamples().empty())) {
        for (const auto &sample : _request.getDrugTreatment().getSamples()) {
            if (sample->getDate() < firstEvent) {
                firstEvent = sample->getDate();
            }
        }
    }

    Tucuxi::Common::DateTime fantomStart = firstEvent - fantomDuration;

    _calculationStartTime = fantomStart;

    IntakeExtractor intakeExtractor;
    double nbPointsPerHour = _traits->getNbPointsPerHour();

    IntakeSeries intakeSeries;
    ComputingStatus intakeExtractionResult = intakeExtractor.extract(_request.getDrugTreatment().getDosageHistory(), fantomStart /*_traits->getStart()*/, _traits->getEnd() /* + Duration(24h)*/, nbPointsPerHour, intakeSeries);

    if (intakeExtractionResult != ComputingStatus::Ok) {
        m_logger.error("Error with the intakes extraction.");
        return intakeExtractionResult;
    }

    size_t nIntakes = intakeSeries.size();


    if (nIntakes > 0) {

        // Ensure that time ranges are correctly handled. We set again the interval based on the start of
        // next intake
        for (size_t i = 0; i < nIntakes - 1; i++) {
            Duration interval = intakeSeries[i + 1].getEventTime() - intakeSeries[i].getEventTime();
            intakeSeries[i].setNbPoints(static_cast<int>(interval.toHours() * nbPointsPerHour) + 1);
            intakeSeries[i].setInterval(interval);
        }

        for (size_t i = 0; i < nIntakes; i++) {
            if (intakeSeries[i].getEventTime() + intakeSeries[i].getInterval() < _traits->getStart()) {
                intakeSeries[i].setNbPoints(2);
            }
        }

        const DosageTimeRangeList& timeRanges = _request.getDrugTreatment().getDosageHistory().getDosageTimeRanges();


        IntakeEvent *lastIntake = &(intakeSeries.back());

        // If the treatement end is before the last point we want to get, then we add an empty dose to get points

//        if (_traits->getEnd() > timeRanges.back()->getEndDate()) {
            if (_traits->getEnd() > lastIntake->getEventTime() + lastIntake->getInterval()) {

            DateTime start = lastIntake->getEventTime() + lastIntake->getInterval();
            Value dose = 0.0;
            Duration interval = _traits->getEnd() - timeRanges.back()->getEndDate();
            auto absorptionModel = lastIntake->getRoute();

            Duration infusionTime;
            if (absorptionModel == AbsorptionModel::Infusion) {
                // We do this because the infusion calculators do not support infusionTime = 0
                infusionTime = Duration(std::chrono::hours(1));
            }
            // We need at least one point. It could be less if the interval is very very small
            int nbPoints = std::max(1, static_cast<int>(nbPointsPerHour * interval.toHours()));

            IntakeEvent intake(start, Duration(), dose, interval, lastIntake->getFormulationAndRoute(), absorptionModel, infusionTime, nbPoints);
            intakeSeries.push_back(intake);
        }

    }

    {
        IntakeSeries recordedIntakes;
        selectRecordedIntakes(recordedIntakes, intakeSeries, _traits->getStart(), _traits->getEnd());

        OverloadEvaluator *overloadEvaluator = SingleOverloadEvaluator::getInstance();
        if (!overloadEvaluator->isAcceptable(recordedIntakes, _traits)) {
            m_logger.error("Computation is too big. Aborting to avoid a computer crash. {}", overloadEvaluator->getErrorMessage());
            return ComputingStatus::TooBig;
        }
    }

    std::vector<FormulationAndRoute> allFormulationAndRoutes;
    for (const auto & intake : intakeSeries) {
        FormulationAndRoute f = intake.getFormulationAndRoute();
        if (std::find(allFormulationAndRoutes.begin(), allFormulationAndRoutes.end(), f) == allFormulationAndRoutes.end()) {
            allFormulationAndRoutes.push_back(f);
        }
    }

    std::map<FormulationAndRoute, const FullFormulationAndRoute *> fullFormulationAndRoutes;

    // Only look for formulation and routes if there is an existing treatment
    if (!intakeSeries.empty()) {
        if (!findFormulationAndRoutes(allFormulationAndRoutes, _request.getDrugModel().getFormulationAndRoutes(), fullFormulationAndRoutes)) {
            m_logger.error("Could not find a suitable formulation and route in the drug model");
            return ComputingStatus::CouldNotFindSuitableFormulationAndRoute;
        }
    }

    if (fullFormulationAndRoutes.size() > 1) {
        m_logger.error("The computing engine does not support multiple formulations and routes");
        return ComputingStatus::MultipleFormulationAndRoutesNotSupported;
    }

    std::map<AnalyteGroupId, AbsorptionModel > absorptionModels;

    for (const auto &analyteSet : _request.getDrugModel().getAnalyteSets()) {
        if (!allFormulationAndRoutes.empty()) {
            const FullFormulationAndRoute *singleFormulationAndRoute = fullFormulationAndRoutes[allFormulationAndRoutes[0]];
            absorptionModels[analyteSet->getId()] = singleFormulationAndRoute->getAbsorptionModel(analyteSet->getId());
        }

        _pkModel[analyteSet->getId()] = _modelCollection->getPkModelFromId(analyteSet->getPkModelId());
        if (_pkModel[analyteSet->getId()] == nullptr) {
            m_logger.error("Can not find a Pk Model for the calculation");
            return ComputingStatus::NoPkModelError;
        }
    }

    for (const auto &analyteSet : _request.getDrugModel().getAnalyteSets()) {
        cloneIntakeSeries(intakeSeries, _intakeSeries[analyteSet->getId()]);

        convertAnalytes(_intakeSeries[analyteSet->getId()], _request.getDrugModel(), analyteSet.get());

        ComputingStatus intakeAssociationResult = IntakeToCalculatorAssociator::associate(_intakeSeries[analyteSet->getId()], *_pkModel[analyteSet->getId()]);

        if (intakeAssociationResult != ComputingStatus::Ok) {
            m_logger.error("Can not associate intake calculators for the specified route");
            return intakeAssociationResult;
        }
    }

    if (_traits->getComputingOption().getParametersType() == PredictionParameterType::Population) {
        PatientVariates emptyPatientVariates;
        CovariateExtractor covariateExtractor(_request.getDrugModel().getCovariates(),
                                              emptyPatientVariates,
                                              fantomStart,
                                              _traits->getEnd());
        ComputingStatus covariateExtractionResult = covariateExtractor.extract(_covariatesSeries);

        if (covariateExtractionResult != ComputingStatus::Ok) {
            m_logger.error("Can not extract covariates");
            return covariateExtractionResult;
        }
    }
    else {
        CovariateExtractor covariateExtractor(_request.getDrugModel().getCovariates(),
                                              _request.getDrugTreatment().getCovariates(),
                                              fantomStart,
                                              _traits->getEnd());
        ComputingStatus covariateExtractionResult = covariateExtractor.extract(_covariatesSeries);

        if (covariateExtractionResult != ComputingStatus::Ok) {
            m_logger.error("Can not extract covariates");
            return covariateExtractionResult;
        }
    }

    /*
    // Evaluate the drug domain constraints with respect to the covariates
    DrugDomainConstraintsEvaluator domainConstraintsEvaluator;
    DrugDomainConstraintsEvaluator::Result domainConstraintsResult =
            domainConstraintsEvaluator.evaluate(_request.getDrugModel().getDomain(),
                                                _covariatesSeries,
                                                fantomStart,
                                                _traits->getEnd());

    if (domainConstraintsResult != DrugDomainConstraintsEvaluator::Result::Compatible) {
        return ComputingResult::UncompatibleDrugDomain;
    }
    */

    for (const auto &analyteSet : _request.getDrugModel().getAnalyteSets()) {
        const AnalyteGroupId analyteGroupId =analyteSet->getId();


        // TODO : This should not necessarily be a single formulation and route
        Formulation formulation = Formulation::Undefined;
        AdministrationRoute route = AdministrationRoute::Undefined;
        if (!allFormulationAndRoutes.empty()) {
            formulation = allFormulationAndRoutes[0].getFormulation();
            route = allFormulationAndRoutes[0].getAdministrationRoute();
        }
        else {
            formulation = _request.getDrugModel().getFormulationAndRoutes().getDefault()->getFormulationAndRoute().getFormulation();
            route = _request.getDrugModel().getFormulationAndRoutes().getDefault()->getFormulationAndRoute().getAdministrationRoute();
        }

        ParameterDefinitionIterator it = _request.getDrugModel().getParameterDefinitions(analyteGroupId, formulation, route);

        ParametersExtractor parameterExtractor(_covariatesSeries,
                                               it,
                                               fantomStart,
                                               _traits->getEnd());

        ComputingStatus parametersExtractionResult;


        if (_traits->getComputingOption().getParametersType() == PredictionParameterType::Population) {
#ifdef POPPARAMETERSFROMDEFAULTVALUES
            parametersExtractionResult = parameterExtractor.extractPopulation(_parameterSeries);
#else
            //parametersExtractionResult = parameterExtractor.extract(_parameterSeries);

            ParameterSetSeries intermediateParameterSeries;

            parametersExtractionResult = parameterExtractor.extract(intermediateParameterSeries);

            if (parametersExtractionResult != ComputingStatus::Ok) {
                m_logger.error("Can not extract parameters");
                return parametersExtractionResult;
            }

            // The intermediateParameterSeries contains changes of parameters, so we build a full set of parameter
            // for each event.
            parametersExtractionResult = parameterExtractor.buildFullSet(intermediateParameterSeries, _parameterSeries[analyteGroupId]);
            if (parametersExtractionResult != ComputingStatus::Ok) {
                m_logger.error("Can not consolidate parameters");
                return parametersExtractionResult;
            }

#endif // POPPARAMETERSFROMDEFAULTVALUES

            if (parametersExtractionResult != ComputingStatus::Ok) {
                m_logger.error("Can not extract parameters");
                return parametersExtractionResult;
            }

        }
        else {
            ParameterSetSeries intermediateParameterSeries;

            parametersExtractionResult = parameterExtractor.extract(intermediateParameterSeries);

            if (parametersExtractionResult != ComputingStatus::Ok) {
                m_logger.error("Can not extract parameters");
                return parametersExtractionResult;
            }

            // The intermediateParameterSeries contains changes of parameters, so we build a full set of parameter
            // for each event.
            parametersExtractionResult = parameterExtractor.buildFullSet(intermediateParameterSeries, _parameterSeries[analyteGroupId]);
            if (parametersExtractionResult != ComputingStatus::Ok) {
                m_logger.error("Can not consolidate parameters");
                return parametersExtractionResult;
            }

        }
    }


    return ComputingStatus::Ok;

}

bool GeneralExtractor::findFormulationAndRoutes(std::vector<FormulationAndRoute> &_treatmentFandR, const FormulationAndRoutes &_drugModelFandR, std::map<FormulationAndRoute, const FullFormulationAndRoute *> &_result)
{
    if (_treatmentFandR.size() > 1) {
        m_logger.error("The computing component does not support multi-formulation and routes treatments");
        return false;
    }
    for (const auto & f : _drugModelFandR) {
        if (f->getFormulationAndRoute().getAbsorptionModel() == _treatmentFandR[0].getAbsorptionModel()) {
            _result[_treatmentFandR[0]] = f.get();
            return true;
        }
    }

    m_logger.error("Could not find a formulation and route compatible with the treatment");
    return false;
}

ComputingStatus GeneralExtractor::convertAnalytes(IntakeSeries &_intakeSeries, const Tucuxi::Core::DrugModel &_drugModel, const AnalyteSet *_analyteGroup)
{
    for (auto & intakeSerie : _intakeSeries) {
        const FullFormulationAndRoute *formulation = _drugModel.getFormulationAndRoutes().get(intakeSerie.getFormulationAndRoute());
        if (formulation == nullptr) {
            return ComputingStatus::AnalyteConversionError;
        }
        // TODO : Here we only support one analyte. To be modified once
        const AnalyteConversion *analyteConversion = formulation->getAnalyteConversion(_analyteGroup->getAnalytes()[0]->getAnalyteId());
        if (analyteConversion != nullptr) {
            double factor = analyteConversion->getFactor();
            DoseValue newDose = intakeSerie.getDose() * factor;
            intakeSerie.setDose(newDose);
        }
    }
    return ComputingStatus::Ok;
}

} // namespace Core
} // namespace Tucuxi
