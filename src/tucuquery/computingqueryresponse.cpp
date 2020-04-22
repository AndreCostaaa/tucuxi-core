#include "computingqueryresponse.h"

#include "tucucore/computingservice/computingresponse.h"

namespace Tucuxi {
namespace Query {


SingleResponseData::SingleResponseData(
        std::unique_ptr<Tucuxi::Core::ComputingResponse> _computingResponse,
        std::unique_ptr<ComputingResponseMetaData> _metaData) :
    m_computingResponse(std::move(_computingResponse)),
    m_metaData(std::move(_metaData))
{
}


ComputingQueryResponse::ComputingQueryResponse()
{

}

Tucuxi::Core::RequestResponseId ComputingQueryResponse::getQueryId() const
{
    return m_queryId;
}

const std::vector<SingleResponseData>& ComputingQueryResponse::getRequestResponses() const
{
    return m_requestResponses;
}


void ComputingQueryResponse::addRequestResponse(std::unique_ptr<Core::ComputingResponse> _computingResponse,
        std::unique_ptr<ComputingResponseMetaData> _metaData)
{
    m_requestResponses.push_back(SingleResponseData(std::move(_computingResponse),
                                                    std::move(_metaData)));
}

void ComputingQueryResponse::setRequestResponseId(Tucuxi::Core::RequestResponseId _requestResponseId)
{
   m_queryId =_requestResponseId;
}

ComputingResponseMetaData::ComputingResponseMetaData(
        std::string _drugModelID ) :
    m_drugModelId(_drugModelID)
{

}



} // namespace Query
} // namespace Tucuxi
