/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef COMPUTINGRESPONSE_H
#define COMPUTINGRESPONSE_H

#include <string>
#include <vector>
#include <memory>

#include "tucucore/definitions.h"
#include "tucucore/dosage.h"
#include "tucucore/targetevaluationresult.h"
#include "tucucore/cyclestatistics.h"

namespace Tucuxi {
namespace Core {


typedef struct {
    std::string m_parameterId;
    Value m_value;
} ParameterValue;

typedef struct {
    std::string m_covariateId;
    Value m_value;
} CovariateValue;


///
/// \brief The CycleData class, meant to embed data about a cycle
/// It contains concentrations and times for a single cycle (or interval)
/// Actually it does contains concentrations of one or more compartments,
/// allowing to store analytes and active moieties. The identification of
/// the analytes and active moieties is not internally stored by the CycleData,
/// it is the responsibility of the user to know what concentration stands each
/// analyte or active moiety.
///
class CycleData
{
public:
    CycleData() {}
    CycleData(const Tucuxi::Common::DateTime &_start, const Tucuxi::Common::DateTime &_end, const TucuUnit &_unit)
        : m_start(_start), m_end(_end), m_unit(_unit)
    {
    }

    void addData(const TimeOffsets &_offsets, const std::vector<Concentration> &_concentrations)
    {
        m_times.push_back(_offsets);
        m_concentrations.push_back(_concentrations);
    }

    const std::vector<TimeOffsets>& getTimes()
    {
        return m_times;
    }

    const std::vector<Concentrations>& getConcentrations()
    {
        return m_concentrations;
    }

    /// \brief Absolute start time of the cycle
    Tucuxi::Common::DateTime m_start;

    /// \brief Absolute end time of the cycle
    /// This value could be computed from m_start and m_times, but it is more convenient
    /// To have it as a member variable
    Tucuxi::Common::DateTime m_end;

    /// \brief A serie of times expressed as offsets in hours to the start of a cycle
    std::vector<TimeOffsets> m_times;

    /// \brief A vector of vector of concentrations
    /// Each inner vector contains the concentrations of an analyte or a compartment
    /// The size of each inner vector has to be the same as m_times.
    std::vector<Concentrations> m_concentrations;

    /// \brief Unit of concentrations
    /// The area under curve corresponds to this unit times hours
    TucuUnit m_unit;

    /// \brief Pk parameter values for this cycle
    /// Can be used or not to store the values of the Pk parameters used for this cycle
    std::vector<ParameterValue> m_parameters;

    /// \brief covariates values for this cycle
    /// Can be used or not to store the values of the covariates used for this cycle
    std::vector<CovariateValue> m_covariates;

    /// \brief The statistics about the cycle data
    /// For each compartment or analyte, the statistics.
    CycleStats m_statistics;
};

///
/// \brief The ComputedData class is the base class for every response
/// It contains all the computed data
///
class ComputedData
{
public:
    virtual ~ComputedData() = 0;
    RequestResponseId getId() const;

protected:

    ComputedData(RequestResponseId _id);

    RequestResponseId m_id;
};

///
/// \brief The SinglePointsResponse class
/// This class contains data generated by a ComputingTraitSinglePoints, that is
/// when values at specific times are asked by a request.
/// It is also the response for ComputingTraitAtMeasures, that calculates
/// points at the measure times found in the DrugTreatment.
/// Therefore it offers the absolute times corresponding to the request as well
/// as the calculated concentrations at these points.
///
class SinglePointsData : public ComputedData
{
public:

    SinglePointsData(RequestResponseId _id) : ComputedData(_id) {}

    /// Absolute time of each concentration
    std::vector<Tucuxi::Common::DateTime> m_times;

    /// Concentration values
    std::vector<Concentrations> m_concentrations;

    /// Unit of concentrations
    TucuUnit m_unit;
};


class ConcentrationData
{
public:

    void addCycleData(const CycleData &_data) { m_data.push_back(_data); }
    const std::vector<CycleData>& getData() const { return m_data; }
    std::vector<CycleData>& getModifiableData() { return m_data; }

    void addAnalyteId(std::string _id) { m_ids.push_back(_id);}

    const std::vector<std::string>& getIds() const { return m_ids;}
protected:
    std::vector<CycleData> m_data;
    std::vector<std::string> m_ids;

};

///
/// \brief The SinglePredictionResponse class
/// It contains data of a single prediction, as a vector of CycleData.
class SinglePredictionData : public ComputedData, public ConcentrationData
{
public:
    SinglePredictionData(RequestResponseId _id) : ComputedData(_id) {}
};

///
/// \brief The FullDosage class
/// This class embeds all information about a potential dosage adjustment:
/// The dosage history, the score (suitability of the dosage), and concentrations
/// if the concentrations have been calculated
///
class DosageAdjustment : public ConcentrationData
{
public:

