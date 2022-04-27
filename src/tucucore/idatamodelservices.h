//@@lisence@@

#ifndef TUCUXI_CORE_IDATAMODELSERVICES_H
#define TUCUXI_CORE_IDATAMODELSERVICES_H

#include "tucucommon/interface.h"

namespace Tucuxi {
namespace Core {

class IDataModelServices : public Tucuxi::Common::Interface
{
public:
    virtual bool loadDrug(const std::string& _xmlDrugDescription) = 0;
    virtual bool loadTreatment(const std::string& _xmlTreatmentDescription) = 0;
};

} // namespace Core
} // namespace Tucuxi

#endif // TUCUXI_CORE_IDATAMODELSERVICES_H
