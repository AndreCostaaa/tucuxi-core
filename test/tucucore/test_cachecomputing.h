#ifndef TEST_CACHECOMPUTING_H
#define TEST_CACHECOMPUTING_H


#include <algorithm>
#include <iostream>
#include <memory>
#include <typeinfo>

#include "fructose/fructose.h"

#include "tucucommon/datetime.h"

#include "tucucore/computingservice/computingrequest.h"
#include "tucucore/computingservice/computingresponse.h"
#include "tucucore/computingservice/computingtrait.h"
#include "tucucore/computingcomponent.h"
#include "tucucore/drugmodel/drugmodel.h"
#include "tucucore/drugtreatment/drugtreatment.h"
#include "drugmodels/buildimatinib.h"
#include "tucucore/montecarlopercentilecalculator.h"
#include "tucucore/cachecomputing.h"

using namespace Tucuxi::Core;

using namespace std::chrono_literals;
using namespace date;

struct TestCacheComputing : public fructose::test_base<TestCacheComputing>
{

    CacheComputing *m_cache{nullptr};

    TestCacheComputing() {

        // We reduce the number of patients to speed up the tests
        MonteCarloPercentileCalculatorBase::setStaticNumberPatients(10);
    }


    void buildDrugTreatment(DrugTreatment *&_drugTreatment, FormulationAndRoute _route)
    {
         _drugTreatment = new DrugTreatment();

         // List of time ranges that will be pushed into the history
         DosageTimeRangeList timeRangeList;

         // Create a time range starting at the beginning of june 2017, with no upper end (to test that the repetitions
         // are handled correctly)
         DateTime startSept2018(date::year_month_day(date::year(2018), date::month(9), date::day(1)),
                                Duration(std::chrono::hours(8), std::chrono::minutes(0), std::chrono::seconds(0)));


         //const FormulationAndRoute route("formulation", AdministrationRoute::IntravenousBolus, AbsorptionModel::Intravascular);
         // Add a treatment intake every ten days in June
         // 200mg via a intravascular at 08h30, starting the 01.06
         LastingDose periodicDose(DoseValue(200.0),
                                  TucuUnit("mg"),
                                  _route,
                                  Duration(),
                                  Duration(std::chrono::hours(6)));
         DosageRepeat repeatedDose(periodicDose, 16);
         std::unique_ptr<Tucuxi::Core::DosageTimeRange> sept2018(new Tucuxi::Core::DosageTimeRange(startSept2018, repeatedDose));


         _drugTreatment->getModifiableDosageHistory().addTimeRange(*sept2018);
    }

    void buildDrugTreatmentSteadyState(DrugTreatment *&_drugTreatment, FormulationAndRoute _route)
    {
         _drugTreatment = new DrugTreatment();

         // List of time ranges that will be pushed into the history
         DosageTimeRangeList timeRangeList;

         // Create a time range starting at the beginning of june 2017, with no upper end (to test that the repetitions
         // are handled correctly)
         DateTime startSept2018(date::year_month_day(date::year(2018), date::month(9), date::day(1)),
                                Duration(std::chrono::hours(8), std::chrono::minutes(0), std::chrono::seconds(0)));


         //const FormulationAndRoute route("formulation", AdministrationRoute::IntravenousBolus, AbsorptionModel::Intravascular);
         // Add a treatment intake every ten days in June
         // 200mg via a intravascular at 08h30, starting the 01.06
         LastingDose periodicDose(DoseValue(200.0),
                                  TucuUnit("mg"),
                                  _route,
                                  Duration(),
                                  Duration(std::chrono::hours(24)));
         DosageRepeat repeatedDose(periodicDose, 90);
         std::unique_ptr<Tucuxi::Core::DosageTimeRange> sept2018(new Tucuxi::Core::DosageTimeRange(startSept2018, repeatedDose));


         _drugTreatment->getModifiableDosageHistory().addTimeRange(*sept2018);
    }

