/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TEST_PERCENTILECALCULATOR_H
#define TEST_PERCENTILECALCULATOR_H

#include <iostream>
#include <memory>

#include "fructose/fructose.h"

#include "tucucommon/general.h"

#include "tucucore/montecarlopercentilecalculator.h"
#include "tucucore/residualerrormodel.h"
#include "tucucore/pkmodels/onecompartmentextra.h"
#include "tucucore/concentrationcalculator.h"
#include "tucucore/percentilesprediction.h"
#include "tucucore/intakeintervalcalculator.h"

struct TestPercentileCalculator : public fructose::test_base<TestPercentileCalculator>
{
    static const int CYCLE_SIZE = 251;

    TestPercentileCalculator() { }

    void testApriori(const std::string& /* _testName */)
    {
        // Simple test with imatinib values

        Tucuxi::Core::PercentilesPrediction percentiles;
        Tucuxi::Core::IntakeSeries intakeSeries;
        Tucuxi::Core::ParameterSetSeries parametersSeries;
        Tucuxi::Core::OmegaMatrix omega;
        Tucuxi::Core::SigmaResidualErrorModel residualErrorModel;
        Tucuxi::Core::Etas etas;
        Tucuxi::Core::PercentileRanks percentileRanks;
        Tucuxi::Core::ComputingAborter *aborter = nullptr;

        // Build parameters as Imatinib ones
        Tucuxi::Core::ParameterDefinitions parameterDefs;
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, std::make_unique<Tucuxi::Core::ParameterVariability>(Tucuxi::Core::ParameterVariabilityType::Proportional, 0.356))));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, std::make_unique<Tucuxi::Core::ParameterVariability>(Tucuxi::Core::ParameterVariabilityType::Proportional, 0.629))));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent parameters(DateTime(), parameterDefs);
        parametersSeries.addParameterSetEvent(parameters);

        DateTime now;
        Tucuxi::Common::Duration offsetTime = 0s;
        Tucuxi::Common::Duration interval = 24h;
        Tucuxi::Common::Duration infusionTime = 0h;
        double dose = 400;
        Tucuxi::Core::RouteModel route = Tucuxi::Core::RouteModel::EXTRAVASCULAR;

        DateTime recordFrom = now;
        DateTime recordTo = now + 96h;

        Tucuxi::Core::TimeOffsets times;
        Tucuxi::Core::IntakeEvent intakeEvent(now, offsetTime, dose, interval, route, infusionTime, CYCLE_SIZE);
        std::shared_ptr<Tucuxi::Core::IntakeIntervalCalculator> calculator2 = std::make_shared<Tucuxi::Core::OneCompartmentExtraMacro>();
        intakeEvent.setCalculator(calculator2);
        intakeSeries.push_back(intakeEvent);

        Tucuxi::Core::IntakeEvent intakeEvent2(now + 24h, offsetTime, dose, interval, route, infusionTime, CYCLE_SIZE);
        intakeEvent2.setCalculator(calculator2);
        intakeSeries.push_back(intakeEvent2);

        Tucuxi::Core::IntakeEvent intakeEvent3(now + 48h, offsetTime, dose, interval, route, infusionTime, CYCLE_SIZE);
        intakeEvent3.setCalculator(calculator2);
        intakeSeries.push_back(intakeEvent3);

        Tucuxi::Core::IntakeEvent intakeEvent4(now + 72h, offsetTime, dose, interval, route, infusionTime, CYCLE_SIZE);
        intakeEvent4.setCalculator(calculator2);
        intakeSeries.push_back(intakeEvent4);

        // std::cout << typeid(calculator).name() << std::endl;

        Tucuxi::Core::ConcentrationPredictionPtr predictionPtr;
        {
            predictionPtr = std::make_unique<Tucuxi::Core::ConcentrationPrediction>();

            Tucuxi::Core::IConcentrationCalculator *concentrationCalculator = new Tucuxi::Core::ConcentrationCalculator();
            concentrationCalculator->computeConcentrations(
                        predictionPtr,
                        false,
                        recordFrom,
                        recordTo,
                        intakeSeries,
                        parametersSeries);
            delete concentrationCalculator;

            predictionPtr->streamToFile("values_imatinib_percentile.dat");
        }
