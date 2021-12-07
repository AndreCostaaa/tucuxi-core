#ifndef TEST_PARAMETER_H
#define TEST_PARAMETER_H


#include "tucucore/pkmodel.h"
#include "tucucore/pkmodels/onecompartmentbolus.h"
#include "tucucore/pkmodels/onecompartmentextra.h"
#include "tucucore/pkmodels/onecompartmentinfusion.h"
#include "tucucore/pkmodels/threecompartmentbolus.h"
#include "tucucore/pkmodels/threecompartmentextra.h"
#include "tucucore/pkmodels/threecompartmentinfusion.h"
#include "tucucore/pkmodels/twocompartmentbolus.h"
#include "tucucore/pkmodels/twocompartmentextra.h"
#include "tucucore/pkmodels/twocompartmentinfusion.h"

#include "fructose/fructose.h"

using namespace Tucuxi::Core;

struct TestParameter : public fructose::test_base<TestParameter>
{
    TestParameter() {}

    /// \brief Test the function Parameter::applyEta().
    ///
    /// \testing{Parameter::applyEta()}
    /// \testing{Parameter::getValue()}
    ///
    void testApplyEta(const std::string& /* _testName */)
    {
        {
            // Test Proportional variability
            ParameterDefinition pDef(
                    "pid", 10.0, std::make_unique<ParameterVariability>(ParameterVariabilityType::Proportional, 1.0));
            Parameter p(pDef, 5.0);
            bool valid = p.applyEta(2.0);
            fructose_assert_eq(valid, true);
            fructose_assert_eq(p.getValue(), 15.0);
        }
        {
            // Test Exponential variability
            ParameterDefinition pDef(
                    "pid", 10.0, std::make_unique<ParameterVariability>(ParameterVariabilityType::Exponential, 1.0));
            Parameter p(pDef, 5.0);
            bool valid = p.applyEta(2.0);
            fructose_assert_eq(valid, true);
            fructose_assert_eq(p.getValue(), 5.0 * std::exp(2.0));
        }
        {
            // Test LogNormal variability
            ParameterDefinition pDef(
                    "pid", 10.0, std::make_unique<ParameterVariability>(ParameterVariabilityType::LogNormal, 1.0));
            Parameter p(pDef, 5.0);
            bool valid = p.applyEta(2.0);
            fructose_assert_eq(valid, true);
            fructose_assert_eq(p.getValue(), 5.0 * std::exp(2.0));
        }
        {
            // Test Normal variability
            ParameterDefinition pDef(
                    "pid", 10.0, std::make_unique<ParameterVariability>(ParameterVariabilityType::Normal, 1.0));
            Parameter p(pDef, 5.0);
            bool valid = p.applyEta(2.0);
            fructose_assert_eq(valid, true);
            fructose_assert_eq(p.getValue(), 7.0);
        }
        {
            // Test logit variability
            ParameterDefinition pDef(
                    "pid", 0.5, std::make_unique<ParameterVariability>(ParameterVariabilityType::Logit, 1.0));
            Parameter p(pDef, 0.6);
            bool valid = p.applyEta(2.0);
            fructose_assert_eq(valid, true);
            fructose_assert_eq(p.getValue(), 1.0 / (1.0 + std::exp(-(std::log(0.6 / (1.0 - 0.6)) + 2.0))));
        }
        {
            // Test logit variability with wrong parameter value

            Tucuxi::Common::LoggerHelper logHelper;
            logHelper.disable();
            ParameterDefinition pDef(
                    "pid", 0.5, std::make_unique<ParameterVariability>(ParameterVariabilityType::Logit, 1.0));
            Parameter p(pDef, 1.6);
            bool valid = p.applyEta(2.0);
            fructose_assert_eq(valid, false);
            logHelper.enable();
        }
    }

    ///
    /// \brief testGetAtTime
    /// Tests that extractime a ParameterSetEvent from a ParameterSetSeries works well through ParameterSeries::getAtTime().
    ///
    /// If the ParameterSetEvent is set at the same time as the one asked by getAtTime() it shall return this one, and not
    /// the previous one.
    ///
    /// \testing{Tucuxi::Core::ParameterSetSeries()}
    ///
    void testGetAtTime(const std::string& /* _testName */)
    {
        ParameterSetSeries series;

        Tucuxi::Core::ParameterDefinitions parameterDefs;
        Tucuxi::Core::ParameterDefinition def("F", 100, Tucuxi::Core::ParameterVariabilityType::None);
        Tucuxi::Core::ParameterSetEvent parameters0(
                DateTime(Tucuxi::Common::Duration(std::chrono::minutes(200))), parameterDefs);
        parameters0.addParameterEvent(def, 200);
        Tucuxi::Core::ParameterSetEvent parameters1(
                DateTime(Tucuxi::Common::Duration(std::chrono::minutes(300))), parameterDefs);
        parameters1.addParameterEvent(def, 300);
        Tucuxi::Core::ParameterSetEvent parameters2(
                DateTime(Tucuxi::Common::Duration(std::chrono::minutes(400))), parameterDefs);
        parameters2.addParameterEvent(def, 400);
        Tucuxi::Core::ParameterSetSeries parametersSeries;
        parametersSeries.addParameterSetEvent(parameters0);
        parametersSeries.addParameterSetEvent(parameters1);
        parametersSeries.addParameterSetEvent(parameters2);
        {
            auto ps = parametersSeries.getAtTime(DateTime(Tucuxi::Common::Duration(std::chrono::minutes(200))));
            for (auto p = ps->begin(); p < ps->end(); p++) {
                fructose_assert_eq(ps->getValue(ParameterId::F), 200);
            }
        }
        {
            auto ps = parametersSeries.getAtTime(DateTime(Tucuxi::Common::Duration(std::chrono::minutes(250))));
            for (auto p = ps->begin(); p < ps->end(); p++) {
                fructose_assert_eq(ps->getValue(ParameterId::F), 200);
            }
        }
        {
            auto ps = parametersSeries.getAtTime(DateTime(Tucuxi::Common::Duration(std::chrono::minutes(300))));
            for (auto p = ps->begin(); p < ps->end(); p++) {
                fructose_assert_eq(ps->getValue(ParameterId::F), 300);
            }
        }
        {
            auto ps = parametersSeries.getAtTime(DateTime(Tucuxi::Common::Duration(std::chrono::minutes(350))));
            for (auto p = ps->begin(); p < ps->end(); p++) {
                fructose_assert_eq(ps->getValue(ParameterId::F), 300);
            }
        }
        {
            auto ps = parametersSeries.getAtTime(DateTime(Tucuxi::Common::Duration(std::chrono::minutes(400))));
            for (auto p = ps->begin(); p < ps->end(); p++) {
                fructose_assert_eq(ps->getValue(ParameterId::F), 400);
            }
        }
    }
};

#endif // TEST_PARAMETER_H