    void compute(DrugTreatment *_drugTreatment, DrugModel *_drugModel, DateTime _start, DateTime _end, double _nbPointsPerHour, bool _isHitExpected)
    {

        RequestResponseId requestResponseId = "1";
        PercentileRanks percentileRanks({5, 25, 50, 75, 95});
        ComputingOption computingOption(PredictionParameterType::Population, CompartmentsOption::MainCompartment);
        std::unique_ptr<ComputingTraitPercentiles> traits =
                std::make_unique<ComputingTraitPercentiles>(
                    requestResponseId, _start, _end, percentileRanks, _nbPointsPerHour, computingOption);

        ComputingRequest request(requestResponseId, *_drugModel, *_drugTreatment, std::move(traits));


        std::unique_ptr<ComputingResponse> response2 = std::make_unique<ComputingResponse>(requestResponseId);

            ComputingStatus result;
            result = m_cache->compute(request, response2);

            fructose_assert_eq( result, ComputingStatus::Ok);

            fructose_assert_eq(m_cache->isLastCallaHit(), _isHitExpected);

            const ComputedData* responseData = response2->getData();
            fructose_assert(dynamic_cast<const PercentilesData*>(responseData) != nullptr);
            const PercentilesData *resp = dynamic_cast<const PercentilesData*>(responseData);

            TMP_UNUSED_PARAMETER(resp);
    }

    void testImatinib1(const std::string& /* _testName */)
    {
        IComputingService *component = dynamic_cast<IComputingService*>(ComputingComponent::createComponent());

        fructose_assert( component != nullptr);

        m_cache = new CacheComputing(component);

        DrugModel* drugModel;
        BuildImatinib builder;
        drugModel = builder.buildDrugModel();
        fructose_assert(drugModel != nullptr);

        DrugTreatment *drugTreatment;
        const FormulationAndRoute route(Formulation::OralSolution, AdministrationRoute::Oral, AbsorptionModel::Extravascular);

        buildDrugTreatment(drugTreatment, route);


        RequestResponseId requestResponseId = "1";
        Tucuxi::Common::DateTime start(2018_y / sep / 1, 8h + 0min);
        Tucuxi::Common::DateTime end(2018_y / sep / 5, 8h + 0min);
        PercentileRanks percentileRanks({5, 25, 50, 75, 95});
        double nbPointsPerHour = 10.0;
        ComputingOption computingOption(PredictionParameterType::Population, CompartmentsOption::MainCompartment);
        std::unique_ptr<ComputingTraitPercentiles> traits =
                std::make_unique<ComputingTraitPercentiles>(
                    requestResponseId, start, end, percentileRanks, nbPointsPerHour, computingOption);

        ComputingRequest request(requestResponseId, *drugModel, *drugTreatment, std::move(traits));

        compute(drugTreatment, drugModel, start, end, nbPointsPerHour, false);
        compute(drugTreatment, drugModel, start, end, nbPointsPerHour, true);
        compute(drugTreatment, drugModel, start, end, nbPointsPerHour, true);
        m_cache->clear();
        compute(drugTreatment, drugModel, start, end, nbPointsPerHour, false);
        compute(drugTreatment, drugModel, start, end, nbPointsPerHour, true);
        compute(drugTreatment, drugModel, start, end, nbPointsPerHour, true);

        Tucuxi::Common::DateTime start2(2018_y / sep / 9, 8h + 0min);
        Tucuxi::Common::DateTime end2(2018_y / sep / 12, 8h + 0min);

        compute(drugTreatment, drugModel, start2, end2, nbPointsPerHour, false);
        compute(drugTreatment, drugModel, start2, end2, nbPointsPerHour, true);
        compute(drugTreatment, drugModel, start2, end2, nbPointsPerHour / 1.1, true);

        compute(drugTreatment, drugModel, start, end, nbPointsPerHour, true);

        compute(drugTreatment, drugModel, start2, end2, nbPointsPerHour * 1.1, false);
        compute(drugTreatment, drugModel, start2, end2, nbPointsPerHour * 1.1, true);

        // Delete all dynamically allocated objects
        delete drugModel;
        delete drugTreatment;
        delete component;
        delete m_cache;
    }


