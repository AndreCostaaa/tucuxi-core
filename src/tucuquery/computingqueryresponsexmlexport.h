//@@license@@

#ifndef COMPUTINGRESPONSEXMLEXPORT_H
#define COMPUTINGRESPONSEXMLEXPORT_H

#include "tucucommon/xmlattribute.h"
#include "tucucommon/xmldocument.h"
#include "tucucommon/xmlnode.h"

#include "tucucore/computingservice/computingresponse.h"

#include "tucuquery/computingqueryresponse.h"

#include "querystatus.h"

struct TestDosageImportExport;

namespace Tucuxi {
namespace Query {

class ComputingQueryResponseXmlExport
{
public:
    ComputingQueryResponseXmlExport();

    bool exportToString(const ComputingQueryResponse& _computingQueryResponse, std::string& _xmlString);

    bool exportToFile(const ComputingQueryResponse& _computingQueryResponse, const std::string& _fileName);

    std::string getComputingStatus(Tucuxi::Core::ComputingStatus _computingStatus, bool _codeEnable) const;

    std::string getQueryStatus(QueryStatus _queryStatus, bool _codeEnable) const;

protected:
    bool exportAdjustment(const Tucuxi::Core::AdjustmentData* _prediction, Tucuxi::Common::XmlNode& _rootNode);

    bool exportSinglePrediction(
            const Tucuxi::Core::SinglePredictionData* _prediction, Tucuxi::Common::XmlNode& _rootNode);

    bool exportSinglePoints(const Tucuxi::Core::SinglePointsData* _prediction, Tucuxi::Common::XmlNode& _rootNode);

    bool exportPercentiles(const Tucuxi::Core::PercentilesData* _prediction, Tucuxi::Common::XmlNode& _rootNode);

    virtual bool exportCycleData(const Tucuxi::Core::CycleData& _cycleData, Tucuxi::Common::XmlNode& _rootNode);

    bool exportCycleDatas(const std::vector<Tucuxi::Core::CycleData>& _cycleDatas, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosageHistory(const Tucuxi::Core::DosageHistory& _history, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosageTimeRange(
            const std::unique_ptr<Tucuxi::Core::DosageTimeRange>& _timeRange, Tucuxi::Common::XmlNode& _rootNode);

    void exportFormulationAndRoute(const Tucuxi::Core::SingleDose& _dosage, Tucuxi::Common::XmlNode& _rootNode);


    void exportDose(const Tucuxi::Core::SingleDose& _dosage, Tucuxi::Common::XmlNode& _rootNode);


    virtual void exportSingleDose(const Tucuxi::Core::SingleDose& _dosage, Tucuxi::Common::XmlNode& _rootNode);


    std::string formulationEnumToString(const Tucuxi::Core::Formulation& _formulation);


    std::string administrationRouteEnumToString(const Tucuxi::Core::AdministrationRoute& _administrationRoute);


    std::string absorptionModelEnumToString(const Tucuxi::Core::AbsorptionModel& _absorptionModel);





    void addNode(Tucuxi::Common::XmlNode& _rootNode, const std::string& _nodeName, const std::string& _nodeValue);

    template<typename T>
    void addNode(Tucuxi::Common::XmlNode& _rootNode, const std::string& _nodeName, T _nodeValue)
    {
        Tucuxi::Common::XmlNode node =
                m_doc.createNode(Tucuxi::Common::EXmlNodeType::Element, _nodeName, std::to_string(_nodeValue));
        _rootNode.addChild(node);
    }


    Tucuxi::Common::XmlDocument m_doc;

public:
    bool exportAbstractDosage(const Tucuxi::Core::Dosage& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::DosageBounded& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::DosageLoop& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::DosageSteadyState& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::DosageRepeat& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::DosageSequence& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::ParallelDosageSequence& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::LastingDose& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::DailyDose& _dosage, Tucuxi::Common::XmlNode& _rootNode);

    bool exportDosage(const Tucuxi::Core::WeeklyDose& _dosage, Tucuxi::Common::XmlNode& _rootNode);


private:
    friend TestDosageImportExport;
};

} // namespace Query
} // namespace Tucuxi

#endif // COMPUTINGRESPONSEXMLEXPORT_H