/*
        for(int i = 0; i < _nbPoints; i++) {
            Tucuxi::Core::Concentrations concentration2;
            concentration2 = predictionPtr->getValues()[0];
            // std::cout << i <<  " :: " << concentrations[0][i] << " : " << concentration2[i] << std::endl;

            // compare concentrations of compartment 1
            fructose_assert_double_eq(concentrations[0][i], concentration2[i]);
        }
*/

        percentileRanks = {5, 10, 25, 50, 75, 90, 95};

        Tucuxi::Core::Sigma sigma(1);
        sigma(0) = 0.3138;
        residualErrorModel.setErrorModel(Tucuxi::Core::SigmaResidualErrorModel::ResidualErrorType::PROPORTIONAL);
        residualErrorModel.setSigma(sigma);


        omega = Tucuxi::Core::OmegaMatrix(2,2);
        omega(0,0) = 0.356 * 0.356; // Variance of CL
        omega(0,1) = 0.798 * 0.356 * 0.629; // Covariance of CL and V
        omega(1,1) = 0.629 * 0.629; // Variance of V
        omega(1,0) = 0.798 * 0.356 * 0.629; // Covariance of CL and V

        // Set initial etas to 0 for CL and V
        etas.push_back(0.0);
        etas.push_back(0.0);

        std::unique_ptr<Tucuxi::Core::IAprioriPercentileCalculator> calculator(new Tucuxi::Core::AprioriMonteCarloPercentileCalculator());

        Tucuxi::Core::IPercentileCalculator::ComputingResult res;

        Tucuxi::Core::ConcentrationCalculator concentrationCalculator;
        res = calculator->calculate(
                    percentiles,
                    recordFrom,
                    recordTo,
                    intakeSeries,
                    parametersSeries,
                    omega,
                    residualErrorModel,
                    etas,
                    percentileRanks,
                    concentrationCalculator,
                    aborter);

        percentiles.streamToFile("apriori_percentiles_imatinib.dat");

        std::cout << "Apriori Percentile result is saved" << std::endl;

        fructose_assert(res == Tucuxi::Core::IPercentileCalculator::ComputingResult::Success);
    }


    void testAposterioriNormal(const std::string& /* _testName */)
    {
        Tucuxi::Core::IntakeSeries intakeSeries;
        Tucuxi::Core::ParameterSetSeries parametersSeries;
        Tucuxi::Core::OmegaMatrix omega;
        Tucuxi::Core::SigmaResidualErrorModel residualErrorModel;
        Tucuxi::Core::Etas etas;
        Tucuxi::Core::PercentilesPrediction percentiles;
        Tucuxi::Core::PercentileRanks percentileRanks;
        Tucuxi::Core::ComputingAborter *aborter = nullptr;

        // Build parameters as Imatinib ones
        Tucuxi::Core::ParameterDefinitions parameterDefs;
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, std::make_unique<Tucuxi::Core::ParameterVariability>(Tucuxi::Core::ParameterVariabilityType::Proportional, 0.356))));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, std::make_unique<Tucuxi::Core::ParameterVariability>(Tucuxi::Core::ParameterVariabilityType::Proportional, 0.629))));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent parameters(DateTime(), parameterDefs);
        parametersSeries.addParameterSetEvent(parameters);

        DateTime now;
        Tucuxi::Common::Duration offsetTime = 0s;
        Tucuxi::Common::Duration interval = 24h;
        Tucuxi::Common::Duration infusionTime = 0h;
        double dose = 400;
        Tucuxi::Core::RouteModel route = Tucuxi::Core::RouteModel::EXTRAVASCULAR;

        DateTime recordFrom = now;
        DateTime recordTo = now + 24h;

        Tucuxi::Core::TimeOffsets times;
        Tucuxi::Core::IntakeEvent intakeEvent(now, offsetTime, dose, interval, route, infusionTime, CYCLE_SIZE);
        std::shared_ptr<Tucuxi::Core::IntakeIntervalCalculator> calculator2 = std::make_shared<Tucuxi::Core::OneCompartmentExtraMacro>();
        intakeEvent.setCalculator(calculator2);
        intakeSeries.push_back(intakeEvent);

        // std::cout << typeid(calculator).name() << std::endl;

        Tucuxi::Core::ConcentrationPredictionPtr predictionPtr;
        {
            predictionPtr = std::make_unique<Tucuxi::Core::ConcentrationPrediction>();

            Tucuxi::Core::IConcentrationCalculator *concentrationCalculator = new Tucuxi::Core::ConcentrationCalculator();
            concentrationCalculator->computeConcentrations(
                        predictionPtr,
                        false,
                        recordFrom,
                        recordTo,
                        intakeSeries,
                        parametersSeries);
            delete concentrationCalculator;

            predictionPtr->streamToFile("values_imatinib_percentile.dat");
        }

        percentileRanks = {5, 10, 25, 50, 75, 90, 95};

        Tucuxi::Core::Sigma sigma(1);
        sigma(0) = 0.3138;
        residualErrorModel.setErrorModel(Tucuxi::Core::SigmaResidualErrorModel::ResidualErrorType::PROPORTIONAL);
        residualErrorModel.setSigma(sigma);

        omega = Tucuxi::Core::OmegaMatrix(2,2);
        omega(0,0) = 0.356 * 0.356; // Variance of CL
        omega(0,1) = 0.798 * 0.356 * 0.629; // Covariance of CL and V
        omega(1,1) = 0.629 * 0.629; // Variance of V
        omega(1,0) = 0.798 * 0.356 * 0.629; // Covariance of CL and V

        // Set initial etas to 0 for CL and V
        etas.push_back(0.0);
        etas.push_back(0.0);

        Tucuxi::Common::Duration sampleOffset = 2h;
        Tucuxi::Core::SampleEvent sampleEvent(now + sampleOffset, 200);
        Tucuxi::Core::SampleSeries sampleSeries;
        sampleSeries.push_back(sampleEvent);

        std::unique_ptr<Tucuxi::Core::IAposterioriNormalApproximationMonteCarloPercentileCalculator> calculator =
                std::unique_ptr<Tucuxi::Core::IAposterioriNormalApproximationMonteCarloPercentileCalculator>(
                    new Tucuxi::Core::AposterioriNormalApproximationMonteCarloPercentileCalculator());

        Tucuxi::Core::IPercentileCalculator::ComputingResult res;

        Tucuxi::Core::ConcentrationCalculator concentrationCalculator;
        res = calculator->calculate(
                    percentiles,
                    recordFrom,
                    recordTo,
                    intakeSeries,
                    parametersSeries,
                    omega,
                    residualErrorModel,
                    etas,
                    sampleSeries,
                    percentileRanks,
                    concentrationCalculator,
                    aborter);

        percentiles.streamToFile("aposteriori_normal_percentiles_imatinib.dat");

        std::cout << "Aposteriori Normal Percentile result is saved" << std::endl;

        fructose_assert(res == Tucuxi::Core::IPercentileCalculator::ComputingResult::Success);
    }

    void testAposteriori(const std::string& /* _testName */)
    {
        Tucuxi::Core::IntakeSeries intakeSeries;
        Tucuxi::Core::ParameterSetSeries parametersSeries;
        Tucuxi::Core::OmegaMatrix omega;
        Tucuxi::Core::SigmaResidualErrorModel residualErrorModel;
        Tucuxi::Core::Etas etas;
        Tucuxi::Core::PercentilesPrediction percentiles;
        Tucuxi::Core::PercentileRanks percentileRanks;
        Tucuxi::Core::ComputingAborter *aborter = nullptr;

        // Build parameters as Imatinib ones
        Tucuxi::Core::ParameterDefinitions parameterDefs;
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, std::make_unique<Tucuxi::Core::ParameterVariability>(Tucuxi::Core::ParameterVariabilityType::Proportional, 0.356))));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, std::make_unique<Tucuxi::Core::ParameterVariability>(Tucuxi::Core::ParameterVariabilityType::Proportional, 0.629))));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        parameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent parameters(DateTime(), parameterDefs);
        parametersSeries.addParameterSetEvent(parameters);

        DateTime now;
        Tucuxi::Common::Duration offsetTime = 0s;
        Tucuxi::Common::Duration interval = 24h;
        Tucuxi::Common::Duration infusionTime = 0h;
        double dose = 400;
        Tucuxi::Core::RouteModel route = Tucuxi::Core::RouteModel::EXTRAVASCULAR;

        DateTime recordFrom = now;
        DateTime recordTo = now + 24h;

        Tucuxi::Core::TimeOffsets times;
        Tucuxi::Core::IntakeEvent intakeEvent(now, offsetTime, dose, interval, route, infusionTime, CYCLE_SIZE);
        std::shared_ptr<Tucuxi::Core::IntakeIntervalCalculator> calculator2 = std::make_shared<Tucuxi::Core::OneCompartmentExtraMacro>();
        intakeEvent.setCalculator(calculator2);
        intakeSeries.push_back(intakeEvent);

        // std::cout << typeid(calculator).name() << std::endl;

        Tucuxi::Core::ConcentrationPredictionPtr predictionPtr;
        {
            predictionPtr = std::make_unique<Tucuxi::Core::ConcentrationPrediction>();

            Tucuxi::Core::IConcentrationCalculator *concentrationCalculator = new Tucuxi::Core::ConcentrationCalculator();
            concentrationCalculator->computeConcentrations(
                        predictionPtr,
                        false,
                        recordFrom,
                        recordTo,
                        intakeSeries,
                        parametersSeries);
            delete concentrationCalculator;

            predictionPtr->streamToFile("values_imatinib_percentile.dat");
        }

        percentileRanks = {5, 10, 25, 50, 75, 90, 95};

        Tucuxi::Core::Sigma sigma(1);
        sigma(0) = 0.3138;
        residualErrorModel.setErrorModel(Tucuxi::Core::SigmaResidualErrorModel::ResidualErrorType::PROPORTIONAL);
        residualErrorModel.setSigma(sigma);

        omega = Tucuxi::Core::OmegaMatrix(2,2);
        omega(0,0) = 0.356 * 0.356; // Variance of CL
        omega(0,1) = 0.798 * 0.356 * 0.629; // Covariance of CL and V
        omega(1,1) = 0.629 * 0.629; // Variance of V
        omega(1,0) = 0.798 * 0.356 * 0.629; // Covariance of CL and V

        // Set initial etas to 0 for CL and V
        etas.push_back(0.0);
        etas.push_back(0.0);

        Tucuxi::Common::Duration sampleOffset = 2h;
        Tucuxi::Core::SampleEvent sampleEvent(now + sampleOffset, 1000);
        Tucuxi::Core::SampleSeries sampleSeries;
        sampleSeries.push_back(sampleEvent);

        std::unique_ptr<Tucuxi::Core::IAposterioriPercentileCalculator> calculator =
                std::unique_ptr<Tucuxi::Core::IAposterioriPercentileCalculator>(
                    new Tucuxi::Core::AposterioriMonteCarloPercentileCalculator());

        Tucuxi::Core::IPercentileCalculator::ComputingResult res;

        Tucuxi::Core::ConcentrationCalculator concentrationCalculator;
        res = calculator->calculate(
                    percentiles,
                    recordFrom,
                    recordTo,
                    intakeSeries,
                    parametersSeries,
                    omega,
                    residualErrorModel,
                    etas,
                    sampleSeries,
                    percentileRanks,
                    concentrationCalculator,
                    aborter);

        percentiles.streamToFile("aposteriori_percentiles_imatinib.dat");

        std::cout << "Aposteriori Percentile result is saved" << std::endl;

        fructose_assert(res == Tucuxi::Core::IPercentileCalculator::ComputingResult::Success);
    }

};

#endif // TEST_PERCENTILECALCULATOR_H
