/*
* Copyright (C) 2017 Tucuxi SA
*/
 
#ifndef TUCUXI_CORE_IPROCESSINGSERVICES_H
#define TUCUXI_CORE_IPROCESSINGSERVICES_H
 
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
 
#include "tucucommon/interface.h"
#include "tucucommon/datetime.h"
#include "tucucommon/general.h"
 
#include "tucucore/definitions.h"
 
namespace Tucuxi {
namespace Core {
 
enum class PredictionType { Population, Apiori, Aposteriori };
 
class PredictionRequest 
{    
public:
    PredictionType getType() const { return m_type; }
    bool getIsAll() const { return m_isAll; }
    const Tucuxi::Common::DateTime& getStart() const { return m_start; }
    const Tucuxi::Common::DateTime& getEnd() const { return m_end; }
    int getNbPoints() const { return m_nbPoints; }
 
protected:
    PredictionRequest() {}
    PredictionRequest(PredictionType _type,
	bool _isAll,
        Tucuxi::Common::DateTime _start,
        Tucuxi::Common::DateTime _end,
        int _nbPoints) :
        m_type(_type), m_isAll(_isAll), m_start(_start), m_end(_end), m_nbPoints(_nbPoints) {}
 
private:
    PredictionType m_type;
    bool m_isAll;
    Tucuxi::Common::DateTime m_start;
    Tucuxi::Common::DateTime m_end;
    int m_nbPoints;
};
 
class ConcentrationRequest : public PredictionRequest
{
public:
    ConcentrationRequest() = delete;
    ConcentrationRequest(const std::string& _xmlRequest) { TMP_UNUSED_PARAMETER(_xmlRequest);}
    ConcentrationRequest(PredictionType _type,
	bool _isAll,
        Tucuxi::Common::DateTime _start,
        Tucuxi::Common::DateTime _end,
        int _nbPoints) :
        PredictionRequest(_type, _isAll, _start, _end, _nbPoints) {}
};
 
typedef int PercentileRank;
typedef std::vector<PercentileRank> PercentileRanks;
typedef Value Percentile;
typedef std::vector<Percentile> Percentiles;
 
class PercentilesRequest : public PredictionRequest
{
public:
    PercentilesRequest() = delete;
    PercentilesRequest(const std::string& _xmlRequest) { TMP_UNUSED_PARAMETER(_xmlRequest);}
    PercentilesRequest(PredictionType _type,
	bool _isAll,
        Tucuxi::Common::DateTime _start,
        Tucuxi::Common::DateTime _end,
        const PercentileRanks &_ranks,
        int _nbPoints) :
        PredictionRequest(_type, _isAll, _start, _end, _nbPoints), m_ranks(_ranks) {}
private:
    PercentileRanks m_ranks;
};
 
class ConcentrationPrediction
{
public:
 
    bool streamToFile(const std::string _fileName) {
        std::ofstream ostrm(_fileName, std::ios::binary);
        if (ostrm.rdstate() & std::ios_base::failbit)
	    return false;

	int nbCycles = this->m_values.size();
	double offset = 0.0;
	for(int cycle = 0; cycle < nbCycles; cycle ++) {
	    const Tucuxi::Core::Concentrations concentrations = getValues()[cycle];
	    const Tucuxi::Core::TimeOffsets times = getTimes()[cycle];
	    int nbPoints = concentrations.size();
	    for(int i = 0; i < nbPoints - 1; i++) {
		ostrm << (times[i]) + offset << " " << concentrations[i] << std::endl;
	    }
	    offset += times[nbPoints - 1];
	}
	return true;
    }
 
    bool allocate(const unsigned int _residualSize, const CycleSize _nbPoints, TimeOffsets &_times, std::vector<Concentrations> &_values)
    {
        _times.reserve(_nbPoints);
	for (unsigned int compartement= 0; compartement<_residualSize; compartement++)
	    _values[compartement].reserve(_nbPoints);
        return true;
    }
    void appendConcentrations(TimeOffsets &_times, Concentrations &_values)
    {
        m_times.push_back(_times);
        m_values.push_back(_values);
    }
 
    std::vector<TimeOffsets> getTimes() const { return m_times;}
    std::vector<Concentrations> getValues() const { return m_values;}
 
private:
    std::vector<TimeOffsets> m_times;
    std::vector<Concentrations> m_values;
};
typedef std::unique_ptr<ConcentrationPrediction> ConcentrationPredictionPtr;
 
class PercentilesPrediction
{
public:

    const PercentileRanks getRanks() const { return m_ranks;}
    const std::vector<TimeOffsets> getTimes() const { return m_times;}
    const std::vector<Percentiles> getValues() const { return m_values;}

private:
    std::vector<TimeOffsets> m_times;
    PercentileRanks m_ranks;
    std::vector<Percentiles> m_values;
};
typedef std::unique_ptr<PercentilesPrediction> PercentilesPredictionPtr;
 
class IProcessingServices : public Tucuxi::Common::Interface
{
public:
    virtual ConcentrationPredictionPtr computeConcentrations(const ConcentrationRequest& request) = 0;
    virtual PercentilesPredictionPtr computePercentiles(const PercentilesRequest& request) = 0;
    virtual void computeAdjustments() = 0;
};
 
}
}
 
#endif // TUCUXI_CORE_IPROCESSINGSERVICES_H
