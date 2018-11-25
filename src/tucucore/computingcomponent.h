#ifndef TUCUXI_CORE_COMPUTINGCOMPONENT_H
#define TUCUXI_CORE_COMPUTINGCOMPONENT_H


#include <memory>

#include "tucucommon/component.h"
#include "tucucommon/loggerhelper.h"
#include "tucucommon/general.h"

#include "tucucore/definitions.h"
#include "tucucore/residualerrormodel.h"
#include "tucucore/dosage.h"
#include "tucucore/sampleevent.h"
#include "tucucore/computingservice/icomputingservice.h"
#include "tucucore/concentrationprediction.h"
#include "tucucore/idatamodelservices.h"
#include "tucucore/pkmodel.h"
#include "tucucore/covariateevent.h"

namespace Tucuxi {
namespace Core {

class DrugModel;
class DrugErrorModel;
class DrugTreatment;
class PkModelCollection;
class ParameterSetSeries;
class ComputingTraitPercentiles;
class ComputingTraitConcentration;
class ComputingTraitAdjustment;
class ComputingTraitAtMeasures;
class ComputingTraitSinglePoints;
class ComputingTraitStandard;
class HalfLife;

///
/// \brief The main entry point for any computation.
/// It offers a method that takes as input a computing request, and that outputs a computing response.
///
class ComputingComponent : public Tucuxi::Common::Component,
        public IComputingService
{
public:

    static Tucuxi::Common::Interface* createComponent();

    /// \brief Destructor
    virtual ~ComputingComponent() override;

    ComputingResult compute(const ComputingRequest &_request, std::unique_ptr<ComputingResponse> &_response) override;
    std::string getErrorString() const override;

protected:
    /// \brief Access other interfaces of the same component.
    Tucuxi::Common::Interface* getInterface(const std::string &_name) override;


private:


    Duration secureStartDuration(const HalfLife &_halfLife);

    PkModelCollection *m_models;

    Tucuxi::Common::LoggerHelper m_logger;

    /// \brief Constructor called from createComponent()
    ComputingComponent();

    bool initialize();


    ComputingResult compute(
            const ComputingTraitConcentration *_traits,
            const ComputingRequest &_request,
            std::unique_ptr<ComputingResponse> &_response);

    ComputingResult compute(
            const ComputingTraitPercentiles *_traits,
            const ComputingRequest &_request,
            std::unique_ptr<ComputingResponse> &_response);

    ComputingResult compute(
            const ComputingTraitAdjustment *_traits,
            const ComputingRequest &_request,
            std::unique_ptr<ComputingResponse> &_response);

    ComputingResult compute(
            const ComputingTraitAtMeasures *_traits,
            const ComputingRequest &_request,
            std::unique_ptr<ComputingResponse> &_response);

    ComputingResult compute(
            const ComputingTraitSinglePoints *_traits,
            const ComputingRequest &_request,
            std::unique_ptr<ComputingResponse> &_response);

    ComputingResult generalExtractions(
            const Tucuxi::Core::ComputingTraitStandard *_traits,
            const ComputingRequest &_request,
            std::shared_ptr<PkModel> &_pkModel,
            IntakeSeries &_intakeSeries,
            CovariateSeries &_covariatesSeries,
            ParameterSetSeries &_parameterSeries,
            Common::DateTime &_calculationStartTime);

    ComputationResult computeConcentrations(
        ConcentrationPredictionPtr &_prediction,
        bool _isAll,
        const DateTime &_recordFrom,
        const DateTime &_recordTo,
        const IntakeSeries &_intakes,
        const ParameterSetSeries& _parameters,
        const Etas& _etas = Etas(0),
        const IResidualErrorModel &_residualErrorModel = EMPTY_RESIDUAL_ERROR_MODEL,
        const Deviations& _eps = Deviations(0),
        bool _isFixedDensity = 0);

    ComputationResult computePopulation(
        ConcentrationPredictionPtr &_prediction,
        bool _isAll,
        const DateTime &_recordFrom,
        const DateTime &_recordTo,
        const IntakeSeries &_intakes,
        const ParameterSetSeries& _parameters)
    {
        return computeConcentrations(_prediction, _isAll, _recordFrom, _recordTo, _intakes, _parameters);
    }

    ComputationResult computeApriori(
        ConcentrationPredictionPtr &_prediction,
        bool _isAll,
        const DateTime &_recordFrom,
        const DateTime &_recordTo,
        const IntakeSeries &_intakes,
        const ParameterSetSeries& _parameters)
    {
        return computeConcentrations(_prediction, _isAll, _recordFrom, _recordTo, _intakes, _parameters);
    }

    ComputationResult computeAposteriori(
        ConcentrationPredictionPtr &_prediction,
        bool _isAll,
        const DateTime &_recordFrom,
        const DateTime &_recordTo,
        const IntakeSeries &_intakes,
        const ParameterSetSeries& _parameters,
        const Etas& _etas,
        const IResidualErrorModel &_residualErrorModel)
    {
        return computeConcentrations(_prediction, _isAll, _recordFrom, _recordTo, _intakes, _parameters, _etas, _residualErrorModel);
    }

    ComputationResult extractError(
        const DrugErrorModel &_errorMode,
        const ParameterDefinitions &_parameterDefs,
        OmegaMatrix &_omega,
        IResidualErrorModel &_residualErrorModel)
    {
        TMP_UNUSED_PARAMETER(_errorMode);
        TMP_UNUSED_PARAMETER(_parameterDefs);
        TMP_UNUSED_PARAMETER(_omega);
        TMP_UNUSED_PARAMETER(_residualErrorModel);
        // TODO YJE
        return ComputationResult::Failure;
    }

    ComputationResult extractOmega(
        const DrugModel &_drugModel,
        OmegaMatrix &_omega);


    friend class ComputingTraitSinglePoints;
    friend class ComputingTraitAtMeasures;
    friend class ComputingTraitAdjustment;
    friend class ComputingTraitConcentration;
    friend class ComputingTraitPercentiles;


};

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_COMPUTINGCOMPONENT_H