    DosageAdjustment() : ConcentrationData() {}

    double getGlobalScore() const {
        if (m_targetsEvaluation.size() == 0) {
            return 0.0;
        }
        double sum = 0.0;
        for (const auto& target: m_targetsEvaluation) {
            sum += target.getScore();
        }
        return sum / static_cast<double>(m_targetsEvaluation.size());
    }

    DosageHistory getDosageHistory() const {return m_history;}

    DosageHistory m_history;
    std::vector<TargetEvaluationResult> m_targetsEvaluation;
};

///
/// \brief The AdjustmentResponse class
/// This class embeds a vector of potential adjustments, each one being a
/// Dosage history, a score, and optionnally a concentration prediction.
///
class AdjustmentData : public SinglePredictionData
{
public:
    AdjustmentData(RequestResponseId _id) : SinglePredictionData(_id) {}

    void addAdjustment(DosageAdjustment _adjustment) { m_adjustments.push_back(_adjustment);}

    void setAdjustments(std::vector<DosageAdjustment> &_adjustments) { m_adjustments = _adjustments;}

    const std::vector<DosageAdjustment> getAdjustments() const { return m_adjustments;}

protected:

    /// A vector of possible dosage adjustments
    std::vector<DosageAdjustment> m_adjustments;
};

///
/// \brief The PercentilesResponse class
/// It shall contain different percentiles, for a certain period of time.
/// In order to embed all data necessary for correct exploitation, it contains:
/// 1. The percentile ranks as a vector of doubles, each one being in [0.0,100.0]
/// 2. The concentration of percentiles, as a vector of CycleMultiData,
///    one CycleData per percentile
///
class PercentilesData : public ComputedData
{
public:
    PercentilesData(RequestResponseId _id) : ComputedData(_id) {}

    void setRanks(const PercentileRanks &_ranks) { m_ranks = _ranks;}

    size_t getNbRanks() const { return m_ranks.size(); }

    PercentileRank getRank(unsigned int _index) const { return m_ranks[_index]; }

    void addPercentileData(const std::vector<CycleData> &_data) { m_data.push_back(_data);}

    const CycleData& getData(size_t _percentileIndex, size_t _cycleIndex) const {
        return m_data[_percentileIndex][_cycleIndex];
    }

    const std::vector<CycleData>& getPercentileData(size_t _percentileIndex) const {
        return m_data[_percentileIndex];
    }

    const PercentileRanks& getRanks() const { return m_ranks;}

    void setNbPointsPerHour(double _nbPointsPerHour) { m_nbPointsPerHour = _nbPointsPerHour;}
    double getNbPointsPerHour() const { return m_nbPointsPerHour;}

private:
    std::vector<std::vector<CycleData> > m_data;
    PercentileRanks m_ranks;
    double m_nbPointsPerHour{0.0};
};



///
/// \brief The ComputingResponse class
/// It is the response to a ComputingRequest object. It has an identifier and
/// a vector of SingleComputingResponse, and as such can embed various responses,
/// like a prediction, various percentiles, and a dosage adjustment.
///
class ComputingResponse
{
public:

    ///
    /// \brief ComputingResponse constructor
    /// \param _id The Id of the request
    ///
    ComputingResponse(RequestResponseId _id);

    ///
    /// \brief get the Id of the request
    /// \return the Id of the request
    ///
    RequestResponseId getId() const;

    ///
    /// \brief Adds a response to this list of responses
    /// \param _response The response to be added, as a unique_ptr
    ///
    void addResponse(std::unique_ptr<ComputedData> _response);

    ///
    /// \brief Gets the vector of responses
    /// \return A const reference to the vector of responses
    ///
    // std::vector<std::unique_ptr<SingleComputingResponse> > & getResponses() { return m_responses;}
    // const std::vector<std::unique_ptr<SingleComputingResponse> > & getResponses() const { return m_responses;}

    const ComputedData* getData() const { return m_data.get() ;}

    std::unique_ptr<ComputedData> getUniquePointerData() { return std::move(m_data);}

    ///
    /// \brief Set the computing time of this request
    /// \param _computingTime the computing time of the request
    ///
    void setComputingTimeInSeconds(std::chrono::duration<double> _computingTime);

    ///
    /// \brief get the computing time of this request
    /// \return the computing time of the request
    ///
    std::chrono::duration<double> getComputingTimeInSeconds() const;

    void setComputingStatus(ComputingStatus _result);

    ComputingStatus getComputingStatus() const;

protected:

    /// Id of the request
    RequestResponseId m_id;

    std::unique_ptr<ComputedData> m_data;

    std::chrono::duration<double, std::ratio<1,1> > m_computingTimeInSeconds{0};

    ComputingStatus m_computingResult{ComputingStatus::Undefined};
};

} // namespace Core
} // namespace Tucuxi

#endif // COMPUTINGRESPONSE_H