    void testImatinibSteadyState(const std::string& /* _testName */)
    {
        return;

        IComputingService *component = dynamic_cast<IComputingService*>(ComputingComponent::createComponent());

        fructose_assert( component != nullptr);

        DrugModel* drugModel;
        BuildImatinib builder;
        drugModel = builder.buildDrugModel();
        fructose_assert(drugModel != nullptr);

        DrugTreatment *drugTreatment;
        const FormulationAndRoute route(Formulation::OralSolution, AdministrationRoute::Oral, AbsorptionModel::Extravascular);

        buildDrugTreatmentSteadyState(drugTreatment, route);


        RequestResponseId requestResponseId = "1";
        Tucuxi::Common::DateTime start(2018_y / date::literals::oct / 1, 8h + 0min);
        Tucuxi::Common::DateTime end(2018_y / date::literals::oct / 5, 8h + 0min);
        PercentileRanks percentileRanks({5, 25, 50, 75, 95});
        double nbPointsPerHour = 10.0;
        ComputingOption computingOption(PredictionParameterType::Population, CompartmentsOption::MainCompartment);
        std::unique_ptr<ComputingTraitPercentiles> traits =
                std::make_unique<ComputingTraitPercentiles>(
                    requestResponseId, start, end, percentileRanks, nbPointsPerHour, computingOption);

        ComputingRequest request(requestResponseId, *drugModel, *drugTreatment, std::move(traits));

        std::unique_ptr<ComputingResponse> response = std::make_unique<ComputingResponse>(requestResponseId);

        auto cache = std::make_unique<CacheComputing>(component);

        ComputingStatus result;
        result = cache->compute(request, response);

        fructose_assert_eq( result, ComputingStatus::Ok);

        const ComputedData* responseData = response->getData();
        fructose_assert(dynamic_cast<const PercentilesData*>(responseData) != nullptr);
        const PercentilesData *resp = dynamic_cast<const PercentilesData*>(responseData);

        TMP_UNUSED_PARAMETER(resp);
        /*
            std::vector<CycleData> data = resp->getData();
            fructose_assert(data.size() == 16);
            fructose_assert(data[0].m_concentrations.size() == 1);
            fructose_assert(data[0].m_concentrations[0][0] == 0.0);
            DateTime startSept2018(date::year_month_day(date::year(2018), date::month(9), date::day(1)),
                                   Duration(std::chrono::hours(8), std::chrono::minutes(0), std::chrono::seconds(0)));

            fructose_assert(data[0].m_start.toSeconds() + data[0].m_times[0][0] * 3600.0 == startSept2018.toSeconds());
            fructose_assert(data[1].m_start.toSeconds() + data[1].m_times[0][0] * 3600.0 == startSept2018.toSeconds() + 3600.0 * 6.0);
            */

/*
        {
            // Ask for 15 intakes, without the first one.
            RequestResponseId requestResponseId = "1";
            Tucuxi::Common::DateTime start(2018_y / sep / 1, 14h + 0min);
            Tucuxi::Common::DateTime end(2018_y / sep / 5, 8h + 0min);
        double nbPointsPerHour = 10.0;
            ComputingOption computingOption(PredictionParameterType::Population, CompartmentsOption::MainCompartment);
            std::unique_ptr<ComputingTraitConcentration> traits =
                    std::make_unique<ComputingTraitConcentration>(
                        requestResponseId, start, end, nbPointsPerHour, computingOption);

            std::unique_ptr<ComputingTraits> computingTraits = std::make_unique<ComputingTraits>();
            computingTraits->addTrait(std::move(traits));

            ComputingRequest request(requestResponseId, *drugModel, *drugTreatment, std::move(computingTraits));

            std::unique_ptr<ComputingResponse> partialResponse = std::make_unique<ComputingResponse>(requestResponseId);

            ComputingResult result;
            result = component->compute(request, partialResponse);

            fructose_assert( result == ComputingResult::Success);

            const std::vector<std::unique_ptr<SingleComputingResponse> > &responses = partialResponse.get()->getResponses();
            for(std::size_t i = 0; i < responses.size(); i++) {
                fructose_assert(dynamic_cast<SinglePredictionResponse*>(responses[i].get()) != nullptr);
                const SinglePredictionResponse *resp = dynamic_cast<SinglePredictionResponse*>(responses[i].get());
                std::vector<CycleData> data = resp->getData();
                fructose_assert(data.size() == 15);
                fructose_assert(data[0].m_concentrations.size() == 1);
                fructose_assert(data[0].m_concentrations[0][0] != 0.0);
                DateTime startSept2018(date::year_month_day(date::year(2018), date::month(9), date::day(1)),
                                       Duration(std::chrono::hours(14), std::chrono::minutes(0), std::chrono::seconds(0)));

                fructose_assert(data[0].m_start.toSeconds() + data[0].m_times[0][0] * 3600.0 == startSept2018.toSeconds());
                fructose_assert(data[1].m_start.toSeconds() + data[1].m_times[0][0] * 3600.0 == startSept2018.toSeconds() + 3600.0 * 6.0);
            }
        }
*/
        // Delete all dynamically allocated objects
        delete drugModel;
        delete drugTreatment;
        delete component;
    }

};



#endif // TEST_CACHECOMPUTING_H
