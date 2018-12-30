/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef TEST_INTAKEINTERVALCALCULATOR_H
#define TEST_INTAKEINTERVALCALCULATOR_H

#include <iostream>
#include <memory>

#include "fructose/fructose.h"

#include "tucucore/pkmodels/onecompartmentbolus.h"
#include "tucucore/pkmodels/onecompartmentextra.h"
#include "tucucore/pkmodels/onecompartmentinfusion.h"
#include "tucucore/pkmodels/twocompartmentbolus.h"
#include "tucucore/pkmodels/twocompartmentextra.h"
#include "tucucore/pkmodels/twocompartmentinfusion.h"
#include "tucucore/pkmodels/threecompartmentbolus.h"
#include "tucucore/pkmodels/threecompartmentextra.h"
#include "tucucore/pkmodels/threecompartmentinfusion.h"

#include "tucucore/pkmodels/rkonecompartmentextra.h"
#include "tucucore/pkmodels/rkonecompartmentgammaextra.h"

using namespace Tucuxi::Core;

struct TestIntervalCalculator : public fructose::test_base<TestIntervalCalculator>
{
    static const int CYCLE_SIZE = 251;

    const double DELTA = 0.000001;
    const int maxResidualSize = 3;

    TestIntervalCalculator() { }

    template <class CalculatorMicroClass, class CalculatorMacroClass>
    void testSteadyState(const Tucuxi::Core::ParameterSetEvent &_microParameters,
                                       const Tucuxi::Core::ParameterSetEvent &_macroParameters,
                                       double _dose,
                                       Tucuxi::Core::AbsorptionModel _route,
                                       std::chrono::hours _interval,
                                       std::chrono::seconds _infusionTime,
                                       int _nbPoints)
    {

        Tucuxi::Core::IntakeIntervalCalculator::Result res;
        CalculatorMicroClass microCalculator;
        CalculatorMacroClass macroCalculator;

        DateTime now;
        Tucuxi::Common::Duration offsetTime = 0s;
        Tucuxi::Common::Duration interval = _interval;
        Tucuxi::Common::Duration infusionTime = _infusionTime;

        unsigned int residualSize = (microCalculator.getResidualSize() == microCalculator.getResidualSize()) ? microCalculator.getResidualSize() : maxResidualSize;
        bool isAll = false;

        std::vector<Tucuxi::Core::Concentrations> concentrations;
        concentrations.resize(residualSize);

        Tucuxi::Core::TimeOffsets times;
        Tucuxi::Core::IntakeEvent intakeEvent(now, offsetTime, _dose, interval, _route, infusionTime, _nbPoints);

        // Checking if steady state is reached by iterative 100 times a calculation and
        // passing residuals to the next iteration
        Tucuxi::Core::Residuals inMicroResiduals(residualSize), inMacroResiduals(residualSize), outMicroResiduals(residualSize), outMacroResiduals(residualSize);

        std::fill(outMicroResiduals.begin(), outMicroResiduals.end(), 0);
        std::fill(outMacroResiduals.begin(), outMacroResiduals.end(), 0);

        for (unsigned int cycle = 0; cycle < 300; cycle ++) {
            // Check Micro class
            inMicroResiduals = outMicroResiduals;
            res = microCalculator.calculateIntakeSinglePoint(
                concentrations,
                intakeEvent,
                _microParameters,
                inMicroResiduals,
                interval.toHours(),
                isAll,
                outMicroResiduals);
            fructose_assert(res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok);

            // Check Macro class
            inMacroResiduals = outMacroResiduals;
            res = macroCalculator.calculateIntakeSinglePoint(
                concentrations,
                intakeEvent,
                _macroParameters,
                inMacroResiduals,
                interval.toHours(),
                isAll,
                outMacroResiduals);
            fructose_assert(res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok);

#if 0
            std::cout << "[" << cycle << "]";
            for (unsigned int i = 0; i < residualSize; i++) {
                std::cout << inMicroResiduals[i] << ":" << outMicroResiduals[i] << " " << inMacroResiduals[i] << ":" << outMacroResiduals[i] << " ";
            }
            std::cout << std::endl;
#endif
        }

        if (res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok) {
            for (unsigned int i = 0; i < residualSize; i++) {
                fructose_assert_double_eq_rel_abs(inMicroResiduals[i], outMicroResiduals[i], 0.01, 0.01)
                fructose_assert_double_eq_rel_abs(inMacroResiduals[i], outMacroResiduals[i], 0.01, 0.01)
            }
        }
    }


