#ifndef BUILDPKASYMPTOTIC_H
#define BUILDPKASYMPTOTIC_H


#include "drugmodelbuilder.h"

#include "tucucore/drugmodel/drugmodel.h"
#include "tucucore/hardcodedoperation.h"

using namespace Tucuxi::Core;

class BuildPkAsymptotic
{
public:
    BuildPkAsymptotic() {}

    Tucuxi::Core::DrugModel *buildDrugModel(
            ResidualErrorType _errorModelType = ResidualErrorType::NONE,
            std::vector<Value> _sigmas = {0.0},
            Tucuxi::Core::ParameterVariabilityType _variabilityTypeR = Tucuxi::Core::ParameterVariabilityType::None,
            Tucuxi::Core::ParameterVariabilityType _variabilityTypeT = Tucuxi::Core::ParameterVariabilityType::None,
            Value _variabilityValueR = 0.0,
            Value _variabilityValueT = 0.0)
    {
        Tucuxi::Core::DrugModel *model;
        model = new Tucuxi::Core::DrugModel();

        model->setDrugId("drugTestPkAsymptotic");
        model->setDrugModelId("pkAsymptotic1");

        std::unique_ptr<DrugModelMetadata> metaData = std::make_unique<DrugModelMetadata>();
        metaData->addAtc("fake Atc");
        Tucuxi::Common::TranslatableString drugName;
        drugName.setString("Asymptotic model test");
        metaData->setDrugName(drugName);
        metaData->setAuthorName("The authors");
        model->setMetadata(std::move(metaData));

        std::unique_ptr<AnalyteSet> analyteSet(new AnalyteSet());

        analyteSet->setId("analyteSet");
        analyteSet->setPkModelId("test.pkasymptotic");

        std::unique_ptr<Analyte> analyte = std::make_unique<Analyte>("analyte", Unit("mg/l"), MolarMass(10.0, Unit("mol/l")));
        analyte->setAnalyteId("analyte");



        model->addCovariate(
                    std::unique_ptr<Tucuxi::Core::CovariateDefinition>(
                        new Tucuxi::Core::CovariateDefinition("covT", "2.0", nullptr, CovariateType::Standard, DataType::Double,
                                                              Tucuxi::Common::TranslatableString("covT"))));

        model->addCovariate(
                    std::unique_ptr<Tucuxi::Core::CovariateDefinition>(
                        new Tucuxi::Core::CovariateDefinition("covR", "0.5", nullptr, CovariateType::Standard, DataType::Double,
                                                              Tucuxi::Common::TranslatableString("covR"))));

        ErrorModel* errorModel = new ErrorModel();

        errorModel->setErrorModel(_errorModelType);
        for (size_t i = 0; i < _sigmas.size(); i++) {
            std::string sigmaName = "sigma" + std::to_string(i);
            errorModel->addOriginalSigma(std::make_unique<PopulationValue>(sigmaName, _sigmas[i]));
        }

        std::unique_ptr<ErrorModel> err(errorModel);

        analyte->setResidualErrorModel(std::move(err));
        analyteSet->addAnalyte(std::move(analyte));

        std::unique_ptr<ParameterSetDefinition> dispositionParameters(new ParameterSetDefinition());

        Operation *opT = new JSOperation(" \
                                            return covT;",
                                            { OperationInput("covT", InputType::DOUBLE)});
        std::unique_ptr<ParameterDefinition> PT(new Tucuxi::Core::ParameterDefinition("TestT", 1.0, opT, std::make_unique<ParameterVariability>(_variabilityTypeT, _variabilityValueT)));
        dispositionParameters->addParameter(std::move(PT));
        Operation *opR = new JSOperation(" \
                                            return covR;",
                                            { OperationInput("covR", InputType::DOUBLE)});
        std::unique_ptr<ParameterDefinition> PR(new Tucuxi::Core::ParameterDefinition("TestR", 0.5, opR, std::make_unique<ParameterVariability>(_variabilityTypeR, _variabilityValueR)));
        dispositionParameters->addParameter(std::move(PR));

        analyteSet->setDispositionParameters(std::move(dispositionParameters));



        model->addAnalyteSet(std::move(analyteSet));

        std::unique_ptr<DrugModelDomain> drugDomain(new DrugModelDomain());

        model->setDomain(std::move(drugDomain));

        {
            AnalyteSetToAbsorptionAssociation *association;
            const AnalyteSet *a = model->getAnalyteSet();
            association = new AnalyteSetToAbsorptionAssociation(*a);
            association->setAbsorptionModel(AbsorptionModel::Extravascular);

            std::unique_ptr<ParameterSetDefinition> absorptionParameters(new ParameterSetDefinition());

            association->setAbsorptionParameters(std::move(absorptionParameters));
            FormulationAndRoute formulationSpecs(Formulation::OralSolution, AdministrationRoute::Oral, AbsorptionModel::Extravascular, "No details");
            std::unique_ptr<FullFormulationAndRoute> formulationAndRoute(new FullFormulationAndRoute(formulationSpecs, "extraId"));
            formulationAndRoute->addAssociation(std::unique_ptr<AnalyteSetToAbsorptionAssociation>(association));

            std::unique_ptr<AnalyteConversion> analyteConversion = std::make_unique<AnalyteConversion>("analyte", 1.0);
            formulationAndRoute->addAnalyteConversion(std::move(analyteConversion));

            ValidDoses *validDoses = new ValidDoses(Unit("mg"), std::make_unique<PopulationValue>(400));
            std::unique_ptr<ValidValuesFixed> specificDoses = std::make_unique<ValidValuesFixed>();
            specificDoses->addValue(DoseValue(100));
            specificDoses->addValue(DoseValue(200));
            specificDoses->addValue(DoseValue(300));
            specificDoses->addValue(DoseValue(400));
            specificDoses->addValue(DoseValue(500));
            specificDoses->addValue(DoseValue(600));
            specificDoses->addValue(DoseValue(700));
            specificDoses->addValue(DoseValue(800));
            specificDoses->addValue(DoseValue(900));
            specificDoses->addValue(DoseValue(1000));
            specificDoses->addValue(DoseValue(1100));
            specificDoses->addValue(DoseValue(1200));
            specificDoses->addValue(DoseValue(1300));
            specificDoses->addValue(DoseValue(1400));
            specificDoses->addValue(DoseValue(1500));
            specificDoses->addValue(DoseValue(1600));
            specificDoses->addValue(DoseValue(1700));
            specificDoses->addValue(DoseValue(1800));
            specificDoses->addValue(DoseValue(1900));
            specificDoses->addValue(DoseValue(2000));

            validDoses->addValues(std::move(specificDoses));

            std::unique_ptr<ValidDoses> doses(validDoses);

            formulationAndRoute->setValidDoses(std::move(doses));

            ValidValuesFixed *fixedIntervals = new ValidValuesFixed();
            fixedIntervals->addValue(6);
            fixedIntervals->addValue(12);
            fixedIntervals->addValue(24);

            ValidDurations *validIntervals = new ValidDurations(Unit("h"), std::make_unique<PopulationValue>("", 24));
            validIntervals->addValues(std::unique_ptr<IValidValues>(fixedIntervals));


            std::unique_ptr<ValidDurations> intervals(validIntervals);
            formulationAndRoute->setValidIntervals(std::move(intervals));

            model->addFormulationAndRoute(std::move(formulationAndRoute));
        }

        std::unique_ptr<TimeConsiderations> timeConsiderations = std::make_unique<TimeConsiderations>();

        // Arbitrary 20h half life. To be checked. Multiplier of 20
        // With a multiplier of 10 it fails
        std::unique_ptr<HalfLife> halfLife = std::make_unique<HalfLife>("halflife", 20, Tucuxi::Core::Unit("h"), 20);
        timeConsiderations->setHalfLife(std::move(halfLife));
        std::unique_ptr<OutdatedMeasure> outdatedMeasure = std::make_unique<OutdatedMeasure>("id", 0.0, Unit());
        timeConsiderations->setOutdatedMeasure(std::move(outdatedMeasure));
        model->setTimeConsiderations(std::move(timeConsiderations));

        Tucuxi::Core::OperationCollection collection;
        collection.populate();
        std::shared_ptr<Operation> sharedOperation = collection.getOperationFromId("direct");

        std::unique_ptr<Operation> activeMoietyOperation = std::unique_ptr<Operation>(sharedOperation.get()->clone());

        std::vector<AnalyteId> analyteList;
        analyteList.push_back(AnalyteId("analyte"));
        std::unique_ptr<ActiveMoiety> activeMoiety = std::make_unique<ActiveMoiety>(ActiveMoietyId("activeMoietyConstant"), Unit("mg/l"), analyteList, std::move(activeMoietyOperation));


        Tucuxi::Common::TranslatableString activeMoietyName;
        activeMoietyName.setString("Test PkAsymptotic");
        activeMoiety->setName(activeMoietyName);

        // I removed the targets from the build, to let tests define various targets
/*
        // Add targets
        std::unique_ptr<SubTargetDefinition> cMin(new SubTargetDefinition("cMin", 750.0, nullptr));
        std::unique_ptr<SubTargetDefinition> cMax(new SubTargetDefinition("cMax", 1500.0, nullptr));
        std::unique_ptr<SubTargetDefinition> cBest(new SubTargetDefinition("cBest", 1000.0, nullptr));
        TargetDefinition *target = new TargetDefinition(TargetType::Residual,
                                                        Unit("mg/l"),
                                                        "analyte",
                                                        std::move(cMin),
                                                        std::move(cMax),
                                                        std::move(cBest),
                                                        std::unique_ptr<SubTargetDefinition>(new SubTargetDefinition("mic", 0.0, nullptr)),
                                                        std::unique_ptr<SubTargetDefinition>(new SubTargetDefinition("tMin", 1000.0, nullptr)),
                                                        std::unique_ptr<SubTargetDefinition>(new SubTargetDefinition("tMax", 1000.0, nullptr)),
                                                        std::unique_ptr<SubTargetDefinition>(new SubTargetDefinition("tBest", 1000.0, nullptr)),
                                                        std::unique_ptr<SubTargetDefinition>(new SubTargetDefinition("toxicity", 10000.0, nullptr)),
                                                        std::unique_ptr<SubTargetDefinition>(new SubTargetDefinition("inefficacy", 000.0, nullptr)));


        activeMoiety->addTarget(std::unique_ptr<TargetDefinition>(target));
*/

        model->addActiveMoiety(std::move(activeMoiety));

        return model;
    }
};


#endif // BUILDPKASYMPTOTIC_H
