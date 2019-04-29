#ifndef TUCUXI_CORE_COMPUTINGRESULT_H
#define TUCUXI_CORE_COMPUTINGRESULT_H


namespace Tucuxi {
namespace Core {

///
/// \brief The ComputingResult enum
/// Describes the return value of a computation
enum class ComputingResult {
    /// Everything went well
    Ok = 0,
    /// Too Big computation required
    TooBig,
    /// Computation aborted by the external agent
    Aborted,
    /// Error in the extraction of parameters
    ParameterExtractionError,
    /// Error in the extraction of samples
    SampleExtractionError,
    /// Error in target extraction
    TargetExtractionError,
    /// The candidate is outside the target range. This is not an error, but more a status
    InvalidCandidate,
    /// There was an internal error during target evaluation
    TargetEvaluationError,
    /// There was an error during covariate extraction
    CovariateExtractionError,
    /// There was an error during intake extraction
    IntakeExtractionError,
    /// There was an error during residual error model extraction
    ErrorModelExtractionError,
    /// The IntakeToCalculatorAssociator could not find a suitable route
    UnsupportedRoute,
    /// Error during conversion of analytes
    AnalyteConversionError,
    /// A posteriori percentiles calculation, but no samples
    AposterioriPercentilesNoSamplesError,
    /// The ConcentrationCalculator did not find parameters
    ConcentrationCalculatorNoParameters,
    /// Bad paramters in the intake calculator
    BadParameters,
    /// Bad concentration detected in the intake calculator
    BadConcentration,
    /// Density error in the intake calculator
    DensityError,
    /// In a posteriori Etas calculation, Omega is empty
    AposterioriEtasCalculationEmptyOmega,
    /// In a posteriori Etas calculation, Omega is not a square matrix
    AposterioriEtasCalculationNoSquareOmega,
    /// ComputingTraitStandard::compute() should not be called
    ComputingTraitStandardShouldNotBeCalled,
    /// Could not find a suitable formulation and route
    CouldNotFindSuitableFormulationAndRoute,
    /// Multiple formulation and routes are not yet supported
    MultipleFormulationAndRoutesNotSupported,
    /// Could not find a suitable Pk Model
    NoPkModelError,
    /// An exception was raised in ComputingComponent
    ComputingComponentExceptionError,
    /// No Pk Models in the collection
    NoPkModels,
    /// The ComputingTraits sent for computation are nullptr
    NoComputingTraits,
    /// The recorded intakes size does not fit the predictions
    RecordedIntakesSizeError,
    /// No percentile calculation was performed
    NoPercentilesCalculation,
    /// The selected intakes size is not correct
    SelectedIntakesSizeError,
    /// No available dose
    NoAvailableDose,
    /// No available interval
    NoAvailableInterval,
    /// No available infusion time
    NoAvailableInfusionTime,
    /// No formulation and route available for adjustments
    NoFormulationAndRouteForAdjustment,
    /// The concentration vector has not a correct size
    ConcentrationSizeError,
    /// Error during calculation of active moiety concentration value
    ActiveMoietyCalculationError,
    /// There are no analytes groups
    NoAnalytesGroup
};


} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_COMPUTINGRESULT_H