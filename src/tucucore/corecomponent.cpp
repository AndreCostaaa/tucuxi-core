/*
* Copyright (C) 2017 Tucuxi SA
*/

#include "tucucore/drugmodel/drugmodel.h"

#include "tucucore/corecomponent.h"
#include "tucucore/drugtreatment/drugtreatment.h"
#include "tucucore/intakeextractor.h"
#include "tucucore/covariateextractor.h"
#include "tucucore/targetextractor.h"
#include "tucucore/sampleextractor.h"
#include "tucucore/parametersextractor.h"
#include "tucucore/intakeintervalcalculator.h"
#include "tucucore/concentrationcalculator.h"
#include "tucucore/aposteriorietascalculator.h"

namespace Tucuxi {
namespace Core {


CoreComponent::CoreComponent(const std::string &_filename)
{
    TMP_UNUSED_PARAMETER(_filename);
    registerInterface(dynamic_cast<IDataModelServices*>(this));
    registerInterface(dynamic_cast<IProcessingService*>(this));
}


CoreComponent::~CoreComponent()
{
}


bool CoreComponent::loadDrug(const std::string& _xmlDrugDescription)
{
    TMP_UNUSED_PARAMETER(_xmlDrugDescription);
    return false;
}


bool CoreComponent::loadTreatment(const std::string& _xmlTreatmentDescription)
{
    TMP_UNUSED_PARAMETER(_xmlTreatmentDescription);
    return false;
}

ProcessingResult CoreComponent::compute(const ProcessingRequest &_request, std::unique_ptr<ProcessingResponse> &_response)
{
    return ProcessingResult::Error;
/*
    ConcentrationPredictionPtr prediction;

    IntakeSeries intakes;
    CovariateSeries covariates;
    ParameterSetSeries parameters;
    // These drug parameters shall be the ones of the drugmodel
    ParameterDefinitions drugParameters;
    // Extract intakes, covariates, parameters and samples
    IntakeExtractor::extract(*m_treatment->getDosageHistory(_request.getType() != PredictionType::Population), _request.getStart(), _request.getEnd(), intakes);
    CovariateExtractor(m_drug->getCovariates(), m_treatment->getCovariates(), _request.getStart(), _request.getEnd()).extract(covariates);
    ParametersExtractor(covariates, drugParameters, _request.getStart(), _request.getEnd()).extract(parameters);

    if (intakes.size() == 0) {
    m_logger.debug("No intakes, no calc.");
    return nullptr;
    }

    prediction = std::make_unique<ConcentrationPrediction>();

    switch (_request.getType())
    {
    case PredictionType::Population:
    computePopulation(prediction, _request.getIsAll(), _request.getNbPoints(), intakes, parameters);
    break;

    case PredictionType::Apiori:
    computeApriori(prediction, _request.getIsAll(), _request.getNbPoints(), intakes, parameters);
    break;

    case PredictionType::Aposteriori: {
    DateTime lastSampleDate;
    SampleSeries samples;
    SampleExtractor::extract(m_treatment->getSamples(), _request.getStart(), _request.getEnd(), samples);
    if (samples.size() > 0) {
    SampleSeries::iterator _smpend = samples.end();
    lastSampleDate = _smpend->getEventTime();
    }
    IntakeSeries intakesForEtas;
    IntakeExtractor::extract(*m_treatment->getDosageHistory(_request.getType() != PredictionType::Population), _request.getStart(), lastSampleDate, intakesForEtas);

    /// TODO YJE
    Etas etas;
    OmegaMatrix omega;
    SigmaResidualErrorModel residualErrorModel;
    //            extractError(m_drug->getErrorModel(), m_drug->getParemeters(), omega, residualErrorModel);
    APosterioriEtasCalculator().computeAposterioriEtas(intakesForEtas, parameters, omega, residualErrorModel, samples, etas);
    computeAposteriori(prediction, _request.getIsAll(), _request.getNbPoints(), intakes, parameters, etas);
    break;
    }

    default:
    break;
    }

    return prediction;
*/
}

std::string CoreComponent::getErrorString() const
{
    return "";
}

Tucuxi::Common::Interface* CoreComponent::getInterface(const std::string &_name)
{
    return Tucuxi::Common::Component::getInterfaceImpl(_name);
}

ComputationResult CoreComponent::computeConcentrations(
    ConcentrationPredictionPtr &_prediction,
    const bool _isAll,
    const int _nbPoints,
    const DateTime &_recordFrom,
    const DateTime &_recordTo,
    const IntakeSeries &_intakes,
    const ParameterSetSeries &_parameterSets,
    const Etas &_etas,
    const IResidualErrorModel &_residualErrorModel,
    const Deviations& _eps,
    const bool _isFixedDensity)
{   
    // TODO : Use a factory for the calculator
    ConcentrationCalculator calculator;
    calculator.computeConcentrations(
        _prediction,
        _isAll,
        _nbPoints,
        _recordFrom,
        _recordTo,
        _intakes,
        _parameterSets,
        _etas,
        _residualErrorModel,
        _eps,
        _isFixedDensity);
    return ComputationResult::Success;
}

}
}
