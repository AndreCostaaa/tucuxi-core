#include "queryimport.h"

#include "tucucommon/xmldocument.h"
#include "tucucommon/xmlnode.h"
#include "tucucommon/xmlattribute.h"
#include "tucucommon/xmliterator.h"
#include "tucucommon/utils.h"
#include "tucucore/computingservice/computingtrait.h"
#include "tucucore/drugmodelimport.h"
#include "tucucommon/xmlimporter.h"


// #include <unistd.h>
#include <chrono>

#include "tucucore/definitions.h"

using namespace std;

namespace Tucuxi {
namespace Query {


static const std::string DATE_FORMAT = "%Y-%m-%dT%H:%M:%S"; // NOLINT(readability-identifier-naming)


QueryImport::QueryImport()
= default;


QueryImport::~QueryImport()
= default;

const std::vector<std::string> &QueryImport::ignoredTags() const
{
    static std::vector<std::string> ignored;
    return ignored;
}

QueryImport::Result QueryImport::importFromFile(Tucuxi::Query::QueryData *&_query, const std::string& _fileName)
{
    // Ensure the function is reentrant
    std::lock_guard<std::mutex> lock(m_mutex);

    setResult(Result::Ok);
    _query = nullptr;

    Tucuxi::Common::XmlDocument document;
    if (!document.open(_fileName)) {
        return Result::CantOpenFile;
    }

    return importDocument(_query, document);
}


QueryImport::Result QueryImport::importFromString(Tucuxi::Query::QueryData *&_query, const std::string& _xml)
{
    // Ensure the function is reentrant
    std::lock_guard<std::mutex> lock(m_mutex);

    setResult(Result::Ok);
    _query = nullptr;

    Tucuxi::Common::XmlDocument document;

    if (!document.fromString(_xml)) {
        return Result::Error;
    }

    return importDocument(_query, document);
}


///////////////////////////////////////////////////////////////////////////////
/// General methods
///////////////////////////////////////////////////////////////////////////////

QueryImport::Result QueryImport::importDocument(
        Tucuxi::Query::QueryData *&_query,
        Tucuxi::Common::XmlDocument & _document)
{

    // TODO try to see if I can catch a parsing exception.

    static const string DRUG_ID_NODE_NAME = "queryId";
    static const string CLIENT_ID_NODE_NAME = "clientId";
    static const string DATE_NODE_NAME = "date";
    static const string LANGUAGE_NODE_NAME = "language";
    static const string REQUESTS_NODE_NAME = "requests";
    static const string REQUEST_REQUESTS_NODE_NAME = "request";

    Common::XmlNode root = _document.getRoot();

    string drugId = root.getChildren(DRUG_ID_NODE_NAME)->getValue();
    string clientId = root.getChildren(CLIENT_ID_NODE_NAME)->getValue();

    Common::XmlNodeIterator dateIterator = root.getChildren(DATE_NODE_NAME);
    Common::DateTime date = extractDateTime(dateIterator);

    string language = root.getChildren(LANGUAGE_NODE_NAME)->getValue();

    unique_ptr<DrugTreatmentData> pParametersData = createDrugTreatmentData(_document);

    Common::XmlNodeIterator requestsRootIterator = root.getChildren(REQUESTS_NODE_NAME);
    isNodeIteratorValid(requestsRootIterator);
    Common::XmlNodeIterator requestsIterator = requestsRootIterator->getChildren(REQUEST_REQUESTS_NODE_NAME);

    vector< unique_ptr<RequestData> > requests;
    while(requestsIterator != Common::XmlNodeIterator::none()) {
        requests.emplace_back(createRequest(requestsIterator));
        requestsIterator++;
    }


    _query = new QueryData(
                drugId,
                clientId,
                date,
                language,
                move(pParametersData),
                requests
                );


    return getResult();

}

unique_ptr<DrugTreatmentData> QueryImport::createDrugTreatmentData(Tucuxi::Common::XmlDocument & _document)
{
    static const string DRUGTREATMENT_NODE_NAME = "drugTreatment";
    static const string PATIENT_NODE_NAME = "patient";
    static const string DRUGS_NODE_NAME = "drugs";

    Common::XmlNode root = _document.getRoot();

    Common::XmlNodeIterator parametersRootIterator = root.getChildren(DRUGTREATMENT_NODE_NAME);
    isNodeIteratorValid(parametersRootIterator);

    Common::XmlNodeIterator patientRootIterator = parametersRootIterator->getChildren(PATIENT_NODE_NAME);
    isNodeIteratorValid(patientRootIterator);

    unique_ptr<PatientData> pPatient = createPatientData(patientRootIterator);

    vector< unique_ptr<DrugData> > drugs;
    Common::XmlNodeIterator drugsRootIterator  = parametersRootIterator->getChildren(DRUGS_NODE_NAME);
    isNodeIteratorValid(drugsRootIterator);
    Common::XmlNodeIterator drugsIterator = drugsRootIterator->getChildren();

    while(drugsIterator != Common::XmlNodeIterator::none()) {
        drugs.push_back(createDrugData(drugsIterator));
        drugsIterator++;
    }

    return make_unique<DrugTreatmentData>(
                move(pPatient),
                move(drugs)
                );
}

unique_ptr<PatientData> QueryImport::createPatientData(Common::XmlNodeIterator& _patientDataRootIterator)
{
    static const string COVARIATES_NODE_NAME = "covariates";
    static const string COVARIATE_NODE_NAME = "covariate";

    vector< unique_ptr<CovariateData> > covariates;

    Common::XmlNodeIterator covariatesRootIterator = _patientDataRootIterator->getChildren(COVARIATES_NODE_NAME);
    Common::XmlNodeIterator covariatesIterator = covariatesRootIterator->getChildren();
    while(covariatesIterator != Common::XmlNodeIterator::none()) {
        covariates.push_back(createCovariateData(covariatesIterator));
        covariatesIterator++;
    }

    return make_unique<PatientData>(covariates);
}

unique_ptr<CovariateData> QueryImport::createCovariateData(Common::XmlNodeIterator& _covariateDataRootIterator)
{
    static const string COVARIATEID_NODE_NAME = "covariateId";
    static const string DATE_NODE_NAME = "date";
    static const string VALUE_NODE_NAME = "value";
    static const string UNIT_NODE_NAME = "unit";
    static const string DATATYPE_NODE_NAME = "dataType";
    static const string NATURE_NODE_NAME = "nature";

    string covariateId = getChildString(_covariateDataRootIterator, COVARIATEID_NODE_NAME);
    Common::DateTime date = getChildDateTime(_covariateDataRootIterator, DATE_NODE_NAME);
    string value = getChildString(_covariateDataRootIterator, VALUE_NODE_NAME);
    string unit = getChildString(_covariateDataRootIterator, UNIT_NODE_NAME);

    string dataTypeString = getChildString(_covariateDataRootIterator, DATATYPE_NODE_NAME);
    Core::DataType dataType;
    if (dataTypeString == "int") {
        dataType = Core::DataType::Int;
    } else if (dataTypeString == "double") {
        dataType = Core::DataType::Double;
    } else if (dataTypeString == "bool") {
        dataType = Core::DataType::Bool;
    } else if (dataTypeString == "date") {
        dataType = Core::DataType::Date;
    } else {
        // TODO : there is an error to notify
        dataType = Core::DataType::Double;
    }

    // If the covariate is a date, go through a DateTime to reconvert it to a string
    // to be sure it has the correct format
    if (dataType == Core::DataType::Date) {
        Common::DateTime valueAsDate;
        valueAsDate = getChildDateTime(_covariateDataRootIterator, VALUE_NODE_NAME);
        value = Tucuxi::Common::Utils::varToString(valueAsDate);
    }

    string nature = getChildString(_covariateDataRootIterator, NATURE_NODE_NAME);

    return make_unique<CovariateData>(
                covariateId,
                date,
                value,
                unit,
                dataType,
                nature
                );
}


struct CmpByDate
{
    inline bool operator()(const std::unique_ptr<SampleData>& _a, const std::unique_ptr<SampleData>& _b)
    {
        return _a->getpSampleDate() < _b->getpSampleDate();
    }
};

unique_ptr<DrugData> QueryImport::createDrugData(Common::XmlNodeIterator& _drugDataRootIterator)
{
    static const string DRUG_ID_NODE_NAME = "drugId";
    static const string DRUG_MODEL_ID_NODE_NAME = "drugModelId";
    static const string ACTIVE_PRINCIPLE_NODE_NAME = "activePrinciple";
    static const string BRAND_NAME_NODE_NAME = "brandName";
    static const string ATC_NODE_NAME = "atc";
    static const string TREATMENT_NODE_NAME = "treatment";
    static const string SAMPLES_NODE_NAME = "samples";
    static const string TARGETS_NODE_NAME = "targets";

    string drugId = getChildString(_drugDataRootIterator, DRUG_ID_NODE_NAME);
    string drugModelId = getChildString(_drugDataRootIterator, DRUG_MODEL_ID_NODE_NAME);
    string activePrinciple = getChildString(_drugDataRootIterator, ACTIVE_PRINCIPLE_NODE_NAME);
    string brandName = getChildString(_drugDataRootIterator, BRAND_NAME_NODE_NAME);
    string atc = getChildString(_drugDataRootIterator, ATC_NODE_NAME);

    Common::XmlNodeIterator treatmentRootIterator = _drugDataRootIterator->getChildren(TREATMENT_NODE_NAME);
    unique_ptr<Treatment> pTreatment = createTreatment(treatmentRootIterator);

    vector< unique_ptr<SampleData> > samples;
    Common::XmlNodeIterator samplesRootIterator = _drugDataRootIterator->getChildren(SAMPLES_NODE_NAME);
    Common::XmlNodeIterator samplesIterator = samplesRootIterator->getChildren();
    while(samplesIterator != Common::XmlNodeIterator::none()) {
        samples.push_back(createSampleData(samplesIterator));
        samplesIterator++;
    }


    sort(samples.begin(), samples.end(), CmpByDate());


    vector< unique_ptr<TargetData> > targets;
    Common::XmlNodeIterator targetsRootIterator = _drugDataRootIterator->getChildren(TARGETS_NODE_NAME);
    Common::XmlNodeIterator targetsIterator = targetsRootIterator->getChildren();
    while(targetsIterator != Common::XmlNodeIterator::none()) {
        targets.push_back(createTargetData(targetsIterator));
        targetsIterator++;
    }

    return make_unique<DrugData>(
                drugId,
                activePrinciple,
                brandName,
                atc,
                move(pTreatment),
                samples,
                targets
                );
}

unique_ptr<TargetData> QueryImport::createTargetData(Common::XmlNodeIterator& _targetDataRootIterator)
{
    static const string ANALYTE_ID_NODE_NAME           = "activeMoietyId";
    static const string TARGET_TYPE_NODE_NAME          = "targetType";
    static const string UNIT_NODE_NAME                 = "unit";
    static const string INEFFICACY_ALARM_ID_NODE_NAME  = "inefficacyAlarm";
    static const string MIN_NODE_NAME                  = "min";
    static const string BEST_NODE_NAME                 = "best";
    static const string MAX_ID_NODE_NAME               = "max";
    static const string TOXICITY_ALARM_NODE_NAME       = "toxicityAlarm";

    string activeMoietyId = getChildString(_targetDataRootIterator, ANALYTE_ID_NODE_NAME);
    string targetType = getChildString(_targetDataRootIterator, TARGET_TYPE_NODE_NAME);
    string unit = getChildString(_targetDataRootIterator, UNIT_NODE_NAME);
    double inefficacyAlarm = getChildDouble(_targetDataRootIterator, INEFFICACY_ALARM_ID_NODE_NAME);
    double min = getChildDouble(_targetDataRootIterator, MIN_NODE_NAME);
    double best= getChildDouble(_targetDataRootIterator, BEST_NODE_NAME);
    double max = getChildDouble(_targetDataRootIterator, MAX_ID_NODE_NAME);
    double toxicityAlarm = getChildDouble(_targetDataRootIterator, TOXICITY_ALARM_NODE_NAME);

    return make_unique<TargetData>(
                activeMoietyId,
                targetType,
                unit,
                inefficacyAlarm,
                min,
                best,
                max,
                toxicityAlarm
                );
}

unique_ptr<SampleData> QueryImport::createSampleData(Common::XmlNodeIterator& _sampleDataRootIterator)
{
    static const string SAMPLE_ID_NODE_NAME = "sampleId";
    static const string SAMPLE_DATE_NODE_NAME = "sampleDate";
    static const string CONCENTRATIONS_NODE_NAME = "concentrations";

    string sampleId = getChildString(_sampleDataRootIterator, SAMPLE_ID_NODE_NAME);
    Common::DateTime sampleDate = getChildDateTime(_sampleDataRootIterator, SAMPLE_DATE_NODE_NAME);

    vector< unique_ptr<ConcentrationData> > concentrations;

    Common::XmlNodeIterator concentrationsRootIterator = _sampleDataRootIterator->getChildren(CONCENTRATIONS_NODE_NAME);
    Common::XmlNodeIterator concentrationsIterator = concentrationsRootIterator->getChildren();
    while(concentrationsIterator != Common::XmlNodeIterator::none()) {
        concentrations.push_back(createConcentrationData(concentrationsIterator));
        concentrationsIterator++;
    }

    return make_unique<SampleData>(
                sampleId,
                sampleDate,
                concentrations
                );
}

unique_ptr<ConcentrationData> QueryImport::createConcentrationData(Common::XmlNodeIterator& _concentrationDataRootIterator)
{
    static const string ANALYTE_ID_NODE_NAME = "analyteId";
    static const string VALUE_NODE_NAME = "value";
    static const string UNIT_NODE_NAME = "unit";

    string analyteId = getChildString(_concentrationDataRootIterator, ANALYTE_ID_NODE_NAME);
    double value = getChildDouble(_concentrationDataRootIterator, VALUE_NODE_NAME);
    string unit = getChildString(_concentrationDataRootIterator, UNIT_NODE_NAME);

    return make_unique<ConcentrationData>(analyteId, value, unit);
}

unique_ptr<Treatment> QueryImport::createTreatment(Common::XmlNodeIterator& _treatmentRootIterator)
{
    static const string DOSAGE_HISTORY_NODE_NAME = "dosageHistory";

    Common::XmlNodeIterator dosageHistoryRootIterator = _treatmentRootIterator->getChildren(DOSAGE_HISTORY_NODE_NAME);
    Common::XmlNodeIterator dosageTimeRangeIterator = dosageHistoryRootIterator->getChildren();

    unique_ptr<Core::DosageHistory> pDosageHistory = make_unique<Core::DosageHistory>();
    while(dosageTimeRangeIterator != Common::XmlNodeIterator::none()) {
        unique_ptr<Core::DosageTimeRange> pDosageTimeRange = createDosageTimeRange(dosageTimeRangeIterator);
        if(pDosageTimeRange != nullptr)
        {
           pDosageHistory->addTimeRange(*pDosageTimeRange);
        }
        dosageTimeRangeIterator++;
    }

    return make_unique<Treatment>(move(pDosageHistory));
}

unique_ptr<Core::DosageTimeRange> QueryImport::createDosageTimeRange(Common::XmlNodeIterator& _dosageTimeRangeRootIterator)
{
    static const string START_NODE_NAME    = "start";
    static const string END_NODE_NAME      = "end";
    static const string DOSAGE_NODE_NAME   = "dosage";

    Common::DateTime start = getChildDateTime(_dosageTimeRangeRootIterator, START_NODE_NAME);
    Common::DateTime end = getChildDateTime(_dosageTimeRangeRootIterator, END_NODE_NAME);

    Common::XmlNodeIterator dosageRootIterator = _dosageTimeRangeRootIterator->getChildren(DOSAGE_NODE_NAME);
    unique_ptr<Core::Dosage> pDosage = createDosage(dosageRootIterator);
    if(pDosage == nullptr)
    {
        return nullptr;
    }

    return make_unique<Core::DosageTimeRange>(
                start,
                end,
                *pDosage
                );

}

unique_ptr<Core::Dosage> QueryImport::createDosage(Common::XmlNodeIterator& _dosageRootIterator)
{
    static const string DOSAGE_LOOP_NODE_NAME  = "dosageLoop";

    Common::XmlNodeIterator dosageIterator = _dosageRootIterator->getChildren();
    if (dosageIterator->getName() == DOSAGE_LOOP_NODE_NAME) {
        // Create dosage loop from a bounded dosage
        unique_ptr<Core::DosageBounded> pDosageBounded = createDosageBounded(dosageIterator);
        if(pDosageBounded == nullptr)
        {
            return nullptr;
        }
        return make_unique<Core::DosageLoop>(*pDosageBounded);


    }
    // Create bounded dosage
    // For this case we give the same root iterator as the one given as parameter for this method
    return createDosageBounded(_dosageRootIterator);
}

unique_ptr<Core::DosageBounded> QueryImport::createDosageBounded(Common::XmlNodeIterator& _dosageBoundedRootIterator)
{
    static const string DOSAGE_REPEAT_NODE_NAME            = "dosageRepeat";
    static const string DOSAGE_SEQUENCE_NODE_NAME          = "dosageSequence";
    static const string LASTING_DOSAGE_NODE_NAME           = "lastingDosage";
    static const string DAILY_DOSAGE_NODE_NAME             = "dailyDosage";
    static const string WEEKLY_DOSAGE_NODE_NAME            = "weeklyDosage";
    static const string DOSE_NODE_NAME                     = "dose";
    static const string DOSE_VALUE_NODE_NAME               = "value";
    static const string DOSE_INFUSIONTIME_NAME             = "infusionTimeInMinutes";
    static const string FORMULATION_AND_ROUTE_NODE_NAME    = "formulationAndRoute";

    unique_ptr<Core::DosageBounded> pDosageBounded;
    Common::XmlNodeIterator dosageBoundedIterator = _dosageBoundedRootIterator->getChildren();
    if (dosageBoundedIterator->getName() == DOSAGE_REPEAT_NODE_NAME) {
        static const string ITERATIONS_NODE_NAME = "iterations";

        int iterations = getChildInt(dosageBoundedIterator, ITERATIONS_NODE_NAME);
        unique_ptr<Core::DosageBounded> pDosageBounded = createDosageBounded(dosageBoundedIterator);
        pDosageBounded = make_unique<Core::DosageRepeat>(*pDosageBounded, iterations);
    } else if (dosageBoundedIterator->getName() == DOSAGE_SEQUENCE_NODE_NAME) {
        Core::DosageBoundedList dosageBoundedList;
        Common::XmlNodeIterator dosageSequenceIterator = dosageBoundedIterator->getChildren();
        while(dosageSequenceIterator != Common::XmlNodeIterator::none()) {
            dosageBoundedList.push_back(createDosageBounded(dosageSequenceIterator));
            dosageSequenceIterator++;
        }

        pDosageBounded = make_unique<Core::DosageSequence>(*(dosageBoundedList.at(0)));
    } else if (dosageBoundedIterator->getName() == LASTING_DOSAGE_NODE_NAME) {
        static const string INTERVAL_NODE_NAME                 = "interval";


        Common::Duration interval = getChildDuration(dosageBoundedIterator, INTERVAL_NODE_NAME);

        Common::XmlNodeIterator doseRootIterator = dosageBoundedIterator->getChildren(DOSE_NODE_NAME);
        Core::DoseValue doseValue = getChildDouble(doseRootIterator, DOSE_VALUE_NODE_NAME);

        double infuTimeInMinutes = getChildDouble(doseRootIterator, DOSE_INFUSIONTIME_NAME);
        Common::Duration infusionTime = Duration(std::chrono::minutes(static_cast<int>(infuTimeInMinutes)));

        Common::XmlNodeIterator formulationAndRouteRootIterator = dosageBoundedIterator->getChildren(FORMULATION_AND_ROUTE_NODE_NAME);
        unique_ptr<Core::FormulationAndRoute> formulationAndRoute = createFormulationAndRoute(formulationAndRouteRootIterator);

        if(interval > Duration(std::chrono::seconds(0)))
        {
            try {
                pDosageBounded = make_unique<Core::LastingDose>(
                            doseValue,
                            *formulationAndRoute,
                            infusionTime,
                            interval
                            );

            } catch (std::invalid_argument &e) {
                setNodeError(dosageBoundedIterator);
                pDosageBounded = nullptr;
            }
        }
        else
        {
            pDosageBounded = nullptr;
        }

    } else if (dosageBoundedIterator->getName() == DAILY_DOSAGE_NODE_NAME) {
        static const string TIME_NODE_NAME = "time";

        string timeValue = dosageBoundedIterator->getChildren(TIME_NODE_NAME)->getValue();
        Common::TimeOfDay time(Common::DateTime(timeValue, "%H:%M:%S").getTimeOfDay());

        Common::XmlNodeIterator doseRootIterator = dosageBoundedIterator->getChildren(DOSE_NODE_NAME);
        Core::DoseValue doseValue = getChildDouble(doseRootIterator, DOSE_VALUE_NODE_NAME);

        Common::XmlNodeIterator formulationAndRouteRootIterator = dosageBoundedIterator->getChildren(FORMULATION_AND_ROUTE_NODE_NAME);
        unique_ptr<Core::FormulationAndRoute> formulationAndRoute = createFormulationAndRoute(formulationAndRouteRootIterator);

        double infuTimeInMinutes = getChildDouble(doseRootIterator, DOSE_INFUSIONTIME_NAME);
        Common::Duration infusionTime = Duration(std::chrono::minutes(static_cast<int>(infuTimeInMinutes)));


        try {
            pDosageBounded = make_unique<Core::DailyDose>(
                        doseValue,
                        *formulationAndRoute,
                        infusionTime,
                        time
                        );
        } catch (std::invalid_argument &e) {
             setNodeError(dosageBoundedIterator);
             pDosageBounded = nullptr;
        }

    } else if (dosageBoundedIterator->getName() == WEEKLY_DOSAGE_NODE_NAME) {
        static const string DAY_NODE_NAME = "day";
        static const string TIME_NODE_NAME = "time";

        unsigned int day = static_cast<unsigned int>(getChildInt(dosageBoundedIterator, DAY_NODE_NAME));

        if (day > SATURDAY) {
            // TODO throw or manage error
            day = day % 7;
        }

        string timeValue = dosageBoundedIterator->getChildren(TIME_NODE_NAME)->getValue();
        Common::TimeOfDay time(Common::DateTime(timeValue, "%H:%M:%S").getTimeOfDay());

        Common::XmlNodeIterator doseRootIterator = dosageBoundedIterator->getChildren(DOSE_NODE_NAME);
        Core::DoseValue doseValue = getChildDouble(doseRootIterator, DOSE_VALUE_NODE_NAME);

        Common::XmlNodeIterator formulationAndRouteRootIterator = dosageBoundedIterator->getChildren(FORMULATION_AND_ROUTE_NODE_NAME);
        unique_ptr<Core::FormulationAndRoute> formulationAndRoute = createFormulationAndRoute(formulationAndRouteRootIterator);

        double infuTimeInMinutes = getChildDouble(doseRootIterator, DOSE_INFUSIONTIME_NAME);
        Common::Duration infusionTime = Duration(std::chrono::minutes(static_cast<int>(infuTimeInMinutes)));

        try{
            pDosageBounded = make_unique<Core::WeeklyDose>(
                        doseValue,
                        *formulationAndRoute,
                        infusionTime,
                        time,
                        Core::DayOfWeek(static_cast<unsigned>(day))
                        );
        } catch (std::invalid_argument &e) {
             setNodeError(dosageBoundedIterator);
             pDosageBounded = nullptr;
        }
    }

    return pDosageBounded;
}

unique_ptr<Core::FormulationAndRoute> QueryImport::createFormulationAndRoute(Common::XmlNodeIterator &_formulationAndRouteRootIterator) const
{
    static const string FORMULATION_NODE_NAME          = "formulation";
    static const string ADMINISTRATION_NAME_NODE_NAME  = "administrationName";
    static const string ADMINISTRATION_ROUTE_NODE_NAME = "administrationRoute";
    static const string ABSORPTION_MODEL_NODE_NAME     = "absorptionModel";

    string formulationValue = getChildString(_formulationAndRouteRootIterator, FORMULATION_NODE_NAME);
    Core::Formulation formulation = Core::Formulation::Undefined;

    if (formulationValue == "Undefined") {
        formulation = Core::Formulation::Undefined;
    } else if (formulationValue == "oralSolution") {
        formulation = Core::Formulation::OralSolution;
    } else if (formulationValue == "parenteralSolution") {
        formulation = Core::Formulation::ParenteralSolution;
    } else if (formulationValue == "test") {
        formulation = Core::Formulation::Test;
    } else {
        // Throw error or manage error
        formulation = Core::Formulation::Undefined;
    }

    string administrationName = getChildString(_formulationAndRouteRootIterator, ADMINISTRATION_NAME_NODE_NAME);
    string administrationRouteValue = getChildString(_formulationAndRouteRootIterator, ADMINISTRATION_ROUTE_NODE_NAME);
    Core::AdministrationRoute administrationRoute = Core::AdministrationRoute::Undefined;

    if (administrationRouteValue == "undefined") {
        administrationRoute = Core::AdministrationRoute::Undefined;
    } else if (administrationRouteValue == "intramuscular") {
        administrationRoute = Core::AdministrationRoute::Intramuscular;
    } else if (administrationRouteValue == "intravenousBolus") {
        administrationRoute = Core::AdministrationRoute::IntravenousBolus;
    } else if (administrationRouteValue == "intravenousDrip") {
        administrationRoute = Core::AdministrationRoute::IntravenousDrip;
    } else if (administrationRouteValue == "nasal") {
        administrationRoute = Core::AdministrationRoute::Nasal;
    } else if (administrationRouteValue == "oral") {
        administrationRoute = Core::AdministrationRoute::Oral;
    } else if (administrationRouteValue == "rectal") {
        administrationRoute = Core::AdministrationRoute::Rectal;
    } else if (administrationRouteValue == "subcutaneous") {
        administrationRoute = Core::AdministrationRoute::Subcutaneous;
    } else if (administrationRouteValue == "sublingual") {
        administrationRoute = Core::AdministrationRoute::Sublingual;
    } else if (administrationRouteValue == "transdermal") {
        administrationRoute = Core::AdministrationRoute::Transdermal;
    } else if (administrationRouteValue == "vaginal") {
        administrationRoute = Core::AdministrationRoute::Vaginal;
    } else {
        // Throw error or manage error
        formulation = Core::Formulation::Undefined;
    }

    string absorptionModelValue = getChildString(_formulationAndRouteRootIterator, ABSORPTION_MODEL_NODE_NAME);
    Core::AbsorptionModel absorptionModel = Core::AbsorptionModel::Undefined;

    if (absorptionModelValue == "Undefined") {
        absorptionModel = Core::AbsorptionModel::Undefined;
    } else if (absorptionModelValue == "extravascular") {
        absorptionModel = Core::AbsorptionModel::Extravascular;
    } else if (absorptionModelValue == "extravascularLag") {
        absorptionModel = Core::AbsorptionModel::ExtravascularLag;
    } else if (absorptionModelValue == "intravascular") {
        absorptionModel = Core::AbsorptionModel::Intravascular;
    } else if (absorptionModelValue == "infusion") {
        absorptionModel = Core::AbsorptionModel::Infusion;
    } else {
        // Throw error or manage error
        absorptionModel = Core::AbsorptionModel::Undefined;
    }

    return make_unique<Core::FormulationAndRoute>(
                formulation,
                administrationRoute,
                absorptionModel,
                administrationName
                );
}

unique_ptr<RequestData> QueryImport::createRequest(Tucuxi::Common::XmlNodeIterator& _requestRootIterator)
{
    static const string REQUEST_ID_NODE_NAME                        = "requestId";
    static const string DRUG_ID_NODE_NAME                           = "drugId";
    static const string DRUGMODEL_ID_NODE_NAME                      = "drugModelId";
    static const string COMPUTING_TRAIT_ADJUSTMENT_NAME             = "adjustmentTraits";
    static const string COMPUTING_TRAIT_CONCENTRATION_NAME          = "predictionTraits";
    static const string COMPUTING_TRAIT_PERCENTILES_NAME            = "percentilesTraits";
    static const string COMPUTING_TRAIT_SINGLE_POINT_NAME           = "predictionAtTimesTraits";
    static const string COMPUTING_TRAIT_AT_MESURE_NAME              = "predictionAtSampleTimesTraits";
    static const string REQUEST_TYPE_NODE_NAME                      = "requestType";
    static const string NBPOINTSPERHOUR_NODE_NAME                   = "nbPointsPerHour";
    static const string DATE_INTERVAL_NODE_NAME                     = "dateInterval";
    static const string DATE_INTERVAL_START_NODE_NAME               = "start";
    static const string DATE_INTERVAL_END_NODE_NAME                 = "end";
    static const string PREDICTION_TYPE_NODE_NAME                   = "parametersType";
    static const string GRAPH_NODE_NAME                             = "graph";
    static const string PERCENTILES_NODE_NAME                       = "percentiles";
    static const string POINTSINTIME_NODE_NAME                      = "pointsInTime";
    static const string BACKEXTRAPOLATION_NODE_NAME                 = "backextrapolation";

    string requestId = getChildString(_requestRootIterator, REQUEST_ID_NODE_NAME);
    string drugId = getChildString(_requestRootIterator, DRUG_ID_NODE_NAME);
    string drugModelId = getChildString(_requestRootIterator, DRUGMODEL_ID_NODE_NAME);

    unique_ptr<Tucuxi::Core::ComputingTrait> computingTrait;
    Common::XmlNodeIterator computingTraitAdjustmentRootIterator = _requestRootIterator->getChildren(COMPUTING_TRAIT_ADJUSTMENT_NAME);

    if(computingTraitAdjustmentRootIterator != Common::XmlNodeIterator::none())
    {
        computingTrait = getChildComputingTraitAdjustment(computingTraitAdjustmentRootIterator, requestId);
    }

    Common::XmlNodeIterator computingTraitConcentrationRootIterator = _requestRootIterator->getChildren(COMPUTING_TRAIT_CONCENTRATION_NAME);

    if(computingTraitConcentrationRootIterator != Common::XmlNodeIterator::none())
    {
        computingTrait = getChildComputingTraitConcentration(computingTraitConcentrationRootIterator, requestId);
    }

    Common::XmlNodeIterator computingTraitPercentilesRootIterator = _requestRootIterator->getChildren(COMPUTING_TRAIT_PERCENTILES_NAME);

    if(computingTraitPercentilesRootIterator != Common::XmlNodeIterator::none())
    {
        computingTrait = getChildComputingTraitPercentiles(computingTraitPercentilesRootIterator, requestId);
    }

    Common::XmlNodeIterator computingTraitSinglePointRootIterator = _requestRootIterator->getChildren(COMPUTING_TRAIT_SINGLE_POINT_NAME);

    if(computingTraitSinglePointRootIterator != Common::XmlNodeIterator::none())
    {
        computingTrait = getChildComputingTraitSinglePoints(computingTraitSinglePointRootIterator, requestId);
    }

    Common::XmlNodeIterator computingTraitAtMeasuresRootIterator = _requestRootIterator->getChildren(COMPUTING_TRAIT_AT_MESURE_NAME);

    if(computingTraitAtMeasuresRootIterator != Common::XmlNodeIterator::none())
    {
        computingTrait = getChildComputingTraitAtMeasures(computingTraitAtMeasuresRootIterator, requestId);
    }

    if(computingTrait == nullptr)
    {
        setResult(Result::Error);
    }



    return make_unique<RequestData>(
                requestId,
                drugId,
                drugModelId,
                move(computingTrait)
                );
}

unique_ptr<GraphData> QueryImport::createGraphData(Common::XmlNodeIterator& _graphDataRootIterator)
{
    static const string START_NODE_NAME        = "start";
    static const string END_NODE_NAME          = "end";
    static const string PERCENTILES_NODE_NAME  = "percentiles";

    Common::DateTime start = getChildDateTime(_graphDataRootIterator, START_NODE_NAME);
    Common::DateTime end = getChildDateTime(_graphDataRootIterator, END_NODE_NAME);

    vector<unsigned short> percentiles;
    Common::XmlNodeIterator percentilesRootIterator = _graphDataRootIterator->getChildren(PERCENTILES_NODE_NAME);
    Common::XmlNodeIterator percentilesIterator = percentilesRootIterator->getChildren();
    while(percentilesIterator != Common::XmlNodeIterator::none()) {
        string value = percentilesIterator->getValue();

        unsigned short finalValue = 0;
        try {
            finalValue = static_cast<unsigned short>(stoul(value));
        } catch (invalid_argument& e) {
            finalValue = 0;
        } catch (out_of_range& e) {
            finalValue = 0;
        }

        percentiles.push_back(finalValue);
        percentilesIterator++;
    }

    unique_ptr<DateInterval> pDateInterval = make_unique<DateInterval>(start, end);
    return make_unique<GraphData>(move(pDateInterval), percentiles);
}

unique_ptr<Backextrapolation> QueryImport::createBackextrapolation(Common::XmlNodeIterator& _backextrapolationRootIterator)
{
    static const string SAMPLE_NODE_NAME               = "sample";
    static const string INCOMPLETE_SAMPLE_NODE_NAME    = "incompleteSample";
    static const string DOSAGE_NODE_NAME               = "dosage";

    Common::XmlNodeIterator sampleRootIterator = _backextrapolationRootIterator->getChildren(SAMPLE_NODE_NAME);
    unique_ptr<SampleData> pSampleData = createSampleData(sampleRootIterator);

    Common::XmlNodeIterator dosageRootIterator = _backextrapolationRootIterator->getChildren(DOSAGE_NODE_NAME);
    //unique_ptr<Core::Dosage> pDosage = createDosage(dosageRootIterator, );
    // TODO

    // WARNING! nullptr for now
    unique_ptr<Backextrapolation> pBackextrapolation;
    return pBackextrapolation;
}

string QueryImport::getChildString(Common::XmlNodeIterator _rootIterator, const string& _childName) const
{
    return _rootIterator->getChildren(_childName)->getValue();
}

Tucuxi::Core::PercentileRanks QueryImport::getChildPercentileRanks(Common::XmlNodeIterator _rootIterator, const string& _childName)
{
    Common::XmlNodeIterator it = _rootIterator->getChildren(_childName);
    Tucuxi::Core::PercentileRanks ranks;
    Tucuxi::Core::PercentileRank rank;
    while(it != Common::XmlNodeIterator::none())
    {
        rank = getChildDouble(_rootIterator, _childName);
        ranks.push_back(rank);
        it ++;
    }

    return ranks;
}

vector<Common::DateTime> QueryImport::getChildDateTimeList(Common::XmlNodeIterator _rootIterator, const string& _childName) const
{
    Common::XmlNodeIterator it = _rootIterator->getChildren(_childName);
    vector<DateTime> times;
    string value;
    while(it != Common::XmlNodeIterator::none())
    {
        value = it->getValue();
        Common::DateTime datetime(value, DATE_FORMAT);
        times.push_back(datetime);

        it ++;
    }

    return times;
}

unique_ptr<Tucuxi::Core::ComputingTraitAtMeasures> QueryImport::getChildComputingTraitAtMeasures(Common::XmlNodeIterator _rootIterator, string _requestResponseId)
{
    static const string COMPUTING_OPTION                = "computingOption";


    Tucuxi::Core::RequestResponseId requestResponseId = _requestResponseId;
    Tucuxi::Core::ComputingOption computingOption = getChildComputingOption(_rootIterator, COMPUTING_OPTION);

    return make_unique<Tucuxi::Core::ComputingTraitAtMeasures> (requestResponseId,
                                                                computingOption);
}


unique_ptr<Tucuxi::Core::ComputingTraitSinglePoints> QueryImport::getChildComputingTraitSinglePoints(Common::XmlNodeIterator _rootIterator, string _requestResponseId)
{
    static const string COMPUTING_OPTION                = "computingOption";
    static const string DATES_NODE_NAME                 = "dates";
    static const string DATES_DATE_NODE_NAME            = "date";

    Tucuxi::Core::RequestResponseId requestResponseId = _requestResponseId;

    Common::XmlNodeIterator timesRootIterator = _rootIterator->getChildren(DATES_NODE_NAME);
    vector<Common::DateTime> times = getChildDateTimeList(timesRootIterator, DATES_DATE_NODE_NAME);

    Tucuxi::Core::ComputingOption computingOption = getChildComputingOption(_rootIterator, COMPUTING_OPTION);

    return make_unique<Tucuxi::Core::ComputingTraitSinglePoints> (requestResponseId,
                                                                  times,
                                                                  computingOption);
}

unique_ptr<Tucuxi::Core::ComputingTraitPercentiles> QueryImport::getChildComputingTraitPercentiles(Common::XmlNodeIterator _rootIterator, string _requestResponseId)
{
    static const string NB_POINTS_PER_HOUR              = "nbPointsPerHour";
    static const string DATE_INTERVAL_NODE_NAME         = "dateInterval";
    static const string DATE_INTERVAL_START_NODE_NAME   = "start";
    static const string DATE_INTERVAL_END_NODE_NAME     = "end";
    static const string RANKS_NODE_NAME                 = "ranks";
    static const string RANKS_RANK_NODE_NAME            = "rank";
    static const string COMPUTING_OPTION                = "computingOption";

    Tucuxi::Core::RequestResponseId requestResponseId = _requestResponseId;

    Common::XmlNodeIterator dateIntervalRootIterator = _rootIterator->getChildren(DATE_INTERVAL_NODE_NAME);
    Common::DateTime start = getChildDateTime(dateIntervalRootIterator, DATE_INTERVAL_START_NODE_NAME);
    Common::DateTime end = getChildDateTime(dateIntervalRootIterator, DATE_INTERVAL_END_NODE_NAME);

    Common::XmlNodeIterator ranksRootIterator = _rootIterator->getChildren(RANKS_NODE_NAME);
    Tucuxi::Core::PercentileRanks ranks = getChildPercentileRanks(ranksRootIterator, RANKS_RANK_NODE_NAME);

    double nbPointsPerHour = getChildDouble(_rootIterator, NB_POINTS_PER_HOUR);

    Tucuxi::Core::ComputingOption computingOption = getChildComputingOption(_rootIterator, COMPUTING_OPTION);

    return make_unique<Tucuxi::Core::ComputingTraitPercentiles> (requestResponseId,
                                                                 start,
                                                                 end,
                                                                 ranks,
                                                                 nbPointsPerHour,
                                                                 computingOption);
}

unique_ptr<Tucuxi::Core::ComputingTraitConcentration> QueryImport::getChildComputingTraitConcentration(Common::XmlNodeIterator _rootIterator, string _requestResponseId)
{
    static const string NB_POINTS_PER_HOUR              = "nbPointsPerHour";
    static const string DATE_INTERVAL_NODE_NAME         = "dateInterval";
    static const string DATE_INTERVAL_START_NODE_NAME   = "start";
    static const string DATE_INTERVAL_END_NODE_NAME     = "end";
    static const string COMPUTING_OPTION                = "computingOption";

    Tucuxi::Core::RequestResponseId requestResponseId = _requestResponseId;

    Common::XmlNodeIterator dateIntervalRootIterator = _rootIterator->getChildren(DATE_INTERVAL_NODE_NAME);
    Common::DateTime start = getChildDateTime(dateIntervalRootIterator, DATE_INTERVAL_START_NODE_NAME);
    Common::DateTime end = getChildDateTime(dateIntervalRootIterator, DATE_INTERVAL_END_NODE_NAME);

    double nbPointsPerHour = getChildDouble(_rootIterator, NB_POINTS_PER_HOUR);

    Tucuxi::Core::ComputingOption computingOption = getChildComputingOption(_rootIterator, COMPUTING_OPTION);


    return make_unique<Tucuxi::Core::ComputingTraitConcentration> (requestResponseId,
                                                                   start,
                                                                   end,
                                                                   nbPointsPerHour,
                                                                   computingOption);
}

unique_ptr<Tucuxi::Core::ComputingTraitAdjustment> QueryImport::getChildComputingTraitAdjustment(Common::XmlNodeIterator _rootIterator, string _requestResponseId)
{
    static const string NB_POINTS_PER_HOUR              = "nbPointsPerHour";
    static const string DATE_INTERVAL_NODE_NAME         = "dateInterval";
    static const string DATE_INTERVAL_START_NODE_NAME   = "start";
    static const string DATE_INTERVAL_END_NODE_NAME     = "end";
    static const string COMPUTING_OPTION                = "computingOption";
    static const string DATE_ADJUSTMENT_TIME            = "adjustmentDate";
    static const string OPTIONS                         = "options";


    Tucuxi::Core::RequestResponseId requestResponseId = _requestResponseId;

    Common::XmlNodeIterator dateIntervalRootIterator = _rootIterator->getChildren(DATE_INTERVAL_NODE_NAME);
    Common::DateTime start = getChildDateTime(dateIntervalRootIterator, DATE_INTERVAL_START_NODE_NAME);
    Common::DateTime end = getChildDateTime(dateIntervalRootIterator, DATE_INTERVAL_END_NODE_NAME);

    double nbPointsPerHour = getChildDouble(_rootIterator, NB_POINTS_PER_HOUR);

    Tucuxi::Core::ComputingOption computingOption = getChildComputingOption(_rootIterator, COMPUTING_OPTION);

    Common::DateTime adjustmentTime = getChildDateTime(_rootIterator, DATE_ADJUSTMENT_TIME);

    Tucuxi::Core::BestCandidatesOption bestCandidateOption = getChildBestCandidatesOptionEnum(_rootIterator, OPTIONS);

    Tucuxi::Core::LoadingOption loadingOption = getChildLoadingOptionEnum(_rootIterator, OPTIONS);

    Tucuxi::Core::RestPeriodOption restPeriodOption = getChildRestPeriodTargetOptionEnum(_rootIterator, OPTIONS);

    Tucuxi::Core::SteadyStateTargetOption steadyStateTargetOption = getChildSteadyStateTargetOptionEnum(_rootIterator, OPTIONS);

    Tucuxi::Core::TargetExtractionOption targetExtractionOption = getChildTargetExtractionOptionEnum(_rootIterator, OPTIONS);

    Tucuxi::Core::FormulationAndRouteSelectionOption formulationAndRouteSelectionOption = getChildFormulationAndRouteSelectionOptionEnum(_rootIterator, OPTIONS);


    return make_unique<Tucuxi::Core::ComputingTraitAdjustment>(requestResponseId,
                                                               start,
                                                               end,
                                                               nbPointsPerHour,
                                                               computingOption,
                                                               adjustmentTime,
                                                               bestCandidateOption,
                                                               loadingOption,
                                                               restPeriodOption,
                                                               steadyStateTargetOption,
                                                               targetExtractionOption,
                                                               formulationAndRouteSelectionOption);

}



Tucuxi::Core::BestCandidatesOption QueryImport::getChildBestCandidatesOptionEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{

    static const string BEST_CANDIDATE_OPTION_NODE          = "bestCandidatesOption";

    Common::XmlNodeIterator optionsRootIterator = _rootIterator->getChildren(_childName);

    Common::XmlNodeIterator BestCandidatesOptionRootIterator = optionsRootIterator->getChildren(BEST_CANDIDATE_OPTION_NODE);

    static std::map<std::string,Tucuxi::Core::BestCandidatesOption> m =
    {
        {"bestDosage", Tucuxi::Core::BestCandidatesOption::BestDosage},
        {"allDosages", Tucuxi::Core::BestCandidatesOption::AllDosages},
        {"bestDosagePerInterval", Tucuxi::Core::BestCandidatesOption::BestDosagePerInterval}
    };

    string value = BestCandidatesOptionRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(BestCandidatesOptionRootIterator);

    return Tucuxi::Core::BestCandidatesOption::BestDosage;
}

Tucuxi::Core::LoadingOption QueryImport::getChildLoadingOptionEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{
    static const string LOADING_OPTION_NODE          = "loadingOption";

    Common::XmlNodeIterator optionsRootIterator = _rootIterator->getChildren(_childName);

    Common::XmlNodeIterator loadingOptionRootIterator = optionsRootIterator->getChildren(LOADING_OPTION_NODE);


    static std::map<std::string,Tucuxi::Core::LoadingOption> m =
    {
        {"noLoadingDose", Tucuxi::Core::LoadingOption::NoLoadingDose},
        {"loadingDoseAllowed", Tucuxi::Core::LoadingOption::LoadingDoseAllowed}
    };

    string value = loadingOptionRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(loadingOptionRootIterator);
    return Tucuxi::Core::LoadingOption::NoLoadingDose;
}

Tucuxi::Core::RestPeriodOption QueryImport::getChildRestPeriodTargetOptionEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{

    static const string REST_PERIOD_OPTION_NODE          = "restPeriodOption";

    Common::XmlNodeIterator optionsRootIterator = _rootIterator->getChildren(_childName);

    Common::XmlNodeIterator restPeriodOptionRootIterator = optionsRootIterator->getChildren(REST_PERIOD_OPTION_NODE);


    static std::map<std::string,Tucuxi::Core::RestPeriodOption> m =
    {
        {"noRestPeriod", Tucuxi::Core::RestPeriodOption::NoRestPeriod},
        {"restPeriodAllowed", Tucuxi::Core::RestPeriodOption::RestPeriodAllowed}
    };

    string value = restPeriodOptionRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(restPeriodOptionRootIterator);
    return Tucuxi::Core::RestPeriodOption::NoRestPeriod;
}

Tucuxi::Core::SteadyStateTargetOption QueryImport::getChildSteadyStateTargetOptionEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{

    static const string STEADY_STATE_TARGET_OPTION_NODE          = "steadyStateTargetOption";

    Common::XmlNodeIterator optionsRootIterator = _rootIterator->getChildren(_childName);

    Common::XmlNodeIterator steadyStateTargetOptionRootIterator = optionsRootIterator->getChildren(STEADY_STATE_TARGET_OPTION_NODE);

    static std::map<std::string,Tucuxi::Core::SteadyStateTargetOption> m =
    {
        {"atSteadyState", Tucuxi::Core::SteadyStateTargetOption::AtSteadyState},
        {"withinTreatmentTimeRange", Tucuxi::Core::SteadyStateTargetOption::WithinTreatmentTimeRange}
    };

    string value = steadyStateTargetOptionRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(steadyStateTargetOptionRootIterator);
    return Tucuxi::Core::SteadyStateTargetOption::AtSteadyState;
}

Tucuxi::Core::TargetExtractionOption QueryImport::getChildTargetExtractionOptionEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{

    static const string TARGET_EXTRACTION_OPTION_NODE          = "targetExtractionOption";

    Common::XmlNodeIterator optionsRootIterator = _rootIterator->getChildren(_childName);

    Common::XmlNodeIterator targetExtractionOptionRootIterator = optionsRootIterator->getChildren(TARGET_EXTRACTION_OPTION_NODE);


    static std::map<std::string,Tucuxi::Core::TargetExtractionOption> m =
    {
        {"populationValues", Tucuxi::Core::TargetExtractionOption::PopulationValues},
        {"aprioriValues", Tucuxi::Core::TargetExtractionOption::AprioriValues},
        {"individualTargets", Tucuxi::Core::TargetExtractionOption::IndividualTargets},
        {"individualTargetsIfDefinitionExists", Tucuxi::Core::TargetExtractionOption::IndividualTargetsIfDefinitionExists},
        {"definitionIfNoIndividualTarget", Tucuxi::Core::TargetExtractionOption::DefinitionIfNoIndividualTarget},
        {"individualTargetsIfDefinitionExistsAndDefinitionIfNoIndividualTarget", Tucuxi::Core::TargetExtractionOption::IndividualTargetsIfDefinitionExistsAndDefinitionIfNoIndividualTarget}

    };

    string value = targetExtractionOptionRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(targetExtractionOptionRootIterator);
    return Tucuxi::Core::TargetExtractionOption::PopulationValues;
}

Tucuxi::Core::FormulationAndRouteSelectionOption QueryImport::getChildFormulationAndRouteSelectionOptionEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{

    static const string FORMULATION_AND_ROUTE_SELECTION_OPTION_NODE          = "formulationAndRouteSelectionOption";

    Common::XmlNodeIterator optionsRootIterator = _rootIterator->getChildren(_childName);

    Common::XmlNodeIterator formulationAndRouteSelectionOptioneRootIterator = optionsRootIterator->getChildren(FORMULATION_AND_ROUTE_SELECTION_OPTION_NODE);


    static std::map<std::string,Tucuxi::Core::FormulationAndRouteSelectionOption> m =
    {
        {"allFormulationAndRoutes", Tucuxi::Core::FormulationAndRouteSelectionOption::AllFormulationAndRoutes},
        {"lastFormulationAndRoute", Tucuxi::Core::FormulationAndRouteSelectionOption::LastFormulationAndRoute},
        {"defaultFormulationAndRoute", Tucuxi::Core::FormulationAndRouteSelectionOption::DefaultFormulationAndRoute}
    };

    string value = formulationAndRouteSelectionOptioneRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(formulationAndRouteSelectionOptioneRootIterator);

    return Tucuxi::Core::FormulationAndRouteSelectionOption::LastFormulationAndRoute;
}


Tucuxi::Core::ComputingOption QueryImport::getChildComputingOption(Common::XmlNodeIterator _rootIterator, const string& _childName)
{
    static const string PARAMETERS_TYPE             = "parametersType";
    static const string COMPARTMENT_OPTION          = "compartmentOption";
    static const string RETRIEVE_STATISTICS         = "retrieveStatistics";
    static const string RETRIEVE_PARAMETERS         = "retrieveParameters";
    static const string RETRIEVE_COVARIATES         = "retrieveCovariates";

    Common::XmlNodeIterator computingOptionRootIterator = _rootIterator->getChildren(_childName);
    Tucuxi::Core::PredictionParameterType predictionParameterType = getChildParametersTypeEnum(computingOptionRootIterator, PARAMETERS_TYPE);
    Tucuxi::Core::CompartmentsOption compartmentOption = getChildCompartmentsOptionEnum(computingOptionRootIterator, COMPARTMENT_OPTION);
    Tucuxi::Core::RetrieveStatisticsOption retrieveStatisticsOption;
    Tucuxi::Core::RetrieveParametersOption retrieveParametersOption;
    Tucuxi::Core::RetrieveCovariatesOption retrieveCovariatesOption;

    if(getChildBool(computingOptionRootIterator, RETRIEVE_STATISTICS))
    {
        retrieveStatisticsOption = Tucuxi::Core::RetrieveStatisticsOption::RetrieveStatistics;
    }else {
        retrieveStatisticsOption = Tucuxi::Core::RetrieveStatisticsOption::DoNotRetrieveStatistics;
    }

    if(getChildBool(computingOptionRootIterator, RETRIEVE_PARAMETERS))
    {
        retrieveParametersOption = Tucuxi::Core::RetrieveParametersOption::RetrieveParameters;
    }else {
        retrieveParametersOption = Tucuxi::Core::RetrieveParametersOption::DoNotRetrieveParameters;
    }

    if(getChildBool(computingOptionRootIterator, RETRIEVE_COVARIATES))
    {
        retrieveCovariatesOption = Tucuxi::Core::RetrieveCovariatesOption::RetrieveCovariates;
    }else {
        retrieveCovariatesOption = Tucuxi::Core::RetrieveCovariatesOption::DoNotRetrieveCovariates;
    }


    return Tucuxi::Core::ComputingOption(predictionParameterType, compartmentOption, retrieveStatisticsOption, retrieveParametersOption, retrieveCovariatesOption);
}



Tucuxi::Core::CompartmentsOption QueryImport::getChildCompartmentsOptionEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{

    Common::XmlNodeIterator compartmentOptionRootIterator = _rootIterator->getChildren(_childName);

    static std::map<std::string,Tucuxi::Core::CompartmentsOption> m =
    {
        {"allActiveMoieties", Tucuxi::Core::CompartmentsOption::AllActiveMoieties},
        {"allAnalytes", Tucuxi::Core::CompartmentsOption::AllAnalytes},
        {"allCompartments", Tucuxi::Core::CompartmentsOption::AllCompartments},
        {"specific", Tucuxi::Core::CompartmentsOption::Specific}
    };

    string value = compartmentOptionRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(compartmentOptionRootIterator);

    return Tucuxi::Core::CompartmentsOption::AllActiveMoieties;
}

Tucuxi::Core::PredictionParameterType QueryImport::getChildParametersTypeEnum(Common::XmlNodeIterator _rootIterator, const std::string& _childName)
{


    Common::XmlNodeIterator parametersTypeRootIterator = _rootIterator->getChildren(_childName);

    static std::map<std::string,Tucuxi::Core::PredictionParameterType> m =
    {
        {"population", Tucuxi::Core::PredictionParameterType::Population},
        {"apriori", Tucuxi::Core::PredictionParameterType::Apriori},
        {"aposteriori", Tucuxi::Core::PredictionParameterType::Aposteriori}
//        {"best", Tucuxi::Core::CompartmentsOption::Specific}
    };

    string value = parametersTypeRootIterator->getValue();
    auto it = m.find(value);
    if (it != m.end()) {
        return it->second;
    }

    setNodeError(parametersTypeRootIterator);

    return Tucuxi::Core::PredictionParameterType::Population;
}


} // namespace Query
} // namespace Tucuxi
