//@@lisence@@

#ifndef QUERYLOGGER_H
#define QUERYLOGGER_H

#include <string>

#include "tucucommon/component.h"

namespace Tucuxi {
namespace Query {

class IQueryLogger : public Tucuxi::Common::Interface
{
public:
    virtual ~IQueryLogger() = default;

    virtual std::string getFolderPath(void) = 0;

    virtual void saveQuery(const std::string& _queryString, const std::string& _queryID) = 0;
};

class QueryLogger : public Tucuxi::Common::Component, public IQueryLogger
{
public:
    static Tucuxi::Common::Interface* createComponent(const std::string& _folderPath);

    QueryLogger(const std::string& _folderPath);

    ~QueryLogger() override;

    void saveQuery(const std::string& _queryString, const std::string& _queryID) override;

    std::string getFolderPath(void) override;

protected:
    Tucuxi::Common::Interface* getInterface(const std::string& _name) override;

private:
    std::string m_folderPath;
};

} // namespace Query
} // namespace Tucuxi

#endif // QUERYLOGGER_H
