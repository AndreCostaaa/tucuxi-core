/*
* Copyright (C) 2017 Tucuxi SA
*/

#ifndef IPROCESSINGSERVICE_H
#define IPROCESSINGSERVICE_H

#include <memory>

#include "tucucommon/interface.h"

namespace Tucuxi {
namespace Core {

class ProcessingRequest;
class ProcessingResponse;

///
/// \brief The ProcessingResult enum
/// Describes the return value of a computation
enum class ProcessingResult {
    /// Everything went well
    Success = 0,
    /// There was an error
    Error
};

///
/// \brief The IProcessingService class
/// This interface exposes the entry point for any computation. All information is embedded into
/// the classes ProcessingRequest and ProcessingResponse
class IProcessingService : public Tucuxi::Common::Interface
{
public:

    ///
    /// \brief computes
    /// \param _request The request containing all information required for the computation
    /// \param _response The result of the computation
    /// \return Success if everything went well with the computation, Error else
    /// The response is a reference to a unique pointer that has to be allocated within compute()
    ///
    virtual ProcessingResult compute(const ProcessingRequest &_request, std::unique_ptr<ProcessingResponse> &_response) = 0;

    ///
    /// \brief returns a description of the last error in case of failed computation
    /// \return A description of the last error
    /// This function should only be called if compute() returned something different from Success
    ///
    virtual std::string getErrorString() const = 0;

};

}
}

#endif // IPROCESSINGSERVICE_H