    template <class CalculatorMicroClass,class CalculatorMacroClass>
    void testSingleVsMultiple(const Tucuxi::Core::ParameterSetEvent &_microParameters,
                              const Tucuxi::Core::ParameterSetEvent &_macroParameters,
                              double _dose,
                              Tucuxi::Core::AbsorptionModel _route,
                              std::chrono::hours _interval,
                              std::chrono::seconds _infusionTime,
                              int _nbPoints)
    {
        Tucuxi::Core::IntakeIntervalCalculator::Result res;
        CalculatorMicroClass microCalculator;
        CalculatorMacroClass macroCalculator;

        DateTime now;
        Tucuxi::Common::Duration offsetTime = 0s;
        Tucuxi::Common::Duration interval = _interval;
        Tucuxi::Common::Duration infusionTime = _infusionTime;

        unsigned int residualSize = (microCalculator.getResidualSize() == microCalculator.getResidualSize()) ? microCalculator.getResidualSize() : maxResidualSize;
        bool isAll = false;

        std::vector<Tucuxi::Core::Concentrations> concentrations;
        concentrations.resize(residualSize);

        Tucuxi::Core::TimeOffsets times;
        Tucuxi::Core::IntakeEvent intakeEvent(now, offsetTime, _dose, interval, _route, infusionTime, _nbPoints);
        Tucuxi::Core::Residuals inResiduals(residualSize), outMicroMultiResiduals(residualSize), outMicroSingleResiduals(residualSize), outMacroMultiResiduals(residualSize), outMacroSingleResiduals(residualSize);

        std::fill(inResiduals.begin(), inResiduals.end(), 0);

        // Calculation of Micro Class
        res = microCalculator.calculateIntakePoints(
            concentrations,
            times,
            intakeEvent,
            _microParameters,
            inResiduals,
            isAll,
            outMicroMultiResiduals,
            true);

        if (verbose()) {
            std::cout << "[Micro Class Calculation]"<< std::endl;
            for(unsigned int i = 0; i < residualSize; i++) {
                std::cout << "Multiple Out residual["
                          << i
                          << "] = "
                          << outMicroMultiResiduals[i]
                          << std::endl;
            }
        }

        fructose_assert(res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok);

        res = microCalculator.calculateIntakeSinglePoint(
            concentrations,
            intakeEvent,
            _microParameters,
            inResiduals,
            interval.toHours(),
            isAll,
            outMicroSingleResiduals);

        if (verbose()) {
            for(unsigned int i = 0; i < residualSize; i++) {
                std::cout << "Single   Out residual["
                          << i
                          << "] = "
                          << outMicroSingleResiduals[i]
                          << std::endl;
            }
        }

        fructose_assert(res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok);

        // Calculation of Macro Class
        res = macroCalculator.calculateIntakePoints(
            concentrations,
            times,
            intakeEvent,
            _macroParameters,
            inResiduals,
            isAll,
            outMacroMultiResiduals,
            true);

        if (verbose()) {
            std::cout << "\n[Macro Class Calculation]"<< std::endl;
            for(unsigned int i = 0; i < residualSize; i++) {
                std::cout << "Multiple Out residual["
                          << i
                          << "] = "
                          << outMacroMultiResiduals[i]
                          << std::endl;
            }
        }

        fructose_assert(res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok);

        res = macroCalculator.calculateIntakeSinglePoint(
            concentrations,
            intakeEvent,
            _macroParameters,
            inResiduals,
            interval.toHours(),
            isAll,
            outMacroSingleResiduals);

        if (verbose()) {
            for(unsigned int i = 0; i < residualSize; i++) {
                std::cout << "Single   Out residual["
                          << i
                          << "] = "
                          << outMacroSingleResiduals[i]
                          << std::endl;
            }
        }

        fructose_assert(res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok);

        if (res == Tucuxi::Core::IntakeIntervalCalculator::Result::Ok) {
            for (unsigned int i = 0; i < residualSize; i++) {
                fructose_assert_double_eq(outMicroMultiResiduals[i], outMicroSingleResiduals[i])
                fructose_assert_double_eq(outMacroMultiResiduals[i], outMacroSingleResiduals[i])
                fructose_assert_double_eq_rel_abs(outMicroMultiResiduals[i], outMacroMultiResiduals[i], 0.001, 0.001)
                fructose_assert_double_eq_rel_abs(outMicroSingleResiduals[i], outMacroSingleResiduals[i], 0.001, 0.001)
            }
        }
    }


    template <class CalculatorMicroClass, class CalculatorMacroClass>
    void testCalculator(const Tucuxi::Core::ParameterSetEvent &_microParameters,
                        const Tucuxi::Core::ParameterSetEvent &_macroParameters,
                        double _dose,
                        Tucuxi::Core::AbsorptionModel _route,
                        std::chrono::hours _interval,
                        std::chrono::seconds _infusionTime,
                        int _nbPoints)
    {
        testSingleVsMultiple<CalculatorMicroClass, CalculatorMacroClass>(_microParameters, _macroParameters, _dose, _route, _interval, _infusionTime, _nbPoints);
        testSteadyState<CalculatorMicroClass, CalculatorMacroClass>(_microParameters, _macroParameters, _dose, _route, _interval, _infusionTime, _nbPoints);
    }


    /// \brief Test the residual calculation of Bolus. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test1compBolusSingleVsMultiple(const std::string& /* _testName */)
    {
        if (verbose()) {
            std::cout << __FUNCTION__ << std::endl;
        }

        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0435331, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);
        testCalculator<Tucuxi::Core::OneCompartmentBolusMicro, Tucuxi::Core::OneCompartmentBolusMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::INTRAVASCULAR,
            1h,
            0s,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of extravascular. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test1compExtraSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0435331, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::OneCompartmentExtraMicro, Tucuxi::Core::OneCompartmentExtraMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::EXTRAVASCULAR,
            12h,
            0s,
            CYCLE_SIZE);
    }


    /// \brief Test the residual calculation of extravascular. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void testRK41compExtraSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0435331, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::RK4OneCompartmentExtraMicro, Tucuxi::Core::RK4OneCompartmentExtraMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::EXTRAVASCULAR,
            12h,
            0s,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of extravascular. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void testRK41compGammaExtraSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0435331, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("a", 6, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("b", 10, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("a", 6, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("b", 10, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::RK4OneCompartmentGammaExtraMicro, Tucuxi::Core::RK4OneCompartmentGammaExtraMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::EXTRAVASCULAR,
            12h,
            0s,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of infusion. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test1compInfusionSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0435331, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V", 347, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::OneCompartmentInfusionMicro, Tucuxi::Core::OneCompartmentInfusionMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::INFUSION,
            12h,
            1h,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of Bolus. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test2compBolusSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0444294, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K12", 0.0588235, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K21", 0.0584795, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q", 20, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V2", 342, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::TwoCompartmentBolusMicro, Tucuxi::Core::TwoCompartmentBolusMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::INTRAVASCULAR,
            12h,
            0s,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of extravascular. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test2compExtraSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0444294, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K12", 0.0588235, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K21", 0.0584795, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q", 20, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V2", 342, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 1, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::TwoCompartmentExtraMicro, Tucuxi::Core::TwoCompartmentExtraMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::EXTRAVASCULAR,
            12h,
            0s,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of infusion. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test2compInfusionSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0444294, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K12", 0.0588235, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K21", 0.0584795, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q", 20, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V2", 342, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::TwoCompartmentInfusionMicro, Tucuxi::Core::TwoCompartmentInfusionMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::INFUSION,
            12h,
            1h,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of Bolus. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test3compBolusSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 2, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0444294, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K12", 0.0588235, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K21", 0.0584795, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K13", 0.0882353, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K31", 0.0877193, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 2, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q1", 20, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q2", 30, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V2", 342, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::ThreeCompartmentBolusMicro, Tucuxi::Core::ThreeCompartmentBolusMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::INTRAVASCULAR,
            12h,
            0s,
            CYCLE_SIZE);
    }

    /// \brief Test the residual calculation of Extra. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test3compExtraSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 2, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0444294, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K12", 0.0588235, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K21", 0.0584795, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K13", 0.0882353, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K31", 0.0877193, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 2, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q1", 20, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q2", 30, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V2", 342, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ka", 0.609, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::ThreeCompartmentExtraMicro, Tucuxi::Core::ThreeCompartmentExtraMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::EXTRAVASCULAR,
            12h,
            0s,
            CYCLE_SIZE);
    }


    /// \brief Test the residual calculation of Infusion. Compares single point vs multiple points
    /// \param _testName Test name.
    /// A calculator is instanciated, and residuals are computed with both
    ///     calculateIntakePoints and calculateIntakeSinglePoint.
    /// The residuals are then compared and shall be identical.
    void test3compInfusionSingleVsMultiple(const std::string& /* _testName */)
    {
        // parameter for micro class
        Tucuxi::Core::ParameterDefinitions microParameterDefs;
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 2, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Ke", 0.0444294, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K12", 0.0588235, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K21", 0.0584795, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K13", 0.0882353, Tucuxi::Core::ParameterVariabilityType::None)));
        microParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("K31", 0.0877193, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent microParameters(DateTime(), microParameterDefs);

        // parameter for macro class
        Tucuxi::Core::ParameterDefinitions macroParameterDefs;
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("CL", 15.106, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("F", 2, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q1", 20, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("Q2", 30, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V1", 340, Tucuxi::Core::ParameterVariabilityType::None)));
        macroParameterDefs.push_back(std::unique_ptr<Tucuxi::Core::ParameterDefinition>(new Tucuxi::Core::ParameterDefinition("V2", 342, Tucuxi::Core::ParameterVariabilityType::None)));
        Tucuxi::Core::ParameterSetEvent macroParameters(DateTime(), macroParameterDefs);

        testCalculator<Tucuxi::Core::ThreeCompartmentInfusionMicro, Tucuxi::Core::ThreeCompartmentInfusionMacro>(
            microParameters,
            macroParameters,
            400.0,
            Tucuxi::Core::AbsorptionModel::INFUSION,
            12h,
            1h,
            CYCLE_SIZE);
    }
};

#endif // TEST_INTAKEINTERVALCALCULATOR_H
